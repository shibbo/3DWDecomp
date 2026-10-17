#include "Enemy/EnemyAttachItem.hpp"
#include "Enemy/Bomb.hpp"
#include "MapObj/CollectItem.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/IllustItem.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/Shards.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"

namespace {
NERVE_DECL(EnemyAttachItem, Attach)
NERVE_DECL(EnemyAttachItem, PopUp)
NERVE_DECL(EnemyAttachItem, BreakBubble)
NERVE_DECL(EnemyAttachItem, Fall)
NERVE_DECL(EnemyAttachItem, Wait)
NERVES_MAKE_NOSTRUCT(EnemyAttachItem, Attach, PopUp, BreakBubble, Fall, Wait)
}

/**
 * @brief Creates an attachment that follows an enemy.
 * @param pHost Enemy carrying the item.
 * @param pName Attachment actor name.
 * @param pItemName Carried item name.
 * @param itemType Kind of carried item.
 * @param pFollowTrans Position to follow when no matrix is supplied.
 * @param pFollowMtx Transform to follow.
 * @param rOffset Offset from the followed transform.
 * @param isBubble Whether the item is enclosed in a bubble.
 */
EnemyAttachItem::EnemyAttachItem(al::LiveActor* pHost, const char* pName, const char* pItemName,
                               s32 itemType, const sead::Vector3f* pFollowTrans,
                               const sead::Matrix34f* pFollowMtx, const sead::Vector3f& rOffset,
                               bool isBubble)
    : al::LiveActor(pName), mItemActor(nullptr), mHost(pHost), mItemName(pItemName),
      mFollowTrans(pFollowTrans), mFollowMtx(pFollowMtx), mOffset(rOffset), mItemType(itemType),
      mIsBubble(isBubble), mKeepItemRotation(false) {}

/** @brief Creates the carried item and initializes attachment. @param rInfo Placement information. */
void EnemyAttachItem::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "EnemyAttachItem", nullptr);
    al::initNerve(this, &NrvEnemyAttachItemAttach, 0);
    mItemActor = mIsBubble ? rc::tryCreateAttachedItemBubble(mItemName, mItemType)
                          : rc::tryCreateAttachedItem(mItemName, mItemType);
    if (!mItemActor) {
        makeActorDead();
        return;
    }
    if (!mIsBubble) {
        switch (mItemType) {
        case rc::ItemType_Coin:
        case rc::ItemType_GreenStar:
        case rc::ItemType_CollectItem:
        case rc::ItemType_IllustItem:
            mKeepItemRotation = true;
            break;
        }
    }
    al::initCreateActorWithPlacementInfo(mItemActor, rInfo);
    al::offCollide(mItemActor);
    makeActorAppeared();
    al::invalidateClipping(this);
    if (mItemType != rc::ItemType_Shards) {
        al::invalidateClipping(mItemActor);
    }
}

/** @brief Initializes collision positions after placing the attachment. */
void EnemyAttachItem::initPosture() {
    updatePosture();
    al::resetPosition(mItemActor, false);
    if (mIsBubble) {
        al::resetPosition(static_cast<ItemBubble*>(mItemActor)->getItemActor(), false);
    }
}

/** @brief Updates the carried item from its followed position or transform. */
void EnemyAttachItem::updatePosture() {
    if (mFollowMtx) {
        sead::Matrix34f offset;
        offset.makeT(mOffset);
        sead::Matrix34f pose;
        pose.setMul(*mFollowMtx, offset);
        al::updatePoseMtx(this, &pose);
    } else {
        al::setTrans(this, *mFollowTrans + mOffset);
    }
    al::setTrans(mItemActor, al::getTrans(this));
    if (!mKeepItemRotation || mItemType == rc::ItemType_IllustItem) {
        al::copyPose(mItemActor, this);
    }
    if (mIsBubble) {
        ItemBubble* pBubble = static_cast<ItemBubble*>(mItemActor);
        if (!pBubble->isDisappear()) {
            pBubble->updatePosture();
        }
    }
}

/** @brief Appears with the carried item and resumes following. */
void EnemyAttachItem::appear() {
    al::LiveActor::appear();
    mItemActor->appear();
    al::setNerve(this, &NrvEnemyAttachItemAttach);
}

/** @brief Sends bubble release messages in the release states.
 * @param pSelf Attachment sensor. @param pOther Contacted sensor. */
void EnemyAttachItem::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if ((al::isNerve(this, &NrvEnemyAttachItemPopUp) && al::isSensorMapObj(pOther)) ||
        (al::isNerve(this, &NrvEnemyAttachItemBreakBubble) && al::isSensorEnemyBody(pOther))) {
        rc::sendMsgItemBubbleBreak(pOther, pSelf);
    }
}

/** @brief Clips the carried item along with the attachment. */
void EnemyAttachItem::startClipped() {
    al::LiveActor::startClipped();
    if (al::isAlive(mItemActor) && !al::isClipped(mItemActor)) {
        mItemActor->startClipped();
    }
}

