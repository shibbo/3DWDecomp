#include "Enemy/SkipperTrampoline.hpp"

#include <math/seadBoundBox.h>
#include <math/seadMatrix.h>

#include "Enemy/EnemyStateUtil.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BallStateFunction.hpp"
#include "MapObj/HoldColliderControl.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(SkipperTrampoline, Wait)
NERVE_DECL(SkipperTrampoline, Fall)
NERVE_DECL(SkipperTrampoline, End)
NERVE_DECL(SkipperTrampoline, Land)
NERVE_DECL(SkipperTrampoline, RecoverSign)
NERVE_DECL(SkipperTrampoline, RecoverStart)
NERVE_DECL(SkipperTrampoline, Reaction)
NERVE_DECL(SkipperTrampoline, ReactionFall)
NERVE_DECL(SkipperTrampoline, ReactionHit)
NERVE_DECL(SkipperTrampoline, ReactionCarry)
NERVE_DECL(SkipperTrampoline, WaitCarry)
NERVE_DECL(SkipperTrampoline, Sink)
NERVE_DECL(SkipperTrampoline, RecoverSignCarry)
NERVE_DECL(SkipperTrampoline, Recover)
NERVE_DECL(SkipperTrampoline, RecoverStartCarry)

SkipperTrampolineNrvWait NrvSkipperTrampolineWait;
SkipperTrampolineNrvFall NrvSkipperTrampolineFall;
SkipperTrampolineNrvEnd NrvSkipperTrampolineEnd;
SkipperTrampolineNrvLand NrvSkipperTrampolineLand;
SkipperTrampolineNrvRecoverSign NrvSkipperTrampolineRecoverSign;
SkipperTrampolineNrvRecoverStart NrvSkipperTrampolineRecoverStart;
SkipperTrampolineNrvReaction NrvSkipperTrampolineReaction;
SkipperTrampolineNrvReactionFall NrvSkipperTrampolineReactionFall;
SkipperTrampolineNrvReactionHit NrvSkipperTrampolineReactionHit;
SkipperTrampolineNrvReactionCarry NrvSkipperTrampolineReactionCarry;
SkipperTrampolineNrvWaitCarry NrvSkipperTrampolineWaitCarry;
SkipperTrampolineNrvSink NrvSkipperTrampolineSink;
SkipperTrampolineNrvRecoverSignCarry NrvSkipperTrampolineRecoverSignCarry;
SkipperTrampolineNrvRecover NrvSkipperTrampolineRecover;
SkipperTrampolineNrvRecoverStartCarry NrvSkipperTrampolineRecoverStartCarry;

/** @brief Ignores other trampolines, and the player's trampoline while this one is carried. */
class SkipperTrampolineCollisionPartsFilter : public al::CollisionPartsFilterBase {
public:
    SkipperTrampolineCollisionPartsFilter(const SkipperTrampoline* pTrampoline)
        : mTrampoline(pTrampoline) {}

    /**
     * @brief Checks whether collision parts are ignored by the trampoline's collider.
     * @param rParts Collision parts to check.
     * @return Whether the parts belong to another trampoline.
     */
    bool isInvalidParts(const al::CollisionParts& rParts) const override {
        const al::LiveActor* host = rParts.getConnectedHost();
        if (al::isEqualString(host->getName(), "SkipperTrampoline")) {
            return true;
        }

        if (al::isEqualString(host->getName(), "トランポリン★")) {
            return mTrampoline->isNerveCarry();
        }

        return false;
    }

private:
    const SkipperTrampoline* mTrampoline;
};

/// Constant parameters of the trampoline, constructed at startup.
struct SkipperTrampolineParam {
    sead::Vector3f punchHitBoxSize;    // box an upper punch has to hit to bounce the trampoline
    sead::Vector3f punchHitBoxOffset;
    PlayerBindEndParam bindEndParamJump;
    ItemStatePlayerHoldParam playerHoldParam;
};

