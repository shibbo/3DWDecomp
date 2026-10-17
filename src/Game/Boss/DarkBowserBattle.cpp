#include "Boss/DarkBowserBattle.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Boss/DarkBowser.hpp"
#include "Boss/DarkBowserDamage.hpp"
#include "Boss/DarkBowserFirebomb.hpp"
#include "Boss/DarkBowserLaser.hpp"
#include "Boss/DarkBowserShellDive.hpp"
#include "Boss/DarkBowserShellSpike.hpp"
#include "Boss/DarkBowserWheel.hpp"
#include "Camera/CameraPoserDarkBowser.hpp"
#include "Course/GuideFrameOutSingleMode.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DarkBowserBattle, Begin);
NERVE_DECL(DarkBowserBattle, Wheel);
NERVE_DECL(DarkBowserBattle, Spike);
NERVE_DECL(DarkBowserBattle, Firebomb);
NERVE_DECL(DarkBowserBattle, Laser);
NERVE_DECL(DarkBowserBattle, Damage);
NERVE_DECL(DarkBowserBattle, Wait);

/** @brief The shell dive attack, which restores the camera when it ends. */
class DarkBowserBattleNrvShellDive : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DarkBowserBattle>()->exeShellDive();
    }

    void executeOnEnd(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DarkBowserBattle>()->endShellDive();
    }
};

// The nerves host nerve states, so they are mutable globals (merged into one block).
DarkBowserBattleNrvBegin NrvDarkBowserBattleBegin;
DarkBowserBattleNrvShellDive NrvDarkBowserBattleShellDive;
DarkBowserBattleNrvWheel NrvDarkBowserBattleWheel;
DarkBowserBattleNrvSpike NrvDarkBowserBattleSpike;
DarkBowserBattleNrvFirebomb NrvDarkBowserBattleFirebomb;
DarkBowserBattleNrvLaser NrvDarkBowserBattleLaser;
DarkBowserBattleNrvDamage NrvDarkBowserBattleDamage;
DarkBowserBattleNrvWait NrvDarkBowserBattleWait;
}  // namespace

/**
 * @brief Constructs the battle state and all of its attack states, camera and HUD marker.
 * @param pDarkBowser Fury Bowser.
 * @param rInfo Actor init info.
 */
