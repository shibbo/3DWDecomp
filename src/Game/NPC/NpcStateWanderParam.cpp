#include "NPC/NpcTargetMetrics.hpp"
#include "NPC/NpcTargetFunction.hpp"
#include "NPC/NpcStateWanderParam.hpp"
#include "NPC/NpcStateWander.hpp"
#include "NPC/NpcTerritoryMover.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "NPC/NpcTargetFinderParam.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include <cmath>
#include <cfloat>

/** @brief Releases the stored action names. */
NpcStateWanderParam::~NpcStateWanderParam() = default;

/** @brief Initializes the default walking, waiting, and territory settings. */
NpcStateWanderParam::NpcStateWanderParam()
    : mWaitFrames(60), mWalkFrames(60), mAcceleration(0.4f), mTurnDegrees(6.0f),
      mReachedDistance(20.0f), mTerritoryRadius(500.0f), mDurationRandomness(0.0f), mParam1C(0.0f),
      mParam20(0.0f), mAvoidFalls(true), mAvoidAreas(true), mWalkAction("Walk"),
      mWaitAction("Wait"), mMoveWhileTurning(true), mTargetInterval(-1) {}

/**
 * @brief Stores the waiting, walking, and territory settings.
 * @param waitFrames Duration of the waiting phase.
 * @param walkFrames Duration of the walking phase.
 * @param acceleration Acceleration along the movement direction.
 * @param turnDegrees Turn speed in degrees per update.
 * @param reachedDistance Distance that counts as reaching the target.
 * @param territoryRadius Radius of the wandering territory.
 * @param durationRandomness Relative range used to randomize phase durations.
 * @param param1C Parameter stored at offset 0x1c; purpose not yet identified.
 * @param param20 Parameter stored at offset 0x20; purpose not yet identified.
 * @param avoidFalls Whether to check for falls.
 * @param avoidAreas Whether to avoid NPC exclusion areas.
 * @param pWalkAction Walking action name.
 * @param pWaitAction Waiting action name.
 * @param moveWhileTurning Whether to accelerate before facing the target.
 * @param targetInterval Frame interval for choosing a new target.
 * @param centerResetDistance Distance that causes the territory center to reset.
 */
NpcStateWanderParam::NpcStateWanderParam(int waitFrames, int walkFrames, float acceleration,
                                         float turnDegrees, float reachedDistance,
                                         float territoryRadius, float durationRandomness,
                                         float param1C, float param20, bool avoidFalls,
                                         bool avoidAreas, const char* pWalkAction,
                                         const char* pWaitAction, bool moveWhileTurning,
                                         int targetInterval, float centerResetDistance)
    : mWaitFrames(waitFrames), mWalkFrames(walkFrames), mAcceleration(acceleration),
      mTurnDegrees(turnDegrees), mReachedDistance(reachedDistance),
      mTerritoryRadius(territoryRadius), mDurationRandomness(durationRandomness), mParam1C(param1C),
      mParam20(param20), mAvoidFalls(avoidFalls), mAvoidAreas(avoidAreas), mWalkAction(pWalkAction),
      mWaitAction(pWaitAction), mMoveWhileTurning(moveWhileTurning),
      mTargetInterval(targetInterval), mCenterResetDistance(centerResetDistance) {}

namespace {
NpcStateWanderParam sDefaultWanderParam;
NERVE_DECL(NpcStateWander, Wait)
NERVE_DECL(NpcStateWander, Walk)
NERVES_MAKE_NOSTRUCT(NpcStateWander, Wait, Walk)
} // namespace
/**
 * @brief Creates the wandering state and its territory mover.
 * @param pActor Actor controlled by this state.
 * @param pDirection Mutable movement direction.
 * @param pMoveParam Passive movement settings.
 * @param pParam Wandering settings; read before the fallback check and therefore expected to be
 * non-null.
 */
NpcStateWander::NpcStateWander(al::LiveActor* pActor, sead::Vector3f* pDirection,
                               const NpcStateParam* pMoveParam, const NpcStateWanderParam* pParam)
    : al::ActorStateBase("NpcStateWander", pActor), mDirection(pDirection), mMover(nullptr),
      mMoveParam(pMoveParam), mParam(pParam) {
    mDurations = new Durations(pParam->mWaitFrames, pParam->mWalkFrames);
    initNerve(&NrvNpcStateWanderWait, 0);
    if (!mParam) {
        mParam = &sDefaultWanderParam;
    }
    mMover = new NpcTerritoryMover(mHostActor, mParam->mTerritoryRadius, 200.0f);
    mMover->setCenter(al::getTrans(mHostActor));
    mMover->calculateTargets();
}
/**
 * @brief Releases the state nerve executor.
 */