SkipperTrampolineParam sParam = {
    sead::Vector3f(130.0f, 130.0f, 130.0f),
    sead::Vector3f(0.0f, 50.0f, 0.0f),
    // TODO: the game's PlayerBindEndParam has a vtable and the action name "Jump" in its first
    // 0x20 bytes; the shared header does not model them yet.
    {{}, 1, 10, true, true, true, 0, 1.2f, 0, false, {}},
    ItemStatePlayerHoldParam(
        sead::Vector3f(20.0f, 0.0f, 5.0f), sead::Vector3f(20.0f, 0.0f, 5.0f),
        sead::Vector3f(10.0f, 0.0f, 5.0f), sead::Vector3f(55.0f, 0.0f, 5.0f),
        sead::Vector3f(15.0f, 0.0f, 5.0f), sead::Vector3f(20.0f, 0.0f, 5.0f),
        sead::Vector3f(20.0f, 0.0f, 5.0f), sead::Vector3f(20.0f, 0.0f, 5.0f),
        sead::Vector3f(45.0f, 0.0f, 5.0f), sead::Vector3f(20.0f, 0.0f, 5.0f),
        sead::Vector3f(0.0f, 0.0f, 270.0f)),
};
}  // namespace

/**
 * @brief Constructs a trampoline.
 * @param pName Actor name.
 */
SkipperTrampoline::SkipperTrampoline(const char* pName)
    : al::LiveActor(pName), mComboCounter(new al::ComboCounter()) {}

/**
 * @brief Initializes the model, nerves, collider filter and hold collider control.
 * @param rInfo Placement info of the actor.
 */
void SkipperTrampoline::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SkipperTrampoline", nullptr);
    al::initNerve(this, &NrvSkipperTrampolineWait, 1);
    al::setColliderFilterCollisionParts(this, new SkipperTrampolineCollisionPartsFilter(this));
    initColliderControl();
    mPadRumbleKeeper = al::createPadRumbleKeeper(this, 0);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}

/** @brief Creates the control that adjusts the collider while the trampoline is held. */
void SkipperTrampoline::initColliderControl() {
    mHoldColliderControl = new HoldColliderControl();
    mHoldColliderControl->init(this);
}

/** @brief Appears and starts falling. */
void SkipperTrampoline::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    mStableStep = 0;
    if (mHoldColliderControl != nullptr) {
        mHoldColliderControl->start();
    }

    al::setNerve(this, &NrvSkipperTrampolineFall);
}

/** @brief Releases the holder, launches bound players and disappears. */
void SkipperTrampoline::kill() {
    if (mHolderSensor != nullptr) {
        rc::requestPlayerRelease(mHolderSensor);
    }

    tryJumpPlayer();
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}

/** @brief Launches every player bound to the trampoline and plays the bounce reaction. */
void SkipperTrampoline::tryJumpPlayer() {
    if (mBindSensors.size() <= 0) {
        return;
    }

    bool isHigh = false;
    for (s32 i = 0; i < mBindSensors.size(); i++) {
        IUsePlayerPuppet* puppet =
            rc::startPuppet(al::getHitSensor(this, "CollisionParts"), mBindSensors[i]);
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        sead::Vector3f velocity = sead::Vector3f::zero;
        if (rc::isPuppetAction(puppet, "HipDrop") ||
            static_cast<u32>(rc::getPuppetTrigJumpFrame(puppet)) < 5) {
            up *= mJumpSpeedHigh;
            rc::startPuppetSe(puppet, "JumpHigh");
            isHigh = true;
        } else {
            up *= mJumpSpeed;
            rc::startPuppetSe(puppet, "JumpVoice");
        }

        velocity += up;
        rc::setPuppetVelocity(puppet, velocity);
        rc::endBindAndPuppetNull(&puppet, &sParam.bindEndParamJump);
        mPadRumbleKeeper->setPort(
            rc::getPadPortByUserId(rc::tryFindControlUserId(mBindSensors[i])));
        al::startHitReaction(this, isHigh ? "ジャンプ大" : "ジャンプ");
    }

    al::startAction(this, isHigh ? "ReactionHigh" : "Reaction");
    mBindSensors.clear();
    mStableStep = 0;
    if (al::isNerve(this, &NrvSkipperTrampolineWaitCarry) ||
        al::isNerve(this, &NrvSkipperTrampolineReactionCarry) ||
        al::isNerve(this, &NrvSkipperTrampolineRecoverSignCarry) ||
        al::isNerve(this, &NrvSkipperTrampolineRecoverStartCarry)) {
        al::setNerve(this, &NrvSkipperTrampolineReactionCarry);
    } else if (al::isNerve(this, &NrvSkipperTrampolineFall)) {
        al::setNerve(this, &NrvSkipperTrampolineReactionFall);
    } else {
        al::setNerve(this, &NrvSkipperTrampolineReaction);
    }
}

