#include "MapObj/RouteDokanBazookaRider.hpp"

#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/BindWarpEffect.hpp"
#include "MapObj/PuppetStickRouteSelecter.hpp"
#include "MapObj/RouteDokan.hpp"
#include "MapObj/RouteDokanEntrance.hpp"
#include "MapObj/RouteDokanInOutEffect.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(RouteDokanBazookaRider, Start);
    NERVE_DECL(RouteDokanBazookaRider, Move);
    NERVE_DECL(RouteDokanBazookaRider, ParabolaFlyLand);
    NERVE_DECL(RouteDokanBazookaRider, Ready);
    NERVE_DECL(RouteDokanBazookaRider, ParabolaFly);
    NERVE_DECL(RouteDokanBazookaRider, Shoot);
    NERVE_DECL(RouteDokanBazookaRider, ParabolaFlyLandStart);
    NERVE_DECL(RouteDokanBazookaRider, BindWarp);
    NERVE_DECL(RouteDokanBazookaRider, Invalid);
    NERVE_DECL(RouteDokanBazookaRider, End);
    NERVE_DECL(RouteDokanBazookaRider, Cancel);
    NERVES_MAKE_NOSTRUCT(RouteDokanBazookaRider, Start, Move, ParabolaFlyLand, Ready, ParabolaFly,
                         Shoot, ParabolaFlyLandStart, BindWarp, Invalid, End, Cancel)

    /// End of the bind after the shot player stopped against a wall.
    PlayerBindEndParam sBindEndParamEnd = {{}, 0, 10, true, false, true, 0, -1.0f, 0};

    /// End of the bind after the flight was cancelled.
    PlayerBindEndParam sBindEndParamCancel = {{}, 0, 10, true, false, true, 0, -1.0f, 0};

    /// Number of collision hits checked per frame while flying.
    constexpr s32 cHitInfoNum = 64;

    /// Radius of the sphere moved along the flight to find walls.
    constexpr f32 cFlyCollisionRadius = 100.0f;

    /// Guide message priority needed while riding in single mode.
    constexpr GuideMessagePriority cRidingGuidePriority = static_cast<GuideMessagePriority>(5);

    /// Guide message priority that lets every message through again.
    constexpr GuideMessagePriority cDefaultGuidePriority = static_cast<GuideMessagePriority>(0);

    /**
     * @brief Copies a vector as one block (the trivial copy of the underlying x/y/z struct).
     * @param pDst The destination.
     * @param rSrc The source.
     */
    inline void copyVec(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
        static_cast<sead::BaseVec3<f32>&>(*pDst) = rSrc;
    }

    /**
     * @brief Copies a quaternion as one block (the trivial copy of the underlying x/y/z/w struct).
     * @param pDst The destination.
     * @param rSrc The source.
     */
    inline void copyQuat(sead::Quatf* pDst, const sead::Quatf& rSrc) {
        static_cast<sead::BaseQuat<f32>&>(*pDst) = rSrc;
    }

    /**
     * @brief Scales a vector to the given length, leaving a zero vector untouched.
     * @param pVec The vector to scale.
     * @param length The length the vector should have afterwards.
     */
    inline void setLength(sead::Vector3f* pVec, f32 length) {
        f32 currentLength = pVec->length();
        if (currentLength > 0.0f) {
            f32 scale = length / currentLength;
            pVec->x *= scale;
            pVec->y *= scale;
            pVec->z *= scale;
        }
    }
}  // namespace

/**
 * @brief Constructs a rider of a pipe cannon.
 * @param pHost The cannon the rider belongs to.
 * @param pName The name of the actor.
 * @param selecterCapacity Number of puppets the route selecter can hold.
 * @param gravity Gravity applied every frame during a parabola flight.
 */
RouteDokanBazookaRider::RouteDokanBazookaRider(RouteDokanBazooka* pHost, const char* pName,
                                               s32 selecterCapacity, f32 gravity)
    : al::LiveActor(pName), mHost(pHost),
      mRouteSelecter(new PuppetStickRouteSelecter(selecterCapacity)),
      mComboCounter(new al::ComboCounter), mGravity(gravity) {}

