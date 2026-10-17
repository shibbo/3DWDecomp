#include "MapObj/AssistSlideMapParts.hpp"
#include "MapObj/AssistSlideMapPartsGroup.hpp"
#include "Layout/GuideBalloon.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/MapObj/EffectMtxSetter.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"

namespace {
NERVE_ACTION_IMPL(AssistSlideMapParts, OffWait)
NERVE_ACTION_IMPL(AssistSlideMapParts, On)
NERVE_ACTION_IMPL(AssistSlideMapParts, OnWait)
NERVE_ACTION_IMPL(AssistSlideMapParts, Off)
NERVE_ACTION_IMPL(AssistSlideMapParts, OffSign)
NERVE_ACTIONS_MAKE_STRUCT(AssistSlideMapParts, OffWait, On, OnWait, Off, OffSign)
}

AssistSlideMapParts::AssistSlideMapParts(const char* name) : al::LiveActor(name) {}
AssistSlideMapParts::~AssistSlideMapParts() {}

void AssistSlideMapParts::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "OffWait", &NrvAssistSlideMapParts.collector, 0);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, info, nullptr, 0);
    al::registerAreaHostMtx(this, info);
    al::tryGetArg(&mMoveAxis, info, "MoveAxis");
    al::tryGetArg(&mMoveDistance, info, "MoveDistance");
    al::tryGetArg(&mMoveSpeed, info, "MoveSpeed");
    al::tryGetArg(&mMoveTime, info, "MoveTime");
    al::tryGetArg(&mOnWaitTime, info, "OnWaitTime");
    al::tryGetArg(&mMoveType, info, "MoveType");
    al::tryGetArg(&mGuideBalloonType, info, "GuideBalloonType");
    if (mGuideBalloonType != 0) {
        sead::Vector3f pos = al::getTrans(this);
        al::tryGetLinksTrans(&pos, info, "GuideBalloonPosition");
        mGuideBalloon = new GuideBalloon("ガイドバルーン", al::getLayoutInitInfo(info),
            al::getTransPtr(this), pos - al::getTrans(this), false, nullptr);
    }
    mConnector = al::tryCreateMtxConnector(this, info);
    mEffectMtxSetter = al::tryCreateEffectMtxSetter(this, "EffectMtxSetter");
    if (mEffectMtxSetter) mEffectMtxSetter->setMtxPtr(&mEffectMtx, "BaseEffectMtx");
    mInitialQuat.set(al::getQuat(this));
    mInitialTrans.set(al::getTrans(this));
    mEffectMtx.makeQT(mInitialQuat, mInitialTrans);
    al::trySyncStageSwitchAppear(this);
}

void AssistSlideMapParts::initAfterPlacement() {
    if (mConnector) {
        sead::Vector3f axis;
        al::calcQuatLocalAxis(&axis, mInitialQuat, mMoveAxis);
        al::attachMtxConnectorToCollision(mConnector, this,
            al::getTrans(this) + axis * 50.0f, axis * -400.0f);
    }
}

bool AssistSlideMapParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgPlayerFloorTouch(msg)) mFloorTouchFrames = 10;
    if (al::isMsgTouchAssist(msg)) {
        if (mGroupHost) mGroupHost->requestTouchAssist();
        else touchAssist();
        return true;
    }
    return false;
}

void AssistSlideMapParts::touchAssist() {
    if (al::isNerve(this, NrvAssistSlideMapParts.OffWait.data())) {
        al::invalidateClipping(this);
        al::startNerveAction(this, "On");
    } else if ((al::isNerve(this, NrvAssistSlideMapParts.OnWait.data()) ||
                al::isNerve(this, NrvAssistSlideMapParts.OffSign.data())) &&
               al::isGreaterEqualStep(this, 30)) {
        if (isTypeOnOff()) al::invalidateClipping(this);
        al::tryStartSe(this, "TouchOff");
        al::startNerveAction(this, "Off");
    }
}

