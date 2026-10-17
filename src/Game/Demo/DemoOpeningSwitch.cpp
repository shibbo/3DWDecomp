#include "Demo/DemoOpeningSwitch.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
const char* const cOpeningSwitchNames[] = {
    "OpeningSwitch1AOn", "OpeningSwitch1BOn", "OpeningSwitch2On",
    "OpeningSwitch3AOn", "OpeningSwitch3BOn"
};
NERVE_DECL(DemoOpeningSwitch, Wait);
NERVES_MAKE_NOSTRUCT(DemoOpeningSwitch, Wait)
}

/** @brief Creates the opening switch actor. @param pName Actor name. */
DemoOpeningSwitch::DemoOpeningSwitch(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes switches and registers with course selection. @param rInfo Actor initialization data. */
void DemoOpeningSwitch::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRMSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvDemoOpeningSwitchWait, 0);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    if (auto* director = CourseSelectDirector::tryGetCourseSelectDirector(this))
        director->setDemoOpeningSwitch(this);
    makeActorDead();
}

/** @brief Turns on an opening-demo switch. @param index Switch index in the five-entry opening sequence. */
void DemoOpeningSwitch::onSwitch(int index) {
    al::onStageSwitch(this, cOpeningSwitchNames[index]);
}

/** @brief Waits for externally issued switch requests. */
void DemoOpeningSwitch::exeWait() {}
