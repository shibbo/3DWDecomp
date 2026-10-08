#include "Enemy/BossBunretsuChip.hpp"

#include <math/seadMathCalcCommon.h>

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/BossBunretsu.hpp"
#include "Enemy/BossBunretsuCore.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateBlowDownNoCollider.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/TargetFinder.hpp"
#include "Enemy/WalkerStateChase.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(BossBunretsuChip, Wait)
NERVE_DECL(BossBunretsuChip, Chase)
NERVE_DECL(BossBunretsuChip, BlowDown)
NERVE_DECL(BossBunretsuChip, SupportFreeze)
NERVE_DECL(BossBunretsuChip, PressDown)
NERVE_DECL(BossBunretsuChip, Splash)
NERVE_DECL(BossBunretsuChip, Death)
NERVE_DECL(BossBunretsuChip, DisappearPrepare)
NERVE_DECL(BossBunretsuChip, Disappear)
NERVE_DECL(BossBunretsuChip, Gather)
NERVE_DECL(BossBunretsuChip, GatherEnd)
NERVE_DECL(BossBunretsuChip, FindPlayer)
NERVE_DECL(BossBunretsuChip, MoveToCore)
NERVE_DECL(BossBunretsuChip, SplashLand)

// Nerves that share the execute function of another nerve.
#define BOSS_BUNRETSU_CHIP_NERVE_SHARED_DECL(Action, ExeFunc)                                      \
    class BossBunretsuChipNrv##Action : public al::Nerve {                                         \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<BossBunretsuChip>())->exe##ExeFunc();                              \
        }                                                                                          \
    };

BOSS_BUNRETSU_CHIP_NERVE_SHARED_DECL(GatherStartAppear, GatherStart)
BOSS_BUNRETSU_CHIP_NERVE_SHARED_DECL(GatherStart, GatherStart)

BossBunretsuChipNrvWait NrvBossBunretsuChipWait;
BossBunretsuChipNrvChase NrvBossBunretsuChipChase;
BossBunretsuChipNrvBlowDown NrvBossBunretsuChipBlowDown;
BossBunretsuChipNrvSupportFreeze NrvBossBunretsuChipSupportFreeze;
BossBunretsuChipNrvPressDown NrvBossBunretsuChipPressDown;
BossBunretsuChipNrvSplash NrvBossBunretsuChipSplash;
BossBunretsuChipNrvGatherStartAppear NrvBossBunretsuChipGatherStartAppear;
BossBunretsuChipNrvGatherStart NrvBossBunretsuChipGatherStart;
BossBunretsuChipNrvDeath NrvBossBunretsuChipDeath;
BossBunretsuChipNrvDisappearPrepare NrvBossBunretsuChipDisappearPrepare;
BossBunretsuChipNrvDisappear NrvBossBunretsuChipDisappear;
BossBunretsuChipNrvGather NrvBossBunretsuChipGather;
BossBunretsuChipNrvGatherEnd NrvBossBunretsuChipGatherEnd;
BossBunretsuChipNrvFindPlayer NrvBossBunretsuChipFindPlayer;
BossBunretsuChipNrvMoveToCore NrvBossBunretsuChipMoveToCore;
BossBunretsuChipNrvSplashLand NrvBossBunretsuChipSplashLand;

TargetFinderParam sTargetFinderParam(5000.0f, 180.0f, 80.0f, 60, -1.0f, -1.0f, -1.0f, 5000.0f,
                                     false);
WalkerStateParam sWalkerStateParam(1.8f, 0.998f, 0.92f, 500.0f, 750.0f, 70.0f, 80.0f, 150.0f);
WalkerStateChaseParam sChaseParam(1.5f, 60.0f, 60.0f, 3.5f, 60.0f, true, false, "Run", "Wait",
                                  -1.0f);
WalkerStateChaseParam sMoveToCoreParam(0.8f, 60.0f, 60.0f, 3.5f, 60.0f, true, false, "Run", "Wait",
                                       -1.0f);
EnemyStateBlowDownParam sBlowDownParam(false);
ActorStateSupportFreezeParam sSupportFreezeParam(true, 15);
}  // namespace

/**
 * @brief Constructs a chunk.
 * @param pParent Boss the chunk belongs to.
 * @param index Index of the chunk, which also delays its gathering.
 * @param pName Actor name.
 */
BossBunretsuChip::BossBunretsuChip(BossBunretsu* pParent, s32 index, const char* pName)
    : al::LiveActor(pName), mParent(pParent), mIndex(index) {}

