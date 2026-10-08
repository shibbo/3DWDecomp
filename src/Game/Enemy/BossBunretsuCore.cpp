#include "Enemy/BossBunretsuCore.hpp"

#include "Enemy/BossBunretsu.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/WalkerStateFunction.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BossBunretsuCore, Wait)
NERVE_DECL(BossBunretsuCore, PressDown)
NERVE_DECL(BossBunretsuCore, BlowDown)
NERVE_DECL(BossBunretsuCore, Demo)
NERVE_DECL(BossBunretsuCore, SplashStart)
NERVE_DECL(BossBunretsuCore, GatherStart)
NERVE_DECL(BossBunretsuCore, Swoon)
NERVE_DECL(BossBunretsuCore, Splash)
NERVE_DECL(BossBunretsuCore, JumpStart)
NERVE_DECL(BossBunretsuCore, Jump)
NERVE_DECL(BossBunretsuCore, Gather)
NERVE_DECL(BossBunretsuCore, Death)
NERVE_DECL(BossBunretsuCore, SplashLand)
NERVE_DECL(BossBunretsuCore, SwoonEnd)
NERVE_DECL(BossBunretsuCore, Standup)
NERVE_DECL(BossBunretsuCore, Turn)
NERVE_DECL(BossBunretsuCore, JumpSign)
NERVE_DECL(BossBunretsuCore, TurnPrepare)
NERVE_DECL(BossBunretsuCore, FarFromPlayerTurn)
NERVE_DECL(BossBunretsuCore, FarFromPlayer)
NERVE_DECL(BossBunretsuCore, Walk)

// Nerves that share the execute function of another nerve.
#define BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(Action, ExeFunc)                                      \
    class BossBunretsuCoreNrv##Action : public al::Nerve {                                         \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<BossBunretsuCore>())->exe##ExeFunc();                              \
        }                                                                                          \
    };

BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(PressDownDeath, PressDown)
BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(BlowDownDeath, BlowDown)
BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(GatherRecoverStart, GatherStart)
BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(RecoverJumpStart, JumpStart)
BOSS_BUNRETSU_CORE_NERVE_SHARED_DECL(GatherRecover, Gather)

const BossBunretsuCoreNrvWait NrvBossBunretsuCoreWait{};
BossBunretsuCoreNrvPressDownDeath NrvBossBunretsuCorePressDownDeath;
BossBunretsuCoreNrvPressDown NrvBossBunretsuCorePressDown;
BossBunretsuCoreNrvBlowDownDeath NrvBossBunretsuCoreBlowDownDeath;
BossBunretsuCoreNrvBlowDown NrvBossBunretsuCoreBlowDown;
BossBunretsuCoreNrvDemo NrvBossBunretsuCoreDemo;
BossBunretsuCoreNrvSplashStart NrvBossBunretsuCoreSplashStart;
BossBunretsuCoreNrvGatherRecoverStart NrvBossBunretsuCoreGatherRecoverStart;
BossBunretsuCoreNrvGatherStart NrvBossBunretsuCoreGatherStart;
BossBunretsuCoreNrvSwoon NrvBossBunretsuCoreSwoon;
BossBunretsuCoreNrvSplash NrvBossBunretsuCoreSplash;
BossBunretsuCoreNrvRecoverJumpStart NrvBossBunretsuCoreRecoverJumpStart;
BossBunretsuCoreNrvJump NrvBossBunretsuCoreJump;
BossBunretsuCoreNrvGather NrvBossBunretsuCoreGather;
BossBunretsuCoreNrvGatherRecover NrvBossBunretsuCoreGatherRecover;
BossBunretsuCoreNrvDeath NrvBossBunretsuCoreDeath;
BossBunretsuCoreNrvSplashLand NrvBossBunretsuCoreSplashLand;
const BossBunretsuCoreNrvSwoonEnd NrvBossBunretsuCoreSwoonEnd{};
const BossBunretsuCoreNrvJumpStart NrvBossBunretsuCoreJumpStart{};
const BossBunretsuCoreNrvStandup NrvBossBunretsuCoreStandup{};
BossBunretsuCoreNrvTurn NrvBossBunretsuCoreTurn;
BossBunretsuCoreNrvJumpSign NrvBossBunretsuCoreJumpSign;
BossBunretsuCoreNrvTurnPrepare NrvBossBunretsuCoreTurnPrepare;
BossBunretsuCoreNrvFarFromPlayerTurn NrvBossBunretsuCoreFarFromPlayerTurn;
BossBunretsuCoreNrvFarFromPlayer NrvBossBunretsuCoreFarFromPlayer;
BossBunretsuCoreNrvWalk NrvBossBunretsuCoreWalk;

