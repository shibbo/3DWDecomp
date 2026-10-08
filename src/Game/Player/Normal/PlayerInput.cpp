#include "Player/Normal/PlayerInput.hpp"

#include <math/seadMathCalcCommon.h>

#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/Normal/PlayerCollisionIterator.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
    /// Frames a jump trigger is remembered for.
    constexpr u32 cPrecedingJumpFrame = 10;
    /// Frames a swim paddle or rolling trigger is remembered for.
    constexpr u32 cPrecedingTrigFrame = 5;
    /// Frames the along-wall snapping is kept after leaving the wall.
    constexpr s32 cAlongWallKeepFrame = 10;

    /**
     * Whether every component of a vector lies within a tolerance of zero.
     * @param rVec the vector to check
     * @param tolerance the largest distance from zero that still counts as zero
     * @return true if the vector is near zero
     */
    inline bool isNearZeroEach(const sead::Vector3f& rVec, f32 tolerance) {
        return (rVec.x <= tolerance && rVec.x >= -tolerance) &&
               (rVec.y <= tolerance && rVec.y >= -tolerance) &&
               (rVec.z <= tolerance && rVec.z >= -tolerance);
    }

    /**
     * Rotates a stick to an angle from the +X axis, keeping its length and vertical side.
     * @param pStick the stick to rotate
     * @param angle the new angle from the +X axis in radians, between 0 and pi
     */
    inline void rotateStickToAngle(sead::Vector2f* pStick, f32 angle) {
        f32 length = pStick->length();
        pStick->x = length * sead::Mathf::cos(angle);
        f32 y = pStick->y;
        f32 sin = sead::Mathf::sin(angle);
        if (y >= 0.0f) {
            pStick->y = length * sin;
        } else {
            pStick->y = -(length * sin);
        }
    }

    /**
     * Calculates the angle of a stick from the +X axis.
     * @param pAngle where the angle in radians (between 0 and pi) is written
     * @param rStick the stick
     * @return false if the stick is zero
     */
    inline bool calcStickAngle(f32* pAngle, const sead::Vector2f& rStick) {
        sead::Vector2f dir = rStick;
        if (al::normalizeOrZero(&dir)) {
            return false;
        }

        f32 cos = dir.x * 1.0f + dir.y * 0.0f;
        *pAngle = acosf(sead::Mathf::clamp(cos, -1.0f, 1.0f));
        return true;
    }
}  // namespace

/**
 * Creates the input with nothing pressed.
 * @param pKeyConfig the button mapping to read the pad with
 */
PlayerInput::PlayerInput(const IUsePlayerKeyConfig* pKeyConfig) : mKeyConfig(pKeyConfig) {}

/**
 * Sets the actor used to look up areas affecting the input.
 * @param pActor the player actor, or nullptr
 */
void PlayerInput::setActorForAreaSearch(al::LiveActor* pActor) {
    mActorForAreaSearch = pActor;
    if (pActor != nullptr) {
        mIsSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(pActor));
    }
}

/**
 * Reads the pad and updates every button state and the move direction.
 * @param rCameraMtx the camera matrix the stick is relative to
 */
