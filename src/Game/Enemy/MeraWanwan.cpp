#include "Enemy/MeraWanwan.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/MeraWanwanTrack.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/ParabolicPath.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/JointAimUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(MeraWanwan, Dead)
NERVE_DECL(MeraWanwan, Chase)
NERVE_DECL(MeraWanwan, SupportFreeze)
NERVE_DECL(MeraWanwan, Hide)
NERVE_DECL(MeraWanwan, Appear)
NERVE_DECL(MeraWanwan, Sink)
NERVE_DECL(MeraWanwan, Blow)
NERVE_DECL(MeraWanwan, SinkHide)
NERVE_DECL(MeraWanwan, AppearLand)
NERVE_DECL(MeraWanwan, Fall)
NERVE_DECL(MeraWanwan, Land)
// Non-const nerve objects: the game keeps them in .data in this order.
MeraWanwanNrvDead NrvMeraWanwanDead;
MeraWanwanNrvChase NrvMeraWanwanChase;
MeraWanwanNrvSupportFreeze NrvMeraWanwanSupportFreeze;
MeraWanwanNrvHide NrvMeraWanwanHide;
MeraWanwanNrvAppear NrvMeraWanwanAppear;
MeraWanwanNrvSink NrvMeraWanwanSink;
MeraWanwanNrvBlow NrvMeraWanwanBlow;
MeraWanwanNrvSinkHide NrvMeraWanwanSinkHide;
MeraWanwanNrvAppearLand NrvMeraWanwanAppearLand;
MeraWanwanNrvFall NrvMeraWanwanFall;
MeraWanwanNrvLand NrvMeraWanwanLand;

/** @brief Freeze and target search parameters shared by every MeraWanwan. */
struct MeraWanwanParam {
    MeraWanwanParam()
        : mSupportFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 100.0f, 0.0f)) {
        setDefaultFindParam();
    }

    /** @brief Sets the default search distance and angles of the target finder. */
    void setDefaultFindParam() {
        mTargetFinderParam._0 = 1600.0f;
        mTargetFinderParam._4 = 180.0f;
        mTargetFinderParam._8 = 90.0f;
    }

    ActorStateSupportFreezeParam mSupportFreezeParam;
    TargetFinderParam mTargetFinderParam;
};

MeraWanwanParam sParam;

/**
 * @brief Copies a quaternion as a plain copy of its storage.
 * @param pDst Destination quaternion.
 * @param rSrc Source quaternion.
 */
inline void copyQuat(sead::Quatf* pDst, const sead::Quatf& rSrc) {
    static_cast<sead::BaseQuat<f32>&>(*pDst) = rSrc;
}
}  // namespace

/**
 * @brief Constructs a MeraWanwan.
 * @param pName Actor name.
 */
MeraWanwan::MeraWanwan(const char* pName) : al::LiveActor(pName) {}

/** @brief Makes the actor appeared unless it is already dying. */
void MeraWanwan::makeActorAppeared() {
    if (al::isNerve(this, &NrvMeraWanwanDead)) {
        return;
    }

    al::LiveActor::makeActorAppeared();
}

/**
 * @brief Initializes the model, states, fire tracks, target finder, appear path and eye aim.
 * @param rInfo Placement info of the actor.
 */
