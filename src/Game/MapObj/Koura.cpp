#include "MapObj/Koura.hpp"

#include <math/seadQuat.h>

#include "Layout/GuideGameWindow.hpp"
#include "Layout/IslandMap.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Actor/ComboCounterWithSe.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/KouraBindPuppeteer.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve whose execute function is shared with another nerve.
#define KOURA_NERVE_SHARED_DECL(Action, ExeFunc)                                                   \
    class KouraNrv##Action : public al::Nerve {                                                    \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<Koura>()->exe##ExeFunc();                                           \
        }                                                                                          \
    };

namespace {
NERVE_DECL(Koura, Wait)
NERVE_DECL(Koura, Hold)
NERVE_DECL(Koura, RouteDokan)
NERVE_DECL(Koura, Release)
NERVE_DECL(Koura, KouraSlide)
NERVE_DECL(Koura, KouraWait)
NERVE_DECL(Koura, Upper)
// Bind end after the rider was hit by another player: the rider gets dizzy.
KOURA_NERVE_SHARED_DECL(BindEndDamage, BindEnd)
NERVE_DECL(Koura, KouraJump)
NERVE_DECL(Koura, Blow)
NERVE_DECL(Koura, WaitRelease)
NERVE_DECL(Koura, KouraStop)
// Blow after the shell was stolen into a goal binder: scores when it disappears.
KOURA_NERVE_SHARED_DECL(BlowGoal, Blow)
// Upper hop when a Koopa Troopa leaves its shell upside down.
KOURA_NERVE_SHARED_DECL(UpperFromNokonoko, Upper)
NERVE_DECL(Koura, BindStart)
NERVE_DECL(Koura, DemoKill)
NERVE_DECL(Koura, WaitForBind)
NERVE_DECL(Koura, RouteDokanEnd)
// Jump started while falling (no extra hop velocity).
KOURA_NERVE_SHARED_DECL(KouraJumpFall, KouraJump)
NERVE_DECL(Koura, KouraJumpBreak)
// Bind end after the shell stopped sliding: the rider gets dizzy.
KOURA_NERVE_SHARED_DECL(BindEndStop, BindEnd)
// Bind end requested by the rider: it exits the shell.
KOURA_NERVE_SHARED_DECL(BindEndExit, BindEnd)
// Bind end requested by the rider during a jump: it exits and stays in the air.
KOURA_NERVE_SHARED_DECL(BindEndExitStay, BindEnd)

// Non-const nerve objects: the game keeps them in .data, merged into one block.
KouraNrvWait NrvKouraWait;
KouraNrvHold NrvKouraHold;
KouraNrvRouteDokan NrvKouraRouteDokan;
KouraNrvRelease NrvKouraRelease;
KouraNrvKouraSlide NrvKouraKouraSlide;
KouraNrvKouraWait NrvKouraKouraWait;
KouraNrvUpper NrvKouraUpper;
KouraNrvBindEndDamage NrvKouraBindEndDamage;
KouraNrvKouraJump NrvKouraKouraJump;
KouraNrvBlow NrvKouraBlow;
KouraNrvWaitRelease NrvKouraWaitRelease;
KouraNrvKouraStop NrvKouraKouraStop;
KouraNrvBlowGoal NrvKouraBlowGoal;
KouraNrvUpperFromNokonoko NrvKouraUpperFromNokonoko;
KouraNrvBindStart NrvKouraBindStart;
KouraNrvDemoKill NrvKouraDemoKill;
KouraNrvWaitForBind NrvKouraWaitForBind;
KouraNrvRouteDokanEnd NrvKouraRouteDokanEnd;
KouraNrvKouraJumpFall NrvKouraKouraJumpFall;
KouraNrvKouraJumpBreak NrvKouraKouraJumpBreak;
KouraNrvBindEndStop NrvKouraBindEndStop;
KouraNrvBindEndExit NrvKouraBindEndExit;
KouraNrvBindEndExitStay NrvKouraBindEndExitStay;

/// Constant parameters of the shell, constructed at startup.
struct KouraParam {
    sead::Vector3f holdOffset;
    sead::Vector3f routeDokanSensorOffset;
    ItemStatePlayerHoldParam playerHoldParam;
};

KouraParam sParam = {
    sead::Vector3f(0.0f, -25.0f, 50.0f),
    sead::Vector3f(0.0f, 0.0f, 0.0f),
    ItemStatePlayerHoldParam(
        sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
        sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
        sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
        sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
        sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
        sead::Vector3f(0.0f, 0.0f, 0.0f)),
};

/**
 * @brief Converts a water sink depth into a buoyancy rate.
 * @param depth Depth below the water surface.
 * @return 0 when barely sunk, 1 when deep under water.
 */
inline f32 calcWaterRate(f32 depth) {
    if (depth <= 5.0f) {
        return 0.0f;
    }

    if (depth >= 50.0f) {
        return 1.0f;
    }

    return depth / 45.0f;
}
}  // namespace

/**
 * @brief Hides the single mode carry guide if this shell shows it.
 */
inline void Koura::tryDisappearGuide() {
    if (mIsShowGuide) {
        rc::disappearGuideGameWindow(this);
        mIsShowGuide = false;
    }
}

/**
 * @brief Shows the single mode carry guide matching the controller layout.
 */
inline void Koura::tryAppearGuide() {
    if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
        mIsShowGuide = rc::appearGuideGameWindowWithPriority(
            this, "SingleMode_GuideMessage", "KouraGuide_SingleJoycons",
            static_cast<GuideMessagePriority>(1), -1, 0.0f);
    } else {
        mIsShowGuide = rc::appearGuideGameWindowWithPriority(
            this, "SingleMode_GuideMessage", "KouraGuide_DualJoycons",
            static_cast<GuideMessagePriority>(1), -1, 0.0f);
    }
}

/**
 * @brief Allows warping between islands again in single mode.
 */
inline void Koura::tryEnableIslandWarp() {
    if (mIsSingleMode) {
        IslandMap::setIslandWarpEnable(this, true);
    }
}

/**
 * @brief Creates the shell with its two bind puppeteers.
 * @param pName Actor name.
 */
Koura::Koura(const char* pName)
    : al::LiveActor(pName), mComboCounter(new al::ComboCounterWithSe(this)) {
    mIsHiddenByBind = false;
    mBindPuppeteers = new KouraBindPuppeteer*[2];
    mBindPuppeteers[0] = new KouraBindPuppeteer(this);
    mBindPuppeteers[1] = new KouraBindPuppeteer(this);
    mBindPuppeteer = mBindPuppeteers[0];
    mTimer = new KouraTimer();
}

/**
 * @brief Initializes the model, the nerve states and the sensor/collision controllers.
 * @param rInfo Actor init info.
 */
void Koura::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, getArchiveName(), nullptr);
    al::initNerve(this, &NrvKouraWait, 2);
    mHoldState = new ItemStatePlayerHold(this, &sParam.playerHoldParam, false, true);
    mRouteDokanState = new ActorStateRouteDokanMove(this, rInfo);
    mRouteDokanState->setRouteSelecter(mBindPuppeteer->getRouteSelecter());
    al::initNerveState(this, mHoldState, &NrvKouraHold, "[state]プレイヤーに持たれる");
    al::initNerveState(this, mRouteDokanState, &NrvKouraRouteDokan, "[state]ルート土管移動");
    mHoldState->initColliderControl();
    mSensorController = al::createActorSensorController(this, "Body");
    mCollisionController = al::createActorCollisionController(this);
    al::setEffectFollowMtxPtr(this, "HitCollision", &mEffectMtx);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    mSlideTimeMax = mIsSingleMode ? 700 : 350;
    makeActorAppeared();
}

/**
 * @brief Kills the shell, resetting it to wait.
 */
void Koura::kill() {
    mIsHiddenByBind = false;
    al::setNerve(this, &NrvKouraWait);
    al::LiveActor::kill();
}

