#include "Enemy/Gamane.hpp"

#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/GamaneChameleon.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/CoinBlow.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Gamane, Wait)
NERVE_DECL(Gamane, SupportFreeze)
NERVE_DECL(Gamane, BlowDown)
NERVE_DECL(Gamane, PressDownBlow)
NERVE_DECL(Gamane, ReactionTail)
NERVE_DECL(Gamane, LandToRecover)
NERVE_DECL(Gamane, LandToDown)
NERVE_DECL(Gamane, ReactionDamageWait)
NERVE_DECL(Gamane, Trampled)
NERVE_DECL(Gamane, Appear)
NERVE_DECL(Gamane, Down)
NERVE_DECL(Gamane, ReactionDamage)
NERVE_DECL(Gamane, Fall)
NERVE_DECL(Gamane, Run)
NERVE_DECL(Gamane, RunInvisible)
NERVE_DECL(Gamane, Recover)
NERVE_DECL(Gamane, HipDropSpew)
// Run, RunInvisible and Recover sit right after their vtables; the rest form one block in .data.
NERVES_MAKE_NOSTRUCT(Gamane, Run, RunInvisible, Recover)
GamaneNrvWait NrvGamaneWait;
GamaneNrvSupportFreeze NrvGamaneSupportFreeze;
GamaneNrvBlowDown NrvGamaneBlowDown;
GamaneNrvPressDownBlow NrvGamanePressDownBlow;
GamaneNrvReactionTail NrvGamaneReactionTail;
GamaneNrvLandToRecover NrvGamaneLandToRecover;
GamaneNrvLandToDown NrvGamaneLandToDown;
GamaneNrvReactionDamageWait NrvGamaneReactionDamageWait;
GamaneNrvTrampled NrvGamaneTrampled;
GamaneNrvAppear NrvGamaneAppear;
GamaneNrvDown NrvGamaneDown;
GamaneNrvReactionDamage NrvGamaneReactionDamage;
GamaneNrvFall NrvGamaneFall;
GamaneNrvHipDropSpew NrvGamaneHipDropSpew;

constexpr s32 cCoinNumMax = 15;
constexpr s32 cDamageCoolTime = 20;

ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 0.0f, 0.0f));
EnemyStateBlowDownParam sBlowDownParam(false);
EnemyStateBlowDownParam sPressDownBlowParam("PressDownBlow");
EnemyStateBlowDownParam sReactionTailParam("ReactionTail", 10.3f, 20.0f, 0.995f, 1.1f, 20);
sead::Vector3f sCoinVelocityOffsetInWater(0.0f, 15.0f, 0.0f);

/**
 * @brief Scales a horizontal vector to a length, falling back to the Z axis when it is zero.
 * @param pVec Horizontal vector to scale (y must be zero).
 * @param length Length of the result.
 */
inline void setLengthHOrDirZ(sead::Vector3f* pVec, f32 length) {
    if (pVec->x == 0.0f && pVec->z == 0.0f) {
        pVec->set(0.0f, 0.0f, length);
        return;
    }

    pVec->normalize();
    pVec->x *= length;
    pVec->z *= length;
}

/**
 * @brief Calculates the horizontal velocity a Gamane jumps away with when it is hit.
 * @param pVelocity Output velocity.
 * @param pActor The Gamane.
 * @param pAttacker Sensor of the attacker, or nullptr to jump straight forward.
 */
void calcAppearVelocity(sead::Vector3f* pVelocity, const al::LiveActor* pActor,
                        const al::HitSensor* pAttacker) {
    if (pAttacker == nullptr) {
        pVelocity->set(0.0f, 30.0f, 9.0f);
        return;
    }

    const sead::Vector3f& rTrans = al::getTrans(pActor);
    const sead::Vector3f& rSensorPos = al::getSensorPos(pAttacker);
    pVelocity->set(rTrans.x - rSensorPos.x, 0.0f, rTrans.z - rSensorPos.z);
    setLengthHOrDirZ(pVelocity, 9.0f);
    pVelocity->y = 30.0f;
}

/**
 * @brief Checks whether a message is a player or projectile attack that hits a Gamane.
 * @param pMsg Received message.
 * @return Whether the message is an attack.
 */
