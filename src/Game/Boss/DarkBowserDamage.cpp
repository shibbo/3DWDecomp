#include "Boss/DarkBowserDamage.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Boss/DarkBowser.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DarkBowserDamage, Recover);
NERVE_DECL(DarkBowserDamage, HipDropHit);
NERVE_DECL(DarkBowserDamage, SideKick);
NERVE_DECL(DarkBowserDamage, BombHit);
NERVE_DECL(DarkBowserDamage, Blocking);
NERVE_DECL(DarkBowserDamage, Stomp);

/** @brief The killing hip drop / scratch that leaves the shell stuck (shares exeHipDropHit). */
class DarkBowserDamageNrvShellStuckHit : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DarkBowserDamage>()->exeHipDropHit();
    }
};

/** @brief The killing hit taken while blocking (shares exeBlocking). */
class DarkBowserDamageNrvBlockingDefeat : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DarkBowserDamage>()->exeBlocking();
    }
};

NERVES_MAKE_NOSTRUCT(DarkBowserDamage, Recover)

// The other nerves are mutable globals (they get merged into one block).
DarkBowserDamageNrvHipDropHit NrvDarkBowserDamageHipDropHit;
DarkBowserDamageNrvShellStuckHit NrvDarkBowserDamageShellStuckHit;
DarkBowserDamageNrvSideKick NrvDarkBowserDamageSideKick;
DarkBowserDamageNrvBlockingDefeat NrvDarkBowserDamageBlockingDefeat;
DarkBowserDamageNrvBombHit NrvDarkBowserDamageBombHit;
DarkBowserDamageNrvBlocking NrvDarkBowserDamageBlocking;
DarkBowserDamageNrvStomp NrvDarkBowserDamageStomp;

/**
 * @brief Whether a climbing player's attack is one that Fury Bowser blocks.
 * @param pPlayer The attacking player actor.
 * @return True if the player's attack type (unknown field at 0x280) is in a blockable range.
 */
bool isBlockableClimbAttack(const al::LiveActor* pPlayer) {
    s32 type = *reinterpret_cast<const s32*>(reinterpret_cast<const u8*>(pPlayer) + 0x280);
    return (type >= 13 && type <= 21) || (type >= 23 && type <= 31);
}
}  // namespace

/**
 * @brief Constructs the damage state.
 * @param pHost Fury Bowser.
 * @param rInfo Actor init info (unused).
 */