/**
 * @brief Initializes the actor, the target finder and the chase, blow down and freeze states.
 * @param rInfo Actor init info.
 */
void BossBunretsuChip::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BossBunretsuChip", nullptr);
    makeActorAppeared();
    al::invalidateClipping(this);

    mTargetFinder = new TargetFinder(this, &sTargetFinderParam);
    mStateChase = new WalkerStateChase(this, al::getFrontPtr(this), mTargetFinder,
                                       &sWalkerStateParam, &sChaseParam, true,
                                       &mIsCollidedGround);
    mStateBlowDown = new EnemyStateBlowDownNoCollider(this, &sBlowDownParam, &mIsOnGround);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);

    al::initNerve(this, &NrvBossBunretsuChipWait, 3);
    al::initNerveState(this, mStateChase, &NrvBossBunretsuChipChase, "[state]追いかけ");
    al::initNerveState(this, mStateBlowDown, &NrvBossBunretsuChipBlowDown, "[state]吹き飛ばし");
    al::initNerveState(this, mStateSupportFreeze, &NrvBossBunretsuChipSupportFreeze,
                       "[state]フリーズ");
}

/**
 * @brief Pushes other chunks away and attacks players.
 * @param pSelf Own sensor.
 * @param pOther Sensor that was hit.
 */
void BossBunretsuChip::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pOther) && al::isSensorName(pOther, "Push")) {
        if (isSplashing()) {
            al::sendMsgPush(pOther, pSelf);
        } else {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) && isAttackable()) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf);
    }
}

/**
 * @brief Checks whether the chunk is flying apart.
 * @return true while splashing.
 */
bool BossBunretsuChip::isSplashing() const {
    return al::isNerve(this, &NrvBossBunretsuChipSplash);
}

/**
 * @brief Checks whether the chunk can hurt players in its current state.
 * @return true if the chunk is attacking.
 */
bool BossBunretsuChip::isAttackable() const {
    if (al::isNerve(this, &NrvBossBunretsuChipSplash)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipDisappearPrepare)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipDisappear)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherStartAppear)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGather)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvBossBunretsuChipDeath);
}

/**
 * @brief Handles pushes between chunks, stomps and player attacks.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Own sensor that received the message.
 * @return true if the message was handled.
 */
bool BossBunretsuChip::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    if (al::isMsgPush(pMsg) || al::isMsgPushStrong(pMsg)) {
        if (al::isNerve(this, &NrvBossBunretsuChipSplash) ||
            al::isNerve(this, &NrvBossBunretsuChipPressDown) ||
            al::isNerve(this, &NrvBossBunretsuChipBlowDown)) {
            return false;
        }

        if (al::isSensorEnemyBody(pSelf) && al::isSensorName(pSelf, "Push")) {
            sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
            sead::Vector3f velocity = al::getVelocity(this);
            if (mIsCollidedGround) {
                velocity.y = 0.0f;
                dir.y = 0.0f;
            }

            al::normalizeOrZero(&dir);
            f32 pushSpeed = isGathering() ? 2.0f : 3.0f;
            if (al::isMsgPushStrong(pMsg)) {
                pushSpeed = 6.0f;
            }

            f32 speed = pushSpeed - velocity.dot(dir);
            sead::Vector3f* velocityPtr = al::getVelocityPtr(this);
            if (speed > 0.0f) {
                *velocityPtr += dir * speed;
            } else {
                *velocityPtr += dir * 0.3f;
            }

            return true;
        }
    }

    if (!isReceivableAttack() || !al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if (EnemyStateUtil::tryRequestPressDown(pMsg, pOther, pSelf, false)) {
        rc::addScore(this, pOther, 0.0f, 0);
        al::setNerve(this, &NrvBossBunretsuChipPressDown);
        return true;
    }

    if (EnemyStateUtil::tryRequestBlowDown(pMsg, pOther, pSelf, mStateBlowDown, false)) {
        mIsOnGround = false;
        rc::addScore(this, pOther, 0.0f, 0);
        al::setNerve(this, &NrvBossBunretsuChipBlowDown);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the chunk was clamped to the floor height on the last update.
 * @return true if the chunk touches the floor.
 */
bool BossBunretsuChip::isCollidedGround() const {
    return mIsCollidedGround;
}

/**
 * @brief Checks whether the chunk is gathering back into the body.
 * @return true while gathering.
 */
bool BossBunretsuChip::isGathering() const {
    return al::isNerve(this, &NrvBossBunretsuChipGatherStart) ||
           al::isNerve(this, &NrvBossBunretsuChipGatherStartAppear) ||
           al::isNerve(this, &NrvBossBunretsuChipGather) ||
           al::isNerve(this, &NrvBossBunretsuChipGatherEnd);
}

/**
 * @brief Checks whether the chunk can currently be attacked.
 * @return true if attacks are received.
 */
bool BossBunretsuChip::isReceivableAttack() const {
    if (al::isNerve(this, &NrvBossBunretsuChipSplash)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipDisappearPrepare)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipDisappear)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherStartAppear)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGather)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipGatherEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipPressDown)) {
        return false;
    }

    if (al::isNerve(this, &NrvBossBunretsuChipBlowDown)) {
        return false;
    }

    return !al::isNerve(this, &NrvBossBunretsuChipDeath);
}