/**
 * @brief Initializes the model, the rail rider and the effects.
 * @param rInfo The actor init info.
 */
void RouteDokanBazookaRider::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "RouteDokanBazookaRider", nullptr);
    al::initNerve(this, &NrvRouteDokanBazookaRiderStart, 0);
    mRailRider = new al::BlockRailRider();
    mRailRider->setRouteSelecter(mRouteSelecter);
    mInOutEffect = new RouteDokanInOutEffect("ルート土管出入りエフェクト");
    al::initCreateActorNoPlacementInfoNoViewId(mInOutEffect, rInfo);
    al::invalidateHitSensor(this, "PlayerAttack");
    al::setHitSensorPosPtr(this, "PlayerAttack", &mTrans);
    al::setEffectFollowMtxPtr(this, "Launch", &mLaunchEffectMtx);
    mBindWarpEffect = new BindWarpEffect();
    mBindWarpEffect->init(rInfo);
    makeActorDead();
}

/**
 * @brief Touches the objects in the pipe and collects items for the carried player.
 * @param pSelf The sensor of the rider.
 * @param pOther The sensor touched.
 */
void RouteDokanBazookaRider::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvRouteDokanBazookaRiderMove)) {
        sead::Vector3f front;
        al::calcQuatFront(&front, mQuat);
        rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, front);
        rc::sendMsgRouteDokanItemGet(pOther, rc::getPuppetSensor(mPuppet));
        return;
    }

    if (mPuppet != nullptr) {
        al::sendMsgPlayerItemGet(pOther, rc::getPuppetSensor(mPuppet));
    }

    if (al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLand)) {
        rc::sendMsgGhostPresentGet(pOther, rc::getPuppetSensor(mPuppet));
    }
}

/**
 * @brief Damages the carried player while it is flying.
 * @param pSender The sensor of the player to damage.
 * @return Whether the carried player was damaged.
 */
bool RouteDokanBazookaRider::damage(al::HitSensor* pSender) {
    if (al::isNerve(this, &NrvRouteDokanBazookaRiderShoot) ||
        al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFly) ||
        al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLandStart) ||
        al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLand)) {
        if (alPlayerFunction::findPlayerHolderIndex(pSender) == mPlayerIndex) {
            rc::damagePuppet(mPuppet);
            return true;
        }

        return false;
    }

    return false;
}

/**
 * @brief Sets the speed the rider moves through the pipes with.
 * @param speed The move speed.
 */
void RouteDokanBazookaRider::setMoveSpeed(f32 speed) {
    mMoveSpeed = speed;
}

/**
 * @brief Sets the velocity the player is launched out of the cannon with.
 * @param rVelocity The launch velocity.
 */
void RouteDokanBazookaRider::setOutVelocity(const sead::Vector3f& rVelocity) {
    copyVec(&mOutVelocity, rVelocity);
}

/**
 * @brief Launches the waiting player out of the cannon.
 */
void RouteDokanBazookaRider::shoot() {
    sead::Vector3f dir = mOutVelocity;
    al::normalize(&dir);
    rc::setPuppetFrontVec(mPuppet, dir);
    rc::setPuppetUpVec(mPuppet, mUpDir);
    // The result is unused; the check only remains from a stripped debug path.
    al::isParallelDirection(dir, mUpDir, 0.01f);

    if (al::isNerve(this, &NrvRouteDokanBazookaRiderReady)) {
        rc::startPuppetSe(mPuppet, "RouteDokanBazookaFly");
        if (mShootType == RouteDokanBazooka::ShootType_Parabola) {
            al::setNerve(this, &NrvRouteDokanBazookaRiderParabolaFly);
        } else {
            al::setNerve(this, &NrvRouteDokanBazookaRiderShoot);
        }
    }
}

/**
 * @brief Whether the rider carries the given player.
 * @param playerIndex The index of the player.
 * @return True if the rider is alive and carries that player.
 */
bool RouteDokanBazookaRider::isActive(s32 playerIndex) const {
    if (al::isDead(this)) {
        return false;
    }

    return mPlayerIndex == playerIndex;
}