bool isMsgAttackGamane(const al::SensorMsg* pMsg) {
    return al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
           al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
           al::isMsgPlayerClimbRollingAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg) ||
           al::isMsgPlayerBodyLanding(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
           al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
           al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) || al::isMsgKeyThrow(pMsg) ||
           rc::isMsgSkateShoesAttack(pMsg) || rc::isMsgPackunEat(pMsg) ||
           al::isMsgGigaEnemyAttack(pMsg) || al::isMsgLaserAttack(pMsg);
}
}  // namespace

/**
 * @brief Constructs a Gamane.
 * @param pName Actor name.
 */
Gamane::Gamane(const char* pName) : al::LiveActor(pName) {
    mCoinNum = cCoinNumMax;
}

/**
 * @brief Initializes the model, states, chameleon model and the coins it spews.
 * @param rInfo Placement info of the actor.
 */
void Gamane::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "GamaneFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    al::createAndSetColliderSpecialPurpose(this, "MoveLimitGamane");
    al::initNerve(this, &NrvGamaneWait, 4);
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    mStateBlowDown = new EnemyStateBlowDown(this, &sBlowDownParam);
    mStatePressDownBlow = new EnemyStateBlowDown(this, &sPressDownBlowParam);
    mStateReactionTail = new EnemyStateBlowDown(this, &sReactionTailParam);
    al::initNerveState(this, mStateSupportFreeze, &NrvGamaneSupportFreeze, "タッチ拘束");
    al::initNerveState(this, mStateBlowDown, &NrvGamaneBlowDown, "吹き飛び");
    al::initNerveState(this, mStatePressDownBlow, &NrvGamanePressDownBlow, "つぶれ吹き飛び");
    al::initNerveState(this, mStateReactionTail, &NrvGamaneReactionTail, "シッポひっくり返り");
    mChameleon = new GamaneChameleon("ガマネカメレオンモデル", this);
    al::initCreateActorWithPlacementInfo(mChameleon, rInfo);
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    }

    mCoins.allocBuffer(cCoinNumMax, nullptr);
    for (s32 i = 0; i < cCoinNumMax; i++) {
        mCoins.pushBack(new CoinBlow("ガマネー掃出しコイン"));
        mCoins.unsafeAt(i)->init(rInfo);
        mCoins[i]->setOffSensor(14);
    }

    makeActorAppeared();
    al::hideModel(this);
    const sead::Vector3f& rFront = al::getFront(this);
    mInitFront.x = rFront.x;
    mInitFront.y = rFront.y;
    mInitFront.z = rFront.z;
}

/**
 * @brief Syncs the model visibility with the chameleon, reacts to the microphone and checks
 * for lava, water and ink.
 */
void Gamane::control() {
    if (mChameleon->isTransparent()) {
        al::hideModelIfShow(this);
    } else {
        al::showModelIfHide(this);
    }

    if (mChameleon->isTransparent() && al::isMicInputOn(this)) {
        mChameleon->requestMicReaction();
    }

    mDamageCoolTime--;
    al::tryKillByDeathArea(this);
    if (!GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) || al::isNerve(this, &NrvGamaneHipDropSpew) ||
        al::isNerve(this, &NrvGamanePressDownBlow) || al::isNerve(this, &NrvGamaneBlowDown) ||
        !al::isAlive(this)) {
        return;
    }

    if (!rc::isInWaterArea(this) && !InkUtil::isInInkLimitSphere(this)) {
        if (!al::isOnGround(this, 0, 0.0f)) {
            return;
        }

        const char* materialName = al::getCollidedFloorMaterialCodeName(this);
        const char* codeName = al::getCollidedFloorCodeName(this);
        if (!al::isEqualString(materialName, "Lava") &&
            !al::isEqualString(materialName, "DamageFire") &&
            !al::isEqualString(codeName, "Lava") && !al::isEqualString(codeName, "DamageFire")) {
            return;
        }
    }

    if (mChameleon->isTransparent()) {
        mChameleon->requestDirectHit();
    }

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    mStateBlowDown->setBlowDirScale(-frontDir);
    al::setNerve(this, &NrvGamaneBlowDown);
}

/**
 * @brief Checks whether the chameleon model is currently transparent.
 * @return Whether the chameleon is transparent.
 */
bool Gamane::isTransparent() {
    return mChameleon->isTransparent();
}