/**
 * @brief Handles touch screen pointer messages, which freeze the chunk.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return true if the message was handled by the freeze state.
 */
bool BossBunretsuChip::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                             al::ScreenPointer* pPointer,
                                             al::ScreenPointTarget* pTarget) {
    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvBossBunretsuChipSupportFreeze) && isReceivableAttack()) {
            al::setNerve(this, &NrvBossBunretsuChipSupportFreeze);
        }

        return true;
    }

    return false;
}

/**
 * @brief Moves the chunk by its velocity, keeping it above the floor height of the boss and
 * inside the arena around the boss start position.
 */
void BossBunretsuChip::updateCollider() {
    const sead::Vector3f& velocity = al::getVelocity(this);
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f next = trans + velocity;
    sead::Vector3f center = mParent->getInitTrans();
    if (next.y <= center.y) {
        next.y = center.y;
        mIsCollidedGround = true;
        mIsOnGround = al::getVelocity(this).y <= 0.0f;
    } else {
        mIsCollidedGround = false;
        mIsOnGround = false;
    }

    sead::Vector3f diff = {next.x - center.x, 0.0f, next.z - center.z};
    if (diff.squaredLength() > 1750.0f * 1750.0f) {
        f32 rate = 1750.0f / diff.length();
        next.x = center.x + diff.x * rate;
        next.z = center.z + diff.z * rate;
    }

    *al::getTransPtr(this) = next;
}

/**
 * @brief Checks whether the chunk stands on the floor.
 * @return true if on the floor and not moving up.
 */
bool BossBunretsuChip::isOnGround() const {
    return mIsOnGround;
}

/** @brief Appears above the boss and flies away in a random direction. */
void BossBunretsuChip::startBreakup() {
    sead::Vector3f trans = al::getTrans(mParent);
    trans.y += 100.0f;
    al::resetPosition(this, trans, false);
    mIsCollidedGround = false;
    mIsOnGround = false;
    appear();

    f32 angle = al::getRandom(sead::Mathf::pi2());
    f32 speed = al::getRandom(0.6f, 1.4f) * 15.0f;
    al::setVelocity(this, {speed * cosf(angle), 40.0f, speed * sinf(angle)});
    al::setNerve(this, &NrvBossBunretsuChipSplash);
}

/** @brief Starts gathering back into the body, appearing again if the chunk was dead. */
void BossBunretsuChip::startGather() {
    const al::Nerve* nerve;
    if (al::isDead(this)) {
        appear();
        al::hideModelIfShow(this);
        nerve = &NrvBossBunretsuChipGatherStartAppear;
    } else {
        nerve = &NrvBossBunretsuChipGatherStart;
    }

    al::invalidateClipping(this);
    al::setVelocityZero(this);
    al::setNerve(this, nerve);
}

/** @brief Ends the gathering by killing the chunk. */
void BossBunretsuChip::endGather() {
    kill();
}

/** @brief Dies together with the boss. */
void BossBunretsuChip::onDeath() {
    if (al::isDead(this)) {
        return;
    }

    al::setNerve(this, &NrvBossBunretsuChipDeath);
}

/** @brief Disappears when the boss is damaged. */
void BossBunretsuChip::onDamage() {
    if (al::isDead(this)) {
        return;
    }

    al::setNerve(this, &NrvBossBunretsuChipDisappearPrepare);
}

/**
 * @brief Checks whether the chunk has finished gathering.
 * @return true if gathered or dead.
 */
