#include "Enemy/Bomb.hpp"

#include "Enemy/BombStateExplosion.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/TouchCarryItemState.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define BOMB_NERVE_SHARED_DECL(Action, ExeFunc)                                                    \
    class BombNrv##Action : public al::Nerve {                                                     \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Bomb>())->exe##ExeFunc();                                          \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Bomb, Wait)
NERVE_DECL(Bomb, PlayerHold)
NERVE_DECL(Bomb, Throw)
NERVE_DECL(Bomb, Explosion)
NERVE_DECL(Bomb, PopUpAppear)
NERVE_DECL(Bomb, DRCHold)
NERVE_DECL(Bomb, Attach)
NERVE_DECL(Bomb, Kicked)
BOMB_NERVE_SHARED_DECL(JumpKicked, Jump)
NERVE_DECL(Bomb, Reaction)
NERVE_DECL(Bomb, Jump)
BOMB_NERVE_SHARED_DECL(TrampledKoura, Trampled)
NERVE_DECL(Bomb, Trampled)
NERVE_DECL(Bomb, Fall)

// Non-const nerve objects: the game merges them into one block.
BombNrvWait NrvBombWait;
BombNrvPlayerHold NrvBombPlayerHold;
BombNrvThrow NrvBombThrow;
BombNrvExplosion NrvBombExplosion;
BombNrvPopUpAppear NrvBombPopUpAppear;
BombNrvDRCHold NrvBombDRCHold;
BombNrvAttach NrvBombAttach;
BombNrvKicked NrvBombKicked;
BombNrvJumpKicked NrvBombJumpKicked;
BombNrvReaction NrvBombReaction;
BombNrvJump NrvBombJump;
BombNrvTrampledKoura NrvBombTrampledKoura;
BombNrvTrampled NrvBombTrampled;
BombNrvFall NrvBombFall;

/// Throw and release tuning while the bomb is carried on the touch screen.
TouchCarryItemStateParam sTouchCarryParam(18.0f, 10.0f, 20.0f, -30.0f, 15.0f, 50.0f);
/// Flight of the bomb after the player throws it.
ItemStatePopUpFrontParam sThrowParam(sead::Vector3f(0.0f, 25.0f, 10.0f), 0.8f, 0.7f, 15, 0.99f,
                                     false, "Throw", true, nullptr);
/// Pop-up of the bomb out of a block.
ItemStatePopUpFrontParam sPopUpParam(sead::Vector3f(0.0f, 20.0f, 6.0f), 1.2f, 1.0f, 15, 0.0f, true,
                                     "PopUp", false, nullptr);
/// Explosion of the bomb.
BombStateExplosionParam sExplosionParam = {500.0f, 150.0f, 30.0f, sead::Vector3f(4.0f, 3.0f, 1.0f),
                                           1.0f, 2};
/// Hold offsets of the bomb for each player size and pose.
ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(7.0f, 0.0f, 0.0f), sead::Vector3f(9.0f, 0.0f, 0.0f),
    sead::Vector3f(7.0f, 0.0f, 0.0f), sead::Vector3f(48.0f, 0.0f, 0.0f),
    sead::Vector3f(7.0f, 0.0f, 0.0f), sead::Vector3f(7.0f, 0.0f, 0.0f),
    sead::Vector3f(7.0f, 0.0f, 0.0f), sead::Vector3f(10.0f, 0.0f, 0.0f),
    sead::Vector3f(40.0f, 0.0f, 0.0f), sead::Vector3f(10.0f, 0.0f, 0.0f),
    sead::Vector3f(90.0f, 100.0f, 0.0f));
}  // namespace

/**
 * @brief Constructs a bomb.
 * @param pName Actor name.
 * @param isItem Whether the bomb comes out of an item container and starts attached.
 */
Bomb::Bomb(const char* pName, bool isItem) : al::LiveActor(pName), mIsItem(isItem) {}

/**
 * @brief Initializes the model, the carry, throw, explosion, pop-up and touch states.
 * @param rInfo Actor init info.
 */