/**
 * @brief Handles route pipes, rider binding and attacks against touched sensors.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void Koura::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isEnableRouteDokan() && mRouteDokanState->tryStart(pSelf, pOther)) {
        al::offCollide(this);
        al::setNerve(this, &NrvKouraRouteDokan);
        return;
    }

    if (al::isSensorEnemyBody(pOther) && al::isSensorName(pOther, "BodyReact")) {
        if (al::isNerve(this, &NrvKouraRelease)) {
            al::sendMsgKouraThrow(pOther, pSelf);
            return;
        }

        if (al::isNerve(this, &NrvKouraWait)) {
            al::sendMsgKickKouraReflect(pOther, pSelf);
            return;
        }
    }

    if (al::isSensorEye(pSelf)) {
        if (al::isSensorBindableAll(pOther) && mBindPuppeteer->isBind() &&
            al::sendMsgBindSteal(pOther, pSelf)) {
            mIsBindGoal = al::isSensorBindableGoal(pOther);
            rc::requestPlayerBind(mPlayerSensor, pOther);
        }

        return;
    }

    if (GameDataFunction::isSingleMode(this) && al::isSensorMapObj(pOther) && isPlayerInside() &&
        mPlayerSensor != nullptr && mBindPuppeteer->getPlayerSensor() != nullptr &&
        al::sendMsgPlayerItemGet(pOther, mPlayerSensor)) {
        auto* player =
            static_cast<PlayerActor*>(al::getSensorHost(mBindPuppeteer->getPlayerSensor()));
        if (player != nullptr) {
            player->logGetItem(pOther);
        }

        return;
    }

    if (al::isNerve(this, &NrvKouraRouteDokan)) {
        if (al::isSensorRide(pOther)) {
            rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, mRouteDokanState->getMoveDirection());
        }

        if (!al::isSensorPlayerOrPlayerWeapon(pOther)) {
            rc::sendMsgRouteDokanItemGet(pOther, pSelf);
            rc::sendMsgRouteDokanKouraAttack(pOther, pSelf);
        }
    } else if (isAttack()) {
        if (al::isSensorPlayerOrPlayerWeapon(pOther)) {
            if (mTimer->mInvalidAttackTime <= 0 &&
                !(mBindPuppeteer->isBind() &&
                  al::getSensorHost(mBindPuppeteer->getPlayerSensor()) ==
                      al::getSensorHost(pOther))) {
                al::sendMsgEnemyAttack(pOther, pSelf);
            }
        } else {
            if (!isPlayerInside() && mTimer->mInvalidBlowTime <= 0 &&
                al::sendMsgKickKouraBlow(pOther, pSelf)) {
                sead::Vector3f pos =
                    (al::getSensorPos(pOther) + al::getSensorPos(pSelf)) * 0.5f;
                al::startHitReactionHitEffect(this, "命中", pos);
                startBlow(pSelf, pOther);
            }

            if (isPlayerInside() && rc::sendMsgPlayerCheckpointTouch(pOther, pSelf)) {
                return;
            }

            if (mIsSingleMode && !isPlayerInside() && al::isSensorPlessie(pOther) &&
                rc::isPlayerOnRaidon(al::findNearestPlayerActor(this))) {
                al::startSe(this, "PgHitPlessie", nullptr);
            }

            al::sendMsgKickKouraGetItem(pOther, pSelf);
            if (al::sendMsgKickKouraReflect(pOther, pSelf)) {
                sead::Vector3f dir = sead::Vector3f::zero;
                al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
                if (al::isNearZero(dir, 0.001f)) {
                    sead::Vector3f velocity = al::getVelocity(this);
                    velocity.x = -velocity.x;
                    velocity.z = -velocity.z;
                    al::setVelocity(this, velocity);
                } else {
                    f32 dot = dir.dot(al::getVelocity(this));
                    if (dot < 0.0f) {
                        al::addVelocity(this, dir * dot * -2.0f);
                        setMoveDir(al::getVelocity(this));
                        sead::Vector3f pos = al::getTrans(this);
                        al::calcPosBetweenSensors(&pos, pOther, pSelf, 0.0f);
                        al::makeMtxFrontUpPos(&mEffectMtx, dir, -al::getGravity(this), pos);
                        al::startHitReaction(this, "衝突");
                    }
                }
            }

            if (al::sendMsgKickKouraBreak(pOther, pSelf)) {
                startBreak();
                return;
            }

            if (al::isNerve(this, &NrvKouraKouraSlide)) {
                if (al::sendMsgPlayerKouraAttack(pOther, pSelf, mComboCounter)) {
                    resetTimer();
                }
            } else {
                al::sendMsgKickKouraAttack(pOther, pSelf, mComboCounter);
            }
        }
    } else if (al::isSensorMapObj(pOther) && al::sendMsgKickKouraGetItem(pOther, pSelf)) {
        return;
    }

    if (al::isNerve(this, &NrvKouraWait) && !al::isSensorPlayer(pOther) &&
        !(mIsSingleMode && al::isSensorRide(pOther))) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (!al::isSensorEnemy(pOther)) {
        return;
    }

    if (al::isNerve(this, &NrvKouraHold) &&
        al::sendMsgKickKouraAttack(pOther, pSelf, mComboCounter)) {
        al::resetActorCollisionController(mCollisionController, 1);
        startBlow(pSelf, pOther);
        return;
    }

    if (al::isNerve(this, &NrvKouraKouraWait) && al::isSensorEnemy(pOther)) {
        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        startKouraSlide(dir);
        al::startHitReactionHitEffect(this, "敵キック", pOther, pSelf);
    }
}

/**
 * @brief Checks whether the shell may enter a route pipe.
 * @return Whether entering a route pipe is allowed.
 */
bool Koura::isEnableRouteDokan() const {
    if (al::isNerve(this, &NrvKouraHold)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraBlow)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraBlowGoal)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraRouteDokan)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraRouteDokanEnd)) {
        return false;
    }

    return mTimer->mInvalidRouteDokanTime < 1;
}

/**
 * @brief Checks whether a player rides inside the shell.
 * @return Whether a player is inside.
 */
bool Koura::isPlayerInside() const {
    if (al::isNerve(this, &NrvKouraBindStart)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraSlide)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraStop)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraWait)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraJump)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraJumpFall)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraJumpBreak)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraBindEndStop)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraBindEndDamage)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraBindEndExit)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraBindEndExitStay)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraRouteDokan)) {
        return mBindPuppeteer->isBind();
    }

    return false;
}

/**
 * @brief Checks whether the shell is moving fast enough to attack.
 * @return Whether the shell is sliding, released or jumping.
 */
bool Koura::isAttack() const {
    return al::isNerve(this, &NrvKouraRelease) || al::isNerve(this, &NrvKouraKouraSlide) ||
           al::isNerve(this, &NrvKouraKouraJump) || al::isNerve(this, &NrvKouraKouraJumpFall);
}

/**
 * @brief Blows the shell away from the attacker.
 * @param pSelf Own sensor.
 * @param pOther Attacking sensor.
 */
void Koura::startBlow(const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    sead::Vector3f velocity;
    al::calcDirBetweenSensorsH(&velocity, pSelf, pOther);
    mBlowDir.set(sead::Vector3f::zero);
    if (al::isNearZero(velocity, 0.001f)) {
        velocity.set(0.0f, 27.5f, 5.3f);
    } else {
        if (isPlayerInside()) {
            mBlowDir = -velocity;
        }

        velocity.set(velocity.x * 5.3f, 27.5f, velocity.z * 5.3f);
    }

    tryDisappearGuide();
    al::setVelocity(this, velocity);
    al::setNerve(this, &NrvKouraBlow);
}

/**
 * @brief Sets the horizontal move direction.
 * @param rDir New direction (flattened and normalized).
 */
void Koura::setMoveDir(const sead::Vector3f& rDir) {
    mMoveDir = rDir;
    mMoveDir.y = 0.0f;
    al::normalizeOrZero(&mMoveDir);
}

/**
 * @brief Breaks the shell, releasing any rider or holder.
 */
void Koura::startBreak() {
    al::setVelocityZero(this);
    al::startHitReactionBreak(this);
    startKill();
}

/**
 * @brief Restarts the slide spin timer.
 */
void Koura::resetTimer() {
    al::startHitReaction(this, "コウラ滑り再スタート");
    mSlideTime = mSlideTimeMax;
    mRotateSpeed = 30.0f;
}

/**
 * @brief Starts sliding with a rider inside.
 * @param rDir Slide direction.
 */
void Koura::startKouraSlide(const sead::Vector3f& rDir) {
    mMoveSpeed = 20.0f;
    sead::Vector3f velocity = rDir * 20.0f;
    if (mIsJumpBoost) {
        mIsJumpBoost = false;
        velocity.y = 12.0f;
    }

    al::setVelocity(this, velocity);
    setMoveDir(rDir);
    mBlowDir.set(sead::Vector3f::zero);
    al::setNerve(this, &NrvKouraKouraSlide);
}

/**
 * @brief Handles the messages received by the shell.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own sensor.
 * @return Whether the message was handled.
 */
