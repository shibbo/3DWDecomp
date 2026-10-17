#include "MapObj/BlockSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(BlockSwitch, OffWait);
    NERVE_DECL(BlockSwitch, OnWait);
    NERVE_DECL(BlockSwitch, Off);
    NERVE_DECL(BlockSwitch, On);
    NERVES_MAKE_NOSTRUCT(BlockSwitch, OffWait, OnWait, Off, On)
    inline int hitDelay(const al::SensorMsg* msg) {
        if (al::isMsgPlayerSpinAttack(msg)) return 40;
        if (al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerClimbAttack(msg)) return 20;
        if (al::isMsgExplosion(msg) || al::isMsgExplosionCollide(msg)) return 20;
        return 10;
    }
}
BlockSwitch::BlockSwitch(const char* name) : al::LiveActor(name) {}
BlockSwitch::~BlockSwitch() {}
void BlockSwitch::init(const al::ActorInitInfo& info) {
    al::initActorSuffix(this, info, al::isSingleMode(info) ? "SM" : nullptr);
    al::initNerve(this, &NrvBlockSwitchOffWait, 0);
    al::listenStageSwitchOnOff(this, "SwitchOnOff", al::Functor(this, &BlockSwitch::listenSwitchOn), al::Functor(this, &BlockSwitch::listenSwitchOff));
    al::tryGetArg(&mDisplayRotateX, info, "DisplayRotateX");
    if (!al::isNearZero(mDisplayRotateX, 0.001f)) {
        al::initJointControllerKeeper(this, 1);
        al::initJointLocalXRotator(this, &mDisplayRotateX, "Rotate");
    }
    al::appearPrePassLight(this, "Off", 0);
    al::killPrePassLight(this, "On", 0);
    makeActorAppeared();
}
void BlockSwitch::listenSwitchOn() {
    if (al::isNerve(this, &NrvBlockSwitchOff) || al::isNerve(this, &NrvBlockSwitchOffWait)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBlockSwitchOn);
    }
}
void BlockSwitch::listenSwitchOff() {
    if (al::isNerve(this, &NrvBlockSwitchOn) || al::isNerve(this, &NrvBlockSwitchOnWait)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBlockSwitchOff);
    }
}
bool BlockSwitch::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (mHitDelay > 0) return false;
    if (!rc::isMsgForBlockAll(msg, sender, receiver, 150.0f)) return false;
    if (al::isSingleMode(this)) {
        if (al::isMsgLaserAttack(msg)) return false;
        if (al::isMsgPlayerGiantTouch(msg) && al::isSensorRide(sender)) return false;
    }
    if (al::isNerve(this, &NrvBlockSwitchOffWait)) {
        listenSwitchOn();
        if (al::isMsgPlayerHipDropAll(msg)) {
            if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) mHitDelay = 70;
            return false;
        }
        mHitDelay = hitDelay(msg);
        return rc::getMsgReturnValueForBlock(msg);
    }
    if (al::isNerve(this, &NrvBlockSwitchOnWait)) {
        mHitDelay = hitDelay(msg);
        listenSwitchOff();
        if (al::isMsgPlayerHipDropAll(msg)) {
            if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) mHitDelay = 70;
            return false;
        }
        return rc::getMsgReturnValueForBlock(msg);
    }
    return false;
}
void BlockSwitch::control() { if (mHitDelay - 1 >= 0) --mHitDelay; }
void BlockSwitch::exeOff() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLight(this, "Off", 10);
        al::killPrePassLight(this, "On", 10);
        al::startAction(this, "Off");
        al::tryOffStageSwitch(this, "SwitchOnOff");
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBlockSwitchOffWait);
}
void BlockSwitch::exeOffWait() { if (al::isFirstStep(this)) al::startAction(this, "OffWait"); }
void BlockSwitch::exeOn() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLight(this, "On", 10);
        al::killPrePassLight(this, "Off", 10);
        al::startAction(this, "On");
        al::tryOnStageSwitch(this, "SwitchOnOff");
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBlockSwitchOnWait);
}
void BlockSwitch::exeOnWait() { if (al::isFirstStep(this)) al::startAction(this, "OnWait"); }