DarkBowserDamage::DarkBowserDamage(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("DarkBowserDamage"), mHost(pHost) {
    initNerve(&NrvDarkBowserDamageRecover, 0);
    al::setEffectFollowMtxPtr(mHost, "HrShellHitWeak", &mEffectMtx);
    al::setEffectFollowMtxPtr(mHost, "HrShellHitWeakJr", &mEffectMtxJr);
    al::setEffectFollowMtxPtr(mHost, "HrShellHitHipDropJr", &mEffectMtxJr);
}

/** @brief Activates the state and clears the per-attack flags. */
void DarkBowserDamage::appear() {
    al::NerveStateBase::appear();
    mIsSideKicked = false;
    mIsBombHit = false;
    mIsStompEnd = false;
}

/** @brief Ends the state, releasing the player and playing the final hit reaction on defeat. */
void DarkBowserDamage::kill() {
    al::NerveStateBase::kill();

    if (mBindSensor != nullptr) {
        pushReleasePlayer();
    }

    mHost->resetHitSensors();
    mScratchCooldown = 0;

    if (!mIsDefeat) {
        return;
    }

    if (al::isNerve(this, &NrvDarkBowserDamageHipDropHit)) {
        al::startHitReaction(mHost, mIsJrHit ? "HrShellHitLastJr" : "HrShellHitLast");
    } else if (al::isNerve(this, &NrvDarkBowserDamageShellStuckHit)) {
        al::startHitReaction(mHost, mIsJrHit ? "HrShellStuckHitLastJr" : "HrShellStuckHitLast");
    } else if (al::isNerve(this, &NrvDarkBowserDamageSideKick)) {
        al::startHitReaction(mHost, mIsJrHit ? "HrShellHitWeakLastJr" : "HrShellHitWeakLast");
    } else if (al::isNerve(this, &NrvDarkBowserDamageBlockingDefeat)) {
        al::startHitReaction(mHost, mIsJrHit ? "HrBlockingHitLastJr" : "HrBlockingHitLast");
    } else if (al::isNerve(this, &NrvDarkBowserDamageBombHit)) {
        al::startHitReaction(mHost, "HrBombHitLast");
    }
}

/** @brief Releases the bound player, pushing them away unless Fury Bowser was defeated. */
void DarkBowserDamage::pushReleasePlayer() {
    if (mIsDefeat) {
        al::sendMsgBindEnd(mBindSensor, al::getHitSensor(mHost, "SoftBelly"));
        return;
    }

    sead::Vector3f front = rc::getPlayerFront(mBindSensor);
    rc::setPlayerVelocity(al::getSensorHost(mBindSensor),
                          -(front * 300.0f) + sead::Vector3f::ey * 380.0f);
    al::sendMsgBindEnd(mBindSensor, al::getHitSensor(mHost, "SoftBelly"));
    mBindSensor = nullptr;
}

/**
 * @brief Handles a message received by one of Fury Bowser's sensors.
 * @param pMsg The message.
 * @param pOther The sensor that sent the message.
 * @param pSelf Fury Bowser's sensor that received it.
 * @return True if the message was consumed.
 */
bool DarkBowserDamage::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    mIsMsgResult = false;

    if (mIsDefeat) {
        return false;
    }

    if (al::isSensorEnemyAttack(pSelf)) {
        return false;
    }

    bool isBlocking = al::isNerve(this, &NrvDarkBowserDamageBlocking);

    if (al::isMsgPlayerClimbRollingAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg)) {
        mIsMsgResult = true;
        return false;
    }

    if ((isDead() || isBlocking) && al::isSensorEnemyBody(pSelf) &&
        ((al::isMsgPlayerClimbAttack(pMsg) &&
          isBlockableClimbAttack(al::getSensorHost(pOther))) ||
         (al::isMsgPlayerSpinAttack(pMsg) && al::isSensorHostName(pOther, "KoopaJr"))) &&
        mScratchCooldown <= 0) {
        bool isJr = al::isSensorHostName(pOther, "KoopaJr");
        mScratchCooldown = 10;
        mHost->requestDamage(isJr ? 1 : 4);
        mScratchCount++;

        bool isDefeat = checkDefeat();

        if (isDefeat) {
            mSmallHitType = SmallHitType_None;
        } else {
            al::stopScene(mHost, 4, 2, true, false);
        }

        if (isJr) {
            if (!al::isActionPlaying(mHost, "Blocking")) {
                al::tryStartActionIfNotPlaying(mHost, "Wait");
            }
        } else {
            al::startAction(mHost, "Blocking");
        }

        mHost->requestSmallBlur(&al::getSensorPos(pOther));
        calcEffectMtx(pOther, pSelf, isJr);

        bool isStart = isDefeat || !isBlocking;

        if (isStart) {
            appear();
            const al::Nerve* nerve = &NrvDarkBowserDamageBlocking;

            if (isDefeat) {
                nerve = &NrvDarkBowserDamageBlockingDefeat;
            }

            al::setNerve(this, nerve);
        }

        if (isDefeat) {
            return true;
        }

        startDamageHitReaction(isJr);

        if (isStart) {
            return true;
        }
    }

    bool isHitMiss = al::isActionPlaying(mHost, "ShellHitMiss");

    if (al::isSensorPlayer(pOther) && (al::isSensorName(pSelf, "SoftBelly") || isHitMiss) &&
        mBindSensor == nullptr) {
        const sead::Vector3f& otherPos = al::getSensorPos(pOther);
        const sead::Vector3f& selfPos = al::getSensorPos(pSelf);
        f32 otherY = otherPos.y;
        f32 selfY = selfPos.y;

        if (al::isMsgPlayerClimbAttack(pMsg) &&
            isBlockableClimbAttack(al::getSensorHost(pOther)) && mScratchCooldown <= 0) {
            if (isHitMiss) {
                return false;
            }

            if (checkScratchCountChanceTime()) {
                return false;
            }

            mScratchCooldown = 10;
            mScratchCount++;
            bool isSaveAction = mPrevActionName == nullptr;
            mSmallHitType = SmallHitType_Scratch;
            mSmallHitSensor = pOther;

            if (isSaveAction) {
                mPrevActionName = al::getActionName(mHost);
                mPrevActionFrame = al::getActionFrame(mHost);
            }

            mSmallHitTimer = 30;
            mHost->requestDamage(4);
            mHost->requestSmallBlur(&al::getSensorPos(pSelf));

            if (checkDefeat()) {
                al::setNerve(this, &NrvDarkBowserDamageShellStuckHit);
                calcEffectMtx(pOther, pSelf, false);
                mBindSensor = pOther;
                mSmallHitType = SmallHitType_None;
                return true;
            }

            al::stopScene(mHost, 4, 2, true, false);
            return false;
        }

        f32 diffY = otherY - selfY;

        if (!al::isMsgTrampleAll(pMsg)) {
            if (al::isSensorName(pOther, "Foot") &&
                diffY < al::getSensorRadius(pOther) * 1.25f) {
                al::sendMsgPushVeryStrong(pOther, pSelf);
            }

            return false;
        }

        if (al::getVelocity(al::getSensorHost(pOther)).y < 0.0f) {
            if (sead::Mathf::abs(diffY) < 5000.0f || isHitMiss) {
                if (isHitMiss && al::getActionFrame(mHost) > 45.0f) {
                    if (al::isSensorName(pSelf, "BodyDM")) {
                        al::sendMsgGigaEnemyAttack(pOther, pSelf);
                    }

                    al::sendMsgGigaStomp(pOther, pSelf);
                    mSmallHitSensor = nullptr;
                    mSmallHitType = SmallHitType_None;
                    mIsMsgResult = false;
                    return false;
                }

                if (rc::isPlayerHipDropping(pOther) || rc::isPlayerClimbGigaBodyAttack(pOther)) {
                    mSmallHitType = SmallHitType_HipDrop;
                    mHost->requestLargeBlur(&al::getSensorPos(pSelf));
                    al::invalidateHitSensors(mHost);
                    appear();
                    al::setNerve(this, &NrvDarkBowserDamageHipDropHit);

                    if (mHost->isFinalBattle()) {
                        mHost->requestDamage(300);
                    } else {
                        mHost->requestDamage(34);
                    }

                    rc::requestPlayerBind(pOther, al::getHitSensor(mHost, "SoftBellyBind"));
                    rc::setPlayerVelocity(al::getSensorHost(pOther), sead::Vector3f::zero);
                    mBindSensor = pOther;
                    return true;
                }

                mSmallHitSensor = pOther;
                mSmallHitType = SmallHitType_Bounce;
                mIsMsgResult = true;
                mPrevActionName = al::getActionName(mHost);
                mPrevActionFrame = al::getActionFrame(mHost);

                if (mSmallHitTimer <= 0) {
                    mSmallHitTimer = 30;
                }
            }
        }

        return false;
    }

    if (al::isSensorHostName(pOther, "KoopaJr")) {
        if (mScratchCooldown > 0) {
            return false;
        }

        if (al::isSensorName(pSelf, "SoftBelly")) {
            if (al::isMsgPlayerSpinAttack(pMsg)) {
                if (al::isActionPlaying(mHost, "ShellHitMiss")) {
                    return false;
                }

                if (checkScratchCountChanceTime()) {
                    return false;
                }

                mScratchCooldown = 10;
                mScratchCount++;
                bool isSaveAction = mPrevActionName == nullptr;
                mSmallHitType = SmallHitType_Scratch;
                mSmallHitSensor = pOther;

                if (isSaveAction) {
                    mPrevActionName = al::getActionName(mHost);
                    mPrevActionFrame = al::getActionFrame(mHost);
                }

                mSmallHitTimer = 30;
                mHost->requestDamage(1);
                mHost->requestSmallBlur(&al::getSensorPos(pSelf));
            } else {
                if (!al::isMsgPlayerHipDropAll(pMsg)) {
                    return false;
                }

                bool isMiss = al::isActionPlaying(mHost, "ShellHitMiss");
                mSmallHitSensor = pOther;
                mSmallHitType = SmallHitType_Bounce;

                if (isMiss && al::getActionFrame(mHost) > 45.0f) {
                    al::sendMsgGigaStomp(pOther, pSelf);
                    return false;
                }

                al::getVelocityPtr(al::getSensorHost(pOther))->y = 300.0f;
                mPrevActionName = al::getActionName(mHost);
                mPrevActionFrame = al::getActionFrame(mHost);

                if (mSmallHitTimer <= 0) {
                    mSmallHitTimer = 30;
                }

                mHost->requestDamage(2);
            }

            al::stopScene(mHost, 4, 2, true, false);

            if (checkDefeat()) {
                al::setNerve(this, &NrvDarkBowserDamageShellStuckHit);
                mSmallHitType = SmallHitType_None;
                return true;
            }

            return false;
        }

        if (!al::isSensorName(pSelf, "SideBelly") || !al::isMsgPlayerSpinAttack(pMsg)) {
            return false;
        }

        bool isRight = al::isActionPlaying(mHost, "ShellWheelRight") ||
                       al::isActionPlaying(mHost, "ShellWheelChargeRight");
        sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
        sead::Vector3f side = isRight ? sead::Vector3f::ex : -sead::Vector3f::ex;
        sead::Quatf quat = al::getQuat(mHost);
        al::normalizeOrZero(&dir);
        side.rotate(quat);

        if (dir.dot(side) < 0.0f) {
            appear();
            al::setNerve(this, &NrvDarkBowserDamageSideKick);
            al::invalidateHitSensor(mHost, "SideBelly");
            calcEffectMtx(pOther, pSelf, false);
            mHost->requestMediumBlur(&al::getSensorPos(pSelf));
            return true;
        }

        al::sendMsgGigaEnemyAttack(pOther, pSelf);
        return false;
    }

    bool isKick = al::isMsgPlayerKick(pMsg);

    if (al::isSensorName(pSelf, "SideBelly") && (isKick || al::isMsgPlayerClimbAttack(pMsg))) {
        bool isRight = al::isActionPlaying(mHost, "ShellWheelRight") ||
                       al::isActionPlaying(mHost, "ShellWheelChargeRight");
        sead::Vector3f dir = al::getSensorPos(pSelf) - al::getSensorPos(pOther);
        sead::Vector3f front = rc::getPlayerFront(pOther);
        sead::Vector3f side = isRight ? sead::Vector3f::ex : -sead::Vector3f::ex;
        sead::Quatf quat = al::getQuat(mHost);
        al::normalizeOrZero(&dir);
        side.rotate(quat);
        f32 speed = rc::getPlayerSpeedH(pOther);

        if (dir.dot(side) < -0.4f) {
            if (front.dot(side) < -0.4f && (speed > 10.0f || !isKick)) {
                appear();
                al::setNerve(this, &NrvDarkBowserDamageSideKick);
                mHost->requestDamage(10);
                al::invalidateHitSensor(mHost, "SideBelly");
                calcEffectMtx(pOther, pSelf, false);
                mBindSensor = pOther;
                mHost->requestMediumBlur(&al::getSensorPos(pSelf));
                return true;
            }
        } else {
            al::sendMsgGigaEnemyAttack(pOther, pSelf);
            return false;
        }
    }

    if (mBombHitCooldown == 0 && (al::isMsgExplosion(pMsg) || al::isMsgBallAttack(pMsg))) {
        appear();
        mSmallHitSensor = pOther;
        al::setNerve(this, &NrvDarkBowserDamageBombHit);
        mHost->requestDamage(4);
        calcEffectMtx(pOther, pSelf, false);
        mHost->requestSmallBlur(&al::getSensorPos(pOther));
        mHost->requestDamage(10);
        mBombHitCooldown = 30;
        return true;
    }

    if (mBindSensor == nullptr) {
        return false;
    }

    if (al::isMsgBindStart(pMsg) || al::isMsgBindInit(pMsg)) {
        return true;
    }

    if (al::isMsgBindEnd(pMsg) || al::isMsgBindCancel(pMsg)) {
        mBindSensor = nullptr;
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the last hit drained the current section of the health bar.
 * @return True if Fury Bowser is defeated (or retreats) after this hit.
 */
bool DarkBowserDamage::checkDefeat() const {
    return mHost->isHealthSectionEmpty();
}

/**
 * @brief Places the hit effect matrix at the hit point, facing away from the attacker.
 * @param pOther The attacking sensor.
 * @param pSelf Fury Bowser's hit sensor.
 * @param isJr Whether Bowser Jr. made the hit (uses the Jr. effect matrix).
 */
void DarkBowserDamage::calcEffectMtx(const al::HitSensor* pOther, const al::HitSensor* pSelf,
                                     bool isJr) {
    if (pOther == nullptr || pSelf == nullptr) {
        return;
    }

    sead::Matrix34f& mtx = isJr ? mEffectMtxJr : mEffectMtx;
    f32 radius = al::getSensorRadius(pSelf);
    sead::Vector3f pos = al::getSensorPos(pSelf);
    sead::Vector3f dir;
    sead::Quatf quat(1.0f, 0.0f, 0.0f, 0.0f);
    al::calcDirBetweenSensors(&dir, pSelf, pOther);

    if (!al::isNearZero(dir, 0.001f)) {
        sead::Vector3f side;
        side.setCross(dir, sead::Vector3f::ey);

        if (al::isNearZero(side, 0.001f)) {
            side = sead::Vector3f::ex;
        }

        al::makeQuatFrontSide(&quat, dir, side);
    }

    mIsJrHit = isJr;
    mtx.makeQT(quat, pos + radius * dir);
}

/**
 * @brief Starts the hit reaction matching the current damage nerve.
 * @param isJr Whether Bowser Jr. made the hit.
 */
void DarkBowserDamage::startDamageHitReaction(bool isJr) const {
    if (mIsDefeat) {
        mHost->requestFinalHit();
        return;
    }

    if (al::isNerve(this, &NrvDarkBowserDamageHipDropHit)) {
        al::startHitReaction(mHost, isJr ? "HrShellHitJr" : "HrShellHit");
    } else if (al::isNerve(this, &NrvDarkBowserDamageBlocking)) {
        al::startHitReaction(mHost, isJr ? "HrBlockingHitJr" : "HrBlockingHit");
    } else if (al::isNerve(this, &NrvDarkBowserDamageBombHit)) {
        al::startHitReaction(mHost, "HrBombHit");
    } else if (al::isNerve(this, &NrvDarkBowserDamageSideKick)) {
        al::startHitReaction(mHost, isJr ? "HrShellHitWeakJr" : "HrShellHitWeak");
    } else {
        al::startHitReaction(mHost, isJr ? "HrShellStuckHitJr" : "HrShellStuckHit");
    }
}

/** @brief Plays pending small hits (bounces and scratches) and counts down the cooldowns. */
void DarkBowserDamage::updateSmallHit() {
    if (mSmallHitType == SmallHitType_Bounce || mSmallHitType == SmallHitType_Scratch) {
        if (mSmallHitTimer == 30) {
            bool isSameAction = al::isEqualString(mPrevActionName, al::getActionName(mHost));
            bool isJr = mSmallHitSensor != nullptr &&
                        al::isSensorHostName(mSmallHitSensor, "KoopaJr");
            bool isRestart = isSameAction &
                             !al::isEqualString("ShellSign", al::getActionName(mHost)) &
                             !al::isEqualString("ShellHitMiss", al::getActionName(mHost));
            calcEffectMtx(mSmallHitSensor, al::getHitSensor(mHost, "SoftBelly"), isJr);
            mSmallHitSensor = nullptr;

            if (mSmallHitType == SmallHitType_Bounce) {
                if (isRestart) {
                    al::startAction(mHost, "ShellHitBounce");
                }

                al::startHitReaction(mHost, isJr ? "HrShellHitHipDropJr" : "HrShellHitBounce");
            } else {
                if (isRestart) {
                    al::startAction(mHost, "ShellHitWeak");
                }

                startDamageHitReaction(isJr);
            }
        }

        mSmallHitTimer--;

        if (mSmallHitTimer <= 0) {
            if (al::isEqualString(al::getActionName(mHost), "ShellHitBounce") ||
                al::isEqualString(al::getActionName(mHost), "ShellHitWeak")) {
                al::startAction(mHost, mPrevActionName);
                al::setActionFrame(mHost, mPrevActionFrame);
            }

            mPrevActionName = nullptr;
            mPrevActionFrame = 0;
            mSmallHitType = SmallHitType_None;
        }
    }

    if (mBombHitCooldown > 0) {
        mBombHitCooldown--;
    }

    if (mScratchCooldown > 0) {
        mScratchCooldown--;
    }
}

/** @brief Hit by a hip drop: holds the player in the belly, then releases them. */
void DarkBowserDamage::exeHipDropHit() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "ShellHit");
        mIsDefeat = checkDefeat();
        startDamageHitReaction(false);
    }

    if (al::isStep(this, 30)) {
        if (mBindSensor != nullptr) {
            pushReleasePlayer();
        }

        if (mIsDefeat) {
            return;
        }
    } else if (mIsDefeat && al::isGreaterEqualStep(this, 5)) {
        if (mBindSensor != nullptr) {
            rc::showPlayer(mBindSensor);
            al::sendMsgBindEnd(mBindSensor, al::getHitSensor(mHost, "SoftBelly"));
        }

        kill();
        return;
    }

    if (al::isGreaterEqualStep(this, 30) && al::isActionEnd(mHost)) {
        al::setNerve(this, &NrvDarkBowserDamageRecover);
        mSmallHitType = SmallHitType_None;
    }
}

