#include "Boss/KoopaChaseMover.hpp"
#include "Boss/KoopaChase.hpp"
#include "Boss/KoopaChaseMovePos.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChaseWarpDummy.hpp"
#include "MapObj/WarpCube.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"

/**
 * @brief Builds the linked chase route and selects its first waypoint.
 * @param pHost Chase actor following the route.
 * @param rInfo Placement information containing the MovePos link.
 */
KoopaChaseMover::KoopaChaseMover(KoopaChase* pHost, const al::ActorInitInfo& rInfo)
    : mHost(pHost), mIsOverPoint(false), mCurrentPoint(nullptr), mPreviousPoint(nullptr) {
    al::PlacementInfo links;
    if (al::tryGetLinksInfo(&links, al::getPlacementInfo(rInfo), "MovePos")) {
        mPoints.allocBuffer(128, nullptr);
        mCurrentPoint = new KoopaChaseMovePos(links, rInfo, nullptr, &mPoints);
    }
}

/** @brief Advances passed waypoints and activates their linked warp actors and switch. */
void KoopaChaseMover::update() {
    sead::Vector3f toPoint = mCurrentPoint->mTrans - al::getTrans(mHost);
    if (mPreviousPoint && mCurrentPoint->mPoint18) {
        sead::Vector3f previousDirection = mCurrentPoint->mTrans - mPreviousPoint->mTrans;
        sead::Vector3f nextDirection = mCurrentPoint->mPoint18->mTrans - mCurrentPoint->mTrans;
        if (previousDirection.dot(nextDirection) < 0.0f) {
            toPoint = -toPoint;
        }
    }
    if (!(toPoint.dot(al::getFront(mHost)) >= 0.0f)) {
        mIsOverPoint = false;
        return;
    }
    mIsOverPoint = true;
    mPreviousPoint = mCurrentPoint;
    mCurrentPoint = mCurrentPoint->mPoint18;
    if (!mCurrentPoint) {
        mCurrentPoint = mPoints.front();
    }
    if (mPreviousPoint->mWarpActor && al::isDead(mPreviousPoint->mWarpActor)) {
        mPreviousPoint->mWarpActor->appear();
    }
    if (mPreviousPoint->mWarpCube && al::isDead(mPreviousPoint->mWarpCube)) {
        mPreviousPoint->mWarpCube->appearByKoopaChase();
    }
    if (mPreviousPoint->mWarpDummy && al::isDead(mPreviousPoint->mWarpDummy)) {
        mPreviousPoint->mWarpDummy->appear(KoopaChaseFunction::getDamageCount(mHost));
    }
    al::tryOnStageSwitch(mPreviousPoint, "SwitchPassOn");
}

/** @brief Returns the distance from the host to its current waypoint. @return Distance in world units. */
float KoopaChaseMover::calcDistanceToCurrentPoint() const {
    return (mCurrentPoint->mTrans - al::getTrans(mHost)).length();
}

/** @brief Kills every active warp cube on the route. */
void KoopaChaseMover::tryKillWarpCubeIfAlive() {
    for (auto it = mPoints.begin(); it != mPoints.end(); ++it) {
        if ((*it).mWarpCube && !al::isDead((*it).mWarpCube)) {
            (*it).mWarpCube->kill();
        }
    }
}

/** @brief Kills every active Bowser warp dummy on the route. */
void KoopaChaseMover::tryKillWarpDummyIfAlive() {
    for (auto it = mPoints.begin(); it != mPoints.end(); ++it) {
        if ((*it).mWarpDummy && !al::isDead((*it).mWarpDummy)) {
            (*it).mWarpDummy->kill();
        }
    }
}
