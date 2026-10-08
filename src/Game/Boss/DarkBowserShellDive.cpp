#include "Boss/DarkBowserShellDive.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>

#include "Boss/DarkBowser.hpp"
#include "Boss/DarkBowserRingBeam.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "MapObj/SePlayObj.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
NERVE_DECL(DarkBowserShellDive, Jump)
NERVE_DECL(DarkBowserShellDive, Stuck)
NERVE_DECL(DarkBowserShellDive, Dive)
NERVE_DECL(DarkBowserShellDive, Hop)
NERVE_DECL(DarkBowserShellDive, Track)
NERVE_DECL(DarkBowserShellDive, End)

NERVES_MAKE_NOSTRUCT(DarkBowserShellDive, Jump, Stuck, Dive, Hop, Track, End)

/** @brief Tuning of the shell dive for one attack level. */
struct ShellDiveLevelParam {
    s32 trackFrame;    // 0x00
    f32 _4;            // 0x04
    f32 diveGravity;   // 0x08
    s32 diveNum;       // 0x0C
    f32 hopSpeed;      // 0x10
    f32 hopMaxDist;    // 0x14
    f32 hopAngle;      // 0x18
    s32 ringBeamStep;  // 0x1C
    s32 hopStep;       // 0x20
    s32 hopFrame;      // 0x24
    f32 diveSpeed;     // 0x28
    s32 stuckFrame;    // 0x2C
    s32 _30;           // 0x30
};
static_assert(sizeof(ShellDiveLevelParam) == 0x34);

constexpr s32 cLevelNum = 5;

const ShellDiveLevelParam cLevelParams[cLevelNum] = {
    {180, 4000.0f, 2.0f, 1, 800.0f, 800.0f, 60.0f, 0, 80, 60, 800.0f, 720, 45},
    {180, 4000.0f, 2.0f, 1, 800.0f, 800.0f, 60.0f, 0, 80, 60, 800.0f, 600, 45},
    {180, 4000.0f, 2.0f, 1, 800.0f, 800.0f, 60.0f, 0, 80, 60, 800.0f, 600, 45},
    {60, 10000.0f, 2.0f, 3, 800.0f, 800.0f, 60.0f, 0, 40, 40, 1000.0f, 540, 45},
    {60, 20000.0f, 2.5f, 3, 900.0f, 800.0f, 60.0f, 0, 20, 20, 1000.0f, 480, 45},
};

/** @brief Height Fury Bowser hovers at while tracking the player. */
constexpr f32 cTrackHeight = 75000.0f;
/** @brief Frames before the dive at which the landing warning appears. */
constexpr s32 cWarningFrame = 30;

/**
 * @brief Horizontal length of a vector.
 * @param rVec The vector.
 * @return Length of the vector in the XZ plane.
 */
inline f32 calcLengthH(const sead::Vector3f& rVec) {
    return sead::Mathf::sqrt(rVec.x * rVec.x + rVec.z * rVec.z);
}
}  // namespace

/**
 * @brief Constructs the shell dive state, reading the dive targets of the final battle.
 * @param pHost Fury Bowser.
 * @param rInfo Actor init info.
 */
