#include "Enemy/WalkerStateRouteDokanMove.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Block/BlockRailRider.hpp"

namespace {
NERVE_DECL(WalkerStateRouteDokanMove, Start)
NERVE_DECL(WalkerStateRouteDokanMove, Move)
NERVE_DECL(WalkerStateRouteDokanMove, Eject)
NERVE_DECL(WalkerStateRouteDokanMove, Land)
NERVES_MAKE_NOSTRUCT(WalkerStateRouteDokanMove, Start, Move, Eject, Land)
const WalkerStateRouteDokanMoveParam cDefaultParam;
}

/** @brief Initializes default pipe movement settings. */
WalkerStateRouteDokanMoveParam::WalkerStateRouteDokanMoveParam()
    : mSpeed(15.0f), mEjectSpeed(15.0f), mGravityLerp(0.6f), mSpinSpeed(17.5f), mReentryDelay(5) {}

/** @brief Initializes custom pipe movement settings.
 * @param speed Travel speed within the pipe.
 * @param ejectSpeed Speed when leaving the pipe.
 * @param gravityLerp Interpolation factor for the spin reference direction.
 * @param spinSpeed Spin angle added per frame.
 * @param reentryDelay Frames before allowing reentry after ejection.
 */
WalkerStateRouteDokanMoveParam::WalkerStateRouteDokanMoveParam(f32 speed, f32 ejectSpeed,
        f32 gravityLerp, f32 spinSpeed, s32 reentryDelay)
    : mSpeed(speed), mEjectSpeed(ejectSpeed), mGravityLerp(gravityLerp),
      mSpinSpeed(spinSpeed), mReentryDelay(reentryDelay) {}

/** @brief Creates the rail rider and pipe entry and exit effects.
 * @param pHost Enemy traveling through pipes.
 * @param rInfo Initialization context for the effect actor.
 * @param pParam Passive movement settings.
 * @param pMoveParam Pipe settings, or nullptr for defaults.
 */
WalkerStateRouteDokanMove::WalkerStateRouteDokanMove(al::LiveActor* pHost,
        const al::ActorInitInfo& rInfo, const WalkerStateParam* pParam,
        const WalkerStateRouteDokanMoveParam* pMoveParam)
    : al::ActorStateBase("歩行敵ルート土管移動状態", pHost), mParam(pParam), mMoveParam(pMoveParam) {
    mEffect = new RouteDokanInOutEffect("ルート土管出入りエフェクト");
    al::initCreateActorNoPlacementInfoNoViewId(mEffect, rInfo);
    mRailRider = new al::BlockRailRider();
    initNerve(&NrvWalkerStateRouteDokanMoveStart, 0);
    if (!mMoveParam) { mMoveParam = &cDefaultParam; }
}

/** @brief Starts the pipe entry sequence. */
void WalkerStateRouteDokanMove::appear() {
    al::setNerve(this, &NrvWalkerStateRouteDokanMoveStart);
    al::ActorStateBase::appear();
}

/** @brief Requests pipe travel when moving toward a compatible pipe sensor.
 * @param pSelf Enemy sensor.
 * @param pOther Potential pipe sensor.
 * @return Whether the pipe accepted the enemy.
 */
bool WalkerStateRouteDokanMove::tryStart(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!isDead()) {
        if (al::isNerve(this, &NrvWalkerStateRouteDokanMoveStart) || isMove()) { return false; }
        if (isEject() && al::isLessEqualStep(this, mMoveParam->mReentryDelay)) { return false; }
    }
    sead::Vector3f dir;
    al::calcDirBetweenSensors(&dir, pSelf, pOther);
    if (!(al::getVelocity(mHostActor).dot(dir) > 0.0f)) { return false; }
    if (!rc::sendMsgBlockRailRide(pOther, pSelf, mRailRider)) { return false; }
    dir.y = 0.0f;
    if (al::normalizeOrZero(&mFront, dir)) {
        al::calcFrontDir(&mFront, mHostActor);
        mFront.y = 0.0f;
        if (al::normalizeOrZero(&mFront)) { mFront.set(sead::Vector3f::ez); }
    }
    return true;
}