void MeraWanwan::init(const al::ActorInitInfo& rInfo) {
    sParam.setDefaultFindParam();
    al::initActor(this, rInfo);
    al::trySetShadowLength(this, rInfo, nullptr);
    al::initNerve(this, &NrvMeraWanwanChase, 1);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sParam.mSupportFreezeParam);
    al::initNerveState(this, mStateSupportFreeze, &NrvMeraWanwanSupportFreeze, "DRC拘束");

    auto* trackGroup = new al::DeriveActorGroup<MeraWanwanTrack>("メラワンワン跡", 16);
    mTrackGroup = trackGroup;
    for (s32 i = 0; i < trackGroup->getMaxActorCount(); i++) {
        auto* track = new MeraWanwanTrack("メラワンワン跡");
        al::initCreateActorWithPlacementInfo(track, rInfo);
        trackGroup->registerActor(track);
    }

    mTargetFinder = new TargetFinder(this, &sParam.mTargetFinderParam);
    mTargetFinder->setFrontDir(&mFrontDir);
    mParabolicPath = new al::ParabolicPath();

    sead::Vector3f landTrans;
    if (al::tryGetLinksTrans(&landTrans, rInfo, "AppearLandPoint")) {
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        up *= 100.0f;
        al::hideModelIfShow(this);
        al::offCollide(this);
        mIsAppearFromLink = true;
        mParabolicPath->initFromUpVectorAddHeight(al::getTrans(this), landTrans + up,
                                                  sead::Vector3f::ey, 200.0f);
        al::setNerve(this, &NrvMeraWanwanHide);
    } else {
        al::addTransOffsetLocalDir(this, 100.0f, 1);
    }

    al::tryGetArg(&mAppearRange, rInfo, "AppearRange");
    al::initJointControllerKeeper(this, 3);
    al::initJointGlobalQuatController(this, &mBodyQuat, "BodyRotate");

    mJointAimInfoL = new al::JointAimInfo();
    mJointAimInfoL->setBaseAimLocalDir(sead::Vector3f::ex);
    mJointAimInfoL->setBaseSideLocalDir(sead::Vector3f::ez);
    mJointAimInfoL->setBaseUpLocalDir(sead::Vector3f::ey);
    mJointAimInfoL->setEnableBackAim(true);
    mJointAimInfoL->setLimitDegreeOval(8.0f, 3.0f, 4.0f, 18.0f);
    mJointAimInfoL->setInterpoleRate(0.3f);
    al::initJointAimController(this, mJointAimInfoL, "EyeL");

    mJointAimInfoR = new al::JointAimInfo();
    mJointAimInfoR->setBaseAimLocalDir(sead::Vector3f::ex);
    mJointAimInfoR->setBaseSideLocalDir(sead::Vector3f::ez);
    mJointAimInfoR->setBaseUpLocalDir(sead::Vector3f::ey);
    mJointAimInfoR->setEnableBackAim(true);
    mJointAimInfoR->setLimitDegreeOval(3.0f, 8.0f, 4.0f, 18.0f);
    mJointAimInfoR->setInterpoleRate(0.3f);
    al::initJointAimController(this, mJointAimInfoR, "EyeR");

    if (al::trySyncStageSwitchAppear(this) && mIsAppearFromLink) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvMeraWanwanAppear);
    }

    mInitTrans = al::getTrans(this);
    mInitQuat = al::getQuat(this);
}

/**
 * @brief Pushes other enemies away and attacks players, NPCs and Koopa Jr.
 * @param pSelf Sensor of this actor.
 * @param pOther Sensor that was hit.
 */
void MeraWanwan::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther) && isEnablePush()) {
        if (rc::sendMsgMeraWanwanPush(pOther, pSelf)) {
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir, 0.001f)) {
                dir = -mFrontDir;
            }

            al::setVelocityToDirection(this, dir, 20.0f);
        }

        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        f32 speed = al::getVelocity(this).length();
        if (speed > 0.1f) {
            al::startSeWithParam(this, "PgBound", speed, nullptr);
        }
    }

    if ((al::isSensorRide(pOther) || al::isSensorPlayer(pOther) || al::isSensorNpc(pOther) ||
         al::isSensorKoopaJr(pOther)) &&
        al::isSensorEnemyAttack(pSelf) && isEnableAttack()) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttack(pOther, pSelf);
        rc::sendMsgMeraWanwanAttack(pOther, pSelf);
    }
}

/**
 * @brief Checks whether the MeraWanwan can be pushed by other actors.
 * @return Whether the current nerve allows pushing.
 */
bool MeraWanwan::isEnablePush() const {
    return al::isNerve(this, &NrvMeraWanwanAppearLand) || al::isNerve(this, &NrvMeraWanwanChase) ||
           al::isNerve(this, &NrvMeraWanwanFall) || al::isNerve(this, &NrvMeraWanwanLand) ||
           al::isNerve(this, &NrvMeraWanwanBlow) ||
           al::isNerve(this, &NrvMeraWanwanSupportFreeze) || al::isNerve(this, &NrvMeraWanwanSink);
}

/**
 * @brief Checks whether the MeraWanwan can currently attack.
 * @return Whether the current nerve allows attacking.
 */
bool MeraWanwan::isEnableAttack() const {
    bool isEnable;
    if (al::isNerve(this, &NrvMeraWanwanAppearLand)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanChase)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanFall)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanLand)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanBlow)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanSupportFreeze)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanAppear)) {
        isEnable = !al::isNoCollide(this);
    } else {
        isEnable = false;
    }

    return isEnable;
}

/**
 * @brief Handles pushes, breaking and blowing attacks and restore messages.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of this actor.
 * @return Whether the message was handled.
 */