/** @brief Restores the initial pose, refills the coins and starts waiting again. */
void Gamane::reappear() {
    if (!GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        return;
    }

    al::setFront(this, mInitFront);
    al::setFront(mChameleon, mInitFront);
    al::setTrans(mChameleon, al::getTrans(this));
    al::setNerve(this, &NrvGamaneWait);
    startGamaneAction("Wait");
    mChameleon->reappear();
    for (s32 i = mCoinNum; i < cCoinNumMax; i++) {
        mCoins[i]->setOffSensor(14);
        mCoins[i]->setWaiting();
        mCoins.unsafeAt(i)->makeActorDead();
    }

    mCoinNum = cCoinNumMax;
    makeActorAppeared();
    al::hideModelIfShow(this);
}

/**
 * @brief Starts an action on both the chameleon model and the Gamane.
 * @param pActionName Action name.
 */
void Gamane::startGamaneAction(const char* pActionName) {
    al::startAction(mChameleon, pActionName);
    al::startAction(this, pActionName);
}

/**
 * @brief Kills the Gamane together with its chameleon model.
 * @param isNoReaction Whether to skip the death hit reaction.
 */
void Gamane::killComplete(bool isNoReaction) {
    if (!al::isDead(this) && !isNoReaction) {
        al::startHitReactionDeath(this);
    }

    al::LiveActor::kill();
    mChameleon->kill();
}