void PlayerInput::update(const sead::Matrix34f& rCameraMtx) {
    mIsControlOff = false;

    sead::Vector3f cameraFront(-rCameraMtx(0, 2), 0.0f, -rCameraMtx(2, 2));
    if (al::normalizeOrZero(&cameraFront)) {
        cameraFront.x = rCameraMtx(0, 1);
        cameraFront.y = 0.0f;
        cameraFront.z = rCameraMtx(2, 1);
        al::normalizeOrZero(&cameraFront);
    }

    if (mAlongWallKeepFrame != 0) {
        mAlongWallKeepFrame--;
    }

    if (mCollision != nullptr) {
        for (PlayerCollisionIterator it(mCollision); !it.isEnd(); ++it) {
            if (!it.isOn()) {
                continue;
            }

            IUsePlayerCollision::Info info;
            it.getInfo(&info);
            if (info.mNormal.dot(cameraFront) <= -0.9961947f) {
                mAlongWallKeepFrame = cAlongWallKeepFrame;
                break;
            }
        }
    }

    if (mActorForAreaSearch != nullptr &&
        rc::isInAreaObj(mActorForAreaSearch, rc::AreaObjType::PlayerAlongWallArea)) {
        mAlongWallKeepFrame = cAlongWallKeepFrame;
    }

    if (mAlongWallKeepFrame != 0) {
        mAlongWallRate += (1.0f - mAlongWallRate) * 0.1f;
    } else {
        mAlongWallRate += (0.0f - mAlongWallRate) * 0.1f;
        if (mAlongWallRate < 0.005f) {
            mAlongWallRate = 0.0f;
        }
    }

    if (mIsWaitFirstLanding) {
        if (mCollision != nullptr && !mCollision->isOnFloor() &&
            !rc::isInWaterArea(mActorForAreaSearch)) {
            mIsControlOff = true;
            clearAll();
            return;
        }

        mIsWaitFirstLanding = false;
    }

    if (mActorForAreaSearch != nullptr && rc::isInPlayerControlOffArea(mActorForAreaSearch)) {
        mIsControlOff = true;
        clearAll();
        return;
    }

    if (mInvalidFrame != 0) {
        mIsControlOff = true;
        clearAll();
        mInvalidFrame--;
        return;
    }

    if (mIsJumpButtonEnabled) {
        if (mKeyConfig->isPadTriggerPlayerJump()) {
            mIsJumpButtonOn = true;
            mFrameFromLastJumpTrig = 0;
            mFrameFromLastSwimPaddleTrig = 0;
        } else {
            mIsJumpButtonOn = mKeyConfig->isPadHoldPlayerJump();
            mFrameFromLastJumpTrig++;
            mFrameFromLastSwimPaddleTrig++;
        }
    } else {
        mIsJumpButtonOn = false;
        resetPrecedingJump();
    }

    mIsSquatTrigOn = mKeyConfig->isPadTriggerPlayerSquat();
    mIsSquatButtonOn = mKeyConfig->isPadHoldPlayerSquat();
    mIsHipDropTrigOn = mKeyConfig->isPadTriggerPlayerHipDrop();
    mIsHipDropButtonOn = mKeyConfig->isPadHoldPlayerHipDrop();
    mIsDashTrigOn = mKeyConfig->isPadTriggerPlayerDash();
    mIsDashButtonOn = mKeyConfig->isPadHoldPlayerDash();
    mIsDashButtonReleased = mKeyConfig->isPadReleasePlayerDash();
    mIsFireBallTrigOn = mKeyConfig->isPadTriggerPlayerDash();
    mIsTailAttackTrigOn = mKeyConfig->isPadTriggerPlayerDash();
    mIsSwimPaddleTrigOn = mKeyConfig->isPadTriggerPlayerJump();
    mIsSwimPaddleButtonOn = mKeyConfig->isPadHoldPlayerJump();
    mIsStoneStatueSustainButtonOn = mKeyConfig->isPadHoldPlayerStatue();
    mIsStoneStatueTrigOn = mKeyConfig->isPadTriggerPlayerStatue();

    if (mKeyConfig->isPadTriggerPlayerRolling()) {
        mIsRollingButtonOn = true;
        mFrameFromLastRollingTrig = 0;
    } else {
        mIsRollingButtonOn = mKeyConfig->isPadHoldPlayerRolling();
        mFrameFromLastRollingTrig++;
    }

    mIsBubbleTrigOn = mKeyConfig->isPadTriggerPlayerBubble();
    mIsClimbAttackTrigOn = mKeyConfig->isPadTriggerPlayerDash();
    mIsClimbAttackButtonOn = mKeyConfig->isPadHoldPlayerDash();
    mIsHoldButtonOn = mKeyConfig->isPadHoldPlayerHold();
    mIsHoldShakeOn = mKeyConfig->isPadShakePlayerHold();
    mIsHoldTrigOn = mKeyConfig->isPadTriggerPlayerHold();
    mIsReleaseTrigOn = mKeyConfig->isPadTriggerPlayerRelease();
    mIsSpinAttackTrigOn = mKeyConfig->isPadTriggerPlayerDash();

    mMoveVec = sead::Vector3f::zero;

    sead::Vector2f stick;
    mKeyConfig->calcLeftStick(&stick);

    sead::Vector3f front;
    sead::Vector3f side;
    if (!calcFrontAndSide(&front, &side, rCameraMtx)) {
        return;
    }

    mMoveVecNoArrange = stick.y * front + stick.x * side;
    mStick = stick;

    snapStick(&stick);
    mMoveVec = stick.y * front + stick.x * side;
    mIsStickOn = mMoveVec.length() > 0.1f;
}

/**
 * Releases every button and centers the stick.
 */
void PlayerInput::clearAll() {
    mIsStickOn = false;
    mMoveVec = {0.0f, 0.0f, 0.0f};
    mMoveVecNoArrange = {0.0f, 0.0f, 0.0f};
    mStick = {0.0f, 0.0f};
    mIsJumpButtonOn = false;
    mFrameFromLastJumpTrig = cPrecedingJumpFrame;
    mIsDashTrigOn = false;
    mIsDashButtonOn = false;
    mIsDashButtonReleased = false;
    mIsSquatTrigOn = false;
    mIsSquatButtonOn = false;
    mIsHipDropTrigOn = false;
    mIsHipDropButtonOn = false;
    mIsFireBallTrigOn = false;
    mIsTailAttackTrigOn = false;
    mFrameFromLastSwimPaddleTrig = cPrecedingTrigFrame;
    mIsSwimPaddleTrigOn = false;
    mIsSwimPaddleButtonOn = false;
    mIsStoneStatueTrigOn = false;
    mIsStoneStatueSustainButtonOn = false;
    mIsRollingButtonOn = false;
    mFrameFromLastRollingTrig = cPrecedingTrigFrame;
    mIsBubbleTrigOn = false;
    mIsClimbAttackTrigOn = false;
    mIsClimbAttackButtonOn = false;
    mIsHoldButtonOn = false;
    mIsHoldShakeOn = false;
    mIsReleaseTrigOn = false;
}

