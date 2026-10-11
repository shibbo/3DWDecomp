#include "MapObj/ItemBubble.hpp"
#include "MapObj/CoinAttach.hpp"
#include "MapObj/KinokoOneUp.hpp"
#include "MapObj/KinokoSuper.hpp"
#include "MapObj/SuperBell.hpp"
#include "MapObj/FireFlower.hpp"
#include "MapObj/SuperLeaf.hpp"
#include "MapObj/BoomerangFlower.hpp"
#include "MapObj/SuperStar.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/CollectItem.hpp"
#include "MapObj/DoubleMario.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Enemy/Bomb.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    const sead::Vector3f sBubbleBellOffset(0.0f, -85.0f, 0.0f);
    const sead::Vector3f sBubbleFlowerOffset(0.0f, -60.0f, 0.0f);
    const sead::Vector3f sBubbleCoinOffset(0.0f, 0.0f, 0.0f);
    const sead::Vector3f sBubbleDefaultOffset(0.0f, -55.0f, 0.0f);
    const sead::Vector3f* const sBubbleItemOffsets[] = {
        &sBubbleCoinOffset, &sBubbleDefaultOffset, &sBubbleDefaultOffset, &sBubbleDefaultOffset,
        &sBubbleDefaultOffset, &sBubbleDefaultOffset, &sBubbleBellOffset, &sBubbleFlowerOffset,
        &sBubbleDefaultOffset, &sBubbleFlowerOffset, &sBubbleDefaultOffset, &sBubbleDefaultOffset,
        &sBubbleDefaultOffset, &sBubbleDefaultOffset, &sBubbleCoinOffset, &sBubbleCoinOffset,
        &sBubbleDefaultOffset, &sBubbleDefaultOffset
    };
    NERVE_DECL(ItemBubble, Wait);
    NERVE_DECL(ItemBubble, Disappear);
    NERVES_MAKE_NOSTRUCT(ItemBubble, Wait, Disappear)
}
ItemBubble::ItemBubble(const char* name, int type) : al::LiveActor("アイテム泡"), mItemName(name), mItemType(type) {}
void ItemBubble::init(const al::ActorInitInfo& info) { initActor(info, "ItemBubble"); }
void ItemBubble::initActor(const al::ActorInitInfo& info, const char* archive) {
    al::initActorWithArchiveName(this, info, archive, nullptr);
    al::initNerve(this, &NrvItemBubbleWait, 0);
    switch (mItemType) {
    case 0: mItemActor = new CoinAttach(mItemName, this); mRotateItem = true; break;
    default: if (!mItemActor) { makeActorDead(); return; } break;
    case 4: mItemActor = new KinokoOneUp(mItemName, this); break;
    case 5: mItemActor = new KinokoSuper(mItemName, this); break;
    case 6: mItemActor = new SuperBell(mItemName, this); break;
    case 7: mItemActor = new FireFlower(mItemName, this); break;
    case 8: mItemActor = new SuperLeaf(mItemName, this); break;
    case 9: mItemActor = new BoomerangFlower(mItemName, this, false); break;
    case 10: mItemActor = new SuperStar(mItemName, this); break;
    case 12: mItemActor = new Bomb(mItemName, true); break;
    case 14: mItemActor = new GreenStar(mItemName, this, true); mRotateItem = true; break;
    case 15: mItemActor = new CollectItem(mItemName, this, true); mRotateItem = true; break;
    case 17: mItemActor = new DoubleMario(mItemName, this, false); break;
    }
    if (mItemType == 14 || mItemType == 15) {
        al::setSensorRadius(this, "Body", 80.0f);
        al::setSensorRadius(this, "EnemyAttach", 80.0f);
    }
    al::initCreateActorWithPlacementInfo(mItemActor, info);
    makeActorAppeared();
    al::invalidateClipping(mItemActor);
}
void ItemBubble::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvItemBubbleDisappear) && al::isGreaterEqualStep(this, 4) && al::isSensorName(sender, "EnemyAttach") && al::isSensorMapObj(receiver)) {
        if (mGetItem) rc::sendMsgItemBubbleBreakAndGetItem(receiver, sender);
        else rc::sendMsgItemBubbleBreak(receiver, sender);
    }
}
bool ItemBubble::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvItemBubbleDisappear)) return false;
    if (al::isSensorName(receiver, "EnemyAttach")) {
        if (rc::isMsgItemBubbleBreak(msg) || al::isMsgPlayerFireBallAttack(msg)) {
            al::setNerve(this, &NrvItemBubbleDisappear);
            if (mItemType == 14 || mItemType == 15) al::startSe(mItemActor, "PgAppearBubble", nullptr);
            else if (mItemType != 0 && mItemType != 12) al::startSe(mItemActor, "PgAppear", nullptr);
            return true;
        }
        if (EnemyStateUtil::isMsgBlowDown(msg) || al::isMsgBallTrample(msg)) {
            al::setNerve(this, &NrvItemBubbleDisappear);
            if (al::isMsgPlayerInvincibleAttack(msg)) { mGetItem = true; mHitPlayerSensor = sender; }
            return al::isMsgBallAttack(msg) || al::isMsgBallTrample(msg);
        }
    }
    if (al::isSensorName(receiver, "Body") && al::isMsgPlayerItemGet(msg)) {
        al::setNerve(this, &NrvItemBubbleDisappear);
        mGetItem = true;
        mHitPlayerSensor = sender;
    }
    return false;
}
void ItemBubble::startDisappear() { al::setNerve(this, &NrvItemBubbleDisappear); }
bool ItemBubble::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvItemBubbleDisappear)) return false;
    if (al::isMsgTouchAssistTrig(msg)) { al::setNerve(this, &NrvItemBubbleDisappear); return true; }
    return al::isMsgTouchAssist(msg);
}
void ItemBubble::appear() {
    al::LiveActor::appear();
    mItemActor->appear();
    al::setNerve(this, &NrvItemBubbleWait);
}
void ItemBubble::startClipped() {
    al::LiveActor::startClipped();
    if (al::isAlive(mItemActor) && !al::isClipped(mItemActor)) mItemActor->startClipped();
}
void ItemBubble::endClipped() {
    al::LiveActor::endClipped();
    if (al::isAlive(mItemActor) && al::isClipped(mItemActor)) mItemActor->endClipped();
}
sead::Vector3f ItemBubble::getItemActorOffset(int type) const {
    sead::Vector3f offset;
    if (static_cast<u32>(type) < 18) offset = *sBubbleItemOffsets[type];
    else offset = sBubbleDefaultOffset;
    return offset;
}