void Bomb::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Bomb", nullptr);
    al::initNerve(this, &NrvBombWait, 5);

    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, false);
    mStateThrow = new ItemStatePopUpFront(this);
    mStateExplosion = new BombStateExplosion(this, true, &sExplosionParam);
    mStatePopUpAppear = new ItemStatePopUpFront(this);
    mStateTouchCarry = new TouchCarryItemState(this, &sTouchCarryParam);
    mStatePopUpAppear->setParam(sPopUpParam, nullptr);

    al::initNerveState(this, mStatePlayerHold, &NrvBombPlayerHold, "プレイヤーに持たれる");
    al::initNerveState(this, mStateThrow, &NrvBombThrow, "投げられる");
    al::initNerveState(this, mStateExplosion, &NrvBombExplosion, "爆発");
    al::initNerveState(this, mStatePopUpAppear, &NrvBombPopUpAppear, "ブロックから出現");
    al::initNerveState(this, mStateTouchCarry, &NrvBombDRCHold, "[state]アイテム持ち運び");
    mStatePlayerHold->initColliderControl();
    rc::createExplosionComboCounter(this);

    if (mIsItem) {
        al::setNerve(this, &NrvBombAttach);
    }

    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) {
        mStatePlayerHold->setFlag23(true);
    }

    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}

/** @brief Kills the bomb, releasing it from the player or the touch screen if held. */
void Bomb::kill() {
    al::LiveActor::kill();

    if (al::isNerve(this, &NrvBombPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
        mHolderSensor = nullptr;
    } else if (al::isNerve(this, &NrvBombDRCHold)) {
        rc::releaseTouchPointerHoldItem(this, mTouchActor);
        mHolderSensor = nullptr;
    }
}

/** @brief Resets the explosion and the count-down, then appears. */
void Bomb::appear() {
    mStateExplosion->reset();
    mUserId = -1;
    mCountDown = -1;
    mComboCounter = nullptr;
    mHolderSensor = nullptr;
    al::validateClipping(this);
    al::LiveActor::appear();
    al::startMclAnim(this, "Default");
}

/**
 * @brief Explodes on contact while flying, otherwise pushes other actors away.
 * @param pSelf Sensor of the bomb.
 * @param pOther Sensor touched.
 */
void Bomb::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvBombThrow) ||
        (al::isNerve(this, &NrvBombKicked) &&
         rc::tryFindRelativeControlUserId(pOther) != mUserId)) {
        if (!al::isSensorKoopaJr(pOther)) {
            mComboCounter = rc::getExplosionComboCounter(this);
            if (al::isSensorName(pSelf, "Body") &&
                al::sendMsgExplosion(pOther, pSelf, mComboCounter)) {
                rc::setExplosionComboCounterNextIndex(this);
                al::setNerve(this, &NrvBombExplosion);
                return;
            }

            mComboCounter = nullptr;
        }
    } else if (al::isNerve(this, &NrvBombExplosion)) {
        mStateExplosion->attackSensor(pSelf, pOther, mComboCounter);
        return;
    }

    if (al::isNerve(this, &NrvBombPlayerHold) || al::isNerve(this, &NrvBombDRCHold) ||
        al::isNerve(this, &NrvBombExplosion)) {
        return;
    }

    if (!al::isSensorName(pSelf, "Body")) {
        return;
    }

    if (al::isSensorPlayer(pOther) && al::isNerve(this, &NrvBombWait)) {
        return;
    }

    if (mIsSingleMode && al::isSensorRide(pOther)) {
        return;
    }

    al::sendMsgPush(pOther, pSelf);
}

/**
 * @brief Sends the bomb rolling.
 * @param rDir Direction of the kick.
 * @param pKicker Sensor that kicked the bomb.
 */
void Bomb::kick(const sead::Vector3f& rDir, al::HitSensor* pKicker) {
    al::getVelocityPtr(this)->setScale(rDir, 15.0f);
    al::makeQuatFrontUp(al::getQuatPtr(this), rDir, sead::Vector3f::ey);
    al::setNerve(this, &NrvBombKicked);
}