WalkerStateParam sWalkerStateParam(1.8f, 0.998f, 0.92f, 500.0f, 1500.0f, 70.0f, 80.0f, 150.0f);
WalkerStateParam sBlowDownWalkerStateParam(1.8f, 0.998f, 0.92f, 500.0f, 1500.0f, 70.0f, 80.0f,
                                           150.0f);

/**
 * @brief Checks whether the nearest active player is far away from an actor.
 * @param pActor Actor to measure from.
 * @return true if there is an active player further than 1500 units away.
 */
inline bool isFarFromNearestPlayer(const al::LiveActor* pActor) {
    al::LiveActor* player = rc::findNearestActivePlayerActor(pActor);
    if (player == nullptr) {
        return false;
    }

    const sead::Vector3f& playerTrans = al::getTrans(player);
    const sead::Vector3f& trans = al::getTrans(pActor);
    sead::Vector3f diff = playerTrans - trans;
    return diff.squaredLength() > 1500.0f * 1500.0f;
}
}  // namespace

/**
 * @brief Constructs the core.
 * @param pParent Boss the core belongs to.
 * @param pName Actor name.
 */
BossBunretsuCore::BossBunretsuCore(BossBunretsu* pParent, const char* pName)
    : al::LiveActor(pName), mParent(pParent) {}

/**
 * @brief Initializes the actor, its nerve and the fireball hit rumble.
 * @param rInfo Actor init info.
 */
void BossBunretsuCore::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BossBunretsuCore", nullptr);
    al::initNerve(this, &NrvBossBunretsuCoreWait, 0);
    makeActorAppeared();
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.3f, 30);
    al::invalidateClipping(this);
}

/**
 * @brief Pushes enemies and players away and attacks players.
 * @param pSelf Own sensor.
 * @param pOther Sensor that was hit.
 */
void BossBunretsuCore::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        if (isAttackable()) {
            al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
        }
    }
}

/**
 * @brief Handles stomps, fireballs and player attacks.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Own sensor that received the message.
 * @return true if the message was handled.
 */
bool BossBunretsuCore::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    if (isReceivableAttack() && al::isSensorEnemyBody(pSelf)) {
        if (EnemyStateUtil::tryRequestPressDown(pMsg, pOther, pSelf, false)) {
            mFireBallHitNum = 0;
            rc::addScore(this, pOther, 100.0f, mParent->receivedDamageNum());
            if (mParent->receiveDamage()) {
                al::setNerve(this, &NrvBossBunretsuCorePressDownDeath);
            } else {
                al::setNerve(this, &NrvBossBunretsuCorePressDown);
            }

            return true;
        }

        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg)) {
            mFireBallHitNum++;
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startMclAnim(this, "ReactionFireball");
            al::startSe(this, "PgFireBallHit");
            if (mFireBallHitNum >= 3) {
                mFireBallHitNum = 0;
                rc::addScore(this, pOther, 100.0f, mParent->receivedDamageNum());
                if (mParent->receiveDamage()) {
                    al::setNerve(this, &NrvBossBunretsuCoreBlowDownDeath);
                } else {
                    al::setNerve(this, &NrvBossBunretsuCoreBlowDown);
                }

                return true;
            }

            mRumble->start(0);
            return true;
        }

        if (al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
            al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
            al::isMsgPlayerSpinAttack(pMsg)) {
            mFireBallHitNum = 0;
            rc::addScore(this, pOther, 100.0f, mParent->receivedDamageNum());
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            if (mParent->receiveDamage()) {
                al::setNerve(this, &NrvBossBunretsuCoreBlowDownDeath);
            } else {
                al::setNerve(this, &NrvBossBunretsuCoreBlowDown);
            }

            return true;
        }
    }

    if (al::isMsgPush(pMsg)) {
        return false;
    }

    return false;
}

/**
 * @brief Handles touch screen pointer messages.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return true if the message is a touch assist message.
 */