NpcStateWander::~NpcStateWander() = default;
/**
 * @brief Begins walking and refreshes a distant territory center.
 */
void NpcStateWander::appear() {
    mIsDead = false;
    if (mParam->mCenterResetDistance > 0.0f &&
        al::calcDistanceH(mHostActor, mMover->getCenter()) > mParam->mCenterResetDistance) {
        mMover->setCenter(al::getTrans(mHostActor));
        mMover->calculateTargets();
    }
    al::setNerve(this, &NrvNpcStateWanderWalk);
}
/**
 * @brief Changes the territory center and regenerates its targets.
 * @param rCenter New territory center.
 */
void NpcStateWander::setWanderCenter(const sead::Vector3f& rCenter) {
    mMover->setCenter(rCenter);
    mMover->calculateTargets();
}
/**
 * @brief Gets the territory center.
 * @return Current territory center.
 */
sead::Vector3f NpcStateWander::getWanderCenter() const { return mMover->getCenter(); }
/**
 * @brief Gets the current walking target.
 * @return Current target position.
 */
sead::Vector3f NpcStateWander::getTargetPos() const { return mMover->getTarget(); }
/**
 * @brief Checks the current nerve.
 * @return Whether the state is waiting.
 */
bool NpcStateWander::isWait() { return al::isNerve(this, &NrvNpcStateWanderWait); }
/**
 * @brief Waits for the randomized duration while applying passive movement.
 */
void NpcStateWander::exeWait() {
    if (al::isFirstStep(this)) {
        if (!al::isSklAnimPlaying(mHostActor, mParam->mWaitAction.cstr(), 0)) {
            al::startAction(mHostActor, mParam->mWaitAction.cstr());
        }
        const float randomness = mParam->mDurationRandomness;
        if (randomness > 0.0f) {
            const float wait = mParam->mWaitFrames, walk = mParam->mWalkFrames;
            Durations* pDurations = mDurations;
            pDurations->wait = al::getRandom(wait - wait * randomness, wait + wait * randomness);
            pDurations->walk = al::getRandom(walk - walk * randomness, walk + walk * randomness);
            if (pDurations->wait <= 0) {
                pDurations->wait = wait;
            }
            if (pDurations->walk <= 0) {
                pDurations->walk = walk;
            }
        }
    }
    NpcStateFunction::calcPassiveMovement(mHostActor, mMoveParam);
    if (al::isGreaterStep(this, mDurations->wait)) {
        al::setNerve(this, &NrvNpcStateWanderWalk);
    }
}
/**
 * @brief Walks toward territory targets while avoiding obstacles.
 */
void NpcStateWander::exeWalk() {
    if (al::isFirstStep(this)) {
        mMover->setTarget(al::getTrans(mHostActor));
        mMover->decideNextTargetPos();
        if (!al::isSklAnimPlaying(mHostActor, mParam->mWalkAction.cstr(), 0)) {
            al::startAction(mHostActor, mParam->mWalkAction.cstr());
        }
    } else if (al::isIntervalStep(this, mParam->mTargetInterval, 0)) {
        mMover->decideNextTargetPos();
    }
    al::turnDirectionToTargetDegree(mHostActor, mDirection, mMover->getTarget(),
                                    mParam->mTurnDegrees);
    if (mParam->mMoveWhileTurning ||
        al::isFaceToTargetDegreeH(mHostActor, mMover->getTarget(), *mDirection, 8.0f)) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(mHostActor);
        pVelocity->setScaleAdd(mParam->mAcceleration, *mDirection, *pVelocity);
    }
    NpcStateFunction::calcPassiveMovement(mHostActor, mMoveParam);
    if ((mParam->mAvoidAreas &&
         NpcStateFunction::isNPCAvoidAreaNextMove(mHostActor, 200.0f, 20.0f, 150.0f, false)) ||
        (mParam->mAvoidFalls &&
         NpcStateFunction::isFallNextMove(mHostActor, 150.0f, 150.0f, 70.0f, false))) {
        al::setVelocityZeroH(mHostActor);
        mMover->setTarget(mMover->getCenter());
    } else if (al::isGreaterStep(this, mDurations->walk) ||
               mMover->isReachedTarget(mHostActor, mParam->mReachedDistance)) {
        al::setNerve(this, &NrvNpcStateWanderWait);
    }
}