/**
 * @brief Pushes other enemies and attacks the player.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void Gamane::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isDead(this) || al::isNerve(this, &NrvGamaneHipDropSpew) ||
        al::isNerve(this, &NrvGamanePressDownBlow) || al::isNerve(this, &NrvGamaneBlowDown)) {
        return;
    }

    if (al::isSensorEnemyBody(pOther)) {
        if (al::isSensorEnemyAttack(pSelf) && !mChameleon->isTransparent()) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
            if (al::isSensorGoalItem(pOther)) {
                al::sendMsgEnemyAttackFire(pOther, pSelf);
                return;
            }

            if (al::isSensorEnemyBody(pSelf)) {
                al::sendMsgPush(pOther, pSelf);
                return;
            }
        }
    }

    if (!al::isSensorPlayer(pOther) && !al::isSensorKoopaJr(pOther)) {
        return;
    }

    if (!al::isSensorEnemyAttack(pSelf)) {
        return;
    }

    if (mChameleon->isTransparent()) {
        if (al::isNerve(this, &NrvGamaneLandToRecover) || al::isNerve(this, &NrvGamaneLandToDown) ||
            al::isNerve(this, &NrvGamaneReactionDamageWait) ||
            al::isNerve(this, &NrvGamaneTrampled) || al::isNerve(this, &NrvGamaneAppear) ||
            al::isNerve(this, &NrvGamaneReactionDamage) ||
            al::isNerve(this, &NrvGamaneReactionTail)) {
            return;
        }

        mChameleon->requestDirectHit();
        calcAppearVelocity(&mAppearVelocity, this, pOther);
        al::setNerve(this, &NrvGamaneAppear);
        return;
    }

    al::sendMsgPush(pOther, pSelf);
    if (al::isNerve(this, &NrvGamaneDown) || al::isNerve(this, &NrvGamaneLandToRecover) ||
        al::isNerve(this, &NrvGamaneLandToDown) || al::isNerve(this, &NrvGamaneTrampled) ||
        al::isNerve(this, &NrvGamaneAppear) || al::isNerve(this, &NrvGamaneReactionTail)) {
        return;
    }

    mChameleon->requestDirectHit();
    if (al::sendMsgEnemyAttack(pOther, pSelf)) {
        al::setNerve(this, &NrvGamaneReactionDamage);
    }
}

/**
 * @brief Handles messages, dispatching to the transparent or real handler.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Gamane::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isDead(this) || al::isNerve(this, &NrvGamaneAppear) ||
        al::isNerve(this, &NrvGamaneReactionDamage) || al::isNerve(this, &NrvGamaneReactionTail) ||
        al::isNerve(this, &NrvGamaneHipDropSpew) || al::isNerve(this, &NrvGamanePressDownBlow) ||
        al::isNerve(this, &NrvGamaneBlowDown)) {
        return false;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isMsgPackunEatStart(pMsg) && !al::isSensorMapObj(pSelf)) {
        return true;
    }

    if (mChameleon->isTransparent()) {
        if (al::isSensorMapObj(pSelf)) {
            if (al::isMsgPlayerHipDropKnockDown(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg)) {
                mChameleon->requestHipDropReaction(pMsg, pOther);
                return true;
            }

            return false;
        }

        if (receiveMsgAtTransparent(pMsg, pOther, pSelf)) {
            mChameleon->requestDirectHit();
            return true;
        }
    } else {
        if (al::isSensorMapObj(pSelf)) {
            return false;
        }

        if (receiveMsgAtReal(pMsg, pOther, pSelf)) {
            mChameleon->requestDirectHit();
            return true;
        }
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isMsgPush(pMsg) &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 3.0f)) {
        return true;
    }

    return false;
}

/**
 * @brief Handles messages while transparent: any attack makes the Gamane appear.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Gamane::receiveMsgAtTransparent(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                     al::HitSensor* pSelf) {
    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgExplosion(pMsg) ||
        al::isMsgExplosionCollide(pMsg) || al::isMsgDisasterSpikeAttack(pMsg) ||
        isMsgAttackGamane(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
        al::isMsgPlayerObjHipDropAll(pMsg) ||
        al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        rc::isMsgSkateShoesAttack(pMsg) || al::isMsgKeyThrow(pMsg) || rc::isMsgPackunEat(pMsg) ||
        al::isMsgGigaEnemyAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
        al::isMsgPlayerGiantTouch(pMsg) ||
        (rc::isMsgNeedleRollerAttack(pMsg) && GameDataFunction::isSingleMode(GameDataHolderAccessor(this)))) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        calcAppearVelocity(&mAppearVelocity, this, pOther);
        al::setNerve(this, &NrvGamaneAppear);
        return true;
    }

    return false;
}

/**
 * @brief Handles messages while visible: attacks blow the Gamane away and make it spew coins.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Gamane::receiveMsgAtReal(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (al::isMsgPlayerRollingAttack(pMsg)) {
        return false;
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgExplosion(pMsg) ||
        al::isMsgExplosionCollide(pMsg) || al::isMsgDisasterSpikeAttack(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        mStateBlowDown->setBlowDir(pOther, pSelf);
        al::setNerve(this, &NrvGamaneBlowDown);
        return true;
    }

    if (al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerGiantTouch(pMsg)) {
        rc::addScoreCombo(this, pOther, pMsg, 0.0f);
        al::setNerve(this, &NrvGamaneHipDropSpew);
        return true;
    }

    if (isMsgAttackGamane(pMsg) || (rc::isMsgNeedleRollerAttack(pMsg) && GameDataFunction::isSingleMode(GameDataHolderAccessor(this)))) {
        if (mDamageCoolTime > 0) {
            return false;
        }

        mDamageCoolTime = cDamageCoolTime;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        appearCoinNormal();
        if (mCoinNum <= 10) {
            rc::addScoreCombo(this, pOther, pMsg, 0.0f);
            mStateBlowDown->setBlowDir(pOther, pSelf);
            al::setNerve(this, &NrvGamaneBlowDown);
        } else {
            mStateReactionTail->setBlowDir(pOther, pSelf);
            al::setNerve(this, &NrvGamaneReactionTail);
        }

        return true;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf)) {
        if (mDamageCoolTime > 0) {
            return false;
        }

        mDamageCoolTime = cDamageCoolTime;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        appearCoinNormal();
        if (mCoinNum <= 10) {
            mStatePressDownBlow->setBlowDir(pOther, pSelf);
            rc::addScoreCombo(this, pOther, pMsg, 0.0f);
            al::setNerve(this, &NrvGamanePressDownBlow);
        } else {
            al::setNerve(this, &NrvGamaneTrampled);
        }

        return true;
    }

    return false;
}

/**
 * @brief Handles touch screen messages, freezing the Gamane or making the chameleon react.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Gamane::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvGamaneHipDropSpew) || al::isNerve(this, &NrvGamanePressDownBlow) ||
        al::isNerve(this, &NrvGamaneBlowDown) || al::isNerve(this, &NrvGamaneFall) ||
        al::isNerve(this, &NrvGamaneReactionDamage)) {
        return false;
    }

    if (!mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (mChameleon->isTransparent()) {
        mChameleon->requestTouchReaction();
        return true;
    }

    mChameleon->requestDirectHit();
    if (!al::isNerve(this, &NrvGamaneSupportFreeze)) {
        al::setNerve(this, &NrvGamaneSupportFreeze);
    }

    return true;
}

/** @brief Kills the Gamane together with its chameleon model. */
void Gamane::kill() {
    al::startHitReactionDeath(this);
    al::LiveActor::kill();
    mChameleon->kill();
}