/**
 * @brief Whether the bind of the carried player has ended.
 * @return True if no player is bound.
 */
bool RouteDokanBazookaRider::isEndBind() const {
    return mPuppet == nullptr;
}

/**
 * @brief Whether the player waits inside the cannon.
 * @return True while waiting to be shot.
 */
bool RouteDokanBazookaRider::isStateReady() const {
    return al::isNerve(this, &NrvRouteDokanBazookaRiderReady);
}

/**
 * @brief Whether the player is flying after being shot.
 * @return True while flying.
 */
bool RouteDokanBazookaRider::isStateFlying() const {
    return al::isNerve(this, &NrvRouteDokanBazookaRiderShoot) ||
           al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFly) ||
           al::isNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLandStart);
}

/**
 * @brief Binds a player that entered the pipe.
 * @param pEntrance The entrance the player went through.
 * @param pMsg The message that requested the bind.
 * @param pSender The sensor of the player.
 * @param pReceiver The sensor of the entrance.
 * @param isContinue Whether the player continues moving instead of entering.
 */
void RouteDokanBazookaRider::startBind(RouteDokanEntrance* pEntrance, const al::SensorMsg* pMsg,
                                       al::HitSensor* pSender, al::HitSensor* pReceiver,
                                       bool isContinue) {
    if (rc::isMsgBindInitRequest(pMsg)) {
        startBindForce(pSender, pEntrance, rc::startPuppet(pReceiver, pSender));
        return;
    }

    s32 playerIndex = alPlayerFunction::findPlayerHolderIndex(pSender);
    IUsePlayerPuppet* puppet = rc::startPuppet(pReceiver, pSender);
    if (isContinue) {
        startBindContinue(playerIndex, pEntrance, puppet);
    } else {
        startBindNormal(playerIndex, pEntrance, puppet);
    }
}

/**
 * @brief Binds a player that is warped into the cannon.
 * @param pSender The sensor of the player.
 * @param pEntrance The entrance the player is bound at.
 * @param pPuppet The puppet of the player.
 */
void RouteDokanBazookaRider::startBindForce(al::HitSensor* pSender, RouteDokanEntrance* pEntrance,
                                            IUsePlayerPuppet* pPuppet) {
    mPlayerIndex = alPlayerFunction::findPlayerHolderIndex(pSender);
    mPuppet = pPuppet;
    mRouteSelecter->addPuppet(mPuppet);
    pEntrance->startRouteDokanRider(mRailRider);
    copyVec(&mTrans, rc::getPuppetTrans(mPuppet));
    rc::setPuppetTrans(mPuppet, mTrans);
    al::setTrans(this, mTrans);
    rc::hidePuppet(mPuppet);
    rc::hidePuppetSilhouette(mPuppet);
    mBindWarpEffect->start(pSender, al::getTrans(mHost), false);
    al::setNerve(this, &NrvRouteDokanBazookaRiderBindWarp);
    makeActorAppeared();
}

/**
 * @brief Binds a player that keeps moving through the pipe.
 * @param playerIndex The index of the player.
 * @param pEntrance The entrance the player went through.
 * @param pPuppet The puppet of the player.
 */
void RouteDokanBazookaRider::startBindContinue(s32 playerIndex, RouteDokanEntrance* pEntrance,
                                               IUsePlayerPuppet* pPuppet) {
    mPlayerIndex = playerIndex;
    mPuppet = pPuppet;
    mRouteSelecter->addPuppet(mPuppet);
    pEntrance->startRouteDokanRider(mRailRider);
    rc::calcPuppetQuat(&mStartQuat, mPuppet);

    sead::Vector3f pos = sead::Vector3f::zero;
    sead::Vector3f dir = sead::Vector3f::ez;
    mRailRider->calcPosAndDir(&pos, &dir);
    mInOutEffect->startIn(pos, dir);

    sead::Quatf quat;
    al::turnQuatZDirRate(&quat, mStartQuat, dir, 1.0f);
    mQuat = quat;
    mTrans = pos;
    rc::setPuppetQuat(mPuppet, mQuat);
    rc::setPuppetTrans(mPuppet, mTrans);
    al::setTrans(this, mTrans);
    copyQuat(al::getQuatPtr(this), mQuat);
    rc::startPuppetAction(mPuppet, "RouteDokanMove");
    al::setNerve(this, &NrvRouteDokanBazookaRiderMove);
    makeActorAppeared();
}