bool MeraWanwan::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (isEnablePush() &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf,
                                            al::isNerve(this, &NrvMeraWanwanSink) ? 0.0f : 5.0f)) {
        return true;
    }

    if (isEnableBreak(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::setVelocityZero(this);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvMeraWanwanDead);
        return true;
    }

    if (isEnableBlow(pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::calcDirBetweenSensorsH(&mBlowDir, pOther, pSelf);
        if (al::isNearZero(mBlowDir, 0.001f)) {
            mBlowDir = -mFrontDir;
        }

        al::invalidateClipping(this);
        al::setVelocityToDirection(this, mBlowDir, 30.0f);
        al::setNerve(this, &NrvMeraWanwanBlow);
        return true;
    }

    if (al::isMsgRestore(pMsg)) {
        makeActorAppeared();
        mIsRestored = true;
        al::resetPosition(this, mInitTrans, false);
        al::setQuat(this, mInitQuat);
        al::setNerve(this, &NrvMeraWanwanChase);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether a message breaks the MeraWanwan instantly.
 * @param pMsg Received message.
 * @return Whether the message is a breaking attack that is currently accepted.
 */
bool MeraWanwan::isEnableBreak(const al::SensorMsg* pMsg) const {
    if (al::isNerve(this, &NrvMeraWanwanHide)) {
        return false;
    }

    if (al::isNerve(this, &NrvMeraWanwanAppear)) {
        return false;
    }

    if (al::isNerve(this, &NrvMeraWanwanSink)) {
        return false;
    }

    if (al::isNerve(this, &NrvMeraWanwanSinkHide)) {
        return false;
    }

    if (al::isNerve(this, &NrvMeraWanwanDead)) {
        return false;
    }

    return al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
           al::isMsgLaserAttack(pMsg);
}

/**
 * @brief Checks whether a message blows the MeraWanwan away.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of this actor.
 * @return Whether the message is a blowing attack that is currently accepted.
 */
bool MeraWanwan::isEnableBlow(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) const {
    if (!isEnableAttack()) {
        return false;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isMsgBallAttack(pMsg) || al::isMsgKickKouraReflect(pMsg) ||
        al::isMsgPlayerKouraAttack(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        al::isMsgPlayerBoomerangBreak(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        rc::isMsgBossGorobonSpinShot(pMsg) || rc::isMsgBossGorobonAttack(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg)) {
        return true;
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerBodyLanding(pMsg) || al::isMsgExplosion(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg)) {
        if (!al::isNerve(this, &NrvMeraWanwanBlow)) {
            return true;
        }

        return al::isGreaterEqualStep(this, 30);
    }

    return false;
}

/**
 * @brief Lets the DRC touch screen bind the MeraWanwan.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target that was hit.
 * @return Whether the message was handled.
 */
bool MeraWanwan::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!isEnableSupportFreeze()) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (!al::isNerve(this, &NrvMeraWanwanSupportFreeze)) {
        al::setNerve(this, &NrvMeraWanwanSupportFreeze);
    }

    return true;
}

/**
 * @brief Checks whether the MeraWanwan can be bound by the DRC touch screen.
 * @return Whether the current nerve allows binding.
 */
bool MeraWanwan::isEnableSupportFreeze() const {
    bool isEnable;
    if (al::isNerve(this, &NrvMeraWanwanAppearLand)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanChase)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanFall)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanLand)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanSupportFreeze)) {
        isEnable = true;
    } else if (al::isNerve(this, &NrvMeraWanwanAppear)) {
        isEnable = !al::isNoCollide(this);
    } else {
        isEnable = false;
    }

    return isEnable;
}

/** @brief Tracks the last fire track and the air time and kills the actor in death areas. */
void MeraWanwan::control() {
    if (mLastTrack != nullptr && al::isDead(mLastTrack)) {
        mLastTrack = nullptr;
    }

    if (al::isCollidedGround(this)) {
        mAirTime = 0;
    } else {
        mAirTime++;
    }

    if (al::isNerve(this, &NrvMeraWanwanHide) || al::isNerve(this, &NrvMeraWanwanSink) ||
        al::isNerve(this, &NrvMeraWanwanSinkHide) || al::isNerve(this, &NrvMeraWanwanDead)) {
        return;
    }

    if (rc::isInDeathArea(this)) {
        al::setNerve(this, &NrvMeraWanwanDead);
        al::startHitReactionDisappear(this);
        kill();
    }
}

/**
 * @brief Sets the distance at which every MeraWanwan finds its target.
 * @param distance Find distance.
 */
void MeraWanwan::setFindDistance(f32 distance) {
    sParam.mTargetFinderParam._0 = distance;
}