DarkBowserBattle::DarkBowserBattle(DarkBowser* pDarkBowser, const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("DarkBowserBattle"), mHost(pDarkBowser) {
    initNerve(&NrvDarkBowserBattleBegin, 7);

    mFirebomb = new DarkBowserFirebomb(pDarkBowser, rInfo);
    mShellDive = new DarkBowserShellDive(pDarkBowser, rInfo);
    mDamage = new DarkBowserDamage(pDarkBowser, rInfo);
    mWheel = new DarkBowserWheel(pDarkBowser, rInfo);
    mSpike = new DarkBowserShellSpike(pDarkBowser, rInfo);
    mLaser = new DarkBowserLaser(pDarkBowser, rInfo);

    al::initNerveState(this, mShellDive, &NrvDarkBowserBattleShellDive, "ShellDive");
    al::initNerveState(this, mWheel, &NrvDarkBowserBattleWheel, "Wheel");
    al::initNerveState(this, mSpike, &NrvDarkBowserBattleSpike, "Spike");
    al::initNerveState(this, mFirebomb, &NrvDarkBowserBattleFirebomb, "Firebomb");
    al::initNerveState(this, mLaser, &NrvDarkBowserBattleLaser, "Laser");
    al::initNerveState(this, mDamage, &NrvDarkBowserBattleDamage, "Damage");

    mHealthStage = mHost->isFinalBattle() ? 2 : mHost->getHealthStage();
    mShellDive->setLevel(getAttackLevel(0));
    mWheel->setLevel(getAttackLevel(1));
    mSpike->setLevel(getAttackLevel(2));
    mFirebomb->setLevel(getAttackLevel(3));
    mLaser->setLevel(getAttackLevel(0));

    mCameraPoser =
        new CameraPoserDarkBowser("塔", pDarkBowser->getShellLandWarningPosPtr());
    mCameraTicket =
        alCameraFunction::initCamera(mCameraPoser, mHost, rInfo, "DarkBowserBattleB", 6);

    mGuideFrameOut =
        new GuideFrameOutSingleMode(al::getLayoutInitInfo(rInfo), pDarkBowser, "Bowser");
    mGuideFrameOut->appear();
    mGuideFrameOut->setDisable();
    mGuideFrameOut->onUnk142();
    mGuideFrameOut->onUnk143();
    mGuideFrameOut->onUnk145();
    mGuideFrameOut->setUnk148(4.0f);

    mFaceMtx = al::getJointMtxPtr(mHost, "Face");
    al::setHitSensorPosPtr(mHost, "LookAt", &mEyePos);
    al::setSensorRadius(mHost, "LookAt", 500.0f);
}

/**
 * @brief Calculates the level of an attack from the current phase and health.
 * @param offset How many levels the attack lags behind the shell dive (clamped to 0-4).
 * @return The attack level, never below 0.
 */
s32 DarkBowserBattle::getAttackLevel(s32 offset) const {
    s32 phaseIndex = sead::Mathi::max(sead::Mathi::min(mHost->getPhase() - 1, 3), 0);
    s32 level;

    switch (phaseIndex) {
    case 1:
        level = mHost->getHitPoint() <= 100 ? 2 : 1;
        break;
    case 2:
        level = mHost->getHitPoint() > 100 ? 3 : 4;
        break;
    case 3:
        level = 4;
        break;
    default:
        level = 0;
        break;
    }

    return sead::Mathi::max(level - sead::Mathi::clamp(offset, 0, 4), 0);
}

/** @brief Hands the HUD marker to the scene layout so it is drawn. */
void DarkBowserBattle::registerGuideFrameOut() {
    auto* layout =
        al::tryGetSceneObj<SingleModeSceneLayout>(mHost, SceneObjID_SingleModeSceneLayout);

    if (layout != nullptr) {
        layout->setBowserGuideFrameOut(mGuideFrameOut);
    }
}

/** @brief Starts the battle: resets the attack counters and starts the battle camera. */
void DarkBowserBattle::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvDarkBowserBattleBegin);
    mShellAttackCount = 0;
    mAttackCount = 0;
    mGuideFrameOut->setEnable();
    al::startCamera_RS(mHost, mCameraTicket, -1);
}

/** @brief Ends the battle, stopping the laser and the spikes and hiding the HUD marker. */
void DarkBowserBattle::kill() {
    if (!mLaser->isDead()) {
        mLaser->kill();
    }

    mSpike->forceKill();
    al::NerveStateBase::kill();
    mGuideFrameOut->kill();
}

/**
 * @brief Handles Fury Bowser's sensors touching something.
 * @param pSelf Fury Bowser's sensor.
 * @param pOther The other sensor.
 */
void DarkBowserBattle::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorNpc(pSelf) || mDamage->isDefeat()) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf)) {
        al::sendMsgPushVeryStrong(pOther, pSelf);

        if (!al::isNerve(this, &NrvDarkBowserBattleShellDive) && al::isSensorPlayer(pOther) &&
            !al::isSensorName(pSelf, "LeftFoot") && !al::isSensorName(pSelf, "RightFoot")) {
            if (al::isNerve(this, &NrvDarkBowserBattleDamage) &&
                mDamage->isBlockingOrStomping()) {
                if (al::isSensorName(pSelf, "Body") &&
                    al::getSensorPos(pOther).y - al::getSensorPos(pSelf).y >
                        al::getSensorRadius(pOther)) {
                    al::sendMsgGigaEnemyAttack(pOther, pSelf);
                }
            } else {
                al::sendMsgGigaEnemyAttack(pOther, pSelf);
            }
        }

        if (!al::isNerve(this, &NrvDarkBowserBattleShellDive) &&
            (al::isSensorNpc(pOther) || al::isSensorMapObj(pOther) ||
             al::isSensorKoopaJr(pOther))) {
            al::sendMsgGigaEnemyAttack(pOther, pSelf);
        }
    }

    if (al::isNerve(this, &NrvDarkBowserBattleShellDive)) {
        mShellDive->attackSensor(pSelf, pOther);
    } else if (al::isNerve(this, &NrvDarkBowserBattleLaser)) {
        mLaser->attackSensor(pSelf, pOther);
    } else if (al::isNerve(this, &NrvDarkBowserBattleSpike)) {
        mSpike->attackSensor(pSelf, pOther);
    }
}

