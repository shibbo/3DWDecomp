#include "Enemy/Donketsu.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateFindPlayer.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Enemy/WalkerStateWander.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/JointAimUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Donketsu, Wander)
NERVE_DECL(Donketsu, FindPlayer)
NERVE_DECL(Donketsu, SupportFreeze)
NERVE_DECL(Donketsu, Fall)
NERVE_DECL(Donketsu, Dead)
NERVE_DECL(Donketsu, Slide)
NERVE_DECL(Donketsu, Stay)
NERVE_DECL(Donketsu, Chase)
NERVE_DECL(Donketsu, WallHit)
NERVE_DECL(Donketsu, WallHitLand)
NERVE_DECL(Donketsu, Land)
// Non-const nerve objects: the game keeps them in .data in this order.
DonketsuNrvWander NrvDonketsuWander;
DonketsuNrvFindPlayer NrvDonketsuFindPlayer;
DonketsuNrvSupportFreeze NrvDonketsuSupportFreeze;
DonketsuNrvFall NrvDonketsuFall;
DonketsuNrvDead NrvDonketsuDead;
DonketsuNrvSlide NrvDonketsuSlide;
DonketsuNrvStay NrvDonketsuStay;
DonketsuNrvChase NrvDonketsuChase;
DonketsuNrvWallHit NrvDonketsuWallHit;
DonketsuNrvLand NrvDonketsuLand;
DonketsuNrvWallHitLand NrvDonketsuWallHitLand;

typedef al::FunctorV0M<Donketsu*, void (Donketsu::*)()> DonketsuFunctor;

/** @brief Freeze, target search and walking parameters shared by every Donketsu. */
struct DonketsuParam {
    DonketsuParam()
        : mSupportFreezeParam(true, 15, false, true, 120, sead::Vector3f(0.0f, 100.0f, 0.0f)) {
        mTargetFinderParam._0 = 1200.0f;
        mTargetFinderParam._4 = 180.0f;
        mTargetFinderParam._8 = 90.0f;
        mTargetFinderParam._C = 90;
        mWalkerStateParam.mGravity = 1.0f;
        mWalkerStateParam.mAirFriction = 0.99f;
        mWalkerStateParam.mGroundFriction = 0.94f;
        mWanderParam.mAccel = 0.2f;
        mWanderParam.mTurnRate = 3.0f;
        mWanderParam.mWaitTime = 120;
        mWanderParam.mWalkTime = 120;
        mWanderParam.mSearchRange = 200.0f;
        mFindPlayerParam.mTurnTime = 10;
        mFindPlayerParam.mTurnDegree = 6.0f;
    }

    ActorStateSupportFreezeParam mSupportFreezeParam;
    u8 _1C[4];  // unused; the target finder param starts at 0x20
    TargetFinderParam mTargetFinderParam;
    WalkerStateParam mWalkerStateParam;
    WalkerStateFindPlayerParam mFindPlayerParam;
    WalkerStateWanderParam mWanderParam;
};

DonketsuParam sParam;

constexpr f32 cScoreFactor = 100.0f;

/**
 * @brief Checks whether a message is an attack by one of Bowser's fire balls.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @return True if the message is a fire ball attack.
 */
inline bool isMsgKoopaFireBallAttack(const al::SensorMsg* pMsg, al::HitSensor* pOther) {
    return (al::isSensorHostName(pOther, "KoopaFireBallGiant") ||
            al::isSensorHostName(pOther, "KoopaFireBall")) &&
           al::isMsgEnemyAttackFire(pMsg) && al::isSensorName(pOther, "Attack");
}
}  // namespace

/**
 * @brief Constructs a Donketsu.
 * @param pName Actor name.
 */
Donketsu::Donketsu(const char* pName) : al::LiveActor(pName) {}

/** @brief Searches for a target again and chases it if found, otherwise starts wandering. */
inline void Donketsu::setNerveByTarget() {
    mTargetFinder->refindTarget();
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvDonketsuFindPlayer);
    } else {
        al::setNerve(this, &NrvDonketsuWander);
    }
}

/**
 * @brief Starts sliding in the slide direction after an attack.
 * @param pMsg Attack message that decides the slide speed.
 */