/** @brief Jumps out from the start of the appear path when called by the boss Gorobon. */
void MeraWanwan::appearByBossGorobon() {
    al::setVelocityZero(this);
    al::setTrans(this, mParabolicPath->getStart());
    al::setNerve(this, &NrvMeraWanwanAppear);
    al::LiveActor::appear();
}

/** @brief Kills the MeraWanwan and its fire tracks when the boss Gorobon is defeated. */
void MeraWanwan::forceDead() {
    if (al::isDead(this) || al::isNerve(this, &NrvMeraWanwanDead)) {
        return;
    }

    deleteTrack();
    al::setVelocityZero(this);
    al::setAppearItemFactor(this, "間接攻撃", nullptr);
    al::setNerve(this, &NrvMeraWanwanDead);
}

/** @brief Kills every fire track that is still alive. */
void MeraWanwan::deleteTrack() {
    for (s32 i = 0; i < mTrackGroup->getActorCount(); i++) {
        MeraWanwanTrack* track = mTrackGroup->getDeriveActor(i);
        if (al::isAlive(track)) {
            track->kill();
        }
    }
}

/** @brief Waits hidden until the player comes close enough. */
void MeraWanwan::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide");
        al::resetPosition(this, mParabolicPath->getStart(), false);
        al::validateClipping(this);
    }

    if (al::isNearPlayer(this, mAppearRange)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvMeraWanwanAppear);
    }
}

/** @brief Jumps along the appear path and lands on the ground. */
void MeraWanwan::exeAppear() {
    if (al::isFirstStep(this)) {
        mJointAimInfoL->setPowerRate(0.0f);
        mJointAimInfoR->setPowerRate(0.0f);
        sead::Vector3f start = mParabolicPath->getStart();
        al::resetPosition(this, start, false);
        al::startAction(this, "Appear");

        sead::Vector3f dir = mParabolicPath->getHorizontalDirection();
        if (!al::isNearZero(dir, 0.001f)) {
            mFrontDir = dir;
        }

        al::makeQuatFrontUp(al::getQuatPtr(this), mFrontDir, -al::getGravity(this));
        copyQuat(&mBodyQuat, al::getQuat(this));
        al::showModelIfHide(this);
        mPrevTrans = start;
        al::offCollide(this);
    }

    s32 pathTime = mParabolicPath->calcPathTimeFromGravityAccel(24.0f);
    sead::Vector3f pos;
    mParabolicPath->calcPosition(&pos, al::getNerveStep(this) / (f32)pathTime);
    al::setVelocity(this, pos - mPrevTrans);
    mPrevTrans = pos;
    if (al::isGreaterEqualStep(this, pathTime / 2) && al::isNoCollide(this)) {
        al::onCollide(this);
    }

    if (trySink()) {
        return;
    }

    if (al::isCollidedGround(this)) {
        al::setNerve(this, &NrvMeraWanwanAppearLand);
        al::onCollide(this);
        al::setVelocityZero(this);
    }
}

/**
 * @brief Sinks into fire ground, or dies there when the MeraWanwan cannot hide again.
 * @return Whether the MeraWanwan touched fire ground.
 */
bool MeraWanwan::trySink() {
    if (!al::isCollidedGround(this)) {
        return false;
    }

    if (!rc::isCollidedDamageFire(this)) {
        return false;
    }

    al::setVelocityZero(this);
    if (mIsAppearFromLink) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvMeraWanwanSink);
    } else {
        kill();
    }

    return true;
}

/** @brief Plays the landing after the appear jump and starts chasing. */
void MeraWanwan::exeAppearLand() {
    if (al::isFirstStep(this)) {
        mAirTime = 0;
        al::startAction(this, "AppearLand");
    }

    if (trySink()) {
        return;
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        mTargetFinder->refindTarget();
        al::setNerve(this, &NrvMeraWanwanChase);
    }
}

/** @brief Applies collision rebound, gravity and friction and updates the rolling sound. */
void MeraWanwan::updateVelocity() {
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    if (al::isCollidedGround(this)) {
        al::addVelocityToGravityFittedGround(this, 1.0f, 0);
    } else {
        al::addVelocityToGravity(this, 1.0f);
    }

    al::scaleVelocity(this, mAirTime < 3 ? 0.94f : 0.98f);
    f32 speed = al::getVelocity(this).length();
    al::holdSeWithParam(this, "PgMove", speed, nullptr);
}