bool BossBunretsuCore::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                             al::ScreenPointer* pPointer,
                                             al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/** @brief Updates the fireball hit rumble. */
void BossBunretsuCore::control() {
    if (mRumble->isEnd()) {
        return;
    }

    mRumble->calc();
    al::setScaleY(this, mRumble->getValueY() + 1.0f);
    if (mRumble->isEnd()) {
        al::setScaleY(this, 1.0f);
    }
}

/** @brief Appears and starts the battle start demo. */
void BossBunretsuCore::startDemo() {
    appear();
    al::setNerve(this, &NrvBossBunretsuCoreDemo);
}

/** @brief Appears at the boss position and jumps out of the body, away from the center. */
void BossBunretsuCore::startBreakup() {
    const BossBunretsu* parent = mParent;
    const sead::Vector3f& parentTrans = al::getTrans(parent);
    sead::Vector3f dir = {parent->getInitTrans().x - parentTrans.x, 0.0f,
                          parent->getInitTrans().z - parentTrans.z};
    al::normalizeOrDirZ(&dir);

    al::setTrans(this, al::getTrans(mParent));
    al::setFront(this, -dir);
    appear();

    dir.x *= 5.0f;
    dir.z *= 5.0f;
    dir.y = 30.0f;
    al::setVelocity(this, dir);
    al::setNerve(this, &NrvBossBunretsuCoreSplashStart);
}

/**
 * @brief Starts gathering the body back. The body recovers if the core was damaged, and also on
 * every third gather once the body has become small.
 */
void BossBunretsuCore::startGather() {
    bool isRecover = mIsRecover;
    if (mParent->isBodySizeSmall() && mSmallGatherNum++ >= 2) {
        isRecover = true;
    }

    const al::Nerve* nerve;
    if (isRecover) {
        mSmallGatherNum = 0;
        mIsRecover = false;
        nerve = &NrvBossBunretsuCoreGatherRecoverStart;
    } else {
        nerve = &NrvBossBunretsuCoreGatherStart;
    }

    al::setNerve(this, nerve);
}

/**
 * @brief Checks whether the core can currently be damaged.
 * @return true if attacks are received.
 */
bool BossBunretsuCore::isReceivableAttack() const {
    if (al::isNerve(this, &NrvBossBunretsuCoreDemo)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSwoon) && al::isLessStep(this, 15)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSplashStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSplash)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreRecoverJumpStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreJump)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherRecoverStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGather)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherRecover)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCorePressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCorePressDownDeath)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreBlowDownDeath)) {
        return false;
    }

    return !al::isNerve(this, &NrvBossBunretsuCoreDeath);
}

/**
 * @brief Checks whether the core can hurt players in its current state.
 * @return true if the core is attacking.
 */
bool BossBunretsuCore::isAttackable() const {
    if (al::isNerve(this, &NrvBossBunretsuCoreDemo)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSplashStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSplash)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreSplashLand)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreRecoverJumpStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreJump)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherRecoverStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGather)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreGatherRecover)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCorePressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCorePressDownDeath)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreBlowDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuCoreBlowDownDeath)) {
        return false;
    }

    return !al::isNerve(this, &NrvBossBunretsuCoreDeath);
}

/** @brief Ends the gathering by killing the core. */
void BossBunretsuCore::endGather() {
    kill();
}

/** @brief Battle start demo. */
void BossBunretsuCore::exeDemo() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLightAll(this, -1);
        al::startAction(this, "DemoBattleStart");
    }

    if (al::isStep(this, 360)) {
        al::killPrePassLightAll(this, -1);
    }
}

/** @brief Waits. */
void BossBunretsuCore::exeWait() {}

/** @brief Swoons after a hit. */
void BossBunretsuCore::exeSwoon() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Swoon");
    }

    if (al::isGreaterStep(this, 120)) {
        al::setNerve(this, &NrvBossBunretsuCoreSwoonEnd);
    }
}

/** @brief Recovers from swooning. */
void BossBunretsuCore::exeSwoonEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonEnd");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreJumpStart);
    }
}

/** @brief Jumps out of the boss body. */
void BossBunretsuCore::exeSplashStart() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLightAll(this, -1);
        al::startAction(this, "SplashStart");
        mMoveTime = 0;
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreSplash);
        return;
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvBossBunretsuCoreSplashLand);
    }
}