inline void Donketsu::startSlide(const al::SensorMsg* pMsg) {
    al::invalidateClipping(this);
    al::setVelocityToDirection(this, mSlideDir, calcSlideSpeed(pMsg));
    al::startAction(this, "Slide");
    mIsSlideToStay = !al::isNerve(this, &NrvDonketsuStay);
    mIsEnableChainPush = true;
    al::setNerve(this, &NrvDonketsuSlide);
}

/** @brief Gives the score to the last attacker and dies. */
inline void Donketsu::addScoreAndKill() {
    if (mLastAttackSensor != nullptr) {
        rc::addScore(this, mLastAttackSensor, cScoreFactor, 0);
    } else if (mLastAttackPointer != nullptr) {
        rc::addScore(this, mLastAttackPointer, cScoreFactor, 0);
    }

    al::startHitReactionDeath(this);
    kill();
}

/**
 * @brief Initializes the model, states, eye aim controllers and stage switches.
 * @param rInfo Placement info of the actor.
 */
void Donketsu::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::syncSensorAndColliderScaleY(this);
    al::initNerve(this, &NrvDonketsuWander, 8);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) {
        mFallCheckOffsetY = 10.0f;
    }

    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    mTargetFinder = new TargetFinder(this, &sParam.mTargetFinderParam);
    mStateWander = new WalkerStateWander(this, al::getFrontPtr(this), &sParam.mWalkerStateParam,
                                         &sParam.mWanderParam, nullptr);
    mStateFindPlayer =
        new WalkerStateFindPlayer(this, al::getFrontPtr(this), mTargetFinder,
                                  &sParam.mWalkerStateParam, &sParam.mFindPlayerParam, nullptr);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sParam.mSupportFreezeParam);
    al::initNerveState(this, mStateWander, &NrvDonketsuWander, "[state]徘徊");
    al::initNerveState(this, mStateFindPlayer, &NrvDonketsuFindPlayer, "[state]プレーヤー発見");
    al::initNerveState(this, mStateSupportFreeze, &NrvDonketsuSupportFreeze, "[state]DRC拘束");

    mJointAimInfo = new al::JointAimInfo();
    mJointAimInfo->setBaseAimLocalDir(sead::Vector3f::ex);
    mJointAimInfo->setBaseSideLocalDir(sead::Vector3f::ez);
    mJointAimInfo->setBaseUpLocalDir(sead::Vector3f::ey);
    mJointAimInfo->setEnableBackAim(true);
    mJointAimInfo->setLimitDegreeRect(15.0f, 3.0f, 0.0f, 0.0f);
    mJointAimInfo->setInterpoleRate(0.1f);
    al::initJointControllerKeeper(this, 2);
    al::initJointAimController(this, mJointAimInfo, "EyeL");
    al::initJointAimController(this, mJointAimInfo, "EyeR");

    al::listenStageSwitchOnKill(this, DonketsuFunctor(this, &Donketsu::killBySwitch));
    if (al::listenStageSwitchOnAppear(this, DonketsuFunctor(this, &Donketsu::appearBySwitch))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

/** @brief Kills the actor when its kill switch turns on. */
void Donketsu::killBySwitch() {
    kill();
}

/** @brief Appears falling when its appear switch turns on. */
void Donketsu::appearBySwitch() {
    appear();
    al::setNerve(this, &NrvDonketsuFall);
}

/**
 * @brief Checks whether the Donketsu is falling or defeated.
 * @return True if it is falling fast, in the fall or dead nerve, or dead.
 */
bool Donketsu::isFallOrDead() const {
    if (!al::isOnGround(this, 0, 0.0f) && al::getVelocity(this).y < -5.0f) {
        return true;
    }

    return al::isNerve(this, &NrvDonketsuFall) || al::isNerve(this, &NrvDonketsuDead) ||
           al::isDead(this);
}

/**
 * @brief Pushes another Donketsu in the slide direction to continue a slide chain.
 * @param pSelf Sensor of this Donketsu.
 * @param pOther Sensor that was touched.
 * @return True if the push was received.
 */
bool Donketsu::trySlideChainPush(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isNerve(this, &NrvDonketsuSlide) || !mIsEnableChainPush ||
        !al::isGreaterEqualStep(this, 2)) {
        return false;
    }

    sead::Vector3f dir;
    al::calcDirBetweenSensorsH(&dir, pSelf, pOther);
    if (mSlideDir.dot(dir) < 0.0f) {
        return false;
    }

    if (!rc::sendMsgDonketsuSlidePush(pOther, pSelf)) {
        return false;
    }

    al::startHitReactionHitEffect(this, "ドンケツ連鎖当たり", pSelf, pOther);
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvDonketsuFindPlayer);
    } else {
        al::setNerve(this, &NrvDonketsuWander);
    }

    return true;
}