/** @brief Kicked from the side: Fury Bowser's shell is sent sliding away. */
void DarkBowserDamage::exeSideKick() {
    if (al::isFirstStep(this)) {
        if (!mHost->isFinalBattle()) {
            mHost->showHealthBar();
        }

        mIsDefeat = checkDefeat();
        mIsSideKicked = true;

        if (mBindSensor != nullptr) {
            rc::requestPlayerBind(mBindSensor, al::getHitSensor(mHost, "SoftBellyBind"));
            rc::setPlayerVelocity(al::getSensorHost(mBindSensor), sead::Vector3f::zero);
        }
    }

    if (al::isLessEqualStep(this, 5)) {
        al::scaleVelocity(mHost, 0.5f);
    }

    if (al::isStep(this, 4)) {
        startDamageHitReaction(false);
    }

    if (al::isStep(this, 5)) {
        bool isRight = al::isActionPlaying(mHost, "ShellWheelRight") ||
                       al::isActionPlaying(mHost, "ShellWheelChargeRight");
        mHost->setHealthBarState(true);

        if (mBindSensor != nullptr) {
            al::sendMsgBindEnd(mBindSensor, al::getHitSensor(mHost, "SoftBellyBind"));
            mBindSensor = nullptr;
        }

        sead::Vector3f dir = isRight ? -sead::Vector3f::ex : sead::Vector3f::ex;
        dir.rotate(al::getQuat(mHost));
        dir.y = 0.35f;
        f32 speed = mIsDefeat ? 225.0f : 900.0f;
        mHost->validateGravity();
        al::setVelocity(mHost, dir * speed);

        if (mIsDefeat) {
            kill();
        }

        return;
    }

    if (al::isStep(this, 18)) {
        al::startAction(mHost, "ShellHitWeak");
    }

    if (al::isGreaterStep(this, 14) && al::isCollidedGround(mHost)) {
        al::tryStartActionIfNotPlaying(mHost, "ShellHitWeak");
        al::setNerve(this, &NrvDarkBowserDamageRecover);
        kill();
    }
}