void AssistSlideMapParts::startClipped() {
    al::LiveActor::startClipped();
    if (mGuideBalloon && al::isNerve(this, NrvAssistSlideMapParts.OffWait.data()))
        mGuideBalloon->endShow();
}
void AssistSlideMapParts::setGroupHost(AssistSlideMapPartsGroup* group) { mGroupHost = group; }
bool AssistSlideMapParts::isTypeOnOff() const { return mMoveType == 1; }
bool AssistSlideMapParts::isTypeTimerOff() const { return mMoveType == 0; }

void AssistSlideMapParts::control() {
    if (mFloorTouchFrames - 1 >= 0) --mFloorTouchFrames;
    al::setQuat(this, mInitialQuat);
    al::setTransOffsetLocalDir(this, mInitialQuat, mInitialTrans, mMoveDistance * mMoveRate, mMoveAxis);
    if (mConnector) {
        al::calcConnectMtx(&mEffectMtx, mConnector, mInitialQuat, mInitialTrans);
        al::connectPoseQT(this, mConnector, mInitialQuat, al::getTrans(this));
    }
}

void AssistSlideMapParts::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryStartAction(this, "OffWaitAnim");
        mMoveRate = 0.0f;
    }
    if (!mGuideBalloon) return;
    if (al::isActorObscured(this, -1.0f, &mGuideBalloon->getOffset())) {
        if (mGuideBalloon->isAlive()) mGuideBalloon->endShow();
    } else if (mGuideBalloonType == 1) {
        if (mGuideBalloon->isAlive()) {
            if (!al::isNearPlayer(this, 2800.0f)) mGuideBalloon->endShow();
        } else if (al::isNearPlayer(this, 2500.0f)) mGuideBalloon->startShowDrcTouch(false);
    } else if (mGuideBalloonType == 2) {
        if (mGuideBalloon->isAlive()) {
            if (mFloorTouchFrames <= 0) mGuideBalloon->endShow();
        } else if (mFloorTouchFrames >= 1) mGuideBalloon->startShowDrcTouch(false);
    }
}

s32 AssistSlideMapParts::calcMoveTime() const {
    if (mMoveTime >= 0) return mMoveTime;
    if (mMoveSpeed < 1.0f) return 0;
    float time = mMoveDistance / mMoveSpeed;
    return static_cast<s32>(time > 0.0f ? time : -time);
}

void AssistSlideMapParts::exeOn() {
    if (al::isFirstStep(this)) {
        if (mGuideBalloon) mGuideBalloon->endShow();
        al::tryStartAction(this, "OnAnim");
    }
    s32 time = calcMoveTime();
    mMoveRate = al::calcNerveRate(this, time);
    if (al::isGreaterEqualStep(this, time)) al::startNerveAction(this, "OnWait");
}

void AssistSlideMapParts::exeOnWait() {
    if (al::isFirstStep(this)) {
        if (isTypeOnOff()) al::validateClipping(this);
        al::tryOnStageSwitch(this, "SwitchSlideOn");
        al::tryStartAction(this, "OnWaitAnim");
        mMoveRate = 1.0f;
    }
    if (isTypeTimerOff() && al::isGreaterEqualStep(this, mOnWaitTime))
        al::startNerveAction(this, "OffSign");
}

void AssistSlideMapParts::exeOffSign() {
    if (al::isFirstStep(this)) al::tryStartAction(this, "OffSignAnim");
    if (al::isGreaterEqualStep(this, 120)) al::startNerveAction(this, "Off");
}

void AssistSlideMapParts::exeOff() {
    if (al::isFirstStep(this)) al::tryStartAction(this, "OffAnim");
    s32 time = calcMoveTime();
    mMoveRate = 1.0f - al::calcNerveRate(this, time);
    if (al::isGreaterEqualStep(this, time)) {
        al::tryStartSe(this, "OffEnd");
        al::startNerveAction(this, "OffWait");
    }
}