/**
 * @brief Pushes enemies and bumps players, riders and Bowser Jr. away.
 * @param pSelf Sensor of the Donketsu.
 * @param pOther Sensor that was touched.
 */
void Donketsu::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther)) {
        trySlideChainPush(pSelf, pOther);
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if ((al::isSensorPlayer(pOther) && al::isSensorEnemyAttack(pSelf)) ||
        al::isSensorRide(pOther) || al::isSensorKoopaJr(pOther)) {
        if (al::isSensorPlayer(pOther) && rc::isPlayerGiant(pOther)) {
            return;
        }

        if (al::isNerve(this, &NrvDonketsuSlide) || al::isNerve(this, &NrvDonketsuSupportFreeze)) {
            al::sendMsgPush(pOther, pSelf);
            return;
        }

        if (!isEnableAttack()) {
            return;
        }

        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pSelf, pOther);
        f32 dot = front.dot(dir);
        if (al::sendMsgHitStrong(pOther, pSelf)) {
            mLastAttackSensor = pOther;
            mLastAttackPointer = nullptr;
            al::invalidateClipping(this);
            bool isFront = dot > 0.0f;
            f32 speed = isFront ? 3.0f : 14.0f;
            mSlideDir = -dir;
            al::startAction(this, isFront ? "PushFront" : "PushBack");
            al::setVelocityToDirection(this, mSlideDir, speed);
            mIsSlideToStay = !al::isNerve(this, &NrvDonketsuStay);
            if (!mIsSlideToStay) {
                al::scaleVelocityHV(this, 1.0f, 0.0f);
                al::addVelocityJump(this, 10.0f);
            }

            mIsEnableChainPush = false;
            al::setNerve(this, &NrvDonketsuSlide);
        }

        if (al::isSensorKoopaJr(pOther)) {
            al::sendMsgEnemyAttack(pOther, pSelf);
        }

        return;
    }

    if (!GameDataFunction::isSingleMode(this)) {
        return;
    }

    if (al::isSensorNpc(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        trySlideChainPush(pSelf, pOther);
        return;
    }

    if (al::isSensorKickKoura(pOther)) {
        al::sendMsgKickKouraReflect(pOther, pSelf);
        trySlideChainPush(pSelf, pOther);
    }
}

/**
 * @brief Checks whether the Donketsu can attack.
 * @return True unless it is sliding, dead or frozen.
 */
bool Donketsu::isEnableAttack() const {
    if (al::isNerve(this, &NrvDonketsuSlide) || al::isNerve(this, &NrvDonketsuDead) ||
        al::isNerve(this, &NrvDonketsuSupportFreeze)) {
        return false;
    }

    return true;
}

/**
 * @brief Handles pushes, slide chain pushes, breaking attacks and slide attacks.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the Donketsu.
 * @return True if the message was handled.
 */