/**
 * @brief Binds a player that enters the pipe.
 * @param playerIndex The index of the player.
 * @param pEntrance The entrance the player went through.
 * @param pPuppet The puppet of the player.
 */
void RouteDokanBazookaRider::startBindNormal(s32 playerIndex, RouteDokanEntrance* pEntrance,
                                             IUsePlayerPuppet* pPuppet) {
    mPlayerIndex = playerIndex;
    mPuppet = pPuppet;
    mRouteSelecter->addPuppet(mPuppet);
    rc::calcPuppetQuat(&mStartQuat, mPuppet);
    mStartTrans = rc::getPuppetTrans(mPuppet);
    mQuat = mStartQuat;
    mTrans = mStartTrans;
    pEntrance->startRouteDokanRider(mRailRider);

    sead::Vector3f pos = sead::Vector3f::zero;
    sead::Vector3f dir = sead::Vector3f::ez;
    mRailRider->calcPosAndDir(&pos, &dir);
    mInOutEffect->startIn(pos, dir);
    al::setNerve(this, &NrvRouteDokanBazookaRiderStart);
    makeActorAppeared();
}

/**
 * @brief Lets another puppet steer the route of the rider.
 * @param pPuppet The puppet to add.
 */
void RouteDokanBazookaRider::addRouteSelectPuppet(IUsePlayerPuppet* pPuppet) {
    mRouteSelecter->addPuppet(pPuppet);
}

/**
 * @brief Releases the carried player if it owns the given sensor.
 * @param pSender The sensor of the player.
 * @return Whether the bind was cancelled.
 */
bool RouteDokanBazookaRider::tryCancelBind(al::HitSensor* pSender) {
    if (mPuppet == nullptr || !rc::isPuppetSensor(mPuppet, pSender)) {
        return false;
    }

    rc::invalidateMaterialRouteDokan(mPuppet);
    if (rc::isPuppetHidden(mPuppet)) {
        rc::showPuppetSilhouette(mPuppet);
        rc::showPuppet(mPuppet);
    }

    mPuppet = nullptr;
    mRouteSelecter->clearPuppetAll();
    makeActorDead();
    return true;
}

/**
 * @brief Releases the carried player unconditionally.
 */
void RouteDokanBazookaRider::forceCancelBind() {
    if (mPuppet == nullptr) {
        return;
    }

    if (rc::isPuppetHidden(mPuppet)) {
        rc::showPuppetSilhouette(mPuppet);
        rc::showPuppet(mPuppet);
    }

    mPuppet = nullptr;
    mRouteSelecter->clearPuppetAll();
    makeActorDead();
}

/**
 * @brief Emits the launch effect at the muzzle of the cannon.
 */
void RouteDokanBazookaRider::emitEffectLaunch() {
    sead::Vector3f front;
    al::calcFrontDir(&front, mHost);
    sead::Vector3f up;
    al::calcUpDir(&up, mHost);
    al::makeMtxFrontUpPos(&mLaunchEffectMtx, front, up, al::getTrans(mHost) + front * 180.0f);
    al::emitEffect(this, "Launch", nullptr);
}

/**
 * @brief Turns the flying player so that its up direction follows the velocity.
 */
void RouteDokanBazookaRider::updatePuppetPose() {
    sead::Vector3f velocityDir = mVelocity;
    if (al::normalizeOrZero(&velocityDir)) {
        return;
    }

    sead::Vector3f up = rc::getPuppetUpVec(mPuppet);
    sead::Vector3f side;
    side.setCross(up, rc::getPuppetFrontVec(mPuppet));
    al::normalize(&side);
    sead::Vector3f front;
    front.setCross(side, up);

    sead::Quatf rotation;
    rotation.makeVectorRotation(up, velocityDir);
    sead::Vector3f newFront;
    newFront.setRotated(rotation, front);
    rc::setPuppetFrontVec(mPuppet, newFront);
    rc::setPuppetUpVec(mPuppet, velocityDir);
}