bool Koura::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    // Leftover bind message checks whose results are unused.
    al::isMsgBindCancel(pMsg) || al::isMsgBindStart(pMsg);

    if (mBindPuppeteers[0]->isTargetSensor(pOther) &&
        mBindPuppeteers[0]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (mBindPuppeteers[1]->isTargetSensor(pOther) &&
        mBindPuppeteers[1]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    al::isMsgBindStart(pMsg);

    if (rc::isMsgAskControlUserId(pMsg, mPlayerSensor)) {
        return true;
    }

    if (al::isMsgPush(pMsg)) {
        if (al::isSensorPlayer(pOther)) {
            return false;
        }

        if (al::isNerve(this, &NrvKouraWait) ||
            (mIsSingleMode && al::isNerve(this, &NrvKouraRelease) &&
             al::isSensorHostName(pOther, "シュモック[レール移動]★"))) {
            return al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 2.0f);
        }

        return false;
    }

    if ((al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
         al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
         al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
         al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
         al::isMsgPlayerSpinAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
         al::isMsgBallTrample(pMsg) || al::isMsgKeyThrow(pMsg) ||
         rc::isMsgPackunThrowAttack(pMsg) || al::isMsgExplosion(pMsg) ||
         (mIsSingleMode &&
          (al::isMsgEnemyAttackBoomerang(pMsg) || al::isMsgEnemyAttackFire(pMsg)))) &&
        mTimer->mInvalidTrampleTime <= 0) {
        if (isEnableKick() || al::isNerve(this, &NrvKouraRelease) ||
            al::isNerve(this, &NrvKouraKouraSlide) || al::isNerve(this, &NrvKouraKouraWait) ||
            (!al::isMsgExplosion(pMsg) && al::isNerve(this, &NrvKouraUpper) &&
             al::getVelocity(this).y < 0.0f)) {
            if (al::isNerve(this, &NrvKouraKouraWait) ||
                al::isNerve(this, &NrvKouraKouraSlide)) {
                if (mPlayerSensor == nullptr) {
                    return false;
                }

                if (al::getSensorHost(pOther) == al::getSensorHost(mPlayerSensor)) {
                    return false;
                }

                al::setNerve(this, &NrvKouraBindEndDamage);
                return true;
            }

            if (!rc::isMsgPackunThrowAttack(pMsg) && !al::isMsgEnemyAttackBoomerang(pMsg) &&
                !al::isMsgEnemyAttackFire(pMsg)) {
                rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            }

            if (mLiftingCount >= 7) {
                al::appearItemTiming(this, mIsSingleMode ? "SuperBellSpecial" : "リフティング");
                al::startHitReactionBreak(this);
                startKill();
                return true;
            }

            rc::addScoreByFactor(this, pOther, "敵", 100.0f, mLiftingCount);
            if (mLiftingCount >= 1) {
                s32 beatCount = mLiftingCount < 7 ? mLiftingCount : 7;
                al::startSeSetSeqLoacalVariableByName(this, "BeatSequentially", 0, beatCount);
            }

            mLiftingCount++;
            sead::Vector3f dir;
            if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg)) {
                dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
            } else {
                bool isBodyAttack = al::isMsgPlayerBodyAttack(pMsg);
                const sead::Vector3f& velocity = al::getVelocity(al::getSensorHost(pOther));
                if (isBodyAttack) {
                    dir = -velocity;
                } else {
                    dir = velocity;
                }
            }

            dir.y = 0.0f;
            al::normalizeOrDirZ(&dir);
            al::setVelocity(this, dir * 4.0f);
            al::setNerve(this, &NrvKouraUpper);
            return true;
        }
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        if (isEnableKick()) {
            rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
            if (!isPlayerInside() && (!mIsSingleMode || rc::isReallyPlayerActor(pOther))) {
                mPlayerSensor = pOther;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::startHitReaction(this, "キック");
            startMove(rc::getPlayerFront(pOther), 21.0f, 20.0f);
            mTimer->mInvalidAttackTime = 8;
            mTimer->mInvalidTrampleTime = 8;
            return true;
        }

        if (!isEnableTrampleStop()) {
            return false;
        }

        rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
        mComboCounter->reset();
        if (al::isNerve(this, &NrvKouraRelease)) {
            al::setVelocityZero(this);
            al::setNerve(this, &NrvKouraWait);
        } else if (al::isNerve(this, &NrvKouraKouraWait) ||
                   al::isNerve(this, &NrvKouraKouraSlide)) {
            if (mPlayerSensor == nullptr) {
                return false;
            }

            if (al::getSensorHost(pOther) == al::getSensorHost(mPlayerSensor)) {
                return false;
            }

            al::setNerve(this, &NrvKouraBindEndDamage);
        }

        al::setVelocityZero(this);
        mTimer->mInvalidTrampleTime = 8;
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReaction(this, "停止");
        return true;
    }

    if (al::isMsgKickKouraReflect(pMsg)) {
        sead::Vector3f unused;
        al::calcDirBetweenSensorsH(&unused, pOther, pSelf);
        if (al::isNerve(this, &NrvKouraRelease)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            startMove(dir, 21.0f, 20.0f);
            return true;
        }

        if (al::isNerve(this, &NrvKouraKouraSlide)) {
            sead::Vector3f normal = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
            normal.y = 0.0f;
            if (al::normalizeOrZero(&normal)) {
                normal.set(sead::Vector3f::ez);
            }

            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
            return true;
        }

        if (al::isNerve(this, &NrvKouraWait) || al::isNerve(this, &NrvKouraKouraWait)) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNerve(this, &NrvKouraKouraWait)) {
                startKouraSlide(dir);
            } else {
                startMove(dir, 30.0f, 20.0f);
            }

            return true;
        }

        if (al::isNerve(this, &NrvKouraKouraJump)) {
            return true;
        }
    }

    if (al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 2.0f)) {
        return true;
    }

    if (al::isMsgPlayerKick(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg)) {
        if (al::isNerve(this, &NrvKouraKouraWait)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (!isPlayerInside() && (!mIsSingleMode || rc::isReallyPlayerActor(pOther))) {
                mPlayerSensor = pOther;
            }

            startKouraSlide(dir);
            mTimer->mInvalidAttackTime = 20;
            return true;
        }

        if (!isEnableKick()) {
            return false;
        }

        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::startHitReaction(this, "キック");
        if (!isPlayerInside() && (!mIsSingleMode || rc::isReallyPlayerActor(pOther))) {
            mPlayerSensor = pOther;
        }

        startMove(rc::getPlayerFront(pOther), 21.0f, 20.0f);
        mTimer->mInvalidBlowTime = 20;
        mTimer->mInvalidAttackTime = 20;
        return true;
    }

    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg)) {
        if (al::isNerve(this, &NrvKouraBlow)) {
            return false;
        }

        if (isInRouteDokan()) {
            if (isPlayerInside()) {
                return false;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
            al::startHitReaction(this, "ルート土管死亡");
            startKill();
            return true;
        }

        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
        startBlow(pSelf, pOther);
        return true;
    }

    if (isEnableHold() && mHoldState->tryStartCarryFront(pMsg, pOther, false) &&
        !(mIsSingleMode &&
          (rc::isPlayerRaccoonDogWhite(pOther) || rc::isPlayerClimbWhite(pOther) ||
           rc::isPlayerInvincible(al::getSensorHost(pOther))))) {
        mPlayerSensor = pOther;
        mLiftingCount = 0;
        al::setNerve(this, &NrvKouraHold);
        return true;
    }

    if (isHold()) {
        if (al::isMsgKickKouraBlow(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (al::isNerve(this, &NrvKouraHold)) {
                endHold();
                startBlow(pSelf, pOther);
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (rc::isPlayerBinded(mPlayerSensor, pSelf)) {
                al::setNerve(this, &NrvKouraWaitRelease);
            }

            if (al::isNerve(this, &NrvKouraHold)) {
                mHoldState->receiveMsg(pMsg, pOther, pSelf);
                endHold();
                sead::Vector3f front = rc::getPlayerFront(mPlayerSensor);
                startMove(front, 30.0f, 20.0f);
                mTimer->mInvalidTrampleTime = 13;
                mTimer->mInvalidAttackTime = 20;
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg) ||
            al::isMsgHoldCancel(pMsg) || al::isMsgWarpStart(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            if (al::isNerve(this, &NrvKouraHold)) {
                mHoldState->receiveMsg(pMsg, pOther, pSelf);
                startBlow(pSelf, pOther);
                return true;
            }

            startBlow(pSelf, pOther);
            mBindPuppeteer->stopBind();
            return true;
        }

        if (!mIsSingleMode) {
            return false;
        }

        if (al::isMsgPlayerHideItem(pMsg)) {
            al::hideModelIfShow(this);
            return false;
        }

        if (al::isMsgPlayerShowItem(pMsg)) {
            al::showModelIfHide(this);
            return false;
        }

        return false;
    }

    if (rc::isMsgDashPanel(pMsg)) {
        if (!mIsSingleMode || mDashPanelTime != 0) {
            return false;
        }

        mDashPanelTime = 50;
        mDashPanelSpeed = 40.0f;
        return true;
    }

    if (mIsSingleMode && !al::isNerve(this, &NrvKouraBlow)) {
        bool isInkTouch = rc::isMsgInkTouch(pMsg);
        if (al::isMsgKouraDestroy(pMsg) || al::isMsgDisasterSpikeAttack(pMsg) || isInkTouch ||
            al::isMsgLaserAttack(pMsg)) {
            if (isInkTouch && isInRouteDokan()) {
                return false;
            }

            if (isPlayerInside()) {
                startBlow(pSelf, pOther);
                return true;
            }

            al::startHitReactionBreak(this);
            startKill();
            return true;
        }

        if (rc::isMsgJumpPanelAction(pMsg)) {
            al::setVelocityY(this, rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 65.0f : 45.0f);
            if ((al::isNerve(this, &NrvKouraKouraSlide) || al::isNerve(this, &NrvKouraRelease)) &&
                !isPlayerInside()) {
                al::setNerve(this, &NrvKouraKouraStop);
            }

            return true;
        }

        if (isPlayerInside() && al::isMsgEnemyAttack(pMsg) &&
            al::isSensorHostName(pOther, "イモゾー")) {
            rc::tryDamagePuppet(mBindPuppeteer->getPlayerPuppet());
        }
    }

    if (al::isNerve(this, &NrvKouraBlow) || al::isNerve(this, &NrvKouraBlowGoal)) {
        return false;
    }

    if (al::isMsgGoalKill(pMsg)) {
        rc::addScoreComboByFactor(this, pOther, "敵", pMsg, 100.0f);
        al::startHitReactionDeath(this);
        startKill();
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the shell can be kicked.
 * @return Whether the shell waits and is not protected.
 */
bool Koura::isEnableKick() const {
    return al::isNerve(this, &NrvKouraWait) && mTimer->mInvalidTrampleTime < 1;
}

/**
 * @brief Checks whether the shell slides.
 * @return Whether the shell is released or sliding with a rider.
 */
bool Koura::isSlide() const {
    return al::isNerve(this, &NrvKouraRelease) || al::isNerve(this, &NrvKouraKouraSlide);
}

/**
 * @brief Ends any bind and kills the shell.
 */
void Koura::startKill() {
    if (mBindPuppeteers[0]->isBind()) {
        mBindPuppeteers[0]->endKouraBind();
    }

    if (mBindPuppeteers[1]->isBind()) {
        mBindPuppeteers[1]->endKouraBind();
    }

    if (al::isNerve(this, &NrvKouraHold)) {
        tryDisappearGuide();
        rc::requestPlayerRelease(mPlayerSensor);
    }

    kill();
}

/**
 * @brief Releases the shell in a direction.
 * @param rDir Move direction.
 * @param speed Move speed (negative for the default).
 * @param rotateSpeed Spin speed in degrees per frame (negative for the default).
 */
void Koura::startMove(const sead::Vector3f& rDir, f32 speed, f32 rotateSpeed) {
    f32 moveSpeed = speed < 0.0f ? 30.0f : speed;
    f32 spinSpeed = rotateSpeed < 0.0f ? 20.0f : rotateSpeed;
    sead::Vector3f dirH = rDir;
    dirH.y = 0.0f;
    al::normalizeOrDirZ(&dirH);
    mMoveSpeed = moveSpeed;
    al::setVelocity(this, moveSpeed * rDir);
    setMoveDir(rDir);
    mRotateSpeed = spinSpeed;
    al::setNerve(this, &NrvKouraRelease);
}

/**
 * @brief Checks whether a trample stops the shell.
 * @return Whether the shell moves and is not protected.
 */
bool Koura::isEnableTrampleStop() const {
    if (al::isNerve(this, &NrvKouraRelease) || al::isNerve(this, &NrvKouraKouraWait) ||
        al::isNerve(this, &NrvKouraKouraSlide)) {
        return mTimer->mInvalidTrampleTime < 1;
    }

    return false;
}

/**
 * @brief Checks whether a player can pick the shell up.
 * @return Whether the shell can be held.
 */
bool Koura::isEnableHold() const {
    if (al::isNerve(this, &NrvKouraHold)) {
        return false;
    }

    if (mTimer->mInvalidHoldTime > 0) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraWait)) {
        return true;
    }

    if (al::isNerve(this, &NrvKouraUpper)) {
        return al::getVelocity(this).y < 0.0f;
    }

    return false;
}

/**
 * @brief Checks whether a player holds the shell.
 * @return Whether the shell is held.
 */
bool Koura::isHold() const {
    if ((al::isNerve(this, &NrvKouraHold) || al::isNerve(this, &NrvKouraBindStart)) &&
        mPlayerSensor != nullptr) {
        return rc::isPlayerHolding(mPlayerSensor, this);
    }

    return false;
}

/**
 * @brief Moves the shell back to the holder's position when it is dropped.
 */
void Koura::endHold() {
    sead::Vector3f trans = al::getTrans(this);
    sead::Vector3f pos = al::getActorTrans(mPlayerSensor);
    pos.y = trans.y;
    al::resetPosition(this, pos, false);
    al::setVelocity(this, trans - pos);
    updateCollider();
    al::setVelocityZero(this);
    if (mIsShowGuide) {
        rc::disappearGuideGameWindow(this);
    }

    mIsShowGuide = false;
}

/**
 * @brief Stops the shell when it is touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Own screen point target.
 * @return Whether the message was handled.
 */
bool Koura::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    if (!al::isMsgTouchAssist(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraRelease)) {
        al::setVelocityZero(this);
        al::startSe(this, "PgTouchStopped", nullptr);
        al::setNerve(this, &NrvKouraWait);
        return true;
    }

    if (al::isNerve(this, &NrvKouraKouraWait) || al::isNerve(this, &NrvKouraKouraSlide)) {
        al::setVelocityZero(this);
        if (al::isNerve(this, &NrvKouraKouraSlide)) {
            al::startSe(this, "PgTouchStopped", nullptr);
        }

        al::setNerve(this, &NrvKouraKouraWait);
        return true;
    }

    return false;
}

/**
 * @brief Appears the shell in its initial waiting state.
 */
void Koura::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvKouraWait);
    al::setVelocityZero(this);
    al::onCollide(this);
    mClippedFrame = 0;
    mRotateSpeed = 0.0f;
    mTimer->reset();
    mSlideTime = 0;
    mMoveSpeed = -1.0f;
    mIsOnGround = false;
    mIsInWater = false;
    mIsJumpBoost = false;
    mPlayerMoveState = KouraPlayerMoveState::None;
    mDashPanelSpeed = -1.0f;
}

