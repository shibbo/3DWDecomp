#include "CourseSelect/CourseSelectRouteDokanEntrance.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Block/BlockRailRider.hpp"

class RouteDokanController : public ICourseSelectActorController {
public:
    /** @brief Wrap a pipe entrance. @param pActor Entrance to control. */
    explicit RouteDokanController(CourseSelectRouteDokanEntrance* pActor) : mpActor(pActor) {}
    /** @brief Get the controlled actor. @return The pipe entrance. */
    al::LiveActor* getActor() override { return mpActor; }
    /** @brief Select the pipe-route cursor. @return Layout type 6. */
    int getCursorLayoutType() const override { return 6; }
    /** @brief Get course metadata. @return Null because entrances have no course metadata. */
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }
    /** @brief Get a course node. @return Null because entrances are rail endpoints. */
    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }
    /** @brief Start riding from the entrance. @param pRider Rider to configure. */
    void startRouteDokanRider(al::BlockRailRider* pRider) override { mpActor->startRouteDokanRider(pRider); }
    /** @brief Keep the entrance active during a demo. @param pGroup Participating puppeteer group. */
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override {
        mpPuppeteerGroup = pGroup;
        al::invalidateClipping(mpActor);
    }
    /** @brief Release the demo group and restore clipping. */
    void endPuppetDemo() override {
        mpPuppeteerGroup = nullptr;
        al::validateClipping(mpActor);
    }
    /** @brief No binding work is needed. @param pPuppeteer Puppeteer; unused. */
    void startBind(CourseSelectPuppeteer* pPuppeteer) override {}
    /** @brief Try to enter the pipe. @param pDirector Input director. @return Whether travel began. */
    bool tryDecide(const CourseSelectDirector* pDirector) override { return mpActor->tryDecide(pDirector); }
    /** @brief Pipe entrances have no opening sequence. */
    void startOpen() override {}
    /** @brief Pipe entrances need no immediate opening work. */
    void startOpenImmediately() override {}
    /** @brief Check opening completion. @return Always true. */
    bool isEndOpen() const override { return true; }

private:
    CourseSelectRouteDokanEntrance* mpActor;
    CourseSelectPuppeteerGroup* mpPuppeteerGroup = nullptr;
};

/**
 * @brief Create a pipe-route entrance with its themed archive category.
 * @param pName Actor name.
 * @param pSuffix Optional course-select theme suffix.
 */
CourseSelectRouteDokanEntrance::CourseSelectRouteDokanEntrance(const char* pName, const char* pSuffix)
    : al::BlockRailParts(pName) {
    mArchiveCategory.format("CourseSelect%s", pSuffix ? pSuffix : "");
}

/**
 * @brief Load the entrance, apply its rail pose, and create its course-select sensor.
 * @param rInfo Actor placement and scene initialization data.
 */
void CourseSelectRouteDokanEntrance::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("RouteDokanTerminate"), mArchiveCategory.cstr());
    al::updatePoseQuat(this, mInitQuat);
    al::resetPosition(this, mInitTrans, false);
    initRailLink();
    mpDirector = CourseSelectDirector::getCourseSelectDirector(this);
    mpController = new RouteDokanController(this);
    mpSensor = new CourseSelectSensor(static_cast<CourseSelectSensorType>(3), mpController);
    makeActorAppeared();
}

/**
 * @brief Create an initially hidden stopper and disable entry.
 * @param rInfo Scene initialization data for the stopper.
 */
void CourseSelectRouteDokanEntrance::createStopper(const al::ActorInitInfo& rInfo) {
    mpStopper = new al::LiveActor("コース選択ルート土管 栓");
    al::initActorWithArchiveName(mpStopper, rInfo, sead::SafeString("RouteDokanStopper"), nullptr);
    al::copyPose(mpStopper, this);
    mIsEnableInput = false;
    mpStopper->appear();
    mpStopper->makeActorDead();
}

/** @brief Disable entry and show the stopper when one exists. */
void CourseSelectRouteDokanEntrance::setDisableInput() {
    mIsEnableInput = false;
    if (mpStopper)
        mpStopper->appear();
}

/**
 * @brief Show the entrance stopper, optionally with its appearance effect.
 * @param isEffect Whether to start the appearance reaction.
 */
void CourseSelectRouteDokanEntrance::appearStopper(bool isEffect) {
    if (mpStopper) {
        mpStopper->appear();
        if (isEffect)
            al::startHitReactionAppear(mpStopper);
    }
}

/** @brief The entrance has no per-frame control work. */
void CourseSelectRouteDokanEntrance::control() {}

/**
 * @brief Report an enabled entrance's contact with a player to the director.
 * @param pSender Entrance sensor.
 * @param pReceiver Contacting actor's sensor.
 */
void CourseSelectRouteDokanEntrance::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isSensorPlayer(pReceiver) && mIsEnableInput) {
        mpSensor->setSensor(pSender);
        mpDirector->touchPlayer(mpSensor, al::getSensorHost(pReceiver));
    }
}

/**
 * @brief Ignore sensor messages.
 * @param pMsg Incoming message; unused.
 * @param pSender Sending sensor; unused.
 * @param pReceiver Receiving sensor; unused.
 * @return Always false.
 */
bool CourseSelectRouteDokanEntrance::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                              al::HitSensor* pReceiver) {
    return false;
}

/**
 * @brief Save the pose to apply during entrance initialization.
 * @param rQuat Initial orientation.
 * @param rTrans Initial position.
 */
void CourseSelectRouteDokanEntrance::setInitQT(const sead::Quatf& rQuat, const sead::Vector3f& rTrans) {
    mInitQuat = rQuat;
    mInitTrans = rTrans;
}

/**
 * @brief Place a rider at this entrance and reverse it into the route.
 * @param pRider Rail rider to configure.
 */
void CourseSelectRouteDokanEntrance::startRouteDokanRider(al::BlockRailRider* pRider) const {
    pRider->setRailPart(getLink(0), 0.0f);
    pRider->reverse();
}

/**
 * @brief Start pipe travel when the main player confirms an enabled selection.
 * @param pDirector Director whose input and selection should be checked.
 * @return Whether pipe travel was started.
 */
bool CourseSelectRouteDokanEntrance::tryDecide(const CourseSelectDirector* pDirector) {
    if (!pDirector->isEnableEnterSelectedSensor())
        return false;
    if (!pDirector->isTriggerDecideMainPlayer())
        return false;
    mpDirector->getPuppeteerGroup()->startRouteDokan(mpController);
    al::startSe(this, "PgEnterStart", nullptr);
    return true;
}

/** @brief Enable entry and remove the stopper with its disappearance effect. */
void CourseSelectRouteDokanEntrance::setEnableInput() {
    mIsEnableInput = true;
    if (mpStopper) {
        al::startHitReactionDisappear(mpStopper);
        mpStopper->kill();
    }
}

/** @brief Destroy the course-select pipe entrance. */
CourseSelectRouteDokanEntrance::~CourseSelectRouteDokanEntrance() = default;