bool ItemBubble::isEnableGetPlayerSensor() const { return mGetItem && mHitPlayerSensor; }
bool ItemBubble::isDisappear() const { return al::isNerve(this, &NrvItemBubbleDisappear); }
al::HitSensor* ItemBubble::getHitPlayerSensor() const { return mHitPlayerSensor; }
void ItemBubble::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
void ItemBubble::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
        if (mItemType == 14) al::startSe(mItemActor, "PgAppearFromBubble", nullptr);
        if (mItemType == 15) al::startSe(mItemActor, "PgAppearFromBubble", nullptr);
        al::updatePoseRotate(this, sead::Vector3f(0.0f, 0.0f, 0.0f));
        al::copyPose(mItemActor, this);
        al::resetPosition(mItemActor, false);
    }
    if (al::isActionEnd(this)) { al::startHitReactionDisappear(this); kill(); }
}
void ItemBubble::updatePosture() {
    if (al::isNerve(this, &NrvItemBubbleDisappear)) return;
    sead::Matrix34f mtx;
    sead::Matrix34f offset;
    offset.makeT(getItemActorOffset(mItemType));
    mtx.setMul(*getBaseMtx(), offset);
    if (mRotateItem) {
        sead::Matrix34f base = mtx;
        sead::Matrix34f rotate;
        rotate.makeR(sead::Vector3f(0.0f, sead::Mathf::deg2rad(rc::getCoinRotateY(mItemActor)), 0.0f));
        mtx.setMul(base, rotate);
    }
    al::updatePoseMtx(mItemActor, &mtx);
}
void ItemBubble::setItemType(int type) { mItemType = type; }