/**
 * @brief Updates the timers, ground state, puppeteers and clipping every frame.
 */
void Koura::control() {
    mTimer->update();
    mIsOnGround = al::isCollidedGround(this);
    if (mIsOnGround) {
        mGroundNormal = al::getOnGroundNormal(this, 0);
    }

    tryBreak();
    al::updateActorCollisionController(mCollisionController);
    mBindPuppeteers[0]->update();
    mBindPuppeteers[1]->update();
    if (isPlayerInside() || (isSlide() && !mIsSingleMode) || isHold() ||
        mBindPuppeteers[0]->isBind() || mBindPuppeteers[1]->isBind()) {
        al::invalidateClipping(this);
    } else if (!mIsHiddenByBind) {
        al::validateClipping(this);
    }

    if (mDashPanelTime > 0) {
        mDashPanelTime--;
    }

    if (mIsSingleMode) {
        if (!isHold()) {
            rc::emitKeepEcho(this, al::getTrans(this), 170.0f, 120);
        }

        if (mJumpStartTime > 0) {
            mJumpStartTime--;
        }
    }
}

/**
 * @brief Breaks the shell when it touches a deadly area, floor or ink.
 * @return Whether the shell broke.
 */
bool Koura::tryBreak() {
    bool isInk = rc::isCollidedInkSlow(this) || InkUtil::isInInkLimitSphere(this);
    if (rc::isInDeathArea(this) || rc::isCollidedDamageFire(this) || rc::isCollidedPoison(this) ||
        (mIsSingleMode && isInk)) {
        if (mIsSingleMode) {
            if (isInk && isInRouteDokan()) {
                return false;
            }

            if (isPlayerInside()) {
                const sead::Vector3f& velocity = al::getVelocity(this);
                sead::Vector3f dir(-velocity.x, 0.0f, -velocity.z);
                al::normalizeOrZero(&dir);
                mBlowDir.set(dir);
                dir.x *= 5.3f;
                dir.z *= 5.3f;
                dir.y = 27.5f;
                al::setVelocity(this, dir);
                al::setNerve(this, &NrvKouraBlow);
                return true;
            }

            if (al::isNerve(this, &NrvKouraBlow)) {
                return false;
            }
        }

        al::setVelocityZero(this);
        al::startHitReactionBreak(this);
        startKill();
        return true;
    }

    return false;
}