/**
 * @brief Kicks the bomb along the horizontal part of a direction.
 * @param pDir Kick direction; flattened in place, falling back to the Z axis when vertical.
 * @param pKicker Sensor that kicked the bomb.
 */
inline void Bomb::kickHorizontal(sead::Vector3f* pDir, al::HitSensor* pKicker) {
    al::verticalizeVec(pDir, sead::Vector3f::ey, *pDir);
    if (al::normalizeOrZero(pDir)) {
        pDir->set(sead::Vector3f::ez);
    }

    kick(*pDir, pKicker);
}

/**
 * @brief Handles carrying, kicks, jump panels, explosions, fire and attacks.
 * @param pMsg Message received.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bomb.
 * @return Whether the message was handled.
 */
bool Bomb::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mUserId)) {
        return true;
    }

    if (al::isNerve(this, &NrvBombExplosion)) {
        if (al::isMsgPlayerReleaseDamage(pMsg)) {
            return true;
        }

        return al::isMsgPlayerReleaseDead(pMsg);
    }

    if (mIsSingleMode && al::isMsgPlayerCanCarry(pMsg)) {
        mIsPlayerCanCarry = true;
        return true;
    }

    if (al::isMsgGoalKill(pMsg)) {
        al::startHitReactionDisappear(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItemTiming(this, "Coin");
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }

    if (!al::isSensorName(pSelf, "Body")) {
        return false;
    }

    if (rc::isMsgJumpPanelAction(pMsg) && isEnableJumpPanel()) {
        al::addVelocity(this, sead::Vector3f(0.0f, 50.0f, 0.0f));
        if (al::isNerve(this, &NrvBombKicked)) {
            al::setNerve(this, &NrvBombJumpKicked);
            return true;
        }

        if (al::isNerve(this, &NrvBombReaction)) {
            al::startAction(this, mCountDown < 0 ? "WaitDefault" : "WaitCountDown");
        }

        al::setNerve(this, &NrvBombJump);
        return true;
    }

    if (!al::isNerve(this, &NrvBombPlayerHold) && !al::isNerve(this, &NrvBombPopUpAppear) &&
        (al::isMsgExplosion(pMsg) || al::isMsgKillerAttack(pMsg) ||
         al::isMsgPlayerGiantAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
         al::isMsgPlayerKouraAttack(pMsg) || al::isMsgLaserAttack(pMsg) ||
         rc::isMsgNeedleRollerAttack(pMsg))) {
        if (al::isNerve(this, &NrvBombTrampledKoura) && al::isLessStep(this, 14)) {
            return false;
        }

        mUserId = rc::tryFindRelativeControlUserId(pOther);
        mComboCounter = rc::tryGetMsgComboCount(pMsg);
        if (al::isNerve(this, &NrvBombDRCHold)) {
            mHolderSensor = nullptr;
            rc::releaseTouchPointerHoldItem(this, mTouchActor);
        }

        al::setNerve(this, &NrvBombExplosion);
        return true;
    }

    if (al::isNerve(this, &NrvBombWait) || al::isNerve(this, &NrvBombAttach) ||
        al::isNerve(this, &NrvBombThrow) || al::isNerve(this, &NrvBombPopUpAppear) ||
        (al::isNerve(this, &NrvBombTrampled) && al::isGreaterStep(this, 14))) {
        if (mStatePlayerHold->tryStartCarryUp(pMsg, pOther, true)) {
            mHolderSensor = pOther;
            mUserId = rc::tryFindRelativeControlUserId(pOther);
            al::setNerve(this, &NrvBombPlayerHold);
            return true;
        }

        if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 4.0f)) {
            return true;
        }
    } else if (al::isNerve(this, &NrvBombPlayerHold) &&
               mStatePlayerHold->receiveMsg(pMsg, pOther, pSelf)) {
        mStateThrow->setParam(sThrowParam, pOther);
        mHolderSensor = nullptr;
        mUserId = rc::tryFindRelativeControlUserId(pOther);
        al::setNerve(this, &NrvBombThrow);
        return true;
    }

    if (al::isNerve(this, &NrvBombWait)) {
        if (al::isSensorPlayer(pOther)) {
            if (al::isMsgPlayerKick(pMsg) || al::isMsgPlayerTrample(pMsg) ||
                al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
                al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
                al::isMsgPlayerObjRollingAttack(pMsg)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
                mUserId = rc::tryFindRelativeControlUserId(pOther);
                sead::Vector3f dir = rc::getPlayerFront(pOther);
                kickHorizontal(&dir, pOther);
                return true;
            }
        } else if (rc::isMsgBullAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            sead::Vector3f dir = al::getSensorPos(pSelf);
            dir -= al::getSensorPos(pOther);
            kickHorizontal(&dir, pOther);
            return true;
        } else if (mIsSingleMode && al::isSensorRide(pOther)) {
            if (rc::isMsgBobsledBodyAttack(pMsg)) {
                if (al::isNerve(this, &NrvBombKicked) && al::isLessStep(this, 10)) {
                    return false;
                }

                mUserId = rc::tryFindRelativeControlUserId(pOther);
                sead::Vector3f dir;
                al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
                kickHorizontal(&dir, pOther);
                return true;
            }
        } else if (al::isSensorKoopaJr(pOther) && al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            mUserId = -1;
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            kickHorizontal(&dir, pOther);
            return true;
        }
    }

    if (al::isMsgPlayerFireBallAttack(pMsg) && al::isSensorPlessie(pOther)) {
        return false;
    }

    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgEnemyAttackFire(pMsg)) {
        if (al::isNerve(this, &NrvBombWait) || al::isNerve(this, &NrvBombTrampled) ||
            al::isNerve(this, &NrvBombTrampledKoura) || al::isNerve(this, &NrvBombPopUpAppear)) {
            tryStartCountDown();
        }

        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        return true;
    }

    if (al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
        al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg)) {
        if (al::isNerve(this, &NrvBombDRCHold) || al::isNerve(this, &NrvBombPlayerHold) ||
            al::isNerve(this, &NrvBombReaction)) {
            return false;
        }

        if (al::isNerve(this, &NrvBombKicked) && al::isLessStep(this, 10)) {
            return false;
        }

        if ((al::isNerve(this, &NrvBombJump) || al::isNerve(this, &NrvBombJumpKicked)) &&
            al::isLessStep(this, 30)) {
            return false;
        }

        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (al::isNerve(this, &NrvBombAttach)) {
            sead::Vector3f dir(0.0f, 0.0f, 0.0f);
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            dir *= 5.0f;
            *al::getVelocityPtr(this) += dir;
        }

        if (al::isSensorKoopaJr(pOther)) {
            al::setVelocityBlowAttack(this, al::getSensorPos(pOther), 20.0f, 5.0f);
            return true;
        }

        al::setNerve(this, &NrvBombReaction);
        return true;
    }

    return false;
}