/**
 * @brief Handles a message sent to Fury Bowser, switching to the damage state when hurt.
 * @param pMsg The message.
 * @param pSelf Fury Bowser's sensor.
 * @param pOther The sender's sensor.
 * @return True if the message was handled.
 */
bool DarkBowserBattle::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                  al::HitSensor* pOther) {
    if (al::isSensorNpc(pOther) || mDamage->isDefeat()) {
        return false;
    }

    if (al::isMsgPlayerDisregard(pMsg)) {
        return true;
    }

    if (mDamage->receiveMsg(pMsg, pSelf, pOther)) {
        if (al::isNerve(this, &NrvDarkBowserBattleDamage)) {
            return true;
        }

        if (al::isNerve(this, &NrvDarkBowserBattleLaser)) {
            mLaser->kill();
        }

        if (al::isNerve(this, &NrvDarkBowserBattleFirebomb)) {
            mFirebomb->kill();
        }

        al::setNerve(this, &NrvDarkBowserBattleDamage);
        return true;
    }

    return mDamage->receiveMsgResult();
}

/** @brief Per-frame update: crushes whatever Fury Bowser lands on and updates the tracking. */
void DarkBowserBattle::control() {
    mDamage->updateSmallHit();
    al::LiveActor* player = mHost->getPlayer();

    if (player != nullptr && rc::isPlayerDead(player)) {
        mCameraPoser->setZoomIn(2000.0f, 1500.0f, 50.0f, 240, false);
        mGuideFrameOut->setDisable();
    }

    if (al::isCollidedGround(mHost)) {
        al::HitSensor* sensor = al::tryGetCollidedGroundSensor(mHost);

        if (sensor != nullptr) {
            al::sendMsgGigaEnemyAttack(sensor, al::getHitSensor(mHost, "ShellDive"));
        }
    }

    if (al::isCollidedWall(mHost)) {
        al::HitSensor* sensor = al::tryGetCollidedWallSensor(mHost);

        if (sensor != nullptr) {
            al::sendMsgGigaEnemyAttack(sensor, al::getHitSensor(mHost, "ShellDive"));
        }
    }

    updateTrackedPosition();
    calcAndSetEyePosition();
    mLaser->updateLaserLight();
}

/** @brief Moves the tracked position towards Fury Bowser's face or above his body. */
void DarkBowserBattle::updateTrackedPosition() {
    f32 rate = al::isNerve(this, &NrvDarkBowserBattleShellDive) ? 0.025f : 0.05f;
    bool isTrackFace;

    if (al::isNerve(this, &NrvDarkBowserBattleShellDive)) {
        al::LiveActor* host = mHost;
        isTrackFace = al::isActionPlaying(host, "ShellRecover") ||
                      al::isActionPlaying(host, "ShellHitMiss");
    } else {
        isTrackFace = !al::isNerve(this, &NrvDarkBowserBattleSpike) &&
                      !al::isNerve(this, &NrvDarkBowserBattleWheel);
    }

    sead::Vector3f target;

    if (isTrackFace) {
        target = {(*mFaceMtx)(0, 3), (*mFaceMtx)(1, 3), (*mFaceMtx)(2, 3)};
    } else {
        target = al::getTrans(mHost) + sead::Vector3f::ey * 2000.0f;
    }

    al::lerpVec(&mTrackedPos, mTrackedPos, target, rate);
}