/** @brief Hit by a bomb: Fury Bowser is knocked back away from the explosion. */
void DarkBowserDamage::exeBombHit() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "BombHit");

        if (!mHost->isFinalBattle()) {
            mHost->showHealthBar();
        }

        mIsDefeat = checkDefeat();
        mIsBombHit = true;
        mHost->setHealthBarState(true);

        sead::Vector3f dir;

        if (mSmallHitSensor != nullptr) {
            const sead::Vector3f& bombPos = al::getSensorPos(mSmallHitSensor);
            const sead::Vector3f& trans = al::getTrans(mHost);
            dir.set(bombPos.x - trans.x, 0.0f, bombPos.z - trans.z);
            al::normalizeOrDirZ(&dir);
        } else {
            al::calcFrontDir(&dir, mHost);
        }

        mSmallHitSensor = nullptr;
        dir.y = -0.35f;
        f32 speed = mIsDefeat ? 225.0f : 900.0f;
        mHost->validateGravity();
        al::setVelocity(mHost, -(dir * speed));
        al::turnToDirection(mHost, dir, 180.0f);
        startDamageHitReaction(false);
        return;
    }

    if (mIsDefeat && al::isStep(this, 5)) {
        kill();
        return;
    }

    if (al::isStep(this, 20)) {
        al::startAction(mHost, "ShellHitWeak");
    }

    if (al::isGreaterStep(this, 14) && al::isCollidedGround(mHost)) {
        al::tryStartActionIfNotPlaying(mHost, "ShellHitWeak");
        al::setNerve(this, &NrvDarkBowserDamageRecover);
        kill();
    }
}