/**
 * @brief Stops the flying player on the ground.
 */
void RouteDokanBazookaRider::doLanding() {
    rc::setPuppetVelocity(mPuppet, sead::Vector3f::zero);
    al::startHitReactionOnGround(this);
}

/**
 * @brief Pulls the player from the entrance onto the rail.
 */
void RouteDokanBazookaRider::exeStart() {
    if (al::isFirstStep(this)) {
        rc::startPuppetSe(mPuppet, "RouteDokanIn");
        rc::startPuppetAction(mPuppet, "RouteDokanMove");
        if (GameDataFunction::isSingleMode(this)) {
            auto* window = al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
            if (window != nullptr) {
                window->setPriorityLimit(cRidingGuidePriority);
            }
        }
    }

    sead::Vector3f pos = sead::Vector3f::zero;
    sead::Vector3f dir = sead::Vector3f::ez;
    mRailRider->calcPosAndDir(&pos, &dir);

    sead::Quatf quat;
    al::turnQuatZDirRate(&quat, mStartQuat, dir, 1.0f);
    f32 rate = al::calcNerveEaseInOutRate(this, 2);
    al::slerpQuat(&mQuat, mStartQuat, quat, rate);
    al::lerpVec(&mTrans, mStartTrans, pos, rate);
    rc::setPuppetQuat(mPuppet, mQuat);
    rc::setPuppetTrans(mPuppet, mTrans);
    al::setTrans(this, mTrans);
    copyQuat(al::getQuatPtr(this), mQuat);

    if (al::isGreaterEqualStep(this, 2)) {
        al::setNerve(this, &NrvRouteDokanBazookaRiderMove);
    }
}

/**
 * @brief Moves the player along the rail until it reaches the cannon.
 */
void RouteDokanBazookaRider::exeMove() {
    if (al::isFirstStep(this)) {
        rc::validateMaterialRouteDokan(mPuppet);
    }

    sead::Vector3f dir = sead::Vector3f::ez;
    f32 speed = al::calcNerveValue(this, 0, 1.0f, mMoveSpeed);
    mRailRider->move(speed, &mTrans, &dir);

    if (!al::isParallelDirection(dir, sead::Vector3f::ey, 0.01f)) {
        sead::Vector3f up;
        al::calcQuatUp(&up, mQuat);
        if (al::isReverseDirection(up, sead::Vector3f::ey, 0.01f)) {
            al::rotateQuatRadian(&mQuat, mQuat, dir, sead::Mathf::deg2rad(10.0f));
        } else {
            al::turnQuatYDirRadian(&mQuat, mQuat, sead::Vector3f::ey, sead::Mathf::deg2rad(10.0f));
        }
    }

    al::turnQuatZDirRate(&mQuat, mQuat, dir, 1.0f);
    rc::setPuppetQuat(mPuppet, mQuat);
    rc::setPuppetTrans(mPuppet, mTrans);
    al::setTrans(this, mTrans);
    copyQuat(al::getQuatPtr(this), mQuat);

    if (mRailRider->isReachEnd()) {
        rc::invalidateMaterialRouteDokan(mPuppet);
        al::startAction(mHost, "In");
        al::startAction(al::getSubActor(mHost, "外側モデル"), "In");
        al::setNerve(this, &NrvRouteDokanBazookaRiderReady);
    }
}

/**
 * @brief Keeps the warped player at the cannon until the warp effect has arrived.
 */
void RouteDokanBazookaRider::exeBindWarp() {
    if (mBindWarpEffect->isMoving()) {
        copyVec(&mTrans, al::getTrans(mHost));
        rc::setPuppetTrans(mPuppet, mTrans);
        al::setTrans(this, mTrans);
    }

    if (al::isDead(mBindWarpEffect)) {
        copyVec(&mTrans, al::getTrans(mHost));
        rc::setPuppetTrans(mPuppet, mTrans);
        al::setTrans(this, mTrans);
        al::setNerve(this, &NrvRouteDokanBazookaRiderReady);
    }
}

/**
 * @brief Hides the player inside the cannon until it is shot.
 */