/** @brief Places the "LookAt" sensor between the player's head and the tracked position. */
void DarkBowserBattle::calcAndSetEyePosition() {
    al::LiveActor* player = mHost->getPlayer();

    if (player == nullptr) {
        return;
    }

    const sead::Matrix34f* headMtx = rc::getPlayerModelJointMtxPtr(player, "Head");
    sead::Vector3f headPos;
    headMtx->getTranslation(headPos);
    sead::Vector3f dir = mTrackedPos - headPos;

    if (!al::normalizeOrZero(&dir) && !al::isNearZero(1.0f - dir.y, 0.01f)) {
        mEyePos = headPos + dir * 2000.0f;
    } else {
        headPos += rc::getPlayerFront(player) * 1000.0f;
        mEyePos = headPos + sead::Vector3f::ey * 1500.0f;
    }
}

/** @brief First nerve: resets the health bar and camera and picks the first attack. */
void DarkBowserBattle::exeBegin() {
    mHost->setHealthBarState(false);
    mCameraPoser->requestAutoCamera();
    decideNextState();
}

/** @brief Updates the attack levels and picks the next attack for the current phase. */
void DarkBowserBattle::decideNextState() {
    if (mDamage->isDefeat()) {
        kill();
        return;
    }

    s32 phase = mHost->getPhase();

    if (!mHost->isFinalBattle()) {
        mHealthStage = mHost->getHealthStage();
        mShellDive->setLevel(getAttackLevel(0));
        mFirebomb->setLevel(getAttackLevel(3));
        mWheel->setLevel(getAttackLevel(1));
        mSpike->setLevel(getAttackLevel(2));
    }

    mAttackCount++;

    switch (phase) {
    case 1:
        decidePhase1();
        break;
    case 2:
        decidePhase2();
        break;
    case 3:
        if (mHealthStage <= 1) {
            decidePhase3();
        } else {
            decideFinal();
        }

        break;
    case 4:
        decidePhase3();
        break;
    default:
        decidePhase3();
        break;
    }
}

/** @brief Roars between attacks, unless the final sequence of the last phase starts. */
void DarkBowserBattle::exeWait() {
    if (al::isFirstStep(this)) {
        if (mHealthStage == 2 && mHost->getPhase() == 3) {
            decideFinal();
            return;
        }

        mHost->setHealthBarState(false);

        if (mHealthStage == 1 && mHost->getPhase() == 3) {
            decideNextState();
            mCameraPoser->requestAutoCamera();
            return;
        }

        al::startAction(mHost, "Roar");
    }

    if (al::isStep(this, 35)) {
        mHost->requestMediumBlur(nullptr);
    }

    mCameraPoser->requestAutoCamera();
    mCameraPoser->requestCameraIn();

    if (al::isActionEnd(mHost)) {
        decideNextState();
    }
}

/** @brief Starts the shell dive used to finish the final phase. */
void DarkBowserBattle::decideFinal() {
    mShellAttackCount++;
    al::setNerve(this, &NrvDarkBowserBattleShellDive);
}

/** @brief Runs the fire bomb attack, then waits. */
void DarkBowserBattle::exeFirebomb() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvDarkBowserBattleWait);
    }
}

/** @brief Runs the shell dive attack and drives the aerial camera and the HUD marker. */
void DarkBowserBattle::exeShellDive() {
    if (al::isFirstStep(this)) {
        mDamage->resetScratchCount();
    }

    if (al::isStep(this, 60)) {
        mCameraPoser->requestCameraOut();

        if (mShellDive->isAerial()) {
            mGuideFrameOut->setAerial(true);
        }
    }

    if (mShellDive->isRequestAerialCamera()) {
        mCameraPoser->requestAerialCamera();
    }

    if (mShellDive->isEnding()) {
        mCameraPoser->requestCameraIn();
        mCameraPoser->endAerialCamera();
        mGuideFrameOut->setAerial(false);
    }

    mGuideFrameOut->setAerial(mShellDive->isAerial());

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvDarkBowserBattleWait);
        mHost->resetHitSensors();
        return;
    }

    if (mDamage->checkScratchCountChanceTime()) {
        mShellDive->forceRecover();
    }
}

