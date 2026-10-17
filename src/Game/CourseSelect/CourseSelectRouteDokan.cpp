#include "CourseSelect/CourseSelectRouteDokan.hpp"
#include "CourseSelect/CourseSelectRouteDokanEntranceGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Block/BlockRailParts.hpp"
#include "Project/Block/BlockRailPartsGroup.hpp"

/**
 * @brief Create a course-select pipe route.
 * @param pName Actor name.
 */
CourseSelectRouteDokan::CourseSelectRouteDokan(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Build the route and its entrances, and connect its activation switch.
 * @param rInfo Actor placement and scene initialization data.
 */
void CourseSelectRouteDokan::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("RouteDokan"), nullptr);
    mpParts = new al::BlockRailPartsGroup;
    mpParts->init(rInfo);
    if (mpParts->getPartsNum() != 0) {
        mpEntrances = new CourseSelectRouteDokanEntranceGroup;
        mpEntrances->init(mpParts, rInfo);
        if (al::listenStageSwitchOnAppear(this, al::Functor(this, &CourseSelectRouteDokan::active))) {
            mIsDeactive = true;
            mpParts->deactive();
            mpEntrances->deactive();
        }
    }
    makeActorDead();
}

/** @brief Activate the pipe parts and entrances. */
void CourseSelectRouteDokan::active() {
    mIsDeactive = false;
    mpParts->active();
    mpEntrances->active();
}

/** @brief Deactivate the pipe parts and entrances. */
void CourseSelectRouteDokan::deactive() {
    mIsDeactive = true;
    mpParts->deactive();
    mpEntrances->deactive();
}

/**
 * @brief Create stoppers for the route entrances.
 * @param rInfo Scene initialization data for the stopper actors.
 */
void CourseSelectRouteDokan::createStopper(const al::ActorInitInfo& rInfo) {
    mpEntrances->createStopper(rInfo);
}

/**
 * @brief Update the entrance stoppers' appearance.
 * @param isAppear Whether to play the stoppers' appearance reaction.
 */
void CourseSelectRouteDokan::appearStopper(bool isAppear) {
    mpEntrances->appearStopper(isAppear);
}

/** @brief Start the World 7 appearance reaction on every route part. */
void CourseSelectRouteDokan::startAppearEffectW7() {
    for (int i = 0; i < mpParts->getPartsNum(); i++) {
        al::startHitReaction(mpParts->getParts(i), "出現W7");
    }
}

/** @brief Enable input at all route entrances. */
void CourseSelectRouteDokan::setEnableInput() {
    mpEntrances->setEnableInput();
}

/** @brief Disable input at all route entrances. */
void CourseSelectRouteDokan::setDisableInput() {
    mpEntrances->setDisableInput();
}

/** @brief Destroy the course-select pipe route. */
CourseSelectRouteDokan::~CourseSelectRouteDokan() = default;