/**
 * @brief Updates the collider, following the holder while the shell is carried.
 */
void Koura::updateCollider() {
    if (mHoldState->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mHoldState->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Kills the shell when its stage switch turns on.
 */
void Koura::startKillBySwitch() {
    al::startHitReactionDeath(this);
    startKill();
}

/**
 * @brief Appears the shell where a Koopa Troopa left it.
 * @param pNokonoko The Koopa Troopa leaving the shell.
 * @param isUpper Whether the shell hops up upside down.
 */
void Koura::appearFromNokonoko(al::LiveActor* pNokonoko, bool isUpper) {
    mComboCounter->reset();
    mTimer->mInvalidTrampleTime = 8;
    al::getTransPtr(this)->set(al::getTrans(pNokonoko));
    al::copyPose(this, pNokonoko);
    appear();
    al::onCollide(this);
    if (isUpper) {
        al::setNerve(this, &NrvKouraUpperFromNokonoko);
    } else {
        al::setNerve(this, &NrvKouraWait);
    }
}

/**
 * @brief Removes the shell when a Koopa Troopa climbs back into it.
 * @param pNokonoko The Koopa Troopa taking the shell.
 */
void Koura::disappearByNokonoko(al::LiveActor* pNokonoko) {
    mComboCounter->reset();
    if (al::isNerve(this, &NrvKouraHold)) {
        al::onCollide(this);
        rc::requestPlayerRelease(mPlayerSensor);
        tryDisappearGuide();
    }

    startKill();
}

/**
 * @brief Checks whether a Koopa Troopa can climb back into the shell.
 * @return Whether the shell is alive and neither moving nor held.
 */
bool Koura::isAttachableWithNokonoko() {
    if (al::isDead(this)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraRelease)) {
        return false;
    }

    if (al::isNerve(this, &NrvKouraKouraSlide)) {
        return false;
    }

    return !al::isNerve(this, &NrvKouraHold);
}

/**
 * @brief Called by a puppeteer when its bind gets cancelled.
 * @param pPuppeteer Puppeteer whose bind was cancelled.
 * @param pSensor Sensor of the cancelled player.
 * @param isKill Whether the shell must disappear.
 */
void Koura::receivedBindCancel(KouraBindPuppeteer* pPuppeteer, al::HitSensor* pSensor,
                               bool isKill) {
    if (isKill) {
        if (al::isNerve(this, &NrvKouraBindStart)) {
            tryEnableIslandWarp();
        }

        al::setVelocity(this, sead::Vector3f::zero);
        al::setNerve(this, &NrvKouraDemoKill);
        return;
    }

    if (mIsSingleMode && mBindPuppeteer == pPuppeteer && mIsBindGoal) {
        al::setVelocity(this, sead::Vector3f::zero);
        if (mIsSingleMode) {
            IslandMap::setIslandWarpEnable(this, false);
        }

        al::setNerve(this, &NrvKouraWaitForBind);
        return;
    }

    if (mBindPuppeteer != pPuppeteer) {
        return;
    }

    if (al::isNerve(this, &NrvKouraBindStart)) {
        tryEnableIslandWarp();
    }

    al::setVelocityZero(this);
    const sead::Vector3f& velocity = al::getVelocity(this);
    sead::Vector3f dir(-velocity.x, 0.0f, -velocity.z);
    al::normalizeOrDirZ(&dir);
    dir.x *= 5.3f;
    dir.z *= 5.3f;
    dir.y = 27.5f;
    al::setVelocity(this, dir);
    if (mIsBindGoal) {
        mIsBindGoal = false;
        al::setNerve(this, &NrvKouraBlowGoal);
    } else {
        al::setNerve(this, &NrvKouraBlow);
    }
}

/**
 * @brief Spins the shell around its up axis by the rotate speed.
 */
void Koura::updateRotatePose() {
    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(mRotateSpeed));
    al::updatePoseQuat(this, quat);
}

/**
 * @brief Hides the shell unless a player rides inside.
 * @return Whether the shell was hidden.
 */
bool Koura::hideActor() {
    if (mBindPuppeteers[0]->isBind() || mBindPuppeteers[1]->isBind()) {
        al::invalidateClipping(this);
        mIsHiddenByBind = true;
        return false;
    }

    return al::LiveActor::hideActor();
}

/**
 * @brief Places the hit effect where the shell collided with a wall or the ceiling.
 */
void Koura::startEffectHitCollision() {
    sead::Vector3f normal;
    sead::Vector3f pos;
    if (al::isCollidedWall(this)) {
        normal = al::getCollidedWallNormal(this);
        pos = al::getCollidedWallPos(this);
    } else if (al::isCollidedCeiling(this)) {
        normal = al::getCollidedCeilingNormal(this);
        pos = al::getCollidedCeilingPos(this);
    } else {
        return;
    }

    mEffectMtx.makeQT(sead::Quatf(1.0f, 0.0f, 0.0f, 0.0f), pos);

    sead::Matrix34f rotateMtx;
    rotateMtx.makeIdentity();
    sead::Quatf rotate;
    if (rotate.makeVectorRotation(sead::Vector3f(0.0f, 1.0f, 0.0f), normal)) {
        rotateMtx.makeQT(rotate, sead::Vector3f(0.0f, 0.0f, 0.0f));
    }

    mEffectMtx = mEffectMtx * rotateMtx;
    al::startHitReaction(this, "衝突");
}

/**
 * @brief Checks whether the waiting shell floats still on the water surface.
 * @param depth Depth of the shell below the water surface.
 * @return Whether the shell should be fixed to the water surface.
 */
bool Koura::isFixWaterSurface(f32 depth) const {
    if (!al::isNerve(this, &NrvKouraWait)) {
        return false;
    }

    if (!mIsInWater) {
        return false;
    }

    const sead::Vector3f& velocity = al::getVelocity(this);
    if (sead::Mathf::abs(depth) > 2.0f) {
        return false;
    }

    if (sead::Mathf::abs(velocity.y) > 0.5f) {
        return false;
    }

    return true;
}

/**
 * @brief Applies gravity, water buoyancy and ground bounce while the shell is not sliding.
 * @param bounceRate Ground bounce rate (0 for no bounce).
 * @param scaleH Horizontal velocity scale.
 * @return Whether the shell landed.
 */
bool Koura::doFall(f32 bounceRate, f32 scaleH) {
    f32 depth;
    if (mIsSingleMode) {
        f32 height = WaterUtil::getOceanWaterHeight(this, al::getTrans(this), true);
        depth = height - al::getTrans(this).y;
        if (isFixWaterSurface(depth)) {
            al::setTransY(this, height - 1.0f);
            depth = 1.0f;
        }
    } else {
        depth = rc::calcWaterSinkDepth(this);
    }

    f32 scaleV;
    f32 buoyancy;
    if (depth > 0.0f) {
        if (!mIsInWater) {
            mIsInWater = true;
            al::startHitReaction(this, "着水");
        }

        f32 rate = calcWaterRate(depth);
        buoyancy = (1.0f - rate) * 1.8f + rate * 2.6f;
        scaleV = 0.7f;
    } else {
        scaleV = 0.998f;
        buoyancy = 0.0f;
        mIsInWater = false;
    }

    if (al::isCollidedCeilingVelocity(this) && al::getVelocity(this).y > 0.0f) {
        al::scaleVelocityHV(this, scaleH, 0.0f);
    } else if (!al::isCollidedGround(this)) {
        al::scaleVelocityHV(this, scaleH, scaleV);
    } else if (al::getVelocity(this).y > 0.0f) {
        return false;
    } else {
        sead::Vector3f velocity;
        al::verticalizeVec(&velocity, al::getOnGroundNormal(this, 0), al::getVelocity(this));
        al::setVelocity(this, velocity * scaleH);
        if (al::isOnGround(this, 0, 0.0f) &&
            al::getOnGroundNormal(this, 0).dot(al::getVelocity(this)) < -15.0f) {
            al::startHitReactionOnGround(this);
        }

        if (bounceRate <= 0.0f) {
            sead::Vector3f gravity = al::getOnGroundNormal(this, 0) * -1.8f;
            gravity.y += buoyancy;
            al::addVelocity(this, gravity);
        } else {
            al::setVelocityY(this, -(al::getVelocityPtr(this)->y * bounceRate));
        }

        return true;
    }

    al::addVelocityToGravity(this, 1.8f - buoyancy);
    return false;
}