/**
 * Calculates the horizontal directions the stick moves the player in.
 * @param pFront where the direction for stick up is written
 * @param pSide where the direction for stick right is written
 * @param rCameraMtx the camera matrix, used outside of stick fix areas
 * @return false if the camera looks straight up or down
 */
bool PlayerInput::calcFrontAndSide(sead::Vector3f* pFront, sead::Vector3f* pSide,
                                   const sead::Matrix34f& rCameraMtx) {
    if (mActorForAreaSearch != nullptr &&
        rc::isInAreaObj(mActorForAreaSearch, rc::AreaObjType::StickFixArea)) {
        const al::AreaObj* area =
            rc::tryFindAreaObj(mActorForAreaSearch, rc::AreaObjType::StickFixArea,
                               al::getTrans(mActorForAreaSearch));
        area->_28.getBase(*pSide, 0);
        al::normalize(pSide);
        area->_28.getBase(*pFront, 2);
        al::normalize(pFront);
        pFront->negate();
        return true;
    }

    const sead::Vector3f up(0.0f, 1.0f, 0.0f);
    pFront->setCross(up, rCameraMtx.getBase(0));
    if (isNearZeroEach(*pFront, 0.001f)) {
        return false;
    }

    al::normalize(pFront);
    pSide->setCross(*pFront, up);
    al::normalize(pSide);
    return true;
}

/**
 * Snaps the stick toward the axes, depending on the areas the player is in.
 * @param pStick the stick to snap
 */
void PlayerInput::snapStick(sead::Vector2f* pStick) {
    if (mIsSingleMode) {
        return;
    }

    if (mActorForAreaSearch != nullptr &&
        rc::isInAreaObj(mActorForAreaSearch, rc::AreaObjType::StickSnapOffArea)) {
        return;
    }

    if (mAlongWallRate > 0.0f) {
        sead::Vector2f wide = *pStick;
        if (pStick->y > 0.0f) {
            snapWideX(&wide);
        }

        snapNormal(pStick);
        f32 rate = mAlongWallRate;
        *pStick = *pStick * (1.0f - rate) + wide * rate;
        return;
    }

    if (mActorForAreaSearch != nullptr &&
        rc::isInAreaObj(mActorForAreaSearch, rc::AreaObjType::PlayerWidenStickXSnapArea) &&
        !mCollision->isOnFloor()) {
        snapWideX(pStick);
        return;
    }

    snapNormal(pStick);
}

/**
 * Whether jump was triggered recently enough to still count.
 * @return true if jump was triggered in the last few frames
 */
bool PlayerInput::isJumpTrigOn() const {
    return mFrameFromLastJumpTrig < cPrecedingJumpFrame;
}

/**
 * Whether jump was triggered, either recently or by the button this frame.
 * @return true if jump was triggered
 */
bool PlayerInput::isJumpButtonTrigOn() const {
    if (isJumpTrigOn()) {
        return true;
    }

    return mKeyConfig->isPadTriggerPlayerJump();
}

/**
 * Whether a swim paddle was triggered recently enough to still count.
 * @return true if a swim paddle was triggered in the last few frames
 */
bool PlayerInput::isPrecedingSwimPaddleTrigOn() const {
    return mFrameFromLastSwimPaddleTrig < cPrecedingTrigFrame;
}

/**
 * Whether rolling was triggered recently enough to still count.
 * @return true if rolling was triggered in the last few frames
 */
bool PlayerInput::isRollingTrigOn() const {
    return mFrameFromLastRollingTrig < cPrecedingTrigFrame;
}

/**
 * Forgets the remembered jump, swim paddle and rolling triggers.
 */
void PlayerInput::resetPrecedingJump() {
    mFrameFromLastJumpTrig = cPrecedingJumpFrame;
    mFrameFromLastRollingTrig = cPrecedingTrigFrame;
    mFrameFromLastSwimPaddleTrig = cPrecedingTrigFrame;
}

/**
 * Ignores the input for some frames.
 * @param frame the number of frames to ignore the input for
 */
void PlayerInput::invalidateFrame(u32 frame) {
    mInvalidFrame = frame;
}