/** @brief Bounces on other trampolines, returns the collider and launches bound players. */
void SkipperTrampoline::control() {
    EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this);
    if (al::isNerve(this, &NrvSkipperTrampolineEnd)) {
        return;
    }

    if (al::isCollidedGround(this)) {
        al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);
        if (groundSensor != nullptr) {
            al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Trampoline"));
            if (rc::sendMsgEnemyFloorTouchTrampoline(groundSensor,
                                                     al::getHitSensor(this, "Trampoline")) &&
                al::getVelocity(this).y <= 0.0f) {
                al::addVelocity(this, sead::Vector3f::ey * 50.0f);
                al::startAction(this, "FallStart");
            }
        }
    }

    if (!isNerveCarry()) {
        BallStateFunction::setColliderReturnedSlowly(this, static_cast<s32>(mColliderRadius), 3);
    }

    tryJumpPlayer();
    if (mStableStep < 500) {
        mStableStep++;
    }
}

/**
 * @brief Checks whether the trampoline is carried by a player.
 * @return Whether the current nerve is one of the carried nerves.
 */
bool SkipperTrampoline::isNerveCarry() const {
    return al::isNerve(this, &NrvSkipperTrampolineWaitCarry) ||
           al::isNerve(this, &NrvSkipperTrampolineReactionCarry) ||
           al::isNerve(this, &NrvSkipperTrampolineRecoverSignCarry) ||
           al::isNerve(this, &NrvSkipperTrampolineRecoverStartCarry);
}

/**
 * @brief Pushes enemies and players, and tramples enemies while falling.
 * @param pSelf Sensor of the trampoline.
 * @param pOther Sensor that was hit.
 */
void SkipperTrampoline::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf)) {
        if (al::isSensorEnemyBody(pOther)) {
            al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        }
    } else if (al::isSensorName(pSelf, "PushPlayer") && al::isSensorPlayer(pOther)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (al::isNerve(this, &NrvSkipperTrampolineFall) && al::isSensorEnemyAttack(pSelf)) {
        if (al::isSensorMapObj(pOther) && al::sendMsgBallItemGet(pOther, pSelf)) {
            return;
        }

        if (al::isSensorEnemy(pOther)) {
            al::sendMsgBallTrample(pOther, pSelf, mComboCounter);
        }
    }
}

/**
 * @brief Handles being carried, released, attacked and bounced on.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the trampoline that received the message.
 * @return Whether the message was handled.
 */