/**
 * @brief Checks whether a jump panel can launch the bomb.
 * @return Whether the bomb is resting, rolling or reacting to an attack.
 */
bool Bomb::isEnableJumpPanel() const {
    return al::isNerve(this, &NrvBombWait) || al::isNerve(this, &NrvBombKicked) ||
           al::isNerve(this, &NrvBombReaction);
}

/**
 * @brief Lights the fuse unless it already burns.
 * @return Whether the count-down started.
 */
bool Bomb::tryStartCountDown() {
    if (mCountDown >= 0) {
        return false;
    }

    mCountDown = 360;
    if (al::isActionPlaying(this, "TrampledDefault")) {
        al::startAction(this, "TrampledCountDown");
    } else {
        al::startAction(this, "WaitCountDown");
    }

    al::invalidateClipping(this);
    return true;
}

/**
 * @brief Lets the touch screen grab the bomb.
 * @param pMsg Message received.
 * @param pPointer Touch pointer.
 * @param pTarget Touched target.
 * @return Whether the bomb is held on the touch screen.
 */
bool Bomb::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if (!mStateTouchCarry->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        return false;
    }

    if (al::isNerve(this, &NrvBombPlayerHold) || al::isNerve(this, &NrvBombAttach) ||
        al::isNerve(this, &NrvBombExplosion)) {
        return false;
    }

    if (!al::isNerve(this, &NrvBombDRCHold)) {
        al::setNerve(this, &NrvBombDRCHold);
        mHolderSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        mTouchActor = DrcFunction::tryFindDrcTouchActor(this, pPointer);
        mUserId = rc::tryFindRelativeControlUserId(mHolderSensor);
    }

    return true;
}