/** @brief Restores the normal camera when the shell dive ends (unless Fury Bowser fell). */
void DarkBowserBattle::endShellDive() {
    if (mDamage->checkDefeat()) {
        return;
    }

    mCameraPoser->requestCameraIn();
    mCameraPoser->endAerialCamera();
    mGuideFrameOut->setAerial(false);
}

/** @brief Runs the shell wheel attack, then waits. */
void DarkBowserBattle::exeWheel() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvDarkBowserBattleWait);
        mHost->resetHitSensors();
    }
}

/** @brief Runs the shell spike attack, then picks the next attack. */
void DarkBowserBattle::exeSpike() {
    if (al::updateNerveState(this)) {
        decideNextState();
    }
}

/** @brief Runs the laser attack, then picks the next attack. */
void DarkBowserBattle::exeLaser() {
    if (al::updateNerveState(this)) {
        decideNextState();
    }
}

/** @brief Runs the damage state and decides what to do once it ends. */
void DarkBowserBattle::exeDamage() {
    if (al::isFirstStep(this) && mDamage->isDefeat()) {
        kill();
        return;
    }

    if (al::isStep(this, 3)) {
        mCameraPoser->requestAutoCamera();
        mCameraPoser->requestCameraIn();
    }

    if (!al::updateNerveState(this)) {
        return;
    }

    if ((mDamage->isSideKicked() || mDamage->isBombHit()) && !mDamage->isDefeat()) {
        mIsKnockedBack = true;
        al::setNerve(this, &NrvDarkBowserBattleShellDive);
        mShellDive->requestKnockBack(mDamage->isBombHit() ? 240 : 0);
        return;
    }

    if (!mDamage->isDefeat()) {
        if (mHealthStage != mHost->getHealthStage() || mDamage->isStompEnd()) {
            decideNextState();
        } else {
            al::setNerve(this, &NrvDarkBowserBattleWait);
        }

        return;
    }

    if (mDamage->isSideKicked()) {
        sead::Vector3f dir = al::isActionPlaying(mHost, "ShellWheelRight") ||
                                     al::isActionPlaying(mHost, "ShellWheelChargeRight") ?
                                 sead::Vector3f::ex :
                                 -sead::Vector3f::ex;
        dir.rotate(al::getQuat(mHost));

        sead::Vector3f cameraPos =
            al::getTrans(mHost) + dir * 12000.0f + sead::Vector3f::ey * 9000.0f;
        sead::Vector3f lookAt = al::getTrans(mHost) + sead::Vector3f::ey * 1000.0f;
        mCameraPoser->toggleFocus(lookAt, cameraPos);
    }

    kill();
}

/** @brief Ends the battle camera if it is still active. */
void DarkBowserBattle::endCamera() {
    if (mCameraTicket->isActiveCamera()) {
        al::endCamera_RS(mHost, mCameraTicket, -1, false);
    }
}

/** @brief Phase 1 alternates between the shell dive and the laser. */
void DarkBowserBattle::decidePhase1() {
    if (mAttackCount % 2 != 0) {
        al::setNerve(this, &NrvDarkBowserBattleLaser);
    } else {
        al::setNerve(this, &NrvDarkBowserBattleShellDive);
        mShellAttackCount++;
    }

    mShellDive->setLevel(getAttackLevel(0));
}