bool Donketsu::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if (rc::isMsgDonketsuSlidePush(pMsg) && isEnableAttack() && mDamageCoolTime <= 0) {
        mDamageCoolTime = 5;
        al::calcDirBetweenSensorsH(&mSlideDir, pOther, pSelf);
        if (al::isNearZero(mSlideDir, 0.001f)) {
            mSlideDir = -al::getFront(this);
        }

        al::invalidateClipping(this);
        al::setVelocityToDirection(this, mSlideDir, 7.0f);
        al::startAction(this, "Slide");
        mIsSlideToStay = !al::isNerve(this, &NrvDonketsuStay);
        mIsEnableChainPush = true;
        al::setNerve(this, &NrvDonketsuSlide);
        return true;
    }

    if (isEnableBreak(pMsg, pSelf)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        al::appearItem(this);
        rc::addScoreCombo(this, pOther, pMsg, cScoreFactor);
        al::startHitReactionDeath(this);
        kill();
        return true;
    }

    if (!isEnableSlide(pMsg, pOther, pSelf)) {
        return false;
    }

    bool isFireBall = isMsgKoopaFireBallAttack(pMsg, pOther);
    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgNekoAttack(pMsg) || al::isMsgKeyThrow(pMsg) || isFireBall) {
        mDamageCoolTime = 25;
    }

    rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
    if (al::isSensorPlayer(pOther) && isMsgUseFrontDirToSlide(pMsg)) {
        mSlideDir = rc::getPlayerFront(pOther);
    } else {
        al::calcDirBetweenSensorsH(&mSlideDir, pOther, pSelf);
        if (al::isNearZero(mSlideDir, 0.001f)) {
            mSlideDir = -al::getFront(this);
        }
    }

    mLastAttackSensor = pOther;
    mLastAttackPointer = nullptr;
    mSlideDir.y = 0.0f;
    if (al::normalizeOrZero(&mSlideDir)) {
        al::calcFrontDir(&mSlideDir, this);
    }

    startSlide(pMsg);
    return true;
}

/**
 * @brief Checks whether a message breaks the Donketsu at once.
 * @param pMsg Received message.
 * @param pSelf Sensor of the Donketsu.
 * @return True for invincible attacks, and lasers in single player mode.
 */
bool Donketsu::isEnableBreak(const al::SensorMsg* pMsg, al::HitSensor* pSelf) const {
    if (al::isNerve(this, &NrvDonketsuDead) || !al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg)) {
        return true;
    }

    if (mIsSingleMode && al::isMsgLaserAttack(pMsg)) {
        return true;
    }

    return false;
}

/**
 * @brief Checks whether a message makes the Donketsu slide.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the Donketsu.
 * @return True if the Donketsu should slide.
 */
bool Donketsu::isEnableSlide(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) const {
    if (al::isNerve(this, &NrvDonketsuDead) || !al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (mIsSingleMode) {
        if (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
            al::isMsgKouraThrow(pMsg)) {
            return true;
        }

        bool isFireBall = isMsgKoopaFireBallAttack(pMsg, pOther);
        if (al::isMsgBallAttack(pMsg) || al::isMsgNekoAttack(pMsg) || al::isMsgKeyThrow(pMsg) ||
            isFireBall) {
            return mDamageCoolTime <= 0;
        }
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerFireBallAttack(pMsg) ||
        al::isMsgBallAttack(pMsg) || al::isMsgPlayerObjRollingAttackFailure(pMsg) ||
        al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
        al::isMsgPlayerBodyLanding(pMsg)) {
        return true;
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg)) {
        return mDamageCoolTime <= 0;
    }

    return false;
}

/**
 * @brief Checks whether a slide attack pushes the Donketsu in the player's facing direction.
 * @param pMsg Received message.
 * @return True for hip drop reflects, failed rolls, climb attacks and body attack reflects.
 */
bool Donketsu::isMsgUseFrontDirToSlide(const al::SensorMsg* pMsg) const {
    return al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
           al::isMsgPlayerObjRollingAttackFailure(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
           al::isMsgPlayerBodyAttackReflect(pMsg);
}

/**
 * @brief Gets the slide speed for an attack message.
 * @param pMsg Attack message that started the slide.
 * @return Initial slide speed.
 */
f32 Donketsu::calcSlideSpeed(const al::SensorMsg* pMsg) {
    if (al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerObjRollingAttackFailure(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgPlayerBodyAttackReflect(pMsg)) {
        return 30.0f;
    }

    if (al::isMsgPlayerGiantAttack(pMsg)) {
        return 40.0f;
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg)) {
        return 10.0f;
    }

    return al::isMsgPlayerFireBallAttack(pMsg) ? 7.0f : 12.0f;
}

/**
 * @brief Handles touch screen pushes and support freezing.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the Donketsu.
 * @return True if the message was handled.
 */
bool Donketsu::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                     al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg) && isEnableAttack()) {
        const sead::Vector3f& rTrans = al::getTrans(this);
        const sead::Vector3f& rHitPos = pPointer->getHitPos();
        mSlideDir.set(rTrans.x - rHitPos.x, 0.0f, rTrans.z - rHitPos.z);
        if (al::normalizeOrZero(&mSlideDir)) {
            al::calcFrontDir(&mSlideDir, this);
        }

        mLastAttackSensor = nullptr;
        mLastAttackPointer = pPointer;
        startSlide(pMsg);
        return true;
    }

    if (isEnableSupportFreeze() &&
        mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvDonketsuSupportFreeze)) {
            al::setNerve(this, &NrvDonketsuSupportFreeze);
        }

        return true;
    }

    return false;
}

