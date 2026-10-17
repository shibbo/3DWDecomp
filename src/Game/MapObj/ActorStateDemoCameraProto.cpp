#include "MapObj/ActorStateDemoCameraProto.hpp"
#include "Camera/CameraLookAtPoint.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTicketId.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/DemoUtil.hpp"
namespace {
    NERVE_DECL(ActorStateDemoCameraProto, Prepare);
    NERVE_DECL(ActorStateDemoCameraProto, PlayWait);
    NERVE_DECL(ActorStateDemoCameraProto, Play);
    NERVE_DECL(ActorStateDemoCameraProto, PlayEnd);
    NERVE_DECL(ActorStateDemoCameraProto, Done);
    NERVES_MAKE_NOSTRUCT(ActorStateDemoCameraProto, PlayWait, Play)
    NERVES_MAKE_STRUCT(ActorStateDemoCameraProto, Done, Prepare, PlayEnd)
}
ActorStateDemoCameraProto::ActorStateDemoCameraProto(al::LiveActor* actor, const al::ActorInitInfo& info, const char* name, int waitTime, int moveTime, bool intro)
    : al::ActorStateBase("PrototypeCamera", actor) {
    mCameraName = name;
    mCameraPoints = nullptr;
    mPointIndex = 0;
    mPlayStep = 0;
    mStartAt = sead::Vector3f::zero;
    mStartPos = sead::Vector3f::zero;
    mStartWaitTime = waitTime;
    mStartMoveTime = moveTime;
    mCapture = false;
    mAtPoint = false;
    mFinished = false;
    mMoving = false;
    mCameraActive = false;
    mSkip = false;
    mFreeze = false;
    mIntro = intro;
    if (actor->mActorSceneInfo->isSingleMode) {
        mStartAt = al::getCameraAt_RS(mHostActor, 0);
        mCameraAt = mStartAt;
        mStartPos = al::getCameraPos_RS(mHostActor, 0);
        mCameraPos = mStartPos;
        mCameraTicket = al::initProgramableCamera_RS(actor, info, name, &mCameraPos, &mCameraAt, nullptr);
        mCameraTicket->getPoser()->setInterpoleStep(0);
    }
    initNerve(&NrvActorStateDemoCameraProto.Prepare, 0);
}
void ActorStateDemoCameraProto::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvActorStateDemoCameraProto.Prepare);
    mStartAt = al::getCameraAt_RS(mHostActor, 0);
    mStartPos = al::getCameraPos_RS(mHostActor, 0);
}
int ActorStateDemoCameraProto::getPlayStep() const { return 0; }
bool ActorStateDemoCameraProto::isPlayStep(int) const { return false; }
bool ActorStateDemoCameraProto::isLessEqualPlayStep(int) const { return false; }
bool ActorStateDemoCameraProto::isGreaterEqualPlayStep(int) const { return false; }
bool ActorStateDemoCameraProto::tryStart(const al::Nerve*) { return false; }
void ActorStateDemoCameraProto::exePrepare() {
    if (mNoDemo || (mIntro ? rc::requestStartDemoIntro(mHostActor, mCameraTicket->getTicketId()->getObjId(), true) : rc::requestStartDemoCamera(mHostActor, mCameraTicket->getTicketId()->getObjId()))) {
        mPointIndex = 0;
        mPlayStep = 0;
        mAtPoint = false;
        mFinished = false;
        mMoving = true;
        mCameraActive = true;
        al::setNerve(this, &NrvActorStateDemoCameraProtoPlayWait);
        if (mFreeze) rc::setUpdateFreeze(mHostActor, false);
        if (!mNoDemo && !mIntro) rc::setOtherActiveDemo(mHostActor, true);
    }
}
bool ActorStateDemoCameraProto::isCameraMoving() { return mMoving; }
void ActorStateDemoCameraProto::exePlayWait() {
    if (al::isFirstStep(this)) {
        mStartAt = al::getCameraAt_RS(mHostActor, 0);
        mCameraAt = mStartAt;
        mStartPos = al::getCameraPos_RS(mHostActor, 0);
        mCameraPos = mStartPos;
        if (mPlayStep != -1) al::startCamera_RS(mHostActor, mCameraTicket, -1);
        mPlayStep = 1;
        mCameraActive = true;
        return;
    }
    sead::Vector3f pos(sead::Vector3f::zero);
    sead::Vector3f at(sead::Vector3f::zero);
    if (mPlayStep >= mStartWaitTime) {
        if (mPlayStep < mStartWaitTime + mStartMoveTime) {
            float rate = mStartMoveTime == 0 ? 1.0f : float(mPlayStep - mStartWaitTime) / mStartMoveTime;
            mCameraPoints->interpolateIn(mPointIndex, mStartAt, mStartPos, rate, &at, &pos);
        } else {
            mPlayStep = -2;
            mCameraPoints->interpolateIn(mPointIndex, mStartAt, mStartPos, 1.0f, &at, &pos);
            al::setNerve(this, &NrvActorStateDemoCameraProtoPlay);
        }
        mCameraPos = pos;
        mCameraAt = at;
    }
    ++mPlayStep;
}
void ActorStateDemoCameraProto::setCameraPoints(const CameraLookAtPoint* points) { mCameraPoints = points; }
void ActorStateDemoCameraProto::exePlay() {
    if (al::isFirstStep(this)) {
        if (mPlayStep != -1) {
            al::startCamera_RS(mHostActor, mCameraTicket, -1);
            al::resetClippingDistanceStates(mHostActor);
        }
        if (mCameraPoints->getPointCount() == 1) {
            mAtPoint = true;
            mPointIndex = -1;
            al::setNerve(this, &NrvActorStateDemoCameraProto.PlayEnd);
            return;
        }
    }
    if (mCapture) {
        mCapture = false;
        al::requestCaptureScreenCover(mHostActor, 2);
        al::resetClippingDistanceStates(mHostActor);
    }
    int wait = mCameraPoints->getWaitTime(mPointIndex);
    int move = mCameraPoints->getMoveTime(mPointIndex);
    sead::Vector3f pos(sead::Vector3f::zero);
    sead::Vector3f at(sead::Vector3f::zero);
    if (mPlayStep == wait) { mAtPoint = true; mMoving = false; }
    if (mPlayStep >= wait) {
        if (move == 0) {
            mCameraPoints->interpolate(mPointIndex, mPointIndex + 1, 1.0f, &mCameraAt, &mCameraPos);
            mAtPoint = false;
            if (mPointIndex + 2 < mCameraPoints->getPointCount()) {
                ++mPointIndex;
                mPlayStep = 0;
                mCapture = true;
                goto advance;
            }
        } else if (mPlayStep < wait + move) {
            mCameraPoints->interpolate(mPointIndex, mPointIndex + 1, float(mPlayStep - wait) / move, &at, &pos);
            if (mAtPoint && mPlayStep > wait) mAtPoint = false;
            goto apply;
        } else if (mPointIndex + 2 < mCameraPoints->getPointCount()) {
            ++mPointIndex;
            mPlayStep = 0;
            mCapture = true;
            goto advance;
        }
        al::setNerve(this, &NrvActorStateDemoCameraProto.PlayEnd);
        mCameraPoints->interpolateIn(mPointIndex + 1, mStartAt, mStartPos, 1.0f, &at, &pos);
    apply:
        mCameraPos = pos;
        mCameraAt = at;
    }
advance:
    if (mPlayStep == 0) al::resetClippingDistanceStates(mHostActor);
    ++mPlayStep;
}
void ActorStateDemoCameraProto::forceCameraDone() {
    if (mIntro ? rc::isActiveSpecificDemo(mHostActor) : (rc::isAnyActiveDemo(mHostActor) && rc::isActiveDemoCamera(mHostActor))) {
        if (!mNoDemo) {
            if (mIntro) rc::requestEndDemoIntro(mHostActor);
            else {
                rc::setOtherActiveDemo(mHostActor, false);
                rc::requestEndDemoCamera(mHostActor);
            }
        }
        al::endCamera_RS(mHostActor, mCameraTicket, -1, false);
        mCameraActive = false;
        al::setNerve(this, &NrvActorStateDemoCameraProto.Done);
    }
}
void ActorStateDemoCameraProto::jumpToFinish() {
    if (al::isNerve(this, &NrvActorStateDemoCameraProto.PlayEnd) || al::isNerve(this, &NrvActorStateDemoCameraProto.Prepare) || mPlayStep == 0) return;
    al::setNerve(this, &NrvActorStateDemoCameraProto.PlayEnd);
    u32 count = mCameraPoints->getPointCount();
    mPointIndex = count - 2;
    mCameraPoints->interpolateIn(count - 1, mStartAt, mStartPos, 1.0f, &mCameraAt, &mCameraPos);
    mSkip = true;
}
void ActorStateDemoCameraProto::exePlayEnd() {
    if (al::isFirstStep(this)) {
        al::resetClippingDistanceStates(mHostActor);
        ++mPointIndex;
        mPlayStep = 0;
    }
    bool end;
    if (mSkip) end = al::isStep(this, 2);
    else end = al::isStep(this, mCameraPoints->getWaitTime(mPointIndex));
    if (end) {
        mCameraTicket->getPoser()->setEndInterpoleStep(mSkip ? 0 : mCameraPoints->getMoveTime(mPointIndex));
        forceCameraDone();
        mAtPoint = false;
        mFinished = true;
        mMoving = false;
    }
}
void ActorStateDemoCameraProto::exeDone() { kill(); }
