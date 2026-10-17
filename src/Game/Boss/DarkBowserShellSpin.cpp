#include "Boss/DarkBowserShellSpin.hpp"
#include "Boss/DarkBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(DarkBowserShellSpin, Begin);
NERVE_DECL(DarkBowserShellSpin, Spin);
NERVE_DECL(DarkBowserShellSpin, Reverse);
NERVES_MAKE_NOSTRUCT(DarkBowserShellSpin, Begin, Spin, Reverse)
struct SpinParam { float speed; float acceleration; float animRate; int duration; int reverseCount; };
const SpinParam sSpinParams[] = {{70.0f,1200.0f,1.5f,180,3},{70.0f,1200.0f,1.5f,180,3},{90.0f,1200.0f,2.0f,180,3},{90.0f,1200.0f,2.0f,180,3}};
}
/** @brief Creates the shell-spin steering state. @param pHost Fury Bowser. @param rInfo Initialization data. */
DarkBowserShellSpin::DarkBowserShellSpin(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<DarkBowser>("ShellSpin", pHost) { initNerve(&NrvDarkBowserShellSpinBegin, 0); }
/** @brief Destroys the shell-spin state. */
DarkBowserShellSpin::~DarkBowserShellSpin() = default;
/** @brief Resets tracking and enters the spin. */
void DarkBowserShellSpin::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvDarkBowserShellSpinBegin);
    mReverseCount = 0;
    mTurnBoost = 0;
    mIsTracking = true;
}
/** @brief Disables the attack sensor and stops the shell sound. */
void DarkBowserShellSpin::kill() {
    al::invalidateHitSensor(getHost(), "ShellSpin");
    al::tryStopSe(getHost(), "PgShellWheelJumpLv");
    al::NerveStateBase::kill();
}
/** @brief Leaves movement control to the active state. */
void DarkBowserShellSpin::control() {}
/** @brief Selects the spin state and animation rate. */
void DarkBowserShellSpin::exeBegin() {
    al::setNerve(this, &NrvDarkBowserShellSpinSpin);
    al::setSklAnimFrameRate(getHost(), sSpinParams[mLevel].animRate, 0);
}
/** @brief Steers toward the projected player position. */
void DarkBowserShellSpin::exeSpin() {
    if (al::isFirstStep(this)) mIsTracking = true;
    const auto position = al::getTrans(getHost());
    if (mIsTracking) calcTargetPos();
    sead::Vector3f delta(mTargetPos.x - position.x, 0.0f, mTargetPos.z - position.z);
    float distance = sead::Mathf::sqrt(delta.x * delta.x + delta.z * delta.z);
    if (al::normalizeOrZero(&delta) || distance < 5000.0f) {
        al::setNerve(this, &NrvDarkBowserShellSpinReverse);
        return;
    }
    if (al::isFirstStep(this)) {
        al::calcFrontDir(&mMoveDir, getHost());
        mTurnDir = delta;
    } else {
        mTurnDir += delta;
        al::normalizeOrZero(&mTurnDir);
        mMoveDir += mTurnDir * 0.05f * (mTurnBoost + 1.0f);
        al::normalizeOrZero(&mMoveDir);
    }
    spinFront();
    if (al::isGreaterEqualStep(this, sSpinParams[mLevel].duration)) al::setNerve(this, &NrvDarkBowserShellSpinReverse);
}
/** @brief Projects a target beyond the player and bounds it to the arena. */
void DarkBowserShellSpin::calcTargetPos() {
    const auto playerPos = al::getTrans(getHost()->getPlayer());
    const auto& hostPos = al::getTrans(getHost());
    sead::Vector3f delta(playerPos.x - hostPos.x, 0.0f, playerPos.z - hostPos.z);
    if (sead::Mathf::sqrt(delta.x * delta.x + delta.z * delta.z) < 20000.0f) mIsTracking = false;
    al::normalizeOrDirZ(&delta);
    delta *= 30000.0f;
    mTargetPos = delta + playerPos;
    if (mTargetPos.x * mTargetPos.x + mTargetPos.z * mTargetPos.z > 90000.0f * 90000.0f) {
        al::normalize(&mTargetPos);
        mTargetPos *= 90000.0f;
    }
}
/** @brief Turns the shell toward the steering direction. */
void DarkBowserShellSpin::spinFront() {
    sead::Vector3f dir = mTurnDir;
    sead::Vector3f front;
    al::calcFrontDir(&front, getHost());
    if (al::isNearZero(dir, 0.001f)) {
        dir = mMoveDir;
        if (al::normalizeOrZero(&dir)) return;
    }
    al::turnToDirection(getHost(), dir, (2.0f - front.dot(dir)) * 1.5f);
}
/** @brief Curves back toward the player and finishes when grounded nearby. */
void DarkBowserShellSpin::exeReverse() {
    if (al::isFirstStep(this)) {
        const auto& playerPos = al::getTrans(getHost()->getPlayer());
        const auto& hostPos = al::getTrans(getHost());
        sead::Vector3f delta(playerPos.x - hostPos.x, 0.0f, playerPos.z - hostPos.z);
        float sign = delta.x * mMoveDir.z - delta.z * mMoveDir.x;
        sead::Vector3f side = delta.cross(sead::Vector3f::ey);
        al::normalizeOrDirZ(&side);
        mTargetPos = side * 30000.0f;
        if (sign < 0.0f) mTargetPos = -mTargetPos;
        ++mReverseCount;
    }
    const auto& playerPos = al::getTrans(getHost()->getPlayer());
    sead::Vector3f delta = playerPos - al::getTrans(getHost());
    if (sead::Mathf::sqrt(delta.x * delta.x + delta.z * delta.z) < 30000.0f &&
        al::isGreaterEqualStep(this, 60) && al::isOnGround(getHost(), 0, 0.0f)) { kill(); return; }
    delta += mTargetPos;
    delta.y = 0.0f;
    al::normalizeOrZero(&delta);
    mTurnDir = delta;
    mMoveDir += mTurnDir * 0.05f * (mTurnBoost + 1.0f);
    al::normalizeOrZero(&mMoveDir);
    mTargetPos *= 0.99f;
    spinFront();
}
/** @brief Returns the horizontal speed limit. @return Maximum horizontal speed. */
float DarkBowserShellSpin::getMaxSpeedH() const { return 600.0f; }
/** @brief Returns the vertical speed limit. @return Maximum vertical speed. */
float DarkBowserShellSpin::getMaxSpeedV() const { return 900.0f; }