/** @brief Flies through the air after jumping out of the boss body. */
void BossBunretsuCore::exeSplash() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Splash");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvBossBunretsuCoreSplashLand);
    }
}

/** @brief Lands after jumping out of the boss body. */
void BossBunretsuCore::exeSplashLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SplashLand");
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreStandup);
    }
}

/** @brief Stands up after landing. */
void BossBunretsuCore::exeStandup() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Standup");
        al::setVelocityZero(this);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreTurn);
    }
}

/** @brief Walks forward, then turns again or jumps back to the body after a while. */
void BossBunretsuCore::exeWalk() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Walk");
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::addVelocity(this, al::getFront(this) * 0.8f);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (mMoveTime++ >= 360) {
        al::setNerve(this, &NrvBossBunretsuCoreJumpSign);
        return;
    }

    if (al::isGreaterEqualStep(this, 120)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBossBunretsuCoreTurnPrepare);
    }
}

/** @brief Waits a bit before turning, to the player if they are far away. */
void BossBunretsuCore::exeTurnPrepare() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait")) {
        al::startAction(this, "Wait");
    }

    if (al::isGreaterEqualStep(this, 10)) {
        if (isFarFromNearestPlayer(this)) {
            al::setNerve(this, &NrvBossBunretsuCoreFarFromPlayerTurn);
        } else {
            al::setNerve(this, &NrvBossBunretsuCoreTurn);
        }

        return;
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
}

/** @brief Turns to the nearest player while they are far away. */
void BossBunretsuCore::exeFarFromPlayerTurn() {
    if (al::isFirstStep(this)) {
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
    }

    if (mMoveTime++ >= 360) {
        al::setNerve(this, &NrvBossBunretsuCoreJumpSign);
        return;
    }

    if (mTargetPlayer == nullptr) {
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
        if (mTargetPlayer == nullptr) {
            return;
        }
    }

    sead::Vector3f dir = al::getTrans(mTargetPlayer) - al::getTrans(this);
    if (al::normalizeOrZero(&dir)) {
        return;
    }

    if (al::turnDirectionDegree(this, al::getFrontPtr(this), dir, 7.0f)) {
        al::setNerve(this, &NrvBossBunretsuCoreFarFromPlayer);
    }
}

/** @brief Waits while the player is far away. */
void BossBunretsuCore::exeFarFromPlayer() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "Wait")) {
        al::startAction(this, "Wait");
    }

    if (mMoveTime++ >= 360) {
        al::setNerve(this, &NrvBossBunretsuCoreJumpSign);
        return;
    }

    if (rc::isPlayerDead(mTargetPlayer) || al::isGreaterStep(this, 90)) {
        if (isFarFromNearestPlayer(this)) {
            al::setNerve(this, &NrvBossBunretsuCoreFarFromPlayerTurn);
        } else {
            al::setNerve(this, &NrvBossBunretsuCoreTurn);
        }
    }
}

/** @brief Turns to a random direction towards the player, away from the center. */
void BossBunretsuCore::exeTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Turn");

        sead::Vector3f dir = {0.0f, 0.0f, -1.0f};
        if (rc::calcActivePlayerNum(this) > 0) {
            al::LiveActor* player = rc::findNearestActivePlayerActor(this);
            if (player == nullptr) {
                return;
            }

            const sead::Vector3f& playerTrans = al::getTrans(player);
            const sead::Vector3f& trans = al::getTrans(this);
            dir.set(playerTrans.x - trans.x, 0.0f, playerTrans.z - trans.z);
            al::normalizeOrDirZ(&dir);

            const sead::Vector3f& initTrans = mParent->getInitTrans();
            const sead::Vector3f& coreTrans = al::getTrans(this);
            sead::Vector3f toCenter = {initTrans.x - coreTrans.x, 0.0f,
                                       initTrans.z - coreTrans.z};
            al::normalizeOrDirZ(&toCenter);

            f32 rate = al::getRandom();
            f32 dot = toCenter.dot(dir);
            f32 angle;
            if (dot > 0.0f) {
                dir -= toCenter * dot;
                al::normalizeOrDirZ(&dir);
                angle = rate * 30.0f;
            } else {
                angle = rate * 60.0f;
            }

            sead::Vector3f cross;
            cross.setCross(dir, toCenter);
            al::rotateVectorDegreeY(&dir, cross.y > 0.0f ? -angle : angle);
            dir = -dir;
        }

        mTurnDir = dir;
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::turnDirectionDegree(this, al::getFrontPtr(this), mTurnDir, 7.0f)) {
        al::setNerve(this, &NrvBossBunretsuCoreWalk);
    }
}

