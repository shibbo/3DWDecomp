#include "MapObj/BlockTransparent.hpp"
#include "MapObj/BlockQuestionChameleon.hpp"
#include "MapObj/BlockStateItem.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
    NERVE_DECL(BlockTransparent, Hide);
    NERVE_DECL(BlockTransparent, AppearItem);
    NERVE_DECL(BlockTransparent, Empty);
    NERVES_MAKE_NOSTRUCT(BlockTransparent, Empty)
    NERVES_MAKE_STRUCT(BlockTransparent, Hide, AppearItem)
}
BlockTransparent::BlockTransparent(const char* name) : al::LiveActor(name) {}
void BlockTransparent::init(const al::ActorInitInfo& info) {
    bool longBlock = al::isObjectName(info, "BlockTransparentLong");
    al::initActorWithArchiveName(this, info, longBlock ? "BlockEmptyLong" : "BlockEmpty",
                               al::isSingleMode(info) ? "TransparentSM" : "Transparent");
    al::initNerve(this, &NrvBlockTransparent.Hide, 1);
    mState = new BlockStateItem(this, info, longBlock, true, false, false, false);
    al::initNerveState(this, mState, &NrvBlockTransparent.AppearItem, "BlockItem");
    mChameleon = new BlockQuestionChameleon();
    al::initCreateActorWithPlacementInfo(mChameleon, info);
    al::setTrans(mChameleon, al::getTrans(this));
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength");
    if (expandClipping) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
    makeActorAppeared();
    al::hideModel(this);
}
bool BlockTransparent::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (al::isNerve(this, &NrvBlockTransparent.Hide)) {
        if (mPunchCooldown == 0 &&
            ((al::isSensorPlayer(sender) && al::isMsgPlayerObjUpperPunch(msg) && rc::getPlayerVelocity(sender).y > 0.0f) || al::isMsgKickKouraReflect(msg))) {
            mState->tryAppearItem(msg, sender, false);
            al::setNerve(this, &NrvBlockTransparent.AppearItem);
            return true;
        }
        if (al::isSensorPlayer(sender) && al::isSensorName(sender, "Body")) mPunchCooldown = 5;
        if (al::isMsgPlayerUpperPunch(msg) && al::isSensorDoorKey(sender)) {
            al::setNerve(this, &NrvBlockTransparent.AppearItem);
            return true;
        }
    }
    if (al::isNerve(this, &NrvBlockTransparent.AppearItem) && al::isLessStep(this, 3) &&
        al::isSensorPlayer(sender) && al::isMsgPlayerObjUpperPunch(msg) && rc::getPlayerVelocity(sender).y > 0.0f)
        return true;
    return false;
}
void BlockTransparent::respawn() {
    if (!al::isNerve(this, &NrvBlockTransparent.Hide) || al::isDead(this)) {
        al::validateHitSensors(this);
        al::setNerve(this, &NrvBlockTransparent.Hide);
        if (al::isDead(this)) makeActorAppeared();
        if (al::isDead(mChameleon)) mChameleon->makeActorAppeared();
        mState->reset();
    }
}
void BlockTransparent::onConnectRailBlock() {
    mConnectedRail = true;
    mState->setConnectedRailBlock();
}
bool BlockTransparent::isLong() const { return mState->isLong(); }
void BlockTransparent::exeHide() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::invalidateCollisionParts(this);
    }
    if (mPunchCooldown - 1 >= 0) --mPunchCooldown;
}
void BlockTransparent::exeAppearItem() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::validateCollisionParts(this);
        mChameleon->kill();
    }
    if (al::updateNerveState(this)) {
        if (mConnectedRail) {
            al::hideModel(this);
            al::setNerve(this, &NrvBlockTransparentEmpty);
        } else kill();
    }
}
void BlockTransparent::exeEmpty() {
    if (al::isFirstStep(this)) al::invalidateHitSensors(this);
    al::copyPose(mState->getBlockEmpty(), this);
}