void RouteDokanBazookaRider::exeReady() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "RouteDokanBazookaWait");
        rc::hidePuppetSilhouette(mPuppet);
        rc::hidePuppet(mPuppet);
    }
}

/**
 * @brief Flies the player along a parabola towards the target.
 */
void RouteDokanBazookaRider::exeParabolaFly() {
    if (al::isFirstStep(this)) {
        rc::showPuppetSilhouette(mPuppet);
        if (rc::isPuppetHidden(mPuppet)) {
            rc::showPuppet(mPuppet);
        }

        rc::startPuppetAction(mPuppet, "RouteDokanBazookaFly");
        copyVec(&mVelocity, mOutVelocity);
        rc::setPuppetVelocity(mPuppet, mVelocity);
        al::validateHitSensor(this, "PlayerAttack");
        mComboCounter->reset();
        mHitCount = 0;
        al::requestStartCameraShake(this, "強");
        emitEffectLaunch();
    }

    mVelocity -= sead::Vector3f::ey * mGravity;
    rc::setPuppetVelocity(mPuppet, mVelocity);
    mTrans += mVelocity;
    rc::setPuppetTrans(mPuppet, mTrans);
    rc::moveSimplePuppet(mPuppet);
    updatePuppetPose();

    if (al::isGreaterEqualStep(this, mShootFrame - 30.0f)) {
        al::setNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLandStart);
    }
}

/**
 * @brief Turns the player upright and drops it until it hits the ground.
 */
void RouteDokanBazookaRider::exeParabolaFlyLandStart() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(mPuppet, "CannonFlyLand");
        sead::Vector3f gravity = al::getGravity(this);
        sead::Vector3f up = -gravity;
        sead::Vector3f side;
        side.setCross(gravity, rc::getPuppetFrontVec(mPuppet));
        al::normalize(&side);
        sead::Vector3f front;
        front.setCross(gravity, side);
        rc::setPuppetUpVec(mPuppet, up);
        rc::setPuppetFrontVec(mPuppet, front);
    }

    alCollisionUtil::SphereMoveHitInfo hitInfos[cHitInfoNum];
    u32 hitNum = alCollisionUtil::checkStrikeSphereMove(this, hitInfos, cHitInfoNum, mTrans,
                                                        cFlyCollisionRadius, mVelocity, nullptr,
                                                        nullptr);
    f32 minTime = 1.0f;
    bool isHit = false;
    for (u32 i = 0; i < hitNum; i++) {
        al::LiveActor* host = al::getSensorHost(hitInfos[i].triangle.getSensor());
        if (host != nullptr) {
            if (al::isEqualString(host->getName(), "発射口")) {
                continue;
            }

            if (al::sendMsgDokanBazookaAttack(hitInfos[i].triangle.getSensor(),
                                              rc::getPuppetSensor(mPuppet))) {
                continue;
            }
        }

        if (hitInfos[i].time < minTime) {
            minTime = hitInfos[i].time;
            isHit = true;
        }
    }

    if (isHit) {
        mHitCount++;
        if (mHitCount >= 1) {
            sead::Vector3f pos = mTrans;
            sead::Vector3f offset = mVelocity;
            setLength(&offset, cFlyCollisionRadius);
            pos += offset;
            rc::setPuppetTrans(mPuppet, pos);
            rc::snapGroundPuppet(mPuppet);
            doLanding();
            al::setNerve(this, &NrvRouteDokanBazookaRiderParabolaFlyLand);
            return;
        }
    } else {
        mHitCount = 0;
    }

    mVelocity -= sead::Vector3f::ey * mGravity;
    rc::setPuppetVelocity(mPuppet, mVelocity);
    mTrans += mVelocity * minTime;
    rc::setPuppetTrans(mPuppet, mTrans);
    rc::moveSimplePuppet(mPuppet);
}

/**
 * @brief Waits for the landing action and releases the player.
 */