/**
 * @brief Checks whether the Donketsu can be frozen by the support player.
 * @return True unless it is dead or sliding.
 */
bool Donketsu::isEnableSupportFreeze() const {
    if (al::isNerve(this, &NrvDonketsuDead) || al::isNerve(this, &NrvDonketsuSlide)) {
        return false;
    }

    return true;
}

/**
 * @brief Attacks the object of a wall the Donketsu runs into.
 * @return True if the attack was received.
 */
bool Donketsu::trySendMsgDonketsuAttackCollide() {
    if (al::isCollidedWall(this) &&
        al::getCollidedWallNormal(this).dot(al::getVelocity(this)) < 0.0f) {
        al::HitSensor* pWallSensor = al::tryGetCollidedWallSensor(this);
        if (al::sendMsgBallAttackCollide(pWallSensor, al::getHitSensor(this, "Body"))) {
            return true;
        }
    }

    return false;
}

/** @brief Updates the damage cool time and eyes, and handles fire and death areas. */
void Donketsu::control() {
    if (mDamageCoolTime > 0) {
        mDamageCoolTime--;
    }

    if (al::isNerve(this, &NrvDonketsuFindPlayer) || al::isNerve(this, &NrvDonketsuChase) ||
        al::isNerve(this, &NrvDonketsuStay)) {
        JointAimUtil::updateEyeJointInfo(this, mJointAimInfo, 1500.0f, 0.2f);
    } else {
        mJointAimInfo->subPowerRate(0.1f);
    }

    if (al::isNerve(this, &NrvDonketsuDead)) {
        return;
    }

    if (rc::isCollidedDamageFire(this)) {
        al::setVelocityZero(this);
        al::offCollide(this);
        al::setNerve(this, &NrvDonketsuDead);
        return;
    }

    if (rc::isInDeathArea(this)) {
        addScoreAndKill();
    }
}

/** @brief Wanders around until a player is found. */
void Donketsu::exeWander() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }

    al::updateNerveState(this);
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget() && al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvDonketsuFindPlayer);
    }
}

/** @brief Turns towards a found player, then starts chasing. */
void Donketsu::exeFindPlayer() {
    al::updateNerveStateAndNextNerve(this, &NrvDonketsuChase);
}

/** @brief Runs after the target, stopping in front of ledges. */
void Donketsu::exeChase() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
    }

    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        f32 distance = al::calcDistanceH(this, mTargetFinder->getTargetPos());
        f32 degree = al::lerpValue(distance, 300.0f, 1000.0f, 1.4f, 5.0f);
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this),
                                        mTargetFinder->getTargetPos(), degree);
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::addVelocityToDirection(this, al::getFront(this), 0.6f);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sParam.mWalkerStateParam);
    bool isStopAtLedge = false;
    if (WalkerStateFunction::isFallNextMoveY(this, mFallCheckOffsetY, 150.0f, 150.0f,
                                             sParam.mWalkerStateParam.mValueC, true) &&
        (!mIsSingleMode ||
         WalkerStateFunction::isFallNextMoveY(this, mFallCheckOffsetY, 200.0f, 150.0f,
                                              sParam.mWalkerStateParam.mValueC, true))) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(this);
        al::verticalizeVec(pVelocity, al::getFront(this), *pVelocity);
        isStopAtLedge = true;
    }

    if (rc::isCollidedDamageFire(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvDonketsuDead);
        return;
    }

    if (trySendMsgDonketsuAttackCollide()) {
        al::scaleVelocityHV(this, 0.6f, 1.0f);
    }

    if (al::isGreaterStep(this, 300) || isStopAtLedge) {
        setNerveByTarget();
        return;
    }

    if (!al::isOnGround(this, 10, 0.0f)) {
        al::setNerve(this, &NrvDonketsuFall);
    }
}