DarkBowserShellDive::DarkBowserShellDive(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::NerveStateBase("ShellDive"), mHost(pHost) {
    initNerve(&NrvDarkBowserShellDiveJump, 0);

    if (pHost->isFinalBattle()) {
        mTargetNum = al::calcLinkChildNum(rInfo, "ShellDiveTarget");

        for (s32 i = 0; i < mTargetNum; i++) {
            al::getChildLinkT(&mTargets[i], rInfo, "ShellDiveTarget", i);
            mTargetOrder[i] = i;
        }

        for (s32 i = 0; i < mTargetNum; i++) {
            s32 index = al::getRandom(0, mTargetNum - 1);
            s32 tmp = mTargetOrder[index];
            mTargetOrder[index] = mTargetOrder[i];
            mTargetOrder[i] = tmp;
        }
    }

    mSeObj = new SePlayObj("DarkBowserShellDiveSoundActor");
    mSeObj->initWithAudioKeeper(rInfo, "DarkBowser");
    al::updatePoseMtx(mSeObj, pHost->getBaseMtx());
    al::invalidateClipping(mSeObj);
}

/** @brief Starts the dive, from the jump or (after a knock back) directly stuck in the ground. */
void DarkBowserShellDive::appear() {
    al::NerveStateBase::appear();

    if (mIsKnockBack) {
        al::setNerve(this, &NrvDarkBowserShellDiveStuck);
        mDiveCount = cLevelParams[mLevel].diveNum;
        al::invalidateHitSensors(mHost);
        al::validateHitSensor(mHost, "LookAt");
    } else {
        al::setNerve(this, &NrvDarkBowserShellDiveJump);
        mDiveCount = 0;
    }

    if (!mIsKnockBack && (mPatternFlags & 1)) {
        mDiveNum = 5;
    } else {
        mDiveNum = cLevelParams[mLevel].diveNum;
    }

    mStuckActionStep = -1;
    _38 = -1;
    al::setShadowMaskSize(mHost, "DropShell", 9000.0f, 0.0f, 0.0f);
    al::setShadowMaskSize(mHost, "Body", 9000.0f, 0.0f, 0.0f);
}

/** @brief Ends the dive and restores Fury Bowser's collision and gravity. */
void DarkBowserShellDive::kill() {
    if (al::isNerve(this, &NrvDarkBowserShellDiveStuck)) {
        mIsShowGuide = false;
    }

    al::onCollide(mHost);
    al::NerveStateBase::kill();
    mIsKnockBack = false;
    rc::disappearGuideGameWindow(mHost);
    mHost->validateGravity();
    mDiveTotal++;
    al::tryDeleteEffect(mHost, "ShellLandWarning");
    mKnockBackFrame = 0;
}

/**
 * @brief Attacks whatever Fury Bowser's shell touches while diving or hopping.
 * @param pSelf Fury Bowser's sensor.
 * @param pOther The touched sensor.
 */
void DarkBowserShellDive::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvDarkBowserShellDiveDive)) {
        al::sendMsgGigaEnemyAttack(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvDarkBowserShellDiveHop)) {
        al::sendMsgPushStrong(pOther, pSelf);
    }
}

/** @brief Retracts into the shell and jumps up high. */
void DarkBowserShellDive::exeJump() {
    if (al::isFirstStep(this)) {
        if (mDiveCount == 0) {
            mHost->setHealthBarState(true);
            al::startAction(mHost, "ShellStart");
        }

        al::validateShadow(mHost);
        al::invalidateShadow(mHost, "DropShell");
    }

    if (mDiveCount == 0 && al::isStep(this, 119)) {
        al::startAction(mHost, "ShellJump");
    }

    if (al::isStep(this, 42)) {
        al::invalidateHitSensors(mHost);
        al::validateHitSensor(mHost, "ShellDive");
    }

    if (al::isStep(this, 120)) {
        al::setVelocity(mHost, sead::Vector3f::ey * 1600.0f);
        mHost->requestMediumBlur(al::getTransPtr(mHost));
        al::offCollide(mHost);
        al::validateHitSensor(mHost, "LookAt");
    }

    if (al::isGreaterEqualStep(this, 150)) {
        al::setNerve(this, &NrvDarkBowserShellDiveTrack);
        mHost->invalidateGravity();
    }
}