/**
 * Snaps the stick to the X axis in a wide range around it, and to straight up in between.
 * @param pStick the stick to snap
 */
void PlayerInput::snapWideX(sead::Vector2f* pStick) {
    f32 angle;
    if (!calcStickAngle(&angle, *pStick)) {
        return;
    }

    const f32 snapRightMax = sead::Mathf::deg2rad(50.0f);
    const f32 snapUpMin = sead::Mathf::deg2rad(80.0f);
    const f32 snapUpMax = sead::Mathf::deg2rad(100.0f);
    const f32 snapLeftMin = sead::Mathf::deg2rad(130.0f);
    const f32 halfPi = sead::Mathf::piHalf();

    f32 snapped;
    if (angle <= snapRightMax) {
        snapped = 0.0f;
    } else if (angle < snapUpMin) {
        f32 rate =
            (angle - snapRightMax) * sead::Mathf::pi() * 0.5f / (snapUpMin - snapRightMax);
        snapped = sead::Mathf::clamp(rate, 0.0f, halfPi);
    } else if (angle <= snapUpMax) {
        snapped = halfPi;
    } else if (angle < snapLeftMin) {
        f32 rate = (angle - snapUpMax) * sead::Mathf::pi() * 0.5f / (snapLeftMin - snapUpMax);
        snapped = sead::Mathf::clamp(rate, 0.0f, halfPi) + halfPi;
    } else {
        snapped = sead::Mathf::pi();
    }

    rotateStickToAngle(pStick, snapped);
}

/**
 * Snaps the stick to the eight directions, stretching the ranges in between.
 * @param pStick the stick to snap
 */
void PlayerInput::snapNormal(sead::Vector2f* pStick) {
    f32 angle;
    if (!calcStickAngle(&angle, *pStick)) {
        return;
    }

    const f32 halfPi = sead::Mathf::piHalf();
    const f32 quarterPi = sead::Mathf::pi() / 4.0f;

    u32 quadrant = static_cast<u32>(angle / halfPi);
    f32 base = static_cast<f32>(quadrant) * halfPi;
    f32 offset = angle - base;

    f32 axisMargin;
    f32 diagonalMargin;
    if (mActorForAreaSearch != nullptr &&
        rc::isInAreaObj(mActorForAreaSearch, rc::AreaObjType::PlayerInclinedControlArea)) {
        diagonalMargin = sead::Mathf::deg2rad(18.0f);
        axisMargin = sead::Mathf::deg2rad(11.7f);
    } else {
        diagonalMargin = sead::Mathf::deg2rad(11.7f);
        axisMargin = sead::Mathf::deg2rad(18.0f);
    }

    f32 diagonalMax = diagonalMargin + quarterPi;
    f32 axisMax = halfPi - axisMargin;
    f32 range = axisMax - diagonalMax;

    f32 snapped;
    if (offset > axisMax) {
        snapped = halfPi;
    } else if (offset > diagonalMax) {
        snapped = (offset - diagonalMax) * quarterPi / range + quarterPi;
    } else if (offset > quarterPi - diagonalMargin) {
        snapped = quarterPi;
    } else if (offset > axisMargin) {
        snapped = (offset - axisMargin) * quarterPi / range;
    } else {
        snapped = 0.0f;
    }

    rotateStickToAngle(pStick, base + snapped);
}

/**
 * Widens the stick's dead zone on the axes the stick was pushed all the way along.
 * @param pStick the stick to arrange
 */
void PlayerInput::arrangeRouteDependence(sead::Vector2f* pStick) {
    if (sead::Mathf::abs(pStick->x) > 0.95f) {
        mRouteDependenceX = 1.0f;
    } else if (sead::Mathf::abs(pStick->y) > 0.95f) {
        mRouteDependenceY = 1.0f;
    }

    f32 length = pStick->length();
    if (length < 0.1f) {
        f32 rate = length / 0.1f;
        mRouteDependenceX *= rate;
        mRouteDependenceY *= rate;
    }

    f32 deadZoneX = mRouteDependenceX * 0.1f;
    f32 deadZoneY = mRouteDependenceY * 0.1f;
    if (pStick->x < 0.0f) {
        pStick->x = sead::Mathf::clamp((deadZoneX + pStick->x) / (1.0f - deadZoneX), -1.0f, 0.0f);
    } else {
        pStick->x = sead::Mathf::clamp((pStick->x - deadZoneX) / (1.0f - deadZoneX), 0.0f, 1.0f);
    }

    if (pStick->y < 0.0f) {
        pStick->y = sead::Mathf::clamp((deadZoneY + pStick->y) / (1.0f - deadZoneY), -1.0f, 0.0f);
    } else {
        pStick->y = sead::Mathf::clamp((pStick->y - deadZoneY) / (1.0f - deadZoneY), 0.0f, 1.0f);
    }
}