/** @brief Explodes in deadly areas, shows the pickup guide and handles water in single mode. */
void Bomb::control() {
    if (EnemyStateUtil::isKillByAreaOrMaterialCode(this) &&
        !al::isNerve(this, &NrvBombExplosion)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvBombExplosion);
        return;
    }

    if (mIsSingleMode) {
        bool isGuideUser = rc::isCurrentGuideGameWindowUser(this);
        if (!isGuideUser) {
            mIsShowGuide = false;
        }

        bool isEnableGuide =
            al::isNerve(this, &NrvBombWait) || al::isNerve(this, &NrvBombPopUpAppear);
        if (isEnableGuide && mIsPlayerCanCarry) {
            auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(this));
            if (player != nullptr && player->getHoldingSensor() == nullptr &&
                !rc::isPlayerEquipHeadgear(player) && !mIsShowGuide) {
                if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
                    mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                        this, "SingleMode_GuideMessage", "PickupGuide_SingleJoycons",
                        GuideMessagePriority(3), -1, 0.0f);
                } else {
                    mIsShowGuide = rc::appearGuideGameWindowWithPriority(
                        this, "SingleMode_GuideMessage", "PickupGuide_DualJoycons",
                        GuideMessagePriority(3), -1, 0.0f);
                }
            }
        } else if (isEnableGuide ? mIsShowGuide :
                                   isGuideUser && rc::isGuideGameWindowActive(this)) {
            rc::disappearGuideGameWindow(this);
            mIsShowGuide = false;
        }

        mIsPlayerCanCarry = false;
        if (mHolderSensor != nullptr && al::isNerve(this, &NrvBombPlayerHold) &&
            rc::isPlayerInWaterSurface(mHolderSensor)) {
            rc::requestPlayerRelease(mHolderSensor);
            mStateThrow->setParam(sThrowParam, mHolderSensor);
            mHolderSensor = nullptr;
            mUserId = rc::tryFindRelativeControlUserId(mHolderSensor);
            al::setNerve(this, &NrvBombThrow);
        } else if (rc::isInWaterArea(this) && !al::isNerve(this, &NrvBombExplosion)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvBombExplosion);
            return;
        }
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
    }

    if (!al::isNerve(this, &NrvBombPlayerHold)) {
        BallStateFunction::setColliderReturnedSlowly(this, static_cast<s32>(mColliderRadius), 1);
    }
}

/**
 * @brief Appears where an enemy carrying the bomb was trampled.
 * @param pTrampler Actor whose pose the bomb takes.
 * @param pMsg Message that defeated the carrier.
 */
void Bomb::appearTrampled(const al::LiveActor* pTrampler, const al::SensorMsg* pMsg) {
    al::copyPose(this, pTrampler);
    al::resetPosition(this, false);
    appear();
    if (al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg)) {
        al::setNerve(this, &NrvBombTrampledKoura);
        return;
    }

    al::setNerve(this, &NrvBombTrampled);
}

/** @brief Appears popping up out of a block. */
void Bomb::appearPopUpFront() {
    appear();
    al::setNerve(this, &NrvBombPopUpAppear);
}