/** @brief Hovers high above the player (or a dive target) until it is time to dive. */
void DarkBowserShellDive::exeTrack() {
    if (al::isFirstStep(this)) {
        al::invalidateShadow(mHost);
        al::validateShadow(mHost, "DropShell");
        al::setVelocity(mHost, sead::Vector3f::zero);
        mHost->setHealthBarState(true);
        mStartPos = al::getTrans(mHost);
        mStartPos.y = cTrackHeight;

        if (mIsUseFixedTarget) {
            mTargetPos = al::getTrans(mHost->getPlayer());
            mTargetPos.y = 0.0f;

            f32 radius = 35000.0f;
            f32 dist = calcLengthH(mTargetPos);
            if (dist + radius > 200000.0f) {
                radius = sead::Mathf::clamp(200000.0f - dist, 0.0f, 200000.0f);
            }

            f32 angle = al::getRandom(0.0f, 360.0f);
            sead::Vector3f dir = sead::Vector3f::ez;
            al::rotateVectorDegreeY(&dir, angle);
            f32 rate = al::getRandom(0.0f, 0.5f) + 0.5f;
            mTargetPos += dir * (radius * rate);
            mTargetPos.y = cTrackHeight;
        }
    }

    const al::LiveActor* player = mHost->getPlayer();
    s32 frame;
    if ((mPatternFlags & 1) && mDiveNum == 3) {
        frame = cLevelParams[mLevel].trackFrame / 2;
    } else {
        frame = cLevelParams[mLevel].trackFrame;
    }

    if (!mIsUseFixedTarget) {
        if (player != nullptr) {
            al::isLessEqualStep(this, frame - cWarningFrame);
            al::setTrans(mHost, calcLinearPos(player, frame));
        }
    } else if (mTargetNum != 0) {
        sead::Vector3f pos = mTargets[mTargetOrder[mTargetNum - 1]];
        pos.y = cTrackHeight;
        f32 rate = al::getNerveStep(this) / 180.0f;
        pos = pos * rate + (1.0f - rate) * al::getTrans(mHost);
        al::setTrans(mHost, pos);
    } else {
        al::lerpVec(al::getTransPtr(mHost), al::getTrans(mHost), mTargetPos, 0.1f);
    }

    if (al::isStep(this, frame - cWarningFrame)) {
        al::emitEffect(mHost, "ShellLandWarning", nullptr);
        mIsRequestAerialCamera = true;
    } else {
        mIsRequestAerialCamera = false;
    }

    if (al::isGreaterEqualStep(this, frame - cWarningFrame)) {
        al::setShadowDropLength(mHost, 120000.0f);
        updateShadowEffects();
    }

    if (al::isGreaterEqualStep(this, frame)) {
        al::setNerve(this, &NrvDarkBowserShellDiveDive);
    }
}

/**
 * @brief Calculates the hover position above a target, easing out from the start position.
 * @param pTarget The actor to hover above.
 * @param frame Frames the tracking lasts.
 * @return The hover position.
 */
sead::Vector3f DarkBowserShellDive::calcLinearPos(const al::LiveActor* pTarget, f32 frame) {
    sead::Vector3f targetTrans = al::getTrans(pTarget);
    f32 rate = al::calcNerveEaseOutRate(this, frame - cWarningFrame);
    sead::Vector3f target = targetTrans + sead::Vector3f::ey * cTrackHeight;
    sead::Vector3f pos;
    al::lerpVec(&pos, mStartPos, target, rate);
    return pos;
}

/** @brief Moves the warning sound to the landing warning effect and keeps it playing. */
void DarkBowserShellDive::updateShadowEffects() {
    const sead::Vector3f* warningPos = al::getEffectPosPtr(mHost, "ShellLandWarning");
    if (warningPos == nullptr) {
        return;
    }

    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, sead::Vector3f::ex, sead::Vector3f::ey, *warningPos);
    al::updatePoseMtx(mSeObj, &mtx);
    al::holdSe(mSeObj, "PgShellDiveSignLv");
}

/** @brief Dives straight down until the ground is hit. */
void DarkBowserShellDive::exeDive() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "ShellDive");
        al::setVelocity(mHost, sead::Vector3f::zero);
        al::onCollide(mHost);
        mDiveCount++;
        al::setVelocity(mHost, -sead::Vector3f::ey * cLevelParams[mLevel].diveSpeed);
        al::invalidateShadow(mHost);
        al::validateShadow(mHost, "DropShell");

        if (mIsUseFixedTarget && mTargetNum != 0) {
            mTargetNum--;
        }
    }

    al::addVelocity(mHost, -sead::Vector3f::ey * cLevelParams[mLevel].diveGravity);
    updateShadowEffects();

    if (al::isCollidedGround(mHost) || al::getTrans(mHost).y < -500.0f) {
        al::deleteEffect(mHost, "ShellLandWarning");
        al::setNerve(this, &NrvDarkBowserShellDiveStuck);
    }
}