/**
 * @brief Moves the sliding shell: reflects on walls and ceilings and keeps its speed.
 */
void Koura::doMove() {
    bool isRelease = al::isNerve(this, &NrvKouraRelease);
    f32 scaleParallel = isRelease ? 1.0f : 0.997f;
    al::getVelocity(this);
    f32 depth = rc::calcWaterSinkDepth(this);
    f32 buoyancy;
    f32 scaleV;
    if (depth > 0.0f) {
        if (!mIsInWater) {
            mIsInWater = true;
            al::startHitReaction(this, "着水");
        }

        f32 rate = calcWaterRate(depth);
        buoyancy = (1.0f - rate) * 1.8f + rate * 2.6f;
        scaleV = 0.7f;
    } else {
        scaleV = isRelease ? 1.0f : 0.995f;
        buoyancy = 0.0f;
        mIsInWater = false;
    }

    bool isCollided = false;
    if (al::isCollidedWallVelocity(this)) {
        al::HitSensor* wallSensor = al::tryGetCollidedWallSensor(this);
        bool isReflect = true;
        if (wallSensor != nullptr) {
            if (al::sendMsgKickKouraBreak(wallSensor, al::getHitSensor(this, "Body"))) {
                startBreak();
                return;
            }

            isReflect =
                !al::sendMsgKickKouraCollideNoReflect(wallSensor, al::getHitSensor(this, "Body"));

            al::sendMsgKickKouraAttackCollide(wallSensor, al::getHitSensor(this, "Body"),
                                              mComboCounter);
        }

        startEffectHitCollision();
        onHitWall();
        sead::Vector3f normal = al::getCollidedWallNormal(this);
        normal.y = 0.0f;
        bool isNormalZero = al::isNearZero(normal, 0.001f);
        if (isReflect && !isNormalZero) {
            al::normalize(&normal);
            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
        }

        isCollided = true;
    } else if (al::isCollidedCeilingVelocity(this)) {
        startEffectHitCollision();
        sead::Vector3f normal = al::getCollidedCeilingNormal(this);
        if (!al::isNearZero(normal, 0.001f)) {
            al::normalize(&normal);
            sead::Vector3f velocity = al::getVelocity(this);
            al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
            al::setVelocity(this, velocity);
        }

        isCollided = true;
    }

    if (al::isCollidedGround(this)) {
        const sead::Vector3f& groundNormal = al::getOnGroundNormal(this, 0);
        sead::Vector3f velocity = al::getVelocity(this);
        if (al::calcReflectionVector(&velocity, groundNormal, 0.0f, 0.0f) &&
            al::isOnGround(this, 0, 0.0f) &&
            al::getOnGroundNormal(this, 0).dot(al::getVelocity(this)) < -15.0f) {
            al::startHitReactionOnGround(this);
        }

        al::setVelocity(this, velocity);
    }

    al::scaleVelocityHV(this, 0.997f, 0.995f);
    if (isCollided) {
        setMoveDir(al::getVelocity(this));
    }

    sead::Vector3f velocity = al::getVelocity(this);
    if (al::isNearZero(velocity, 0.001f)) {
        al::setVelocityZero(this);
    } else {
        sead::Vector3f velocityH = velocity;
        velocityH.y = 0.0f;
        sead::Vector3f parallel;
        al::parallelizeVec(&parallel, mMoveDir, velocityH);
        sead::Vector3f vertical;
        al::verticalizeVec(&vertical, mMoveDir, velocityH);
        f32 velocityY = scaleV * velocity.y;
        velocity = scaleParallel * parallel + vertical * 0.9f;
        f32 speed = mIsSingleMode && mDashPanelTime >= 1 ? mDashPanelSpeed : mMoveSpeed;
        velocity *= speed / velocity.length();
        velocity.y = velocityY;
        al::setVelocity(this, velocity);
    }

    if (al::isCollidedGround(this)) {
        sead::Vector3f gravity = al::getOnGroundNormal(this, 0) * -1.8f;
        gravity.y += buoyancy;
        al::addVelocity(this, gravity);
    } else {
        al::addVelocityToGravity(this, 1.8f - buoyancy);
    }
}

/**
 * @brief Checks whether the shell slides without a rider.
 * @return Whether the shell is released.
 */
bool Koura::isSlideWithoutPlayer() const {
    return al::isNerve(this, &NrvKouraRelease);
}

/**
 * @brief Checks whether the shell moves through a route pipe.
 * @return Whether the shell is in a route pipe.
 */
bool Koura::isInRouteDokan() const {
    return al::isNerve(this, &NrvKouraRouteDokan);
}

/**
 * @brief Checks whether a jump with a rider inside just started.
 * @return Whether the jump start timer runs.
 */
bool Koura::isJumpStart() const {
    return mJumpStartTime > 0;
}

/**
 * @brief Waits on the ground, slowing down its spin.
 */
void Koura::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mPlayerSensor = nullptr;
        if (mIsHiddenByBind) {
            al::setVelocityZero(this);
            al::startHitReactionBreak(this);
            startKill();
            return;
        }
    }

    doFall(0.0f, 0.9f);
    if (mRotateSpeed > 0.0f) {
        updateRotatePose();
        if (al::isOnGround(this, 0, 0.0f) || mIsInWater) {
            mRotateSpeed *= 0.8f;
        }

        if (mRotateSpeed <= 0.0001f) {
            mRotateSpeed = 0.0f;
        }
    }
}

/**
 * @brief Waits for the player to get out of a goal binder, then rides the shell again.
 */
void Koura::exeWaitForBind() {
    al::getSensorHost(mPlayerSensor);
    if (rc::isPlayerBinded(mPlayerSensor)) {
        if (al::isGreaterEqualStep(this, 500)) {
            tryEnableIslandWarp();
            al::setNerve(this, &NrvKouraWait);
        }

        return;
    }

    if (rc::isPlayerOnGround(mPlayerSensor)) {
        mIsJumpBoost = true;
    } else {
        mIsJumpBoost = rc::isPlayerInWater(mPlayerSensor);
    }

    endHold();
    al::resetActorCollisionController(mCollisionController, 1);
    al::setNerve(this, &NrvKouraBindStart);
}

/**
 * @brief Hops up after being kicked from below or lifted.
 */
void Koura::exeUpper() {
    bool isUpper = al::isNerve(this, &NrvKouraUpper);
    if (al::isFirstStep(this)) {
        if (isUpper) {
            al::startSe(this, "Hop", nullptr);
        }

        al::startAction(this, "StayWait");
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        mRotateSpeed = 20.0f;
        if (isUpper) {
            al::addVelocity(this, sead::Vector3f(0.0f, 30.0f, 0.0f));
        } else {
            al::setVelocity(this, sead::Vector3f(0.0f, 25.0f, 0.0f));
        }

        mDashPanelSpeed = -1.0f;
        return;
    }

    updateRotatePose();
    f32 velocityY = al::getVelocityPtr(this)->y;
    if (doFall(0.3f, isUpper ? 0.998f : 0.9f) || (velocityY < 0.0f && mIsInWater)) {
        mLiftingCount = 0;
        al::setNerve(this, &NrvKouraWait);
    }
}

/**
 * @brief Is carried by a player; starts the ride when the player squats.
 */