void RouteDokanBazookaRider::exeParabolaFlyLand() {
    if (rc::isPuppetActionEnd(mPuppet)) {
        mRouteSelecter->clearPuppet(mPuppet);
        rc::endBindOnGroundAndPuppetNull(&mPuppet);
        auto* window = al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
        if (window != nullptr) {
            window->setPriorityLimit(cDefaultGuidePriority);
        }

        al::setNerve(this, &NrvRouteDokanBazookaRiderInvalid);
    }
}

/**
 * @brief Flies the player in a straight line until it hits a wall.
 */
void RouteDokanBazookaRider::exeShoot() {
    if (al::isFirstStep(this)) {
        rc::showPuppetSilhouette(mPuppet);
        if (rc::isPuppetHidden(mPuppet)) {
            rc::showPuppet(mPuppet);
        }

        rc::startPuppetAction(mPuppet, "RouteDokanBazookaFly");
        copyVec(&mVelocity, mOutVelocity);
        rc::setPuppetVelocity(mPuppet, mVelocity);
        al::validateHitSensor(this, "PlayerAttack");
        mComboCounter->reset();
        mHitCount = 0;
        al::requestStartCameraShake(this, "強");
        emitEffectLaunch();
    }

    alCollisionUtil::SphereMoveHitInfo hitInfos[cHitInfoNum];
    u32 hitNum = alCollisionUtil::checkStrikeSphereMove(this, hitInfos, cHitInfoNum, mTrans,
                                                        cFlyCollisionRadius, mVelocity, nullptr,
                                                        nullptr);
    f32 minTime = 1.0f;
    bool isHit = false;
    for (u32 i = 0; i < hitNum; i++) {
        al::LiveActor* host = al::getSensorHost(hitInfos[i].triangle.getSensor());
        if (host != nullptr) {
            if (al::isEqualString(host->getName(), "発射口")) {
                continue;
            }

            if (al::sendMsgDokanBazookaAttack(hitInfos[i].triangle.getSensor(),
                                              rc::getPuppetSensor(mPuppet))) {
                continue;
            }
        }

        if (hitInfos[i].time < minTime) {
            minTime = hitInfos[i].time;
            isHit = true;
        }
    }

    mTrans += mVelocity * minTime;
    rc::setPuppetTrans(mPuppet, mTrans);
    rc::moveSimplePuppet(mPuppet);
    updatePuppetPose();

    if (isHit) {
        mHitCount++;
        if (mHitCount >= 5) {
            doLanding();
            al::setNerve(this, &NrvRouteDokanBazookaRiderEnd);
            return;
        }
    } else {
        mHitCount = 0;
    }

    if (al::isGreaterEqualStep(this, 600)) {
        al::invalidateHitSensor(this, "PlayerAttack");
        al::setNerve(this, &NrvRouteDokanBazookaRiderCancel);
    }
}

/**
 * @brief Releases the player after it stopped against a wall.
 */
void RouteDokanBazookaRider::exeEnd() {
    al::invalidateHitSensor(this, "PlayerAttack");
    mRouteSelecter->clearPuppet(mPuppet);
    rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamEnd);
    auto* window = al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
    if (window != nullptr) {
        window->setPriorityLimit(cDefaultGuidePriority);
    }

    al::setNerve(this, &NrvRouteDokanBazookaRiderInvalid);
}

/**
 * @brief Releases the player after the flight took too long.
 */
void RouteDokanBazookaRider::exeCancel() {
    if (rc::isPuppetHidden(mPuppet)) {
        rc::showPuppetSilhouette(mPuppet);
        rc::showPuppet(mPuppet);
    }

    mRouteSelecter->clearPuppet(mPuppet);
    rc::endBindAndPuppetNull(&mPuppet, &sBindEndParamCancel);
    auto* window = al::tryGetSceneObj<GuideGameWindow>(this, SceneObjID_GuideGameWindow);
    if (window != nullptr) {
        window->setPriorityLimit(cDefaultGuidePriority);
    }

    al::setNerve(this, &NrvRouteDokanBazookaRiderInvalid);
}

/**
 * @brief Waits a moment after the release before the rider can be used again.
 */
void RouteDokanBazookaRider::exeInvalid() {
    if (al::isGreaterEqualStep(this, 30)) {
        mPlayerIndex = -1;
        makeActorDead();
    }
}
