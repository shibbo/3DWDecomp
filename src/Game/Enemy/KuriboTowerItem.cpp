#include "Enemy/KuriboTowerItem.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/EnemyAttachItem.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ItemUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"

namespace {
NERVE_DECL(KuriboTowerItem, StandBy)
NERVE_DECL(KuriboTowerItem, Fall)
NERVE_DECL(KuriboTowerItem, RootOnGround)
NERVES_MAKE_NOSTRUCT(KuriboTowerItem, StandBy, Fall, RootOnGround)
}
/** @brief Creates an item node for a Goomba tower.
 * @param pName Actor name. @param pItemName Carried item name. @param isItem Whether to carry a loose item. */
KuriboTowerItem::KuriboTowerItem(const char* pName, const char* pItemName, bool isItem)
    : KuriboTowerNode(pName), mItemName(pItemName), mIsItem(isItem) { mIsStopWalk = true; }
/** @brief Creates and attaches the carried item. @param rInfo Tower placement information. */
void KuriboTowerItem::init(const al::ActorInitInfo& rInfo) {
    al::ActorInitInfo info;
    al::PlacementInfo placement;
    info.initViewIdHost(&placement, rInfo);
    al::initActorSceneInfo(this, info);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, info);
    initCollider(80.0f, 80.0f, 0);
    al::initNerve(this, &NrvKuriboTowerItemStandBy, 0);
    al::initExecutorEnemyMovement(this, info);
    al::initActorClipping(this, info);
    al::invalidateClipping(this);
    makeActorAppeared();
    mItemType = rc::getItemTypeByName(mItemName);
    f32 height = 20.0f;
    if (mIsItem) {
        switch (mItemType) {
        case rc::ItemType_Coin: height = 80.0f; break;
        case rc::ItemType_KinokoOneUp:
        case rc::ItemType_KinokoSuper:
        case rc::ItemType_SuperBell:
        case rc::ItemType_FireFlower:
        case rc::ItemType_SuperLeaf:
        case rc::ItemType_BoomerangFlower:
        case rc::ItemType_SuperStar:
        case rc::ItemType_DoubleMario: height = 40.0f; break;
        case rc::ItemType_Bomb: break;
        case rc::ItemType_Shards: mUpdateFront = false;
        case rc::ItemType_GreenStar:
        case rc::ItemType_CollectItem: height = 100.0f; break;
        default: mItemType = rc::ItemType_KinokoSuper; height = 40.0f; break;
        }
    } else height = 80.0f;
    alActorPoseFunction::calcBaseMtx(&mItemMtx, this);
    mItem = new EnemyAttachItem(this, "\u30bf\u30ef\u30fc\u30af\u30ea\u30dc\u30fc\u30a2\u30a4\u30c6\u30e0", mItemName, mItemType,
                               nullptr, &mItemMtx, {0.0f, height, 0.0f}, !mIsItem);
    const al::ActorInitInfo* pItemInfo = &info;
    switch (mItemType) {
    case rc::ItemType_GreenStar:
    case rc::ItemType_CollectItem:
    case rc::ItemType_Shards: pItemInfo = &rInfo; break;
    }
    al::initCreateActorWithPlacementInfo(mItem, *pItemInfo);
    al::offCollide(mItem);
}
/** @brief Synchronizes the carried item after the tower is placed. */
void KuriboTowerItem::initAfterPlacement() {
    updateItemPosture();
    al::resetPosition(mItem, false);
    al::resetPosition(this, false);
    alActorPoseFunction::calcBaseMtx(&mItemMtx, this);
    mItem->initPosture();
}
/** @brief Updates bomb alignment and the rotation of carried shards. */
void KuriboTowerItem::updateItemPosture() {
    if (mItemType == rc::ItemType_Bomb && mHost->isBottom(this) && mHost->getChildNum() == 1 && al::isOnGround(this, 2, 0.0f))
        return;
    if (mItemType == rc::ItemType_Shards) {
        mItemRotate.y += 3.0f;
        mItemRotate.y = al::wrapAngle(mItemRotate.y);
        mItemMtx.makeRT(mItemRotate * sead::Mathf::deg2rad(1.0f), al::getTrans(this));
    } else alActorPoseFunction::calcBaseMtx(&mItemMtx, this);
}
/** @brief Clips the attached actor together with the tower node. */
void KuriboTowerItem::startClipped() {
    al::LiveActor::startClipped();
    if (al::isAlive(mItem) && !al::isClipped(mItem)) mItem->startClipped();
}
/** @brief Restores the attached actor when the node becomes visible. */
void KuriboTowerItem::endClipped() {
    al::LiveActor::endClipped();
    if (al::isAlive(mItem) && al::isClipped(mItem)) mItem->endClipped();
}
/** @brief Enables collision and starts falling as the bottom node. */
void KuriboTowerItem::requestRootBehavior() {
    al::onCollide(this);
    al::onCollide(mItem);
    al::setNerve(this, &NrvKuriboTowerItemFall);
}
/** @brief Enables collision after release from the tower. */
void KuriboTowerItem::requestRelease() { al::onCollide(this); }
/** @brief Updates the carried item in the StandBy state. */
void KuriboTowerItem::exeStandBy() {
    if (al::isDead(mItem)) {
        mHost->releaseChild(this);
        kill();
        return;
    }
    updatePosture(mUpdateFront);
    updateItemPosture();
}
/** @brief Updates the carried item in the Fall state. */
void KuriboTowerItem::exeFall() {
    if (al::isDead(mItem)) {
        mHost->releaseChild(this);
        kill();
        return;
    }
    updatePosture(mUpdateFront);
    updateItemPosture();
    if (al::isOnGround(this, 0, 0.0f)) al::setNerve(this, &NrvKuriboTowerItemRootOnGround);
}
/** @brief Updates the carried item in the RootOnGround state. */
void KuriboTowerItem::exeRootOnGround() {
    if (al::isFirstStep(this)) {
        mItem->endAttach(false);
        mItem->setVelocity(al::getVelocity(this));
        if (mItem->getItemType() == rc::ItemType_Shards) {
            al::offCollide(this);
            al::offCollide(mItem);
            al::offCollide(mItem->getItemActor());
        }
    }
    if (al::isDead(mItem)) {
        mHost->releaseChild(this);
        kill();
        return;
    }
    updatePosture(mUpdateFront);
    updateItemPosture();
}
/** @brief Gets the carried item actor. @return Item actor, or nullptr before initialization. */
al::LiveActor* KuriboTowerItem::getLinkedActor() { return mItem ? mItem->getItemActor() : nullptr; }
/** @brief Gets the node height. @return Height occupied in the tower. */
f32 KuriboTowerItem::getOffsetY() const { return 130.0f; }