void Koura::exeHold() {
    if (al::isFirstStep(this)) {
        al::setColliderRadius(mCollisionController, 30.0f);
        al::setColliderOffsetY(mCollisionController, 80.0f);
        mComboCounter->reset();
        al::startAction(this, "HoldWait");
        mIsInWater = false;
        al::startSe(this, "Hold", nullptr);
        al::setVelocityZero(this);
        if (mIsSingleMode) {
            tryAppearGuide();
        }
    }

    if (mIsSingleMode) {
        if (!rc::isCurrentGuideGameWindowUser(this)) {
            mIsShowGuide = false;
        }

        if (!rc::isGuideGameWindowActive(this) && !mIsShowGuide &&
            GameDataFunction::isSingleMode(this)) {
            tryAppearGuide();
        }
    }

    al::updateNerveState(this);
    auto* player = static_cast<PlayerActor*>(al::getSensorHost(mPlayerSensor));
    if (!rc::isPlayerBinded(mPlayerSensor) && player->getInput()->isSquatTrigOn()) {
        if (rc::isPlayerOnGround(mPlayerSensor)) {
            mIsJumpBoost = true;
        } else {
            mIsJumpBoost = rc::isPlayerInWater(mPlayerSensor);
        }

        endHold();
        al::resetActorCollisionController(mCollisionController, 1);
        if (mIsSingleMode) {
            rc::disableGuideGameWindowPriority(this);
            IslandMap::setIslandWarpEnable(this, false);
        }

        al::setNerve(this, &NrvKouraBindStart);
        return;
    }

    mPlayerMoveState = KouraPlayerMoveState::None;
    if (rc::isPlayerWait(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraPlayerMoveState::Wait;
    } else if (rc::isPlayerDash(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraPlayerMoveState::Dash;
    } else if (rc::isPlayerDashFast(al::getSensorHost(mPlayerSensor))) {
        mPlayerMoveState = KouraPlayerMoveState::DashFast;
    }
}

/**
 * @brief Called when the shell gets held (nothing to do).
 */
void Koura::startHold() {}

/**
 * @brief Follows the holder while the release is pending, then is released forward.
 */
void Koura::exeWaitRelease() {
    if (al::isFirstStep(this)) {
        al::resetActorSensorController(mSensorController);
    }

    // Follow the hands of the holder (scoped so the temporaries end here).
    {
        sead::Vector3f holdPos;
        rc::calcPlayerHoldPos(&holdPos, mPlayerSensor);
        sead::Quatf quat;
        sead::Vector3f front = rc::getPlayerFront(mPlayerSensor);
        front.y = 0.0f;
        al::normalizeOrDirZ(&front);
        al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
        sead::Vector3f pos = sParam.holdOffset;
        pos.rotate(quat);
        pos += holdPos;
        al::updatePoseQuat(this, quat);
        al::setTrans(this, pos);
    }

    if (!rc::isPlayerBinded(mPlayerSensor)) {
        endHold();
        startMove(rc::getPlayerFront(mPlayerSensor), 30.0f, 20.0f);
    }
}

/**
 * @brief Slides without a rider until it stops or leaves the screen for too long.
 */
void Koura::exeRelease() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
    }

    if (mIsSingleMode && rc::isActiveDemo(this) && !mIsInWater) {
        if (!mIsDemoStopped) {
            mIsDemoStopped = true;
            mDemoStopVelocity = al::getVelocity(this);
        }

        al::setVelocity(this, sead::Vector3f::zero);
        return;
    }

    if (mIsDemoStopped) {
        mIsDemoStopped = false;
        al::setVelocity(this, mDemoStopVelocity);
        mTimer->mInvalidAttackTime = 20;
    }

    doMove();
    const char* actionName =
        mIsInWater ? "WaterMove" : (al::isCollidedGround(this) ? "GroundMove" : "AirMove");
    if (!al::isActionPlaying(this, actionName)) {
        al::startAction(this, actionName);
    }

    updateRotatePose();
    if (rc::tryFindAreaObj(this, rc::AreaObjType::FrameOutCtrlArea, al::getTrans(this)) ==
            nullptr &&
        al::isJudgedToClipFrustum(this, 100.0f, 300.0f)) {
        if (mClippedFrame++ >= 200) {
            makeActorDead();
            return;
        }
    } else {
        mClippedFrame = 0;
    }

    if (al::isOnGround(this, 0, 0.0f) && al::isVelocitySlow(this, 5.0f)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvKouraWait);
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        if (GameDataFunction::isSingleMode(this)) {
            al::sendMsgKouraThrow(groundSensor, al::getHitSensor(this, "Body"));
        }

        if (mPlayerSensor != nullptr) {
            al::sendMsgPlayerFloorTouch(groundSensor, mPlayerSensor);
        } else {
            al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));
        }
    }
}

/**
 * @brief Lets the player enter the shell, then starts sliding.
 */
void Koura::exeBindStart() {
    if (al::isFirstStep(this)) {
        if (!mBindPuppeteer->startEnter(mPlayerSensor, mIsSingleMode)) {
            tryEnableIslandWarp();
            al::setNerve(this, &NrvKouraBlow);
            return;
        }

        rc::tryRequestClearFlingPoleDashFlag(al::getSensorHost(mPlayerSensor));
        if (mIsSingleMode) {
            mJumpStartTime = 0;
        }
    }

    if (mIsSingleMode && al::isFirstStep(this)) {
        return;
    }

    if (mBindPuppeteer->isBind()) {
        tryEnableIslandWarp();
        startKouraSlide(rc::getPlayerFront(mPlayerSensor));
    } else if (mIsSingleMode) {
        IslandMap::setIslandWarpEnable(this, true);
        al::setNerve(this, &NrvKouraBlow);
    }
}

/**
 * @brief Slides with a rider inside, steered by the rider's stick.
 */
void Koura::exeKouraSlide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mSlideTime = mSlideTimeMax;
        mRotateSpeed = 30.0f;
        al::startHitReaction(this, "コウラ滑り開始");
    }

    f32 rate = 1.0f - static_cast<f32>(mSlideTime) / static_cast<f32>(mSlideTimeMax);
    if (rate < 0.0f) {
        rate = 0.0f;
    } else if (rate > 1.0f) {
        rate = 1.0f;
    }

    mRotateSpeed = ((1.0f - rate * (rate * rate)) * 0.5f + 0.5f) * 30.0f;
    doMove();
    const char* actionName =
        mIsInWater ? "WaterMove" : (al::isCollidedGround(this) ? "GroundMove" : "AirMove");
    if (!al::isActionPlaying(this, actionName)) {
        al::startAction(this, actionName);
    }

    updateRotatePose();
    sead::Vector3f dir = al::getVelocity(this);
    dir.y = 0.0f;
    if (!al::isNearZero(dir, 0.001f)) {
        al::normalize(&dir);
        sead::Vector3f stickH =
            rc::getPuppetStickWorldWithoutSnap(mBindPuppeteer->getPlayerPuppet());
        stickH.y = 0.0f;
        if (!al::isNearZero(stickH, 0.001f)) {
            al::normalize(&stickH);
            f32 cross = dir.z * stickH.x - stickH.z * dir.x;
            sead::Vector3f side(dir.z, 0.0f, -dir.x);
            if (cross < -0.5f) {
                al::addVelocity(this, side * -0.4f);
            } else if (cross > 0.5f) {
                al::addVelocity(this, side * 0.4f);
            }

            sead::Vector3f moveDir = al::getVelocity(this);
            al::normalizeOrDirZ(&moveDir);
            setMoveDir(moveDir);
        }
    }

    al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
    if (groundSensor != nullptr) {
        al::sendMsgPlayerFloorTouch(groundSensor, mPlayerSensor);
    }

    const sead::Vector3f& newVelocity = al::getVelocity(this);
    if (al::isCollidedGround(this) &&
        newVelocity.x * newVelocity.x + newVelocity.z * newVelocity.z < 100.0f) {
        if (!mIsSingleMode || mSlideTime <= 694) {
            al::setNerve(this, &NrvKouraBindEndStop);
            if (mIsSingleMode) {
                rc::enableGuideGameWindowPriority(this);
            }
        }

        return;
    }

    if (mSlideTime-- <= 0) {
        al::setNerve(this, &NrvKouraKouraStop);
        if (mIsSingleMode) {
            rc::enableGuideGameWindowPriority(this);
        }

        return;
    }

    doKouraInputProc();
}

/**
 * @brief Handles the rider's input: jump or exit.
 */
void Koura::doKouraInputProc() {
    if (!mBindPuppeteer->isBind()) {
        return;
    }

    if (rc::isPuppetTrigJumpButtonWithoutPrecedeInput(mBindPuppeteer->getPlayerPuppet())) {
        bool isEnableJump = al::isCollidedGround(this);
        if (!isEnableJump) {
            f32 depth = rc::calcWaterSinkDepth(this);
            isEnableJump = depth < 100.0f && depth >= 0.0f;
        }

        if (isEnableJump && !al::isNerve(this, &NrvKouraKouraJump) &&
            !al::isNerve(this, &NrvKouraKouraJumpBreak)) {
            if (al::isNerve(this, &NrvKouraKouraWait)) {
                al::setNerve(this, &NrvKouraKouraJump);
            } else {
                sead::Vector3f velocity = al::getVelocity(this);
                velocity.y = 25.0f;
                al::setVelocity(this, velocity);
                al::startSe(this, "Jump", nullptr);
            }

            if (mIsInWater) {
                al::startHitReaction(this, "水中ジャンプ");
            }

            if (mIsSingleMode) {
                mJumpStartTime = 2;
            }
        }
    }

    if (rc::isUsingOldPlayerParams()) {
        if (rc::isPuppetHoldDashButton(mBindPuppeteer->getPlayerPuppet())) {
            return;
        }
    } else if (!rc::isPuppetTrigSquatButton(mBindPuppeteer->getPlayerPuppet())) {
        return;
    }

    if (mTimer->mInvalidRouteDokanTime > 0) {
        return;
    }

    if (al::isNerve(this, &NrvKouraKouraJump)) {
        al::setNerve(this, &NrvKouraBindEndExitStay);
        if (mIsSingleMode) {
            rc::enableGuideGameWindowPriority(this);
        }

        return;
    }

    if (mIsSingleMode) {
        rc::enableGuideGameWindowPriority(this);
    }

    al::setNerve(this, &NrvKouraBindEndExit);
}