bool SkipperTrampoline::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                   al::HitSensor* pSelf) {
    if (al::isDead(this) || isEnd()) {
        return false;
    }

    if (al::isMsgGoalKill(pMsg)) {
        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
        }

        al::startHitReactionDeath(this);
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::appearItemTiming(this, "Coin");
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        kill();
        return true;
    }

    if (al::isSensorEnemyBody(pSelf)) {
        if (al::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 4.0f)) {
            return true;
        }

        if (al::isMsgExplosion(pMsg)) {
            return true;
        }
    }

    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (isNerveCarry()) {
        if (al::isMsgPlayerRelease(pMsg) || al::isMsgPlayerReleaseDamage(pMsg) ||
            al::isMsgPlayerReleaseDead(pMsg)) {
            sead::Vector3f playerVelocity = rc::getPlayerVelocity(mHolderSensor);
            const sead::Vector3f& playerFront = rc::getPlayerFront(mHolderSensor);
            al::setVelocity(this, playerFront * 15.0f + sead::Vector3f::ey * 20.0f);
            al::addVelocity(this, playerVelocity * 1.5f);
            setRelease();
            al::setNerve(this, &NrvSkipperTrampolineFall);
            return true;
        }

        if (al::isMsgBindCancel(pMsg) || al::isMsgWarpStart(pMsg) || al::isMsgHoldCancel(pMsg)) {
            if (al::isMsgWarpStart(pMsg) && mHolderSensor != nullptr) {
                rc::requestPlayerRelease(mHolderSensor);
            }

            setRelease();
            al::setNerve(this, &NrvSkipperTrampolineFall);
            return true;
        }
    } else if (al::isNerve(this, &NrvSkipperTrampolineLand) ||
               al::isNerve(this, &NrvSkipperTrampolineWait) ||
               al::isNerve(this, &NrvSkipperTrampolineRecoverSign) ||
               al::isNerve(this, &NrvSkipperTrampolineRecoverStart) ||
               (al::isNerve(this, &NrvSkipperTrampolineFall) && al::isGreaterStep(this, 10)) ||
               al::isNerve(this, &NrvSkipperTrampolineReaction) ||
               al::isNerve(this, &NrvSkipperTrampolineReactionFall) ||
               al::isNerve(this, &NrvSkipperTrampolineReactionHit)) {
        if (al::isSensorHoldObj(pSelf) && al::isMsgPlayerCarryUp(pMsg)) {
            mHolderSensor = pOther;
            mStableStep = 0;
            al::invalidateClipping(this);
            mHoldColliderControl->start();
            al::invalidateHitSensor(this, "PushPlayer");
            al::invalidateHitSensor(this, "Trampoline");
            al::invalidateCollisionParts(this);
            al::setVelocityZero(this);
            al::startSe(this, "PgHoldStart");
            if (al::isNerve(this, &NrvSkipperTrampolineReaction) ||
                al::isNerve(this, &NrvSkipperTrampolineReactionFall) ||
                al::isNerve(this, &NrvSkipperTrampolineReactionHit)) {
                al::setNerve(this, &NrvSkipperTrampolineReactionCarry);
            } else {
                al::setNerve(this, &NrvSkipperTrampolineWaitCarry);
            }

            return true;
        }

        if (al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
            al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
            al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
            al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
            al::isMsgKickKouraReflect(pMsg) ||
            (al::isMsgPlayerObjUpperPunch(pMsg) && rc::getPlayerVelocity(pOther).y > 1.0f)) {
            if (al::isNerve(this, &NrvSkipperTrampolineReactionHit) ||
                al::isNerve(this, &NrvSkipperTrampolineFall) ||
                al::isNerve(this, &NrvSkipperTrampolineLand)) {
                return false;
            }

            if (al::isMsgPlayerObjUpperPunch(pMsg)) {
                sead::BoundBox3f hitBox(sParam.punchHitBoxSize * -0.5f, sParam.punchHitBoxSize * 0.5f);
                sead::Matrix34f hitBoxMtx = *getBaseMtx();
                hitBoxMtx.setTranslation(al::getTrans(this) + sParam.punchHitBoxOffset);
                if (!al::isHitBoxSensor(pOther, hitBoxMtx, hitBox)) {
                    return false;
                }
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvSkipperTrampolineReactionHit);
            // The fire ball check has no effect anymore, but the game still calls it.
            if (al::isMsgPlayerFireBallAttack(pMsg)) {
                return true;
            }

            return true;
        }
    }

    if (isEnd() || mStableStep < 13) {
        return false;
    }

    if (al::isMsgPlayerFloorTouch(pMsg)) {
        rc::requestPlayerBind(pOther, al::getHitSensor(this, "Trampoline"));
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (rc::getPlayerVelocity(pOther).y > 0.0f) {
            return false;
        }

        sead::Vector3f sensorPos = al::getSensorPos(pOther);
        if (!alCollisionUtil::getFirstPolyOnArrow(this, nullptr, nullptr, sensorPos,
                                                  sead::Vector3f::ey * 100.0f, nullptr,
                                                  nullptr)) {
            return true;
        }

        for (s32 i = 0; i < mSinkSensors.size(); i++) {
            if (mSinkSensors[i] == pOther) {
                return false;
            }
        }

        mSinkSensors.pushBack(pOther);
        if (al::isNerve(this, &NrvSkipperTrampolineSink)) {
            return false;
        }

        al::invalidateCollisionParts(this);
        al::setNerve(this, &NrvSkipperTrampolineSink);
        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        mBindSensors.pushBack(pOther);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mBindSensors.erase(mBindSensors.indexOf(pOther));
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the trampoline finished recovering.
 * @return Whether the trampoline is in the end nerve.
 */
bool SkipperTrampoline::isEnd() const {
    return al::isNerve(this, &NrvSkipperTrampolineEnd);
}

/** @brief Puts the trampoline back on its feet after a player let go of it. */
void SkipperTrampoline::setRelease() {
    al::onCollide(this);
    al::resetPosition(this, false);
    sead::Vector3f front;
    if (mHolderSensor != nullptr) {
        front.e = rc::getPlayerFront(mHolderSensor).e;
    } else {
        const sead::Matrix34f* baseMtx = getBaseMtx();
        front.set((*baseMtx)(0, 2), (*baseMtx)(1, 2), (*baseMtx)(2, 2));
    }

    sead::Matrix34f poseMtx;
    al::makeMtxUpFront(&poseMtx, sead::Vector3f::ey, front);
    poseMtx.setTranslation(getBaseMtx()->getTranslation());
    al::updatePoseMtx(this, &poseMtx);
    al::setColliderRadius(this, 20.0f);
    al::validateCollisionParts(this);
    mStableStep = 0;
}

/**
 * @brief Checks whether the trampoline ignores messages.
 * @return Whether the trampoline is in the end nerve.
 */
bool SkipperTrampoline::isInvalidNerve() const {
    return al::isNerve(this, &NrvSkipperTrampolineEnd);
}

/**
 * @brief Reacts to being touched on the touch screen.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the trampoline.
 * @return Whether the touch was handled.
 */
bool SkipperTrampoline::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                              al::ScreenPointer* pPointer,
                                              al::ScreenPointTarget* pTarget) {
    if (isInvalidNerve()) {
        return false;
    }

    if (!al::isNerve(this, &NrvSkipperTrampolineLand) &&
        !al::isNerve(this, &NrvSkipperTrampolineWait) &&
        !al::isNerve(this, &NrvSkipperTrampolineRecoverSign) &&
        !al::isNerve(this, &NrvSkipperTrampolineReactionHit)) {
        return false;
    }

    if (!al::isMsgTouchAssistTrig(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvSkipperTrampolineReactionHit) && al::isLessStep(this, 10)) {
        return false;
    }

    al::startHitReaction(this, "タッチ");
    al::setNerve(this, &NrvSkipperTrampolineReactionHit);
    return true;
}

