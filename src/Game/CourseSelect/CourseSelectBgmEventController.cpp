#include "CourseSelect/CourseSelectBgmEventController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

namespace {
NERVE_DECL(CourseSelectBgmEventController, CheckWorldStartDemo);
NERVES_MAKE_NOSTRUCT(CourseSelectBgmEventController, CheckWorldStartDemo)
}

/**
 * @brief Create an actor that signals course-select music events through stage switches.
 * @param pName Actor name.
 */
CourseSelectBgmEventController::CourseSelectBgmEventController(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the event type and wait for the requested world's opening demo.
 * @param rInfo Placement containing EventType, WorldId, and stage-switch connections.
 */
void CourseSelectBgmEventController::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRMSV(this);
    al::initActorSRT(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    al::initActorClipping(this, rInfo);
    al::invalidateClipping(this);

    if (!al::tryGetArg(&mEventType, rInfo, "EventType")) {
        mEventType = 0;
    }
    if (mEventType == 0) {
        if (!al::tryGetArg(&mWorldId, rInfo, "WorldId")) {
            mWorldId = 0;
            kill();
        }
        if (GameDataFlagFunction::isShowWorldStartDemo(GameDataHolderAccessor(this), mWorldId)) {
            al::tryOnStageSwitch(this, "SwitchWorldStartDemoWatchedOn");
            kill();
        } else {
            al::initNerve(this, &NrvCourseSelectBgmEventControllerCheckWorldStartDemo, 0);
        }
    } else {
        kill();
    }
    makeActorAppeared();
}

/** @brief Turn on the watched-demo switch and stop checking once the world demo is recorded. */
void CourseSelectBgmEventController::exeCheckWorldStartDemo() {
    if (GameDataFlagFunction::isShowWorldStartDemo(GameDataHolderAccessor(this), mWorldId)) {
        al::tryOnStageSwitch(this, "SwitchWorldStartDemoWatchedOn");
        kill();
    }
}

/** @brief Destroy the course-select event controller. */
CourseSelectBgmEventController::~CourseSelectBgmEventController() = default;