/** @brief Rolls after the target and leaves fire tracks. */
void MeraWanwan::exeChase() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Chase");
        al::validateClipping(this);
    }

    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoL, 1500.0f, 0.2f);
    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoR, 1500.0f, 0.2f);
    if (trySink()) {
        return;
    }

    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::turnDirectionToTargetDegree(this, &mFrontDir, mTargetFinder->getTargetPos(), 1.15f);
    }

    f32 accel = al::calcNerveValue(this, 15, 0.0f, 0.45f);
    if (al::isCollidedGround(this) && mTargetFinder->isExistTarget()) {
        al::addVelocityToDirection(this, mFrontDir, accel);
    }

    updateVelocity();
    emitTrack();
    al::calcMomentRollBall(&mMoment, al::getVelocity(this), -al::getGravity(this), 100.0f);
    al::rotateQuatMoment(&mBodyQuat, mBodyQuat, mMoment);
    if (mAirTime >= 7) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvMeraWanwanFall);
    }
}

/** @brief Leaves a fire track on the ground at regular intervals while rolling. */
void MeraWanwan::emitTrack() {
    if (!al::isCollidedGround(this) || !al::isIntervalStep(this, 16, 0)) {
        return;
    }

    sead::Vector3f groundPos = al::getCollidedGroundPos(this);
    if (mLastTrack != nullptr && al::calcDistance(mLastTrack, groundPos) <= 30.0f) {
        return;
    }

    MeraWanwanTrack* track = mTrackGroup->tryFindDeadDeriveActor();
    if (track == nullptr) {
        return;
    }

    track->start(groundPos, al::getOnGroundNormal(this, 0),
                 al::tryGetCollidedGroundCollisionParts(this));
    mLastTrack = track;
}

/** @brief Falls while keeping the rolling spin until it lands. */
void MeraWanwan::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Fall");
    }

    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoL, 1500.0f, 0.2f);
    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoR, 1500.0f, 0.2f);
    if (trySink()) {
        return;
    }

    updateVelocity();
    mMoment *= 0.98f;
    al::rotateQuatMoment(&mBodyQuat, mBodyQuat, mMoment);
    if (al::isCollidedGround(this)) {
        al::setNerve(this, &NrvMeraWanwanLand);
    }
}

/** @brief Plays the landing after a fall and chases again. */
void MeraWanwan::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
    }

    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoL, 1500.0f, 0.2f);
    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoR, 1500.0f, 0.2f);
    if (trySink()) {
        return;
    }

    updateVelocity();
    emitTrack();
    al::calcMomentRollBall(&mMoment, al::getVelocity(this), -al::getGravity(this), 100.0f);
    al::rotateQuatMoment(&mBodyQuat, mBodyQuat, mMoment);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvMeraWanwanChase);
    }
}

/** @brief Rolls away after being hit and chases again after a while. */
void MeraWanwan::exeBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Blow");
    }

    if (trySink()) {
        return;
    }

    updateVelocity();
    al::calcMomentRollBall(&mMoment, al::getVelocity(this), -al::getGravity(this), 100.0f);
    al::rotateQuatMoment(&mBodyQuat, mBodyQuat, mMoment);
    if (al::isGreaterEqualStep(this, 40)) {
        mTargetFinder->refindTarget();
        al::setNerve(this, &NrvMeraWanwanChase);
    }
}

/** @brief Sinks into the fire ground, then hides again or dies. */
void MeraWanwan::exeSink() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sink");
    }

    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoL, 1500.0f, 0.2f);
    JointAimUtil::updateEyeJointInfo(this, mJointAimInfoR, 1500.0f, 0.2f);
    updateVelocity();
    if (al::isActionEnd(this)) {
        if (_1C9) {
            al::setNerve(this, &NrvMeraWanwanSinkHide);
        } else {
            kill();
        }
    }
}

/** @brief Moves back to the start of the appear path and hides for a while. */
void MeraWanwan::exeSinkHide() {
    if (al::isFirstStep(this)) {
        al::resetPosition(this, mParabolicPath->getStart(), false);
        al::startAction(this, "Hide");
        al::setVelocityZero(this);
        al::hideModelIfShow(this);
        al::offCollide(this);
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvMeraWanwanHide);
    }
}

/** @brief Stays bound by the DRC touch screen and chases again when released. */
void MeraWanwan::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        mTargetFinder->refindTarget();
        al::setNerve(this, &NrvMeraWanwanChase);
    }
}

/** @brief Plays the death animation, drops the item and dies. */
void MeraWanwan::exeDead() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Die");
    }

    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        al::appearItem(this);
        kill();
    }
}
