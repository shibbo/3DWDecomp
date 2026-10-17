#include "MapObj/ActorStateDemoCamera.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTicketId.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Util/DemoUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(ActorStateDemoCamera, Prepare)
NERVE_DECL(ActorStateDemoCamera, PlayWait)
NERVE_DECL(ActorStateDemoCamera, Play)
NERVE_DECL(ActorStateDemoCamera, PlayEnd)
NERVES_MAKE_NOSTRUCT(ActorStateDemoCamera, Prepare)
NERVES_MAKE_STRUCT(ActorStateDemoCamera, PlayEnd, Play, PlayWait)
}

ActorStateDemoCameraParam::ActorStateDemoCameraParam(int playTime, int waitTime, int interpoleTime,
                                                   const sead::Vector3f* pos, const sead::Vector3f* at)
    : _0(waitTime), _4(playTime), _8(interpoleTime), _10(pos), _18(at),
      mPositionOffset(0, 0, 0), mCameraOffset(0, 0, 0) {}
ActorStateDemoCamera::ActorStateDemoCamera(al::LiveActor* actor, const al::ActorInitInfo& info,
                                         const char* name, const ActorStateDemoCameraParam* param,
                                         bool programable)
    : al::ActorStateBase("デモカメラ使用状態", actor), mParam(param), mEndCameraName("") {
    initNerve(&NrvActorStateDemoCameraPrepare, 0);
    mSingleMode = actor->getSceneInfo()->isSingleMode;
    if (mSingleMode) {
        if (programable)
            mCameraTicket = al::initProgramableCamera_RS(actor, info, name, &mCameraPos, &mCameraAt, nullptr);
        else
            mCameraTicket = al::initObjectCamera_RS(actor, info, nullptr);
    } else {
        if (programable) mCamera = al::initProgramableCamera(actor, name);
        else mCamera = al::initObjectCamera(actor, info, name);
        al::isEqualCameraPoserName("Programable", mCamera);
        mEndCameraName.format("%sEnd", name);
        mEndCamera = al::initProgramableCamera(actor, mEndCameraName.cstr());
    }
    mStartArea = al::createLinkAreaGroup(mHostActor, info, "PlayerAllInCheckArea",
                                       "プレイヤー全員居るかチェックエリアグループ", "子供エリア");
}
int ActorStateDemoCamera::getPlayStep() const {
    if (al::isNerve(this, &NrvActorStateDemoCamera.PlayEnd)) return mParam->_4 + al::getNerveStep(this);
    if (al::isNerve(this, &NrvActorStateDemoCamera.Play)) return al::getNerveStep(this);
    return -1;
}
bool ActorStateDemoCamera::isPlayStep(int step) const { return getPlayStep() == step; }
bool ActorStateDemoCamera::isLessEqualPlayStep(int step) const { return getPlayStep() <= step; }
bool ActorStateDemoCamera::isGreaterEqualPlayStep(int step) const { return getPlayStep() >= step; }
bool ActorStateDemoCamera::tryStart(const al::Nerve* nerve) {
    if (!mStartArea) {
        al::setNerve(mHostActor, nerve);
        return true;
    }
    if (al::isInAreaObjPlayerAll(mHostActor, mStartArea)) {
        al::setNerve(mHostActor, nerve);
        return true;
    }
    return false;
}
void ActorStateDemoCamera::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvActorStateDemoCameraPrepare);
}
void ActorStateDemoCamera::exePrepare() {
    if (rc::requestStartDemoCamera(mHostActor, nullptr)) {
        if (mParam->_0 >= 1) al::setNerve(this, &NrvActorStateDemoCamera.PlayWait);
        else al::setNerve(this, &NrvActorStateDemoCamera.Play);
    }
}
void ActorStateDemoCamera::exePlayWait() {
    if (al::isGreaterEqualStep(this, mParam->_0)) al::setNerve(this, &NrvActorStateDemoCamera.Play);
}
void ActorStateDemoCamera::exePlay() {
    if (al::isFirstStep(this)) {
        if (mSingleMode) {
            if (al::isEqualString("Programable", mCameraTicket->getTicketId()->getObjId())) {
                if (mParam->_10) mCameraPos = *mParam->_10 + mParam->mPositionOffset;
                if (mParam->_18) mCameraAt = *mParam->_18 + mParam->mCameraOffset;
            }
            al::CameraPoser_RS* poser = mCameraTicket->getPoser();
            if (al::isEqualString(poser->getName(), "FixedActor"))
                static_cast<al::CameraPoserFixActor*>(poser)->setTargetActor(mHostActor);
            rc::setOtherActiveDemo(mHostActor, true);
            al::startCamera_RS(mHostActor, mCameraTicket, mParam->_8);
        } else {
            al::setCameraPos(mEndCamera, al::getCameraPos(mHostActor));
            al::setCameraLookAtPos(mEndCamera, al::getCameraLookAt(mHostActor));
            al::setCameraUpDir(mEndCamera, al::getCameraUpDir(mHostActor));
            al::setCameraFovyDegree(mEndCamera, al::getCameraFovyDegree(mHostActor));
            if (al::isEqualCameraPoserName("Programable", mCamera)) {
                if (mParam->_10) al::setCameraPos(mCamera, *mParam->_10 + mParam->mPositionOffset);
                if (mParam->_18) al::setCameraLookAtPos(mCamera, *mParam->_18 + mParam->mCameraOffset);
            }
            al::startCamera(mHostActor, mCamera, mParam->_8);
        }
    }
    if (mSingleMode) {
        if (al::isGreaterEqualStep(this, mParam->_4)) {
            al::endCamera_RS(mHostActor, mCameraTicket, -1, false);
            al::setNerve(this, &NrvActorStateDemoCamera.PlayEnd);
            rc::setOtherActiveDemo(mHostActor, false);
        }
    } else if (al::isGreaterEqualStep(this, mParam->_4)) {
        al::endCamera(mHostActor, mCamera, -1);
        al::setNerve(this, &NrvActorStateDemoCamera.PlayEnd);
    }
}
void ActorStateDemoCamera::exePlayEnd() {
    if (al::isFirstStep(this)) {
        if (mSingleMode) { kill(); return; }
        al::startCamera(mHostActor, mEndCamera, 60);
    }
    if (al::isGreaterEqualStep(this, 60) && !mSingleMode) {
        rc::requestEndDemoCamera(mHostActor);
        al::endCamera(mHostActor, mEndCamera, -1);
        kill();
    }
}