/** @brief Hops back up out of the ground for another dive. */
void DarkBowserShellDive::exeHop() {
    if (al::isStep(this, cLevelParams[mLevel].ringBeamStep)) {
        startRingBeam();
    }

    if (al::isLessStep(this, cLevelParams[mLevel].hopStep) && mIsWaitWater &&
        rc::isInWaterArea(mHost)) {
        mIsWaitWater = false;
    }

    if (al::isStep(this, cLevelParams[mLevel].hopStep)) {
        al::setVelocity(mHost, cLevelParams[mLevel].hopSpeed * sead::Vector3f::ey);
        al::offCollide(mHost);
    }

    if (al::isGreaterEqualStep(this,
                               cLevelParams[mLevel].hopStep + cLevelParams[mLevel].hopFrame)) {
        al::setNerve(this, &NrvDarkBowserShellDiveTrack);
        mHost->invalidateGravity();
        const sead::Vector3f& trans = al::getTrans(mHost);
        sead::Vector3f velocity = calcHopAwayVelocity();
        mTargetPos = velocity * 15.0f + trans;
        mTargetPos.y = cTrackHeight;
    }
}

/** @brief Fires a ring beam from where Fury Bowser landed. */
void DarkBowserShellDive::startRingBeam() {
    sead::Vector3f pos = al::getTrans(mHost);
    pos.y += 1200.0f;
    mRingBeam = mHost->getDeadRingBeam();
    mRingBeam->setActiveWithPosture(pos, sead::Quatf(1.0f, 0.0f, 0.0f, 0.0f));
}

/**
 * @brief Calculates the velocity of a hop that moves away from the player at an angle.
 * @return The hop velocity.
 */
sead::Vector3f DarkBowserShellDive::calcHopAwayVelocity() const {
    sead::Vector3f dir = al::getTrans(mHost->getPlayer()) - al::getTrans(mHost);
    dir.y = 0.0f;
    al::normalizeOrDirZ(&dir);
    al::rotateVectorDegreeY(&dir, cLevelParams[mLevel].hopAngle);
    dir.y = 1.0f;
    return cLevelParams[mLevel].hopSpeed * dir;
}

/** @brief Stuck in the ground after a dive, vulnerable to hip drops. */
void DarkBowserShellDive::exeStuck() {
    if (al::isFirstStep(this)) {
        if (mIsKnockBack) {
            al::startAction(mHost, "ShellLandWeak");
        } else {
            al::startAction(mHost, "ShellLand");
        }

        mIsWaitWater = !rc::isInWaterArea(mHost);
        al::invalidateHitSensor(mHost, "ShellDive");
        al::validateHitSensor(mHost, "SoftBelly");
        mHost->requestMediumBlur(nullptr);

        if (mDiveCount < mDiveNum) {
            al::setNerve(this, &NrvDarkBowserShellDiveHop);
            return;
        }

        al::validateShadow(mHost);
        al::invalidateShadow(mHost, "DropShell");

        if (!mHost->isFinalBattle()) {
            mHost->showHealthBar();
            mHost->setHealthBarState(true);
        }

        mHost->validateGravity();
    } else {
        tryAppearGuideMessage();
    }

    if (mIsWaitWater && rc::isInWaterArea(mHost)) {
        mIsWaitWater = false;
    }

    if (al::isStep(this, cLevelParams[mLevel].ringBeamStep) && !mIsKnockBack) {
        startRingBeam();
    }

    if (!al::isActionPlaying(mHost, "ShellStuck") && al::isActionEnd(mHost)) {
        al::startAction(mHost, "ShellStuck");
        mStuckActionStep = al::getNerveStep(this);
    }

    if (mStuckActionStep >= 1 && al::isActionPlaying(mHost, "ShellStuck")) {
        al::setActionFrameRate(mHost, calcAnimRate());
    }

    f32 stuckFrame = cLevelParams[mLevel].stuckFrame;
    if (al::isStep(this, stuckFrame - al::getActionFrameMax(mHost, "ShellSign"))) {
        al::startAction(mHost, "ShellSign");
    }

    if (al::isGreaterEqualStep(this, cLevelParams[mLevel].stuckFrame)) {
        mIsKnockBack = false;
        rc::disappearGuideGameWindow(mHost);
        al::setNerve(this, &NrvDarkBowserShellDiveEnd);
    }
}