bool BossBunretsuChip::isGatherd() const {
    if (al::isNerve(this, &NrvBossBunretsuChipGatherEnd)) {
        return true;
    }

    return al::isDead(this);
}

/** @brief Waits a step, then chases a found player or walks back to the core. */
void BossBunretsuChip::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    movementProc();
    if (al::isGreaterStep(this, 0)) {
        mTargetFinder->update();
        if (mTargetFinder->isExistTarget()) {
            al::setNerve(this, &NrvBossBunretsuChipFindPlayer);
        } else {
            al::setNerve(this, &NrvBossBunretsuChipMoveToCore);
        }
    }
}

/** @brief Applies friction, and gravity while in the air. */
void BossBunretsuChip::movementProc() {
    al::scaleVelocityHV(this, 0.92f, 0.998f);
    if (mIsCollidedGround) {
        return;
    }

    al::addVelocityToGravity(this, 1.8f);
}

/** @brief Walks back to the core until a player is found. */
void BossBunretsuChip::exeMoveToCore() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
    }

    al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), al::getTrans(mParent->getCore()),
                                    sMoveToCoreParam.getRunAnimRate());
    if (mIsOnGround) {
        al::addVelocity(this, al::getFront(this) * sMoveToCoreParam.getAccel());
    }

    movementProc();
    mTargetFinder->update();
    if (mTargetFinder->isExistTarget()) {
        al::setNerve(this, &NrvBossBunretsuChipFindPlayer);
    }
}

/** @brief Reacts to a found player before chasing it. */
void BossBunretsuChip::exeFindPlayer() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Find");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuChipChase);
    }

    movementProc();
}

/** @brief Chases the player, then walks back to the core when it is lost. */
void BossBunretsuChip::exeChase() {
    mTargetFinder->refindTarget();
    if (al::updateNerveStateAndNextNerve(this, &NrvBossBunretsuChipMoveToCore)) {
        return;
    }

    movementProc();
}

/** @brief Flies apart until landing. */
void BossBunretsuChip::exeSplash() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startAction(this, "Splash");
    }

    al::scaleVelocity(this, 0.998f);
    if (mIsOnGround) {
        al::setNerve(this, &NrvBossBunretsuChipSplashLand);
        al::setVelocityZero(this);
        return;
    }

    al::addVelocityToGravity(this, 1.8f);
}

/** @brief Lands after flying apart. */
void BossBunretsuChip::exeSplashLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SplashLand");
    }

    movementProc();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuChipWait);
    }
}

/** @brief Starts gathering back into the body. */
void BossBunretsuChip::exeGatherStart() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::offCollide(this);
        if (al::isNerve(this, &NrvBossBunretsuChipGatherStartAppear)) {
            al::startAction(this, "GatherStartAppear");
        } else {
            al::startAction(this, "GatherStart");
        }
    }

    movementProc();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBossBunretsuChipGather);
    }
}

/** @brief Flies to the core, delayed by the chunk index. */
void BossBunretsuChip::exeGather() {
    if (al::isStep(this, mIndex)) {
        al::startAction(this, "Gather");
    }

    if (al::isGreaterStep(this, mIndex)) {
        sead::Vector3f coreTrans = al::getTrans(mParent->getCore());
        al::addVelocityToTarget(this, coreTrans, 20.0f);
        al::scaleVelocity(this, 0.8f);
        if (al::isGreaterEqualStep(this, mIndex + 10)) {
            al::setNerve(this, &NrvBossBunretsuChipGatherEnd);
        }
    }
}

/** @brief Ends the gathering by killing the chunk. */
void BossBunretsuChip::exeGatherEnd() {
    kill();
}

/** @brief Gets stomped and dies. */
void BossBunretsuChip::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "PressDown");
    }

    if (al::isActionEnd(this)) {
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Gets blown away and dies. */
void BossBunretsuChip::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::startHitReactionDeath(this);
        al::setVelocityZero(this);
        kill();
    }
}

/** @brief Stops before disappearing. */
void BossBunretsuChip::exeDisappearPrepare() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Wait");
    }

    al::setNerve(this, &NrvBossBunretsuChipDisappear);
}

/** @brief Disappears, then gets killed. */
void BossBunretsuChip::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Disappear");
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Stays frozen by the touch screen, then waits again. */
void BossBunretsuChip::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvBossBunretsuChipWait);
    }
}

/** @brief Dies together with the boss. */
void BossBunretsuChip::exeDeath() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "Death");
    }

    if (al::isActionEnd(this)) {
        kill();
    }
}