/** @brief Waits on the ground until it is time to recover. */
void SkipperTrampoline::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (!al::isOnGround(this, 3, 0.0f)) {
        al::setNerve(this, &NrvSkipperTrampolineFall);
        return;
    }

    updateVelocity();
    if (mStableStep >= 500) {
        al::setNerve(this, &NrvSkipperTrampolineRecoverSign);
    }
}

/** @brief Applies friction and gravity. */
void SkipperTrampoline::updateVelocity() {
    if (al::isOnGround(this, 0, 0.0f)) {
        al::scaleVelocity(this, 0.7f);
        al::addVelocity(this, sead::Vector3f::ey * -0.5f);
    } else {
        al::scaleVelocity(this, 0.98f);
        al::addVelocity(this, sead::Vector3f::ey * -2.0f);
    }
}

/** @brief Follows the player carrying the trampoline. */
void SkipperTrampoline::exeWaitCarry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isStep(this, 3)) {
        al::validateCollisionParts(this);
    }

    setHoldPos();
    if (mStableStep >= 500) {
        al::setNerve(this, &NrvSkipperTrampolineRecoverSignCarry);
    }
}

/** @brief Places the trampoline in the hands of the player carrying it. */
void SkipperTrampoline::setHoldPos() {
    sead::Vector3f holdPos = sParam.playerHoldParam.getPlayerHoldPos(mHolderSensor);
    sead::Matrix34f holdMtx;
    rc::calcPlayerHoldMtx(&holdMtx, mHolderSensor);
    sead::Vector3f trans;
    al::calcTransLocalOffsetByMtx(&trans, holdMtx, holdPos);
    sead::Matrix34f rotateMtx;
    rotateMtx.makeR(sParam.playerHoldParam.getHoldRotate() * (sead::Mathf::pi() / 180.0f));
    holdMtx = holdMtx * rotateMtx;
    holdMtx.setTranslation(trans);
    al::updatePoseMtx(this, &holdMtx);
}

/** @brief Sinks while players stand on it, as long as there is ground right below them. */
void SkipperTrampoline::exeSink() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        al::stopAction(this);
    }

    updateVelocity();
    mStableStep = 0;
    if (mSinkSensors.size() <= 0) {
        return;
    }

    for (s32 i = 0; i < mSinkSensors.size();) {
        sead::Vector3f sensorPos = al::getSensorPos(mSinkSensors[i]);
        if (alCollisionUtil::getFirstPolyOnArrow(this, nullptr, nullptr, sensorPos,
                                                 sead::Vector3f::ey * 100.0f, nullptr, nullptr)) {
            i++;
        } else {
            mSinkSensors.erase(i);
        }
    }

    if (mSinkSensors.size() == 0) {
        al::restartAction(this);
        al::validateCollisionParts(this);
        al::setNerve(this, &NrvSkipperTrampolineReaction);
    }
}

