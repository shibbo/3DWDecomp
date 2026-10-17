#include "Game/MapObj/IslandFlag.hpp"
#include "Game/MapObj/GoalItem.hpp"
#include "Game/System/Data/SingleModeDataFunction.hpp"
#include "Game/System/GameDataHolderAccessor.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include <container/seadSafeArray.h>
namespace {
NERVE_DECL(IslandFlag, CameraIn)
NERVE_DECL(IslandFlag, CameraOut)
NERVES_MAKE_NOSTRUCT(IslandFlag, CameraIn, CameraOut)
struct FlagSpringParam {
    sead::Vector3f pos;
    float stability;
    float friction;
    float limit;
};
const FlagSpringParam cFlagSpringParams[] = {
    {{0.0f, 100.0f, 0.0f}, 0.9f, 0.85f, 20.0f}, {{0.0f, 100.0f, 10.0f}, 0.8f, 0.4f, 40.0f},
    {{0.0f, 100.0f, 20.0f}, 0.8f, 0.4f, 60.0f}, {{0.0f, 100.0f, 30.0f}, 0.8f, 0.4f, 60.0f},
    {{0.0f, 100.0f, 20.0f}, 0.8f, 0.4f, 60.0f}, {{0.0f, 100.0f, 10.0f}, 0.8f, 0.4f, 60.0f},
};
const char* const cFlagEffects[4][2] = {
    {nullptr, nullptr}, {"Wait", "WaitLod01"}, {"Wait_02", "WaitLod02"}, {"Wait_03", "WaitLod03"}};
const sead::SafeArray<const char*, 5> cFlagActions = {
    {"After_1", "After_2", "After_3", "After_4", "After_5"}};
} // namespace
IslandFlag::IslandFlag(const char* name)
    : FlingPole(name, false), mPlacementId(new al::PlacementId()) {
    for (int i = 0; i < 5; ++i) {
        mCameraInStep[i] = 40;
        mCameraHoldStep[i] = 60;
        mCameraOutStep[i] = 20;
    }
}
void IslandFlag::init(const al::ActorInitInfo& info) {
    FlingPole::init(info);
    if (!al::tryGetPlacementID(mPlacementId, info)) {
        makeActorDead();
        return;
    }
    al::listenStageSwitchOn(
        this, "SwitchChange",
        al::FunctorV0M<IslandFlag*, void (IslandFlag::*)()>(this, &IslandFlag::changeFlag));
    int count = al::calcLinkChildNum(info, "GoalItem");
    if (count != 0) {
        int zone = mPlacementHolder->getZoneNo();
        mGoalItems.allocBuffer(count, nullptr);
        for (int i = 0; i < count; ++i) {
            GoalItem* goal = new GoalItem("GoalItem");
            al::initLinksActor(goal, info, "GoalItem", i);
            mGoalItems.pushBack(goal);
            goal->setIslandId(zone);
            if (SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), zone - 1,
                                                           goal->getShineId() - 1)) {
                ++mCompletedScenario;
                setScenarioAnim();
            }
        }
    }
    mCamera = al::initObjectCamera_RS(this, info, nullptr);
    al::tryGetArg(&mCameraInStep[0], info, "FocusCameraInStep");
    al::tryGetArg(&mCameraOutStep[0], info, "FocusCameraOutStep");
    al::tryGetArg(&mCameraHoldStep[0], info, "FocusCameraHoldStep");
    al::tryGetArg(&mCameraInStep[1], info, "FocusCameraInStep_2");
    al::tryGetArg(&mCameraOutStep[1], info, "FocusCameraOutStep_2");
    al::tryGetArg(&mCameraHoldStep[1], info, "FocusCameraHoldStep_2");
    al::tryGetArg(&mCameraInStep[2], info, "FocusCameraInStep_3");
    al::tryGetArg(&mCameraOutStep[2], info, "FocusCameraOutStep_3");
    al::tryGetArg(&mCameraHoldStep[2], info, "FocusCameraHoldStep_3");
    al::tryGetArg(&mCameraInStep[3], info, "FocusCameraInStep_4");
    al::tryGetArg(&mCameraOutStep[3], info, "FocusCameraOutStep_4");
    al::tryGetArg(&mCameraHoldStep[3], info, "FocusCameraHoldStep_4");
    al::tryGetArg(&mCameraInStep[4], info, "FocusCameraInStep_5");
    al::tryGetArg(&mCameraOutStep[4], info, "FocusCameraOutStep_5");
    al::tryGetArg(&mCameraHoldStep[4], info, "FocusCameraHoldStep_5");
    al::CameraPoser_RS* poser = mCamera->getPoser();
    poser->setInterpoleStep(mCameraInStep[0]);
    poser->setEndInterpoleStep(mCameraOutStep[0]);
    if (mCompletedScenario == -1)
        al::startAction(this, "Before");
    else
        al::startAction(this, "After_1");
}
void IslandFlag::changeFlag() {
    if (mCompletedScenario >= 5) {
        startNrvWait();
        return;
    }
    if (!mCameraStarted && rc::requestStartDemoCamera(this, nullptr)) {
        al::startCamera_RS(this, mCamera, -1);
        al::CameraPoser_RS* poser = mCamera->getPoser();
        if (al::isEqualString(poser->getName(), "FixedActor")) {
            auto* fixed = static_cast<al::CameraPoserFixActor*>(poser);
            fixed->mTargetActor = this;
            fixed->mOffset.set(0.0f, 300.0f, 0.0f);
            fixed->mDistance = 3200.0f;
        }
        int scenario = SingleModeDataFunction::getCurActiveScenarioIndex(
            GameDataHolderAccessor(this), mPlacementHolder->getZoneNo() - 1);
        if (scenario >= -1) {
            mCameraScenario = scenario + 1;
            poser->setInterpoleStep(mCameraInStep[mCameraScenario]);
            poser->setEndInterpoleStep(mCameraOutStep[mCameraScenario]);
        }
        al::setNerve(this, &NrvIslandFlagCameraIn);
        mCameraStarted = true;
        mCameraDone = false;
    }
}
void IslandFlag::setScenarioAnim() {
    int zone = mPlacementHolder->getZoneNo();
    int scenario =
        SingleModeDataFunction::getCurActiveScenarioIndex(GameDataHolderAccessor(this), zone - 1);
    int count = SingleModeDataFunction::getScenarioNum(GameDataHolderAccessor(this), zone - 1);
    mPreviousFlag = mFlag;
    if (scenario == 0 && count > 1)
        mFlag = 2;
    else if (scenario == 1 && count > 2)
        mFlag = 3;
    else if (scenario >= 2)
        mFlag = 4;
    updateLod();
}
void IslandFlag::initJointKeeper() {
    al::initJointControllerKeeper(this, 18);
    mStickRotators.allocBuffer(6, nullptr);
    const char* const sticks[] = {"Stick1", "Stick2", "Stick3", "Stick4", "Stick5", "Stick6"};
    const char* const flags[] = {"JntFlag1", "JntFlag2", "JntFlag3",
                                 "JntFlag4", "JntFlag5", "JntFlag6"};
    for (int i = 0; i < 6; ++i) {
        mStickRotators.pushBack(
            al::initJointLocalAxisRotator_RS(this, mRotateAxis, &mRotateAngle, sticks[i], false));
        al::initJointLocalXRotator(this, &mStickXAngle, sticks[i]);
    }
    for (int i = 0; i < 6; ++i) {
        auto* spring = al::initJointSpringController(this, flags[i]);
        spring->setChildLocalPos(cFlagSpringParams[i].pos);
        spring->setStability(cFlagSpringParams[i].stability);
        spring->setFriction(cFlagSpringParams[i].friction);
        spring->setLimitDegree(cFlagSpringParams[i].limit);
    }
}
void IslandFlag::control() {
    FlingPole::control();
    if (isBindPole() || isOnTop()) {
        sead::Vector3f flagPos, playerPos;
        al::calcJointPos(&flagPos, this, "JntFlag1");
        if (calcPuppetTrans(&playerPos) && playerPos.y > flagPos.y - 300.0f)
            al::tryStartMclAnimIfNotPlaying(this, "FlagHide");
        else
            al::tryStartMclAnimIfNotPlaying(this, "FlagShow");
    } else if (isIdle())
        al::tryStartMclAnimIfNotPlaying(this, "FlagShow");
    updateLod();
}
void IslandFlag::updateLod() {
    if (mFlag == 4) {
        if (mPreviousFlag != 4) {
            al::tryDeleteEffect(this, "Wait_02");
            al::tryDeleteEffect(this, "Wait_03");
            al::tryDeleteEffect(this, "WaitLod02");
            al::tryDeleteEffect(this, "WaitLod03");
        }
        return;
    }
    bool previousLod = mLod[mPreviousFlag];
    bool lod = isLod();
    mLod[mFlag] = lod;
    if (mPreviousFlag != mFlag || previousLod != lod) {
        const char* previous = cFlagEffects[mPreviousFlag][previousLod];
        const char* current = cFlagEffects[mFlag][lod];
        if (previous)
            al::tryDeleteEffect(this, previous);
        if (current)
            al::tryEmitEffect(this, current, nullptr);
        mPreviousFlag = mFlag;
    }
}
bool IslandFlag::isLod() const {
    return (al::getCameraPos_RS(this, 0) - al::getTrans(this)).squaredLength() > 12000000.0f;
}
void IslandFlag::exeCameraIn() {
    if (al::isStep(this, mCameraInStep[mCameraScenario])) {
        ++mCompletedScenario;
        if (mCompletedScenario <= 4)
            al::startAction(this, cFlagActions[mCompletedScenario]);
    }
    if (al::isGreaterEqualStep(this,
                               mCameraInStep[mCameraScenario] + mCameraHoldStep[mCameraScenario])) {
        al::setNerve(this, &NrvIslandFlagCameraOut);
        al::endCamera_RS(this, mCamera, -1, false);
    }
}
void IslandFlag::exeCameraOut() {
    if (al::isGreaterEqualStep(this, mCameraOutStep[0])) {
        startNrvWait();
        mCameraStarted = false;
        mCameraDone = true;
        rc::requestEndDemoCamera(this);
    }
}
bool IslandFlag::isCameraSequenceStarted() { return mCameraStarted; }
bool IslandFlag::isCameraSequenceDone() { return mCameraDone; }