/** @brief Phase 2 cycles through the shell dive, wheel, spike and laser attacks. */
void DarkBowserBattle::decidePhase2() {
    switch (mHealthStage) {
    case 0:
        switch (mAttackCount % 3) {
        case 0:
            if (mIsKnockedBack) {
                al::setNerve(this, &NrvDarkBowserBattleWheel);
                mAttackCount++;
            } else {
                al::setNerve(this, &NrvDarkBowserBattleShellDive);
            }

            break;
        case 1:
            al::setNerve(this, &NrvDarkBowserBattleWheel);
            return;
        case 2:
            al::setNerve(this, &NrvDarkBowserBattleLaser);
            return;
        default:
            al::setNerve(this, &NrvDarkBowserBattleWait);
            return;
        }

        break;
    case 1:
        switch (mAttackCount % 4) {
        case 0:
            if (mIsKnockedBack) {
                al::setNerve(this, &NrvDarkBowserBattleWheel);
            } else {
                al::setNerve(this, &NrvDarkBowserBattleShellDive);
            }

            break;
        case 1:
            al::setNerve(this, &NrvDarkBowserBattleSpike);
            return;
        case 2:
            al::setNerve(this, &NrvDarkBowserBattleWheel);
            return;
        case 3:
            al::setNerve(this, &NrvDarkBowserBattleLaser);
            return;
        default:
            al::setNerve(this, &NrvDarkBowserBattleWait);
            return;
        }

        break;
    default:
        return;
    }

    s32 count = mShellAttackCount + 1;
    mIsKnockedBack = false;
    mShellAttackCount = count;
}

/** @brief Phase 3 mixes all attacks, randomly once the opening sequence is over. */
void DarkBowserBattle::decidePhase3() {
    switch (mHealthStage) {
    case 0:
        switch ((mAttackCount - 1) % 3) {
        case 0:
            al::setNerve(this, &NrvDarkBowserBattleSpike);
            return;
        case 1:
            al::setNerve(this, &NrvDarkBowserBattleFirebomb);
            return;
        case 2:
            al::setNerve(this, &NrvDarkBowserBattleWheel);
            mShellAttackCount++;
            return;
        default:
            al::setNerve(this, &NrvDarkBowserBattleWait);
            return;
        }
    case 1: {
        if (mHost->getHitPoint() < 35) {
            mShellDive->enableExtraPattern();
        }

        if (mAttackCount <= 3) {
            switch (mAttackCount % 4) {
            case 1:
                al::setNerve(this, &NrvDarkBowserBattleSpike);
                return;
            case 2:
                al::setNerve(this, &NrvDarkBowserBattleWheel);
                return;
            case 3:
                al::setNerve(this, &NrvDarkBowserBattleShellDive);
                mShellAttackCount++;
                return;
            default:
                al::setNerve(this, &NrvDarkBowserBattleWait);
                return;
            }
        }

        s32 random = al::getRandom(0, 255) % 3 + mRandomAttack;
        s32 step = (mAttackCount - 4) % 6;
        mRandomAttack = (random + 1) % 4;
        const al::Nerve* nerve;

        switch (mRandomAttack) {
        case 1:
            nerve = &NrvDarkBowserBattleWheel;
            break;
        case 2:
            nerve = &NrvDarkBowserBattleLaser;
            break;
        case 3:
            nerve = &NrvDarkBowserBattleSpike;
            break;
        default:
            nerve = &NrvDarkBowserBattleFirebomb;
            break;
        }

        switch (step) {
        case 0:
            al::setNerve(this, &NrvDarkBowserBattleSpike);
            mRandomAttack = 3;
            return;
        case 1:
        case 2:
        case 3:
        case 4:
            al::setNerve(this, nerve);
            return;
        case 5:
            al::setNerve(this, &NrvDarkBowserBattleShellDive);
            mShellAttackCount++;
            return;
        default:
            al::setNerve(this, &NrvDarkBowserBattleWait);
            return;
        }
    }
    default:
        decideFinal();
        return;
    }
}

/** @brief Destroys the battle state. */
DarkBowserBattle::~DarkBowserBattle() = default;