/** @brief Blocks an attack, then counters with a stomp. */
void DarkBowserDamage::exeBlocking() {
    if (al::isFirstStep(this)) {
        mIsDefeat = checkDefeat();

        if (mIsDefeat) {
            startDamageHitReaction(false);
        }
    }

    if (al::isNerve(this, &NrvDarkBowserDamageBlockingDefeat) &&
        al::isGreaterEqualStep(this, 5)) {
        kill();
        return;
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvDarkBowserDamageStomp);
    }
}

/** @brief Counter stomp after blocking, which hits the player if they are close enough. */
void DarkBowserDamage::exeStomp() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "AttackStomp");
    }

    if (al::isStep(this, 45)) {
        mHost->requestMediumBlur(&al::getSensorPos(al::getHitSensor(mHost, "LeftLeg")));
        al::LiveActor* player = mHost->getPlayer();

        if (player != nullptr) {
            sead::Vector3f playerTrans = al::getTrans(player);

            if ((playerTrans - al::getTrans(mHost)).length() < 16000.0f) {
                al::sendMsgGigaStomp(al::getHitSensor(player, 0),
                                     al::getHitSensor(mHost, "ShellDive"));
            }
        }
    }

    if (al::isActionEnd(mHost)) {
        mIsStompEnd = true;
        kill();
    }
}