/** @brief Waits in place and lets the chameleon disappear. */
void Gamane::exeWait() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Wait");
        al::setVelocityZero(this);
        al::validateClipping(this);
        if (!mChameleon->isInvisible()) {
            mChameleon->requestDisappear();
        }
    }
}

/**
 * @brief Checks whether the chameleon model is currently invisible.
 * @return Whether the chameleon is invisible.
 */
bool Gamane::isInvisible() {
    return mChameleon->isInvisible();
}

/** @brief Jumps out of the chameleon after being hit and lands. */
void Gamane::exeAppear() {
    if (al::isFirstStep(this)) {
        mIsRunStopped = false;
        startGamaneAction("Appear");
        al::setVelocity(this, mAppearVelocity);
    }

    al::addVelocityToGravity(this, 1.6f);
    al::scaleVelocity(this, 0.98f);
    if (isGamaneActionEnd() && al::isOnGround(this, 0, 0.0f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvGamaneLandToRecover);
    }
}

/**
 * @brief Checks whether the current action of both the chameleon and the Gamane has ended.
 * @return Whether both actions have ended.
 */
bool Gamane::isGamaneActionEnd() {
    return al::isActionEnd(mChameleon) && al::isActionEnd(this);
}

/** @brief Notices the player, then starts running away. */
void Gamane::exeFind() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Find");
        al::setVelocityZero(this);
    }

    al::addVelocityToGravity(this, 1.6f);
    al::scaleVelocity(this, 0.98f);
    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneRun);
    }
}

/** @brief Runs away from the player until the chameleon becomes invisible. */
void Gamane::exeRun() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Run");
    }

    if (al::isGreaterEqualStep(this, 50)) {
        mChameleon->requestDisappear();
    }

    if (!runAway()) {
        mIsRunStopped = true;
    }

    if (mChameleon->isInvisible()) {
        al::setNerve(this, &NrvGamaneRunInvisible);
    }
}

/**
 * @brief Runs in a random direction, falling when off the ground and stopping when no player
 * is near.
 * @return Whether the Gamane keeps running.
 */
bool Gamane::runAway() {
    if (al::isFirstStep(this) || al::getNerveStep(this) % 80 == 0) {
        al::getRandomVector(&mRunDir, 1.0f);
        al::verticalizeVec(&mRunDir, sead::Vector3f::ey, mRunDir);
        al::normalizeOrZero(&mRunDir);
        if (al::isNearZero(mRunDir, 0.001f)) {
            mRunDir.e = sead::Vector3f::ex.e;
        }
    }

    al::addVelocityToGravity(this, 1.6f);
    al::scaleVelocity(this, 0.98f);
    al::walkAndTurnToDirection(this, mRunDir, 1.0f, 0.0f, 0.8f, 10.0f, false);
    if (!al::isLessStep(this, 2)) {
        if (!al::isOnGround(this, 0, 0.0f)) {
            al::setNerve(this, &NrvGamaneFall);
            return true;
        }

        if (al::findNearestPlayerId(this, 1500.0f) < 0) {
            if (!al::isOnGround(this, 0, 0.0f)) {
                al::setNerve(this, &NrvGamaneFall);
                return true;
            }

            al::setNerve(this, &NrvGamaneWait);
            return false;
        }
    }

    return true;
}

/** @brief Keeps running away while the chameleon is invisible. */
void Gamane::exeRunInvisible() {
    if (al::isFirstStep(this)) {
        startGamaneAction("RunInvisible");
    }

    if (!runAway()) {
        mIsRunStopped = true;
    }

    if (!mChameleon->isInvisible()) {
        al::setNerve(this, &NrvGamaneRun);
    }
}

/** @brief Falls until landing. */
void Gamane::exeFall() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        startGamaneAction("Fall");
    }

    al::addVelocityToGravity(this, 1.6f);
    al::scaleVelocity(this, 0.98f);
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvGamaneLandToRecover);
    }
}

/** @brief Lands and then lies down. */
void Gamane::exeLandToDown() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Land");
        al::setVelocityZero(this);
    }

    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneDown);
    }
}

