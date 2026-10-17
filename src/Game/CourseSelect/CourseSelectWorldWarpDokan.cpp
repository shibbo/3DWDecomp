#include "CourseSelect/CourseSelectWorldWarpDokan.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(CourseSelectWorldWarpDokan, Wait);
NERVE_DECL(CourseSelectWorldWarpDokan, Demo);
NERVE_DECL(CourseSelectWorldWarpDokan, Disappear);
class CourseSelectWorldWarpDokanNrvEnd : public al::Nerve {
public:
    /** @brief Remain in the completed state. @param pKeeper Unused keeper. */
    void execute(al::NerveKeeper* pKeeper) const override {}
};
NERVES_MAKE_NOSTRUCT(CourseSelectWorldWarpDokan, Wait, Demo, End, Disappear)
}

class WorldWarpDokanController : public ICourseSelectActorController {
public:
    /** @brief Wrap the warp pipe. @param pActor Pipe to control. */
    explicit WorldWarpDokanController(CourseSelectWorldWarpDokan* pActor) : mpActor(pActor) {}
    /** @brief Get the pipe actor. @return Controlled pipe. */
    al::LiveActor* getActor() override { return mpActor; }
    /** @brief Get the warp cursor type. @return Layout type 5. */
    int getCursorLayoutType() const override { return 5; }
    /** @brief Get course metadata. @return Null for this demo-only pipe. */
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }
    /** @brief Get a course node. @return Null for this demo-only pipe. */
    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }
    /** @brief Start the pipe demo. @param pGroup Participating puppeteers. */
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override { mpActor->startPuppetDemo(pGroup); }
    /** @brief End the pipe demo. */
    void endPuppetDemo() override { mpActor->endPuppetDemo(); }
    /** @brief No binding is needed. @param pPuppeteer Unused puppeteer. */
    void startBind(CourseSelectPuppeteer* pPuppeteer) override {}
    /** @brief Ignore selection input. @param pDirector Unused director. @return False. */
    bool tryDecide(const CourseSelectDirector* pDirector) override { return false; }
    /** @brief No opening sequence is supported. */
    void startOpen() override {}
    /** @brief No immediate opening is supported. */
    void startOpenImmediately() override {}
    /** @brief Check opening completion. @return False. */
    bool isEndOpen() const override { return false; }
private:
    CourseSelectWorldWarpDokan* mpActor;
};

/** @brief Create a world-warp pipe. @param pName Actor name. */
CourseSelectWorldWarpDokan::CourseSelectWorldWarpDokan(const char* pName) : al::LiveActor(pName) {}
/** @brief Initialize the pipe model and controller. @param rInfo Actor initialization data. */
void CourseSelectWorldWarpDokan::init(const al::ActorInitInfo& rInfo) {
    mpController = new WorldWarpDokanController(this);
    al::initActorWithArchiveName(this, rInfo, "Dokan", nullptr);
    al::initNerve(this, &NrvCourseSelectWorldWarpDokanWait, 0);
    al::isExistRail(this);
    makeActorDead();
}
/** @brief Push players touching the pipe body. @param pSender Pipe sensor. @param pReceiver Other sensor. */
void CourseSelectWorldWarpDokan::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isSensorPlayer(pReceiver) && al::isSensorMapObj(pSender))
        al::sendMsgPush(pReceiver, pSender);
}
/** @brief Ignore sensor messages. @param pMsg Message. @param pSender Sender. @param pReceiver Receiver. @return False. */
bool CourseSelectWorldWarpDokan::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) { return false; }
/** @brief Keep the pipe active throughout its demo. @param pGroup Participating puppeteers. */
void CourseSelectWorldWarpDokan::startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) {
    mpPuppeteerGroup = pGroup;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCourseSelectWorldWarpDokanDemo);
}
/** @brief Release the demo group and restore idle clipping. */
void CourseSelectWorldWarpDokan::endPuppetDemo() {
    mpPuppeteerGroup = nullptr;
    al::validateClipping(this);
    al::setNerve(this, &NrvCourseSelectWorldWarpDokanWait);
}
/** @brief Activate the pipe and start the world's warp demo. */
void CourseSelectWorldWarpDokan::startDemo() {
    appear();
    CourseSelectDirector::getCourseSelectDirector(this)->getPuppeteerGroup()->startWorldWarpDokanDemo(mpController);
}
/** @brief Check whether the disappearance completed. @return Whether the end state is active. */
bool CourseSelectWorldWarpDokan::isEndDemo() const { return al::isNerve(this, &NrvCourseSelectWorldWarpDokanEnd); }
/** @brief Remain idle until a demo starts. */
void CourseSelectWorldWarpDokan::exeWait() {}
/** @brief Wait for all puppeteers, then disappear. */
void CourseSelectWorldWarpDokan::exeDemo() {
    al::isFirstStep(this);
    if (mpPuppeteerGroup->isDemoEnd()) {
        mpPuppeteerGroup = nullptr;
        al::setNerve(this, &NrvCourseSelectWorldWarpDokanDisappear);
    }
}
/** @brief Play the disappearance action and deactivate the pipe. */
void CourseSelectWorldWarpDokan::exeDisappear() {
    if (al::isFirstStep(this)) al::startAction(this, "Disappear");
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCourseSelectWorldWarpDokanEnd);
        kill();
    }
}
/** @brief Destroy the world-warp pipe. */
CourseSelectWorldWarpDokan::~CourseSelectWorldWarpDokan() = default;