/** @brief Waits after a chase, then looks for the next target. */
void Donketsu::exeChaseEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sParam.mWalkerStateParam);
    if (rc::isCollidedDamageFire(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvDonketsuDead);
        return;
    }

    if (al::isGreaterStep(this, 60)) {
        setNerveByTarget();
    }
}

/** @brief Slides away after being hit, bouncing off walls and stopping at ledges. */
void Donketsu::exeSlide() {
    al::turnDirectionDegree(this, al::getFrontPtr(this), -mSlideDir, 0.0f);
    al::addVelocityToDirection(this, mSlideDir,
                               al::calcNerveEaseInOutValue(this, 0, 25, 1.0f, 0.0f));
    al::addVelocityToGravity(this, 1.0f);
    al::scaleVelocity(this, 0.94f);
    if (mIsSlideToStay && al::isGreaterEqualStep(this, 5) &&
        WalkerStateFunction::isFallNextMove(this, 100.0f, 150.0f,
                                            sParam.mWalkerStateParam.mValueC, true)) {
        al::scaleVelocityHV(this, 0.83f, 1.0f);
        al::setNerve(this, &NrvDonketsuStay);
        return;
    }

    if (trySendMsgDonketsuAttackCollide()) {
        f32 speed = -al::getCollidedWallNormal(this).dot(al::getVelocity(this));
        al::setVelocity(this, al::getCollidedWallNormal(this) * speed);
        al::scaleVelocityHV(this, 0.0f, 1.0f);
        al::addVelocityJump(this, 15.0f);
        al::setNerve(this, &NrvDonketsuWallHit);
        return;
    }

    if (al::isGreaterEqualStep(this, 40) && al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        setNerveByTarget();
        return;
    }

    if (!al::isOnGround(this, 10, 0.0f)) {
        al::setNerve(this, &NrvDonketsuFall);
    }
}

/** @brief Stops at a ledge, wobbling, then looks for the next target. */
void Donketsu::exeStay() {
    if (al::isFirstStep(this)) {
        if (al::getFront(this).dot(mSlideDir) > 0.0f) {
            al::startAction(this, "StayBack");
        } else {
            al::startAction(this, "StayFront");
            mSlideDir = -mSlideDir;
        }
    }

    al::turnDirectionDegree(this, al::getFrontPtr(this), mSlideDir, 6.0f);
    WalkerStateFunction::calcPassiveMovement(this, &sParam.mWalkerStateParam);
    al::scaleVelocityHV(this, 0.83f, 1.0f);
    if (al::isGreaterEqualStep(this, 120)) {
        setNerveByTarget();
        return;
    }

    if (!al::isOnGround(this, 4, 0.0f)) {
        al::setNerve(this, &NrvDonketsuFall);
    }
}

/** @brief Bounces back from a wall until landing. */
void Donketsu::exeWallHit() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WallHit");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sParam.mWalkerStateParam);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvDonketsuWallHitLand);
    }
}

/** @brief Lands after bouncing off a wall, then looks for the next target. */
void Donketsu::exeWallHitLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WallHitLand");
    }

    if (al::isActionEnd(this)) {
        setNerveByTarget();
    }
}

/** @brief Falls until landing. */
void Donketsu::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Fall");
    }

    al::addVelocityToGravity(this, 2.0f);
    al::scaleVelocity(this, 0.99f);
    if (rc::isCollidedDamageFire(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvDonketsuDead);
        return;
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        alPadRumbleFunction::startPadRumble(this, "LandStrong", -1, true);
        al::setNerve(this, &NrvDonketsuLand);
    }
}

/** @brief Lands after a fall, then looks for the next target. */
void Donketsu::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sParam.mWalkerStateParam);
    if (rc::isCollidedDamageFire(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvDonketsuDead);
        return;
    }

    if (al::isActionEnd(this)) {
        setNerveByTarget();
    }
}

/** @brief Stays frozen by the support player until released. */
void Donketsu::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvDonketsuWander);
    }
}

/** @brief Sinks into fire or a death area, then gives the score and dies. */
void Donketsu::exeDead() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Sink");
    }

    if (al::isActionEnd(this)) {
        addScoreAndKill();
    }
}