/**
 * @brief Stops after a jump panel, then ends the bind.
 */
void Koura::exeKouraStop() {
    doMove();
    if (al::isGreaterEqualStep(this, 0)) {
        al::setNerve(this, &NrvKouraBindEndStop);
        return;
    }

    doKouraInputProc();
}

/**
 * @brief Waits with a rider inside.
 */
void Koura::exeKouraWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        al::setVelocityZero(this);
        mDashPanelSpeed = -1.0f;
    }

    doFall(0.0f, 0.0f);
    doKouraInputProc();
}

/**
 * @brief Jumps with a rider inside.
 */
void Koura::exeKouraJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StayWait");
        al::startSe(this, "Jump", nullptr);
        mMoveSpeed = -1.0f;
        mTimer->mInvalidTrampleTime = 8;
        mRotateSpeed = 20.0f;
        if (al::isNerve(this, &NrvKouraKouraJump)) {
            sead::Vector3f velocity = al::getVelocity(this);
            velocity.y = 25.0f;
            al::setVelocity(this, velocity);
        }

        mDashPanelSpeed = -1.0f;
        return;
    }

    updateRotatePose();
    f32 velocityY = al::getVelocityPtr(this)->y;
    bool isLanded = doFall(0.3f, 0.0f);
    doKouraInputProc();
    if (isLanded || (velocityY < 0.0f && mIsInWater)) {
        al::setNerve(this, &NrvKouraKouraJumpBreak);
    }
}

/**
 * @brief Lands after a jump with a rider inside, slowing down the spin.
 */
void Koura::exeKouraJumpBreak() {
    doFall(0.3f, 0.0f);
    doKouraInputProc();
    if (al::getNerveStep(this) <= 10) {
        f32 rate = al::getNerveStep(this) / 10.0f;
        f32 rotateSpeed = mRotateSpeed * (1.0f - rate) + rate * 5.0f;
        sead::Quatf quat = al::getQuat(this);
        al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(rotateSpeed));
        al::updatePoseQuat(this, quat);
    }

    if (al::isGreaterEqualStep(this, 15)) {
        al::setNerve(this, &NrvKouraKouraWait);
    }
}

/**
 * @brief Ends the rider's bind and goes back to waiting.
 */
void Koura::exeBindEnd() {
    if (!al::isFirstStep(this)) {
        return;
    }

    mComboCounter->reset();
    if (al::isNerve(this, &NrvKouraBindEndExit)) {
        mBindPuppeteer->startExit();
    } else if (al::isNerve(this, &NrvKouraBindEndExitStay)) {
        mBindPuppeteer->startExitStay();
    } else {
        mBindPuppeteer->startDizzy();
        if (mBindPuppeteer == mBindPuppeteers[1]) {
            mBindPuppeteer = mBindPuppeteers[0];
        } else {
            mBindPuppeteer = mBindPuppeteers[1];
        }

        mRouteDokanState->setRouteSelecter(mBindPuppeteer->getRouteSelecter());
        if (mBindPuppeteer->isBind()) {
            mBindPuppeteer->endBindForce();
        }
    }

    if ((al::isNerve(this, &NrvKouraBindEndExit) && al::isCollidedGround(this)) || mIsInWater) {
        mTimer->mInvalidTrampleTime = 40;
        al::setVelocity(this, sead::Vector3f(0.0f, 15.0f, 0.0f));
        mRotateSpeed = 20.0f;
        if (mIsSingleMode && mIsInWater) {
            al::setNerve(this, &NrvKouraBlow);
            return;
        }
    } else {
        mTimer->mInvalidTrampleTime = 8;
        al::setVelocityZero(this);
        mRotateSpeed = 0.0f;
    }

    mTimer->mInvalidHoldTime = 10;
    al::setNerve(this, &NrvKouraWait);
}

/**
 * @brief Flies away after being blown, then disappears.
 */
void Koura::exeBlow() {
    if (al::isFirstStep(this)) {
        mIsInWater = false;
        al::startAction(this, "Blow");
        if (mPlayerSensor != nullptr) {
            rc::requestPlayerRelease(mPlayerSensor);
        }
    }

    al::calcSpeed(this);
    sead::Quatf quat = al::getQuat(this);
    al::rotateQuatRadian(&quat, quat, sead::Vector3f::ez, 0.0f);
    al::setQuat(this, quat);
    al::scaleVelocityHV(this, 0.9f, 0.998f);
    if (!al::isCollidedGround(this)) {
        al::addVelocityToGravity(this, 1.1f);
    }

    if (mIsSingleMode && al::isStep(this, 5) && !al::isNearZero(mBlowDir, 0.001f)) {
        if (mBindPuppeteers[0]->isBind()) {
            mBindPuppeteers[0]->endKouraBindWallHit(mBlowDir);
        }

        if (mBindPuppeteers[1]->isBind()) {
            mBindPuppeteers[1]->endKouraBindWallHit(mBlowDir);
        }
    }

    if (al::isGreaterEqualStep(this, 30) && al::isAlive(this)) {
        al::startHitReactionDisappear(this);
        if (al::isNerve(this, &NrvKouraBlowGoal)) {
            rc::addScoreByFactor(this, mPlayerSensor, "成功", 100.0f, 0);
        }

        startKill();
    }
}

/**
 * @brief Waits for the bind to end, then kills the shell.
 */
void Koura::exeWaitBindEndForKill() {
    if (mBindPuppeteer->isBind()) {
        return;
    }

    startKill();
}

/**
 * @brief Moves through a route pipe, then leaves it sliding, jumping or waiting.
 */
void Koura::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RouteDokanMove");
        mRouteDokanState->setMoveSpeed(20.0f);
        mRouteDokanState->setEndSpeed(20.0f);
        mRotateSpeed = 30.0f;
        mSlideTime = mSlideTimeMax;
        mUp.set(sead::Vector3f::ey);
        al::setSensorRadius(mSensorController, 40.0f);
        al::setSensorFollowPosOffset(mSensorController, sParam.routeDokanSensorOffset);
    }

    if (al::updateNerveState(this)) {
        al::resetActorSensorController(mSensorController);
        al::onCollide(this);
        al::setNerve(this, &NrvKouraRouteDokanEnd);
        sead::Quatf quat;
        sead::Vector3f front = mRouteDokanState->getMoveDirection();
        front.y = 0.0f;
        al::normalizeOrDirZ(&front);
        al::makeQuatFrontUp(&quat, front, sead::Vector3f::ey);
        al::rotateQuatRadian(&quat, quat, sead::Vector3f::ey, sead::Mathf::deg2rad(mRotateDegree));
        al::updatePoseQuat(this, quat);
        al::startSe(this, "RouteDokanOut", nullptr);
        mTimer->mInvalidRouteDokanTime = 8;
        if (al::isNearZero(al::getVelocity(this).y, 0.001f)) {
            if (mBindPuppeteer->isBind()) {
                startKouraSlide(mRouteDokanState->getMoveDirection());
            } else {
                startMove(mRouteDokanState->getMoveDirection(), 30.0f, 20.0f);
            }
        } else if (mBindPuppeteer->isBind()) {
            if (al::getVelocity(this).y < 0.0f) {
                al::setNerve(this, &NrvKouraKouraJumpFall);
            } else {
                al::setNerve(this, &NrvKouraKouraJump);
            }
        } else {
            al::setNerve(this, &NrvKouraWait);
        }

        return;
    }

    updateRotatePose();
}

/**
 * @brief Leaves the route pipe.
 */
void Koura::exeRouteDokanEnd() {
    al::isFirstStep(this);
    doMove();
    al::setNerve(this, &NrvKouraWait);
}

/**
 * @brief Disappears during a demo.
 */
void Koura::exeDemoKill() {
    al::tryKillEmitterAndParticleAll(this);
    startKill();
}

/**
 * @brief Gets the name of the shell's model archive.
 * @return The archive name.
 */
const char* Koura::getArchiveName() const {
    return "Koura";
}