/** @brief Turns to the nearest player before jumping back to the body. */
void BossBunretsuCore::exeJumpSign() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "JumpSign");
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
    }

    if (mTargetPlayer != nullptr) {
        sead::Vector3f dir = al::getTrans(mTargetPlayer) - al::getTrans(this);
        if (!al::normalizeOrZero(&dir)) {
            al::turnDirectionDegree(this, al::getFrontPtr(this), dir, 4.0f);
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreJumpStart);
    }
}

/** @brief Prepares to jump back to the body. */
void BossBunretsuCore::exeJumpStart() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "JumpStart");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuCoreJump);
    }
}

/** @brief Jumps up, then starts gathering the body back. */
void BossBunretsuCore::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Jump");
        al::setVelocity(this, {0.0f, 40.0f, 0.0f});
    }

    WalkerStateFunction::calcPassiveMovement(this, &sWalkerStateParam);
    if (al::isGreaterStep(this, 10)) {
        startGather();
    }
}

/** @brief Starts gathering the body back. */
void BossBunretsuCore::exeGatherStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, al::isNerve(this, &NrvBossBunretsuCoreGatherRecoverStart) ?
                                  "GatherRecoverStart" :
                                  "GatherStart");
        al::setVelocityZero(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, al::isNerve(this, &NrvBossBunretsuCoreGatherRecoverStart) ?
                               static_cast<const al::Nerve*>(&NrvBossBunretsuCoreGatherRecover) :
                               &NrvBossBunretsuCoreGather);
    }
}

/** @brief Gathers the body back. */
void BossBunretsuCore::exeGather() {
    if (al::isFirstStep(this)) {
        bool isRecover = al::isNerve(this, &NrvBossBunretsuCoreGatherRecover);
        al::startAction(this, isRecover ? "GatherRecover" : "Gather");
        mParent->startGather(isRecover);
    }

    if (al::isStep(this, 105) && al::isNerve(this, &NrvBossBunretsuCoreGatherRecover)) {
        al::killPrePassLightAll(this, -1);
    }
}

/** @brief Gets stomped, which ends the battle on the last hit. */
void BossBunretsuCore::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        if (al::isNerve(this, &NrvBossBunretsuCorePressDownDeath)) {
            al::startAction(this, "DemoBattleEnd");
            al::stopBgm(this, "Boss", 20, -1);
        } else {
            al::startAction(this, "PressDown");
        }

        al::setFront(this, sead::Vector3f::ez);
        mIsRecover = true;
    }

    if (al::isStep(this, 2)) {
        al::startHitReactionPressDown(this);
    }

    if (al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvBossBunretsuCorePressDownDeath)) {
            mParent->startDeath();
            al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
            kill();
        } else {
            al::setNerve(this, &NrvBossBunretsuCoreRecoverJumpStart);
        }
    }
}

/** @brief Gets blown away, which ends the battle on the last hit. */
void BossBunretsuCore::exeBlowDown() {
    if (al::isFirstStep(this)) {
        // Blown backwards, but with a speed of zero.
        al::setVelocity(this, al::getFront(this) * -0.0f);
        if (al::isNerve(this, &NrvBossBunretsuCoreBlowDownDeath)) {
            al::startAction(this, "DeathBlowDown");
            al::stopBgm(this, "Boss", 20, -1);
        } else {
            al::startAction(this, "BlowDown");
        }

        al::setFront(this, sead::Vector3f::ez);
        mIsRecover = true;
    }

    if (al::isStep(this, 2)) {
        al::startHitReactionPressDown(this);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sBlowDownWalkerStateParam);
    if (al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvBossBunretsuCoreBlowDownDeath)) {
            mParent->startDeath();
            al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
            kill();
        } else {
            al::setNerve(this, &NrvBossBunretsuCoreRecoverJumpStart);
        }
    }
}

/** @brief Plays the battle end demo and dies. */
void BossBunretsuCore::exeDeath() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "DemoBattleEnd");
    }

    if (al::isActionEnd(this)) {
        mParent->startDeath();
        kill();
    }
}