/** @brief Shows the hip drop guide in the first battle when the player is close enough. */
void DarkBowserShellDive::tryAppearGuideMessage() {
    if (mHost->getPhase() != 1 || !mIsShowGuide) {
        return;
    }

    if (mRingBeam == nullptr || mRingBeam->isActive()) {
        return;
    }

    if (rc::isGuideGameWindowActive(mHost) || mHost->getPlayer() == nullptr) {
        return;
    }

    sead::Vector3f diff = al::getTrans(mHost) - al::getTrans(mHost->getPlayer());
    if (calcLengthH(diff) < 18000.0f) {
        if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
            rc::appearGuideGameWindow(mHost, "SingleMode_GuideMessage",
                                      "HipDropGuide_SingleJoycons", -1, 0.0f);
        } else {
            rc::appearGuideGameWindow(mHost, "SingleMode_GuideMessage",
                                      "HipDropGuide_DualJoycons", -1, 0.0f);
        }
    }
}

/**
 * @brief Calculates the speed of the stuck animation, which speeds up the longer it plays.
 * @return The animation frame rate.
 */
f32 DarkBowserShellDive::calcAnimRate() const {
    f32 rate = static_cast<f32>(al::getNerveStep(this) - mStuckActionStep) /
               cLevelParams[mLevel].stuckFrame;
    rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    return rate * 2.0f + 1.0f;
}

/** @brief Pulls out of the ground and turns back to the player. */
void DarkBowserShellDive::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "ShellHitMiss");
        mHost->setHealthBarState(false);
    }

    if (al::isGreaterEqualStep(this, 22) && al::isLessEqualStep(this, 66)) {
        f32 angle = al::calcAngleToTargetH(mHost, al::getTrans(mHost->getPlayer()));
        al::turnToTarget(mHost, mHost->getPlayer(), angle * 0.05f);
    }

    if (al::isStep(this, 10)) {
        mHost->resetHitSensors();
    }

    if (al::isActionEnd(mHost)) {
        kill();
    }
}

/**
 * @brief Sets the attack level, which selects the dive tuning.
 * @param level The attack level (clamped to the available levels).
 */
void DarkBowserShellDive::setLevel(s32 level) {
    level = level < cLevelNum - 1 ? level : cLevelNum - 1;
    mLevel = level < 0 ? 0 : level;
}

/**
 * @brief Whether the dive is ending (pulling out of the ground).
 * @return True in the end nerve.
 */
bool DarkBowserShellDive::isEnding() const {
    return al::isNerve(this, &NrvDarkBowserShellDiveEnd);
}

/**
 * @brief Whether Fury Bowser is in the air.
 * @return True while tracking, diving or hopping.
 */
bool DarkBowserShellDive::isAerial() const {
    return al::isNerve(this, &NrvDarkBowserShellDiveTrack) ||
           al::isNerve(this, &NrvDarkBowserShellDiveDive) ||
           al::isNerve(this, &NrvDarkBowserShellDiveHop);
}

/** @brief Pulls Fury Bowser out of the ground early if he is stuck. */
void DarkBowserShellDive::forceRecover() {
    if (al::isNerve(this, &NrvDarkBowserShellDiveStuck)) {
        al::setNerve(this, &NrvDarkBowserShellDiveEnd);
    }
}

/** @brief Hops towards the player, at most the level's maximum hop distance. */
void DarkBowserShellDive::hopToPlayer() const {
    sead::Vector3f dir = al::getTrans(mHost->getPlayer()) - al::getTrans(mHost);
    dir.y = 0.0f;
    f32 maxDist = cLevelParams[mLevel].hopMaxDist;
    f32 dist = calcLengthH(dir);
    f32 rate = static_cast<f32>(al::getNerveStep(this) - cLevelParams[mLevel].hopStep) / 15.0f;
    rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    f32 speed = calcLengthH(dir) < maxDist ? dist : maxDist;
    al::normalizeOrZero(&dir);
    dir *= rate * speed;
    dir.y = al::getVelocity(mHost).y;
    al::setVelocity(mHost, dir);
    al::addVelocityToGravity(mHost, 4.0f);
    al::scaleVelocity(mHost, 0.99f);
}

DarkBowserShellDive::~DarkBowserShellDive() = default;