/** @brief Sets the route selection policy.
 * @param pSelecter Policy used at pipe junctions.
 */
void WalkerStateRouteDokanMove::setRouteSelecter(al::BlockRailRouteSelecter* pSelecter) {
    mRailRider->setRouteSelecter(pSelecter);
}

/** @brief Tests whether the enemy is traveling through a pipe.
 * @return Whether the move nerve is active.
 */
bool WalkerStateRouteDokanMove::isMove() const { return al::isNerve(this, &NrvWalkerStateRouteDokanMoveMove); }

/** @brief Tests whether the enemy is leaving a pipe.
 * @return Whether the eject nerve is active.
 */
bool WalkerStateRouteDokanMove::isEject() const { return al::isNerve(this, &NrvWalkerStateRouteDokanMoveEject); }

/** @brief Plays the entry effect and prepares movement through the pipe. */
void WalkerStateRouteDokanMove::exeStart() {
    if (al::isFirstStep(this)) {
        sead::Vector3f pos, dir;
        mRailRider->calcPosAndDir(&pos, &dir);
        mEffect->startIn(pos, dir);
        al::setVelocityZero(mHostActor);
        al::offCollide(mHostActor);
        al::invalidateClipping(mHostActor);
        al::startAction(mHostActor, "RouteDokan");
        mGravity.set(0.0f, -1.0f, 0.0f);
        mSpinAngle = 0.0f;
    }
    al::setNerve(this, &NrvWalkerStateRouteDokanMoveMove);
}

/** @brief Travels along the pipe, spins the actor, and handles ejection. */
void WalkerStateRouteDokanMove::exeMove() {
    mRailRider->move(mMoveParam->mSpeed, al::getTransPtr(mHostActor), &mMoveDir);
    sead::Vector3f horizontal(mMoveDir);
    horizontal.y = 0.0f;
    if (!al::normalizeOrZero(&horizontal)) { mFront.set(horizontal); }
    if (mRailRider->isReachEnd()) {
        al::setVelocity(mHostActor, mMoveParam->mEjectSpeed * mMoveDir);
        al::onCollide(mHostActor);
        al::setFront(mHostActor, mFront);
        al::setGravity(mHostActor, sead::Vector3f(0.0f, -1.0f, 0.0f));
        mEffect->startOut(al::getTrans(mHostActor), mMoveDir);
        al::setNerve(this, &NrvWalkerStateRouteDokanMoveEject);
        return;
    }
    mSpinAngle += mMoveParam->mSpinSpeed;
    if (mSpinAngle > 180.0f) { mSpinAngle -= 360.0f; }
    sead::Vector3f dir(mMoveDir);
    if (al::normalizeOrZero(&dir)) { return; }
    sead::Vector3f gravity(mGravity);
    sead::Vector3f side;
    side.setCross(dir, gravity);
    al::normalizeOrDirZ(&side);
    gravity.setCross(side, dir);
    al::normalizeOrDirZ(&gravity);
    sead::Vector3f front(gravity);
    al::rotateVectorDegree(&front, front, dir, mSpinAngle);
    al::setFront(mHostActor, front);
    al::setGravity(mHostActor, -dir);
    al::lerpVec(&mGravity, gravity, sead::Vector3f(0.0f, -1.0f, 0.0f), mMoveParam->mGravityLerp);
    if (al::normalizeOrZero(&mGravity)) { mGravity.set(0.0f, -1.0f, 0.0f); }
}

/** @brief Restores clipping and falls until reaching the ground. */
void WalkerStateRouteDokanMove::exeEject() {
    if (al::isFirstStep(this)) {
        al::validateClipping(mHostActor);
        al::startAction(mHostActor, "RouteDokanEnd");
    }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isOnGround(mHostActor, 0, 0.0f)) { al::setNerve(this, &NrvWalkerStateRouteDokanMoveLand); }
}

/** @brief Plays the landing action and finishes pipe travel. */
void WalkerStateRouteDokanMove::exeLand() {
    if (al::isFirstStep(this)) { al::startAction(mHostActor, "RouteDokanLand"); }
    WalkerStateFunction::calcPassiveMovement(mHostActor, mParam);
    if (al::isActionEnd(mHostActor)) { kill(); }
}