/** @brief Lands and then recovers. */
void Gamane::exeLandToRecover() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Land");
        al::setVelocityZero(this);
    }

    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneRecover);
    }
}

/** @brief Plays the damage reaction after attacking the player. */
void Gamane::exeReactionDamage() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        startGamaneAction("ReactionDamage");
    }

    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneReactionDamageWait);
    }
}

/** @brief Runs away for a moment after the damage reaction. */
void Gamane::exeReactionDamageWait() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Run");
    }

    if (!runAway()) {
        mIsRunStopped = true;
    }

    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvGamaneRun);
    }
}

/** @brief Gets flipped over by a tail attack and lies down after landing. */
void Gamane::exeReactionTail() {
    if (al::isFirstStep(this)) {
        al::startAction(mChameleon, "ReactionTail");
    }

    if (al::updateNerveState(this) && al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvGamaneLandToDown);
    }
}

/** @brief Gets trampled and lies down. */
void Gamane::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        startGamaneAction("Trampled");
    }

    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneDown);
    }
}

/** @brief Lies down for a while. */
void Gamane::exeDown() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Down");
        al::setVelocityZero(this);
    }

    if (al::isGreaterEqualStep(this, 70)) {
        al::setNerve(this, &NrvGamaneRecover);
    }
}

/** @brief Gets back up and starts running. */
void Gamane::exeRecover() {
    if (al::isFirstStep(this)) {
        startGamaneAction("Recover");
        al::setVelocityZero(this);
    }

    if (isGamaneActionEnd()) {
        al::setNerve(this, &NrvGamaneRun);
    }
}

/** @brief Gets squashed and blown away, spewing all remaining coins before dying. */
void Gamane::exePressDownBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(mChameleon, "PressDownBlow");
    }

    if (al::updateNerveState(this)) {
        appearCoinDie();
        kill();
    }
}

/** @brief Spews all remaining coins. */
void Gamane::appearCoinDie() {
    s32 coinNum = mCoinNum;
    for (s32 i = 0; i < coinNum; i++) {
        appearCoinNormal();
    }
}

/** @brief Spews a coin every few frames after a hip drop and dies once all are out. */
void Gamane::exeHipDropSpew() {
    if (al::isFirstStep(this)) {
        startGamaneAction("HipDropSpew");
        al::setVelocityZero(this);
        al::changeEnvTextureStamp(this);
    }

    if (mCoinNum > 0 && al::getNerveStep(this) % 4 == 0) {
        appearCoinNormal();
    }

    if (mCoinNum <= 0 && isGamaneActionEnd()) {
        al::resetEnvTexture(this);
        kill();
    }
}

/** @brief Spews one coin forward from the mouth. */
void Gamane::appearCoinNormal() {
    mCoinNum--;
    if (mCoinNum < 0) {
        return;
    }

    sead::Vector3f trans = al::getTrans(this) + sead::Vector3f(0.0f, 50.0f, 0.0f) +
                           al::getFront(this) * (al::getRandom(0.0f, 35.0f) + 35.0f);
    sead::Vector3f dir = al::getFront(this);
    al::rotateVectorDegreeY(&dir, al::getRandom(-70.0f, 70.0f));
    bool isInWater = rc::isInWaterArea(this);
    sead::Vector3f velocity = dir * (isInWater ? 2.5f : 6.0f);
    CoinBlow* coin = mCoins[mCoinNum];
    al::setTrans(coin, trans);
    al::resetPosition(coin, false);
    sead::Vector3f offset =
        isInWater ? sCoinVelocityOffsetInWater : sead::Vector3f(0.0f, 28.0f, 0.0f);
    al::setVelocity(coin, velocity + offset);
    coin->setLifeTime(600);
    coin->appearWithHitReaction();
    al::startSe(coin, "PgAppearLight", nullptr);
}

/** @brief Gets blown away, spewing all remaining coins before dying. */
void Gamane::exeBlowDown() {
    if (al::isFirstStep(this)) {
        al::startAction(mChameleon, "BlowDown");
    }

    if (al::updateNerveState(this)) {
        appearCoinDie();
        kill();
    }
}

/** @brief Stays frozen by touch until released, then runs away. */
void Gamane::exeSupportFreeze() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGamaneRun);
    }
}