/** @brief Gets back up after a hit, turning towards the player. */
void DarkBowserDamage::exeRecover() {
    if (al::isFirstStep(this)) {
        if (mIsDefeat) {
            kill();
            return;
        }

        al::startAction(mHost, "ShellRecover");
        mHost->setHealthBarState(false);
        mHost->resetHitSensors();
    }

    if (al::isGreaterEqualStep(this, 30) && al::isLessEqualStep(this, 66)) {
        f32 angle = al::calcAngleToTargetH(mHost, al::getTrans(mHost->getPlayer()));
        al::turnToTarget(mHost, mHost->getPlayer(), angle * 0.055f);
    }

    if (al::isActionEnd(mHost)) {
        kill();
    }
}

/**
 * @brief Whether the last hit defeated Fury Bowser.
 * @return True if defeated.
 */
bool DarkBowserDamage::isDefeat() const {
    return mIsDefeat;
}

/**
 * @brief Whether Fury Bowser is blocking or doing the counter stomp.
 * @return True while blocking or stomping.
 */
bool DarkBowserDamage::isBlockingOrStomping() const {
    return al::isNerve(this, &NrvDarkBowserDamageBlocking) ||
           al::isNerve(this, &NrvDarkBowserDamageStomp);
}

/**
 * @brief Whether the player has scratched Fury Bowser too often for another scratch to count.
 * @return True after more than three scratches.
 */
bool DarkBowserDamage::checkScratchCountChanceTime() const {
    return mScratchCount > 3;
}

/**
 * @brief Reads and clears the extra result of the last receiveMsg call.
 * @return True if the last message should be treated as received.
 */
bool DarkBowserDamage::receiveMsgResult() {
    bool result = mIsMsgResult;
    mIsMsgResult = false;
    return result;
}

DarkBowserDamage::~DarkBowserDamage() = default;