/**
 * @brief Checks whether the bomb still sits in its item container.
 * @return Whether the bomb is attached.
 */
bool Bomb::isAttach() const {
    return al::isNerve(this, &NrvBombAttach);
}

/** @brief Explodes, releasing the bomb from the player first. */
void Bomb::setNerveExplosion() {
    if (mUserId >= 0) {
        mComboCounter = rc::getExplosionComboCounterAndNextIndex(this);
    }

    if (al::isNerve(this, &NrvBombPlayerHold)) {
        rc::requestPlayerRelease(mHolderSensor);
        mHolderSensor = nullptr;
    }

    al::setNerve(this, &NrvBombExplosion);
}

/** @brief Explodes without hurting players. */
void Bomb::setNerveExplosionOffAttackToPlayer() {
    setNerveExplosion();
    mStateExplosion->setIsAttackToPlayer(false);
}

/**
 * @brief Explodes in deadly areas and on deadly floors.
 * @return Whether the bomb exploded.
 */
bool Bomb::tryExplosionByAreaOrMaterialCode() {
    if (!EnemyStateUtil::isKillByAreaOrMaterialCode(this)) {
        return false;
    }

    setNerveExplosionOffAttackToPlayer();
    return true;
}

/**
 * @brief Advances a burning fuse, blinking near the end and exploding at zero.
 * @param isAttackToPlayer Whether the explosion hurts players.
 * @return Whether the bomb exploded.
 */
bool Bomb::updateCountDown(bool isAttackToPlayer) {
    if (mCountDown < 0) {
        return false;
    }

    mCountDown--;
    if (mCountDown <= 0) {
        if (isAttackToPlayer) {
            setNerveExplosion();
        } else {
            setNerveExplosionOffAttackToPlayer();
        }

        return true;
    }

    al::holdSe(this, "PgFuseLv", nullptr);
    if (mCountDown == 180) {
        al::startMclAnim(this, "Blink");
    }

    if (mCountDown <= 180) {
        al::holdSe(this, "BlinkFast", nullptr);
    }

    return false;
}

/** @brief Updates the collider, following the holder while the bomb is carried. */
void Bomb::updateCollider() {
    if (mStatePlayerHold->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mStatePlayerHold->updateCollider(al::getHitSensor(this, "Body"));
}

/** @brief Rests on the ground, burning the fuse, until the bomb starts to fall. */
void Bomb::exeWait() {
    if (al::isFirstStep(this) && !mIsSingleMode) {
        al::validateClipping(this);
    }

    updateVelocity();
    if (updateCountDown(true)) {
        return;
    }

    if (al::isOnGround(this, 3, 0.0f)) {
        return;
    }

    if (mIsSingleMode) {
        al::invalidateClipping(this);
    }

    al::setNerve(this, &NrvBombFall);
}

/** @brief Applies ground friction, or gravity while airborne. */
void Bomb::updateVelocity() {
    if (al::isOnGround(this, 0, 0.0f)) {
        if (al::isCollidedGroundEdgeOrCorner(this)) {
            al::addVelocityToGravity(this, 1.0f);
        }

        al::scaleVelocity(this, 0.9f);
    } else {
        al::getVelocityPtr(this)->y += -1.2f;
    }
}

/** @brief Waits in the item container until the bomb lands. */
void Bomb::exeAttach() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvBombTrampled);
    }
}

/** @brief Falls until landing. */
void Bomb::exeFall() {
    if (updateCountDown(true)) {
        return;
    }

    updateVelocity();
    if (al::isOnGround(this, 0, 0.0f)) {
        al::startAction(this, mCountDown < 0 ? "LandDefault" : "LandCountDown");
        al::setNerve(this, &NrvBombWait);
    }
}

/** @brief Carried by the player: the fuse is lit on pickup. */
void Bomb::exePlayerHold() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensors(this);
        al::startSe(this, "HoldItem", nullptr);
        tryStartCountDown();
        al::startAction(this, "WaitCountDown");
        if (mIsSingleMode) {
            rc::disappearGuideGameWindow(this);
            mIsShowGuide = false;
        }
    }

    al::updateNerveState(this);
    updateCountDown(true);
}

