#include "Demo/DemoTimerStageSwitchController.hpp"
#include "Demo/DemoTimerStageSwitchInfo.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
const char* const cSwitchNames[] = {
    "Switch1On", "Switch2On", "Switch3On", "Switch4On",
    "Switch5On", "Switch6On", "Switch7On", "Switch8On"
};
}

/** @brief Reads one scheduled switch transition. @param rInfo Linked transition placement. */
inline DemoTimerStageSwitchInfo::DemoTimerStageSwitchInfo(const al::PlacementInfo& rInfo) {
    int switchId = 0;
    al::tryGetArg(&switchId, rInfo, "SwitchId");
    mSwitchName = cSwitchNames[switchId];
    al::tryGetArg(&mStep, rInfo, "Step");
    al::tryGetArg(&mIsOn, rInfo, "IsOn");
}

/** @brief Creates a timed switch controller. @param pName Actor name. */
DemoTimerStageSwitchController::DemoTimerStageSwitchController(const char* pName) : al::LiveActor(pName) {}

/** @brief Loads transitions and establishes each switch's initial state. @param rInfo Actor initialization data. */
void DemoTimerStageSwitchController::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    mInfoCount = al::calcLinkChildNum(rInfo, "StageSwitchInfo");
    mInfos = new DemoTimerStageSwitchInfo*[mInfoCount];
    for (int i = 0; i < mInfoCount; ++i) {
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, al::getPlacementInfo(rInfo), "StageSwitchInfo", i);
        mInfos[i] = new DemoTimerStageSwitchInfo(placement);
        if (mMaxStep < mInfos[i]->mStep) mMaxStep = mInfos[i]->mStep;
    }
    for (int switchId = 0; switchId < 8; ++switchId) {
        const char* switchName = cSwitchNames[switchId];
        DemoTimerStageSwitchInfo* first = nullptr;
        for (int i = 0; i < mInfoCount; ++i) {
            if (al::isEqualString(switchName, mInfos[i]->mSwitchName)) {
                if (!first || mInfos[i]->mStep < first->mStep) first = mInfos[i];
            }
        }
        if (first && !first->mIsOn) al::onStageSwitch(this, switchName);
    }
    if (auto* director = CourseSelectDirector::tryGetCourseSelectDirector(this)) {
        director->setDemoTimerStageSwitchController(this);
        makeActorDead();
    } else {
        al::trySyncStageSwitchAppear(this);
    }
}

/** @brief Activates the controller through the standard actor path. */
void DemoTimerStageSwitchController::appear() { al::LiveActor::appear(); }

/** @brief Applies transitions with negative frame numbers before playback. @param isOn Whether to apply their configured state or its inverse. */
void DemoTimerStageSwitchController::prepare(bool isOn) {
    for (int i = 0; i < mInfoCount; ++i) {
        if (mInfos[i]->mStep < 0) {
            if (mInfos[i]->mIsOn == isOn) al::onStageSwitch(this, mInfos[i]->mSwitchName);
            else al::offStageSwitch(this, mInfos[i]->mSwitchName);
        }
    }
}

/** @brief Applies the current frame's transitions and stops after the last scheduled frame. */
void DemoTimerStageSwitchController::control() {
    for (int i = 0; i < mInfoCount; ++i) {
        if (mStep == mInfos[i]->mStep) {
            if (mInfos[i]->mIsOn) al::onStageSwitch(this, mInfos[i]->mSwitchName);
            else al::offStageSwitch(this, mInfos[i]->mSwitchName);
        }
    }
    int step = mStep++;
    if (mMaxStep <= step) kill();
}
