#include "MapObj/BlockBrickBreakable.hpp"
#include "MapObj/SnowCover.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(BlockBrickBreakable, Wait);
    NERVES_MAKE_NOSTRUCT(BlockBrickBreakable, Wait)
    NERVE_DECL(BlockBrickBreakable, Break);
    class BlockBrickBreakableNrvBreakHipDrop : public al::Nerve {
        void execute(al::NerveKeeper* keeper) const override { keeper->getParent<BlockBrickBreakable>()->exeBreak(); }
    };
    NERVE_DECL(BlockBrickBreakable, Reaction);
    class BlockBrickBreakableNrvReactionHipDrop : public al::Nerve {
        void execute(al::NerveKeeper* keeper) const override { keeper->getParent<BlockBrickBreakable>()->exeReaction(); }
    };
    NERVES_MAKE_STRUCT(BlockBrickBreakable, Break, BreakHipDrop, Reaction, ReactionHipDrop)
}
BlockBrickBreakable::BlockBrickBreakable(const char* name) : al::LiveActor(name), mClippingCenter(sead::Vector3f::zero), mComboCounter(new al::ComboCounter) {}
void BlockBrickBreakable::init(const al::ActorInitInfo& info) {
    const char* modelName = "BlockBrick";
    alPlacementFunction::tryGetModelName(&modelName, info);
    al::initActorWithArchiveName(this, info, modelName, rc::getBlockSuffixName(info, false));
    al::initNerve(this, &NrvBlockBrickBreakableWait, 0);
    al::offCollide(this);
    al::syncSensorScaleY(this);
    bool light = true;
    al::tryGetArg(&light, info, "IsValidLightPrePass");
    if (!light) al::killPrePassLightAll(this, -1);
    al::tryGetArg(&mAppearBreakModel, info, "IsAppearBreakModel");
    if (mAppearBreakModel) {
        al::StringTmp<128> breakName("%sBreak", modelName);
        mBreakModel = new al::BreakModel(this, "レンガブロック壊れモデル", breakName.cstr(), nullptr, nullptr, "Break", true);
        al::initCreateActorNoPlacementInfo(mBreakModel, info);
        al::setScale(mBreakModel, al::getScale(this));
    }
    mConnector = al::tryCreateMtxConnector(this, info);
    mSnowCover = SnowCoverFunction::tryCreateSnowCover(this, info, "BlockSnowCover", true, nullptr);
    float shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, info, "ShadowLength");
    if (shadowLength > 0.0f) al::setShadowDropLength(this, shadowLength, "シャドウマスク");
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength");
    if (expandClipping) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
    al::invalidateHitSensor(this, "UpperPunch");
    if (al::listenStageSwitchOnOffAppear(this, al::FunctorV0M(this, &BlockBrickBreakable::respawn), al::FunctorV0M(this, &BlockBrickBreakable::killBySwitch))) makeActorDead();
    else makeActorAppeared();
}
void BlockBrickBreakable::killBySwitch() { if (mAppearBreakModel) mBreakModel->kill(); kill(); }
void BlockBrickBreakable::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    al::updateMaterialCodeWater(this);
}
void BlockBrickBreakable::respawn() {
    if (al::isDead(this)) {
        al::validateCollisionParts(this);
        al::validateHitSensors(this);
        al::validateClipping(this);
        al::setNerve(this, &NrvBlockBrickBreakableWait);
        al::showModelIfHide(this);
        makeActorAppeared();
    }
}
void BlockBrickBreakable::control() { if (mConnector) al::connectPoseQT(this, mConnector); }
void BlockBrickBreakable::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) { trySendMsgToUpperLowerObj(receiver, sender); }
bool BlockBrickBreakable::trySendMsgToUpperLowerObj(al::HitSensor* receiver, al::HitSensor* sender) {
    if (al::isNerve(this, &NrvBlockBrickBreakable.Break) || (al::isNerve(this, &NrvBlockBrickBreakable.Reaction) && al::isLessStep(this, 5))) {
        if (mSendToBothSides) {
            if (rc::trySendMsgBlockToUpperObj(receiver, sender, 0, mComboCounter)) return true;
            return rc::trySendMsgBlockToLowerObj(receiver, sender, mComboCounter);
        }
        rc::trySendMsgBlockToUpperObj(receiver, sender, mControlUserId, mComboCounter);
    }
    if (al::isNerve(this, &NrvBlockBrickBreakable.BreakHipDrop) || (al::isNerve(this, &NrvBlockBrickBreakable.ReactionHipDrop) && al::isLessStep(this, 5)))
        rc::trySendMsgBlockToLowerObj(receiver, sender, mComboCounter);
    return false;
}
bool BlockBrickBreakable::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (rc::isMsgAskControlUserId(msg, mControlUserId)) return true;
    if (al::isNerve(this, &NrvBlockBrickBreakable.Break) || al::isNerve(this, &NrvBlockBrickBreakable.BreakHipDrop)) return false;
    if (!rc::isMsgForBlockAll(msg, sender, receiver, 100.0f)) return false;
    if (!isBreakable(msg, sender)) {
        al::startAction(this, al::isMsgPlayerHipDropAll(msg) ? "ReactionHipDrop" : "Reaction");
        mControlUserId = rc::tryFindRelativeControlUserId(sender);
        if (mSnowCover) mSnowCover->tryBreak();
        al::setNerve(this, &NrvBlockBrickBreakable.Reaction);
        if (al::isMsgPlayerHipDropAll(msg)) al::setNerve(this, &NrvBlockBrickBreakable.ReactionHipDrop);
        al::validateHitSensor(this, "UpperPunch");
        if (al::isNerve(this, &NrvBlockBrickBreakable.Reaction)) al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0, 100, 0));
        else if (al::isNerve(this, &NrvBlockBrickBreakable.ReactionHipDrop)) {
            al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0, 0, 0));
            return false;
        }
        return rc::getMsgReturnValueForBlock(msg);
    }
    rc::addScoreByFactor(this, sender, "壊れ", 0.0f, 0);
    mControlUserId = rc::tryFindRelativeControlUserId(sender);
    doBreak(al::isMsgPlayerGiantTouch(msg) || al::isMsgLaserAttack(msg));
    if (al::isMsgPlayerHipDropAll(msg)) al::setNerve(this, &NrvBlockBrickBreakable.BreakHipDrop);
    al::validateHitSensor(this, "UpperPunch");
    al::setSensorRadius(this, "UpperPunch", 50.0f);
    if (al::isNerve(this, &NrvBlockBrickBreakable.Break)) al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0, 100, 0));
    else al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0, 0, 0));
    return true;
}
bool BlockBrickBreakable::isBreakable(const al::SensorMsg* msg, al::HitSensor* sender) const {
    if (!al::isSensorPlayer(sender)) return true;
    if (al::isMsgPlayerGiantTouch(msg) || al::isMsgDokanBazookaAttack(msg) || al::isMsgLaserAttack(msg)) return true;
    return !rc::isPlayerMini(sender);
}
void BlockBrickBreakable::doBreak(bool immediate) {
    al::tryOnSwitchDeadOn(this);
    if (mAppearBreakModel) {
        mBreakModel->appear();
        al::updatePoseRotate(mBreakModel, sead::Vector3f::zero);
        al::addRotateAndRepeatY(mBreakModel, al::getRandomDegree());
        al::updateMaterialCodeWater(mBreakModel);
    } else al::startHitReaction(this, "Break");
    if (mSnowCover) mSnowCover->tryBreak();
    if (immediate) kill();
    else {
        al::hideModel(this);
        al::invalidateCollisionParts(this);
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBlockBrickBreakable.Break);
    }
}
bool BlockBrickBreakable::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvBlockBrickBreakable.Break) || al::isNerve(this, &NrvBlockBrickBreakable.BreakHipDrop)) return false;
    if (!al::isMsgTouchAssistTrig(msg)) return false;
    rc::addScoreByFactor(this, pointer, "壊れ", 0.0f, 0);
    mControlUserId = rc::tryFindRelativeControlUserId(this, pointer);
    al::validateHitSensor(this, "UpperPunch");
    al::setSensorRadius(this, "UpperPunch", 60.0f);
    al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0, 50, 0));
    doBreak(false);
    return true;
}
void BlockBrickBreakable::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
void BlockBrickBreakable::exeReaction() {
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBlockBrickBreakableWait);
        al::invalidateHitSensor(this, "UpperPunch");
    }
}
void BlockBrickBreakable::exeBreak() { if (al::isStep(this, 5)) kill(); }