/** @brief Flies after a throw, exploding on collision. */
void Bomb::exeThrow() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    if (al::isStep(this, 10)) {
        al::validateHitSensor(this, "Body");
    }

    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    if (al::isCollidedVelocity(this) || al::updateNerveState(this)) {
        al::getVelocity(this);
        al::getCollidedWallNormal(this);
        al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this);
        if (wallSensor != nullptr) {
            al::sendMsgExplosionCollide(wallSensor, al::getHitSensor(this, "Body"),
                                        mComboCounter);
        }

        setNerveExplosionOffAttackToPlayer();
        return;
    }

    updateCountDown(false);
}

/** @brief Explodes, then kills the bomb. */
void Bomb::exeExplosion() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/** @brief Recovers after the carrier was trampled. */
void Bomb::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TrampledDefault");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        if (al::isActionPlaying(this, "TrampledDefault")) {
            al::startAction(this, "WaitDefault");
        }

        al::setNerve(this, &NrvBombWait);
    }
}

/** @brief Pops up out of a block. */
void Bomb::exePopUpAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitDefault");
    }

    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    if (al::updateNerveState(this)) {
        al::startAction(this, "LandDefault");
        al::setVelocityZero(this);
        al::setNerve(this, &NrvBombWait);
    }
}

/** @brief Rolls after a kick, exploding against walls. */
void Bomb::exeKicked() {
    if (al::isFirstStep(this)) {
        tryStartCountDown();
        al::startAction(this, "Rolling");
    }

    if (tryExplosionByAreaOrMaterialCode()) {
        return;
    }

    if (al::isCollidedWall(this)) {
        al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this);
        if (wallSensor != nullptr) {
            al::sendMsgExplosionCollide(wallSensor, al::getHitSensor(this, "Body"),
                                        mComboCounter);
        }

        setNerveExplosionOffAttackToPlayer();
        return;
    }

    if (updateCountDown(false)) {
        return;
    }

    al::addVelocityToGravity(this, 1.0f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
}

/** @brief Reacts to an attack, lighting the fuse. */
void Bomb::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mCountDown < 0 ? "TrampledDefault" : "TrampledCountDown");
    }

    if (updateCountDown(true)) {
        return;
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvBombWait);
    }
}

/** @brief Held on the touch screen until it is thrown or released. */
void Bomb::exeDRCHold() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mCountDown < 0 ? "WaitDefault" : "WaitCountDown");
    }

    if (al::updateNerveState(this)) {
        if (mStateTouchCarry->isItemThrow()) {
            sead::Vector3f velocity = mStateTouchCarry->getThrowVelocity();
            al::makeQuatFrontUp(al::getQuatPtr(this), velocity, sead::Vector3f::ey);
            al::setVelocity(this, velocity);
            mStateThrow->setParam(sThrowParam, nullptr);
            mHolderSensor = nullptr;
            al::setNerve(this, &NrvBombThrow);
            tryStartCountDown();
        } else {
            al::setVelocity(this, mStateTouchCarry->getReleaseVelocity());
            mHolderSensor = nullptr;
            al::setNerve(this, &NrvBombFall);
            al::startHitReaction(this, "DRC放す");
        }
    }

    if (updateCountDown(true)) {
        rc::releaseTouchPointerHoldItem(this, mTouchActor);
    }
}

/** @brief Flies off a jump panel, exploding on landing if it was kicked. */
void Bomb::exeJump() {
    updateVelocity();
    if (updateCountDown(true)) {
        return;
    }

    if (!al::isOnGround(this, 0, 0.0f)) {
        return;
    }

    if (al::isNerve(this, &NrvBombJumpKicked)) {
        setNerveExplosion();
        return;
    }

    al::startAction(this, mCountDown < 0 ? "LandDefault" : "LandCountDown");
    al::setNerve(this, &NrvBombWait);
}