/** @brief Restores the carried item after clipping. */
void EnemyAttachItem::endClipped() {
    al::LiveActor::endClipped();
    if (al::isAlive(mItemActor) && al::isClipped(mItemActor)) {
        mItemActor->endClipped();
    }
}

/** @brief Follows the enemy until the item is taken or its host dies. */
void EnemyAttachItem::exeAttach() {
    if (al::isFirstStep(this)) {
        al::updateMaterialCodeWater(mItemActor);
    }
    updatePosture();
    if (mIsBubble) {
        if (mHost && al::isDead(mHost)) {
            al::setNerve(this, &NrvEnemyAttachItemFall);
            return;
        }
    } else {
        switch (mItemType) {
        case rc::ItemType_CollectItem:
            if (static_cast<CollectItem*>(mItemActor)->isAcquiredInScene()) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        case rc::ItemType_GreenStar:
            if (static_cast<GreenStar*>(mItemActor)->isAcquiredInScene()) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        case rc::ItemType_Bomb:
            if (!static_cast<Bomb*>(mItemActor)->isAttach()) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        case rc::ItemType_IllustItem:
            if (static_cast<IllustItem*>(mItemActor)->isGot()) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        case rc::ItemType_Shards:
            if (static_cast<Shards*>(mItemActor)->isGot()) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        default:
            if (mHost && al::isDead(mHost)) {
                al::setNerve(this, &NrvEnemyAttachItemPopUp);
                return;
            }
            break;
        }
    }
    if (al::isDead(mItemActor)) {
        kill();
    }
}

/** @brief Drops a bubble and starts breaking it when it reaches the ground. */
void EnemyAttachItem::exeFall() {
    bool isWater = rc::isInWaterArea(this);
    f32 gravity = isWater ? 0.2f : 0.4f;
    f32 friction = isWater ? 0.9f : 0.996f;
    al::addVelocityToGravity(this, gravity);
    al::scaleVelocity(this, friction);
    al::copyPose(mItemActor, this);
    al::resetPosition(mItemActor, false);
    if (mIsBubble) {
        static_cast<ItemBubble*>(mItemActor)->updatePosture();
    }
    al::Triangle triangle;
    sead::Vector3f hit(0.0f, 0.0f, 0.0f);
    if (alCollisionUtil::getFirstPolyOnArrow(this, &hit, &triangle, al::getTrans(this),
                                           sead::Vector3f::ey * -50.0f, nullptr, nullptr)) {
        al::setNerve(this, &NrvEnemyAttachItemBreakBubble);
    } else if (al::isDead(mItemActor)) {
        kill();
    }
}

/** @brief Waits for the bubble to disappear before removing the attachment. */
void EnemyAttachItem::exeBreakBubble() {
    if (al::isDead(mItemActor)) {
        kill();
    }
}

/** @brief Releases the item with an upright pose and restores bomb collision. */
void EnemyAttachItem::exePopUp() {
    al::updatePoseRotate(mItemActor, sead::Vector3f(0.0f, 0.0f, 0.0f));
    al::resetPosition(mItemActor, false);
    if (mItemType == rc::ItemType_Bomb) {
        al::onCollide(mItemActor);
    }
    kill();
}

/** @brief Holds an explicitly detached item without further updates. */
void EnemyAttachItem::exeWait() {}

/** @brief Restores an attached shard while its host is still alive. */
void EnemyAttachItem::respawn() {
    if (mItemType != rc::ItemType_Shards) {
        return;
    }
    al::LiveActor::appear();
    if (al::isDead(mHost)) {
        return;
    }
    static_cast<Shards*>(mItemActor)->respawnShard();
    al::setNerve(this, &NrvEnemyAttachItemAttach);
}

/** @brief Changes the followed position when no matrix is used. @param pTrans New position. */
void EnemyAttachItem::setFollowTransPtr(const sead::Vector3f* pTrans) {
    if (!mFollowMtx) {
        mFollowTrans = pTrans;
    }
}

/** @brief Changes the followed matrix when no position is used. @param pMtx New transform. */
void EnemyAttachItem::setFollowMtxPtr(const sead::Matrix34f* pMtx) {
    if (!mFollowTrans) {
        mFollowMtx = pMtx;
    }
}

/** @brief Sets the carried item's velocity. @param rVelocity New velocity. */
void EnemyAttachItem::setVelocity(const sead::Vector3f& rVelocity) {
    al::setVelocity(mItemActor, rVelocity);
}

/** @brief Stops following and selects the release behavior. @param isPopUp Whether to keep the attachment waiting. */
void EnemyAttachItem::endAttach(bool isPopUp) {
    if (mIsBubble) {
        al::setNerve(this, &NrvEnemyAttachItemBreakBubble);
    } else if (isPopUp) {
        al::setNerve(this, &NrvEnemyAttachItemWait);
    } else {
        al::setNerve(this, &NrvEnemyAttachItemPopUp);
    }
}

