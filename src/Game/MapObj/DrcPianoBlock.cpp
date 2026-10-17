#include "MapObj/DrcPianoBlock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
NERVE_DECL(DrcPianoBlock, OffWait);
NERVE_DECL(DrcPianoBlock, On);
NERVE_DECL(DrcPianoBlock, OnWait);
NERVE_DECL(DrcPianoBlock, Off);
NERVES_MAKE_STRUCT(DrcPianoBlock, OffWait, On, OnWait, Off)
}
DrcPianoBlock::DrcPianoBlock(const char* name) : al::LiveActor(name) {}
DrcPianoBlock::~DrcPianoBlock() {}
void DrcPianoBlock::init(const al::ActorInitInfo& info) {
    al::initActorPoseTQSV(this);
    al::initActorWithArchiveName(this, info, "TestDrcPianoBlock", nullptr);
    al::initNerve(this, &NrvDrcPianoBlock.OffWait, 0);
    al::tryGetArg(&mMoveAxis, info, "MoveAxis");
    al::tryGetArg(&mMoveDistance, info, "MoveDistance");
    al::tryGetArg(&mMoveSpeed, info, "MoveSpeed");
    al::tryGetArg(&mMoveTime, info, "MoveTime");
    al::tryGetArg(&mOnWaitTime, info, "OnWaitTime");
    al::tryGetArg(&mMoveType, info, "MoveType");
    al::tryGetArg(&mNoteId, info, "NoteId");
    mConnector = al::tryCreateMtxConnector(this, info);
    mBaseQuat.set(al::getQuat(this));
    mBaseTrans.set(al::getTrans(this));
    al::trySyncStageSwitchAppear(this);
}
void DrcPianoBlock::initAfterPlacement() {
    if (mConnector) {
        sead::Vector3f axis;
        al::calcQuatLocalAxis(&axis, mBaseQuat, mMoveAxis);
        al::attachMtxConnectorToCollision(mConnector, this, al::getTrans(this) + axis * 50.0f, axis * -400.0f);
    }
}
bool DrcPianoBlock::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgPlayerTouch(msg) || al::isMsgTouchAssist(msg)) {
        if (al::isNerve(this, &NrvDrcPianoBlock.OffWait)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvDrcPianoBlock.On);
        } else {
            if (al::isNerve(this, &NrvDrcPianoBlock.OnWait)) mTouchTimer = 3;
            if (isTypeOnOff() && al::isNerve(this, &NrvDrcPianoBlock.OnWait)) {
                al::invalidateClipping(this);
                al::setNerve(this, &NrvDrcPianoBlock.Off);
            }
        }
        return true;
    }
    return false;
}
bool DrcPianoBlock::isTypeOnOff() const { return mMoveType == 1; }
void DrcPianoBlock::control() {
    al::setTransOffsetLocalDir(this, mBaseQuat, mBaseTrans, mMoveDistance * mMoveRate, mMoveAxis);
    if (mConnector) al::connectPoseQT(this, mConnector, mBaseQuat, al::getTrans(this));
}
void DrcPianoBlock::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryStartAction(this, "OffWait");
        mMoveRate = 0.0f;
    }
}
void DrcPianoBlock::exeOn() {
    if (al::isFirstStep(this)) al::tryStartAction(this, "On");
    int time = calcMoveTime();
    mMoveRate = al::calcNerveRate(this, time);
    if (getAudioKeeper()) al::holdSeSetSeqLoacalVariable(this, "SeOjDrcBlockPianoOnLv", 0, mNoteId);
    if (al::isGreaterEqualStep(this, time)) al::setNerve(this, &NrvDrcPianoBlock.OnWait);
}
int DrcPianoBlock::calcMoveTime() const {
    if (mMoveTime >= 0) return mMoveTime;
    if (mMoveSpeed < 1.0f) return 0;
    float time = mMoveDistance / mMoveSpeed;
    return int(time > 0.0f ? time : -time);
}
void DrcPianoBlock::exeOnWait() {
    if (al::isFirstStep(this)) {
        if (isTypeOnOff()) al::validateClipping(this);
        al::tryStartAction(this, "OnWait");
        mMoveRate = 1.0f;
    }
    if (isTypeTimerOff()) {
        if (mTouchTimer - 1 >= 0) --mTouchTimer;
        if (al::isGreaterEqualStep(this, mOnWaitTime) && mTouchTimer == 0) {
            al::setNerve(this, &NrvDrcPianoBlock.Off);
            return;
        }
    }
    if (getAudioKeeper()) al::holdSeSetSeqLoacalVariable(this, "SeOjDrcBlockPianoOnLv", 0, mNoteId);
}
bool DrcPianoBlock::isTypeTimerOff() const { return mMoveType == 0; }
void DrcPianoBlock::exeOff() {
    if (al::isFirstStep(this)) al::tryStartAction(this, "Off");
    int time = calcMoveTime();
    mMoveRate = 1.0f - al::calcNerveRate(this, time);
    if (al::isGreaterEqualStep(this, time)) al::setNerve(this, &NrvDrcPianoBlock.OffWait);
}