/** @brief Plays the bounce reaction on the ground. */
void SkipperTrampoline::exeReaction() {
    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineWait);
    }
}

/** @brief Plays the bounce reaction in the air. */
void SkipperTrampoline::exeReactionFall() {
    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineFall);
        return;
    }

    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvSkipperTrampolineLand);
    }
}

/** @brief Plays the bounce reaction while carried. */
void SkipperTrampoline::exeReactionCarry() {
    if (al::isStep(this, 3)) {
        al::validateCollisionParts(this);
    }

    setHoldPos();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineWaitCarry);
    }
}

/** @brief Falls until it lands on the ground. */
void SkipperTrampoline::exeFall() {
    if (al::isFirstStep(this)) {
        if (!al::isActionPlaying(this, "FallStart")) {
            al::startAction(this, "Fall");
        }

        mComboCounter->reset();
    }

    if (al::isActionPlaying(this, "FallStart") && al::isActionEnd(this)) {
        al::startAction(this, "Fall");
    }

    updateVelocity();
    if (al::isOnGround(this, 0, 0.0f)) {
        mHolderSensor = nullptr;
        al::validateClipping(this);
        al::setNerve(this, &NrvSkipperTrampolineLand);
    }
}

/** @brief Lands on the ground and becomes bounceable again. */
void SkipperTrampoline::exeLand() {
    if (al::isFirstStep(this)) {
        al::validateHitSensor(this, "PushPlayer");
        al::validateHitSensor(this, "Trampoline");
        al::startAction(this, "Land");
    }

    if (!al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvSkipperTrampolineFall);
        return;
    }

    updateVelocity();
    if (mStableStep >= 500) {
        al::setNerve(this, &NrvSkipperTrampolineRecoverStart);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineWait);
    }
}

/** @brief Shakes before turning back into a Skipper. */
void SkipperTrampoline::exeRecoverSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverSign");
    }

    updateVelocity();
    if (al::isGreaterStep(this, 90)) {
        al::setNerve(this, &NrvSkipperTrampolineRecoverStart);
    }
}

/** @brief Starts turning back into a Skipper. */
void SkipperTrampoline::exeRecoverStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverStart");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineRecover);
    }
}

/** @brief Shakes in the player's hands before turning back into a Skipper. */
void SkipperTrampoline::exeRecoverSignCarry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverSign");
        mPadRumbleKeeper->setPort(
            rc::getPadPortByUserId(rc::tryFindControlUserId(mHolderSensor)));
    }

    al::startHitReaction(this, "復帰予兆");
    setHoldPos();
    if (al::isGreaterStep(this, 90)) {
        al::setNerve(this, &NrvSkipperTrampolineRecoverStartCarry);
    }
}

/** @brief Starts turning back into a Skipper in the player's hands. */
void SkipperTrampoline::exeRecoverStartCarry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RecoverStart");
    }

    al::startHitReaction(this, "復帰開始");
    setHoldPos();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineRecover);
    }
}

/** @brief Jumps up and turns back into a Skipper. */
void SkipperTrampoline::exeRecover() {
    if (al::isFirstStep(this)) {
        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
        }

        if (mSinkSensors.size() > 0) {
            mSinkSensors.clear();
        }

        setRelease();
        al::setVelocity(this, sead::Vector3f::ey * 40.0f);
        al::startAction(this, "Recover");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineEnd);
    }
}

/** @brief Keeps moving after recovering, until the Skipper takes over. */
void SkipperTrampoline::exeEnd() {
    updateVelocity();
}

/** @brief Plays the reaction to being hit. */
void SkipperTrampoline::exeReactionHit() {
    if (al::isFirstStep(this)) {
        mStableStep = 0;
        al::startAction(this, "ReactionHit");
    }

    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSkipperTrampolineWait);
    }
}

/** @brief Updates the hold collider while carried, or the regular collider otherwise. */
void SkipperTrampoline::updateCollider() {
    if (isNerveCarry()) {
        mHoldColliderControl->update(mHolderSensor, al::getHitSensor(this, "Hold"));
        return;
    }

    al::LiveActor::updateCollider();
}
