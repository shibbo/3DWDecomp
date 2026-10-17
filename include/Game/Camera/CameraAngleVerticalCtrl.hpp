#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Camera/CameraAngleUpdateInfo.hpp"
#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class ByamlIter;
class CameraPoser_RS;
class Nerve;
}  // namespace al

class CameraPoserFollowLimit;

/**
 * @brief Controls the vertical angle of a follow camera.
 */
class CameraAngleVerticalCtrl : public al::NerveExecutor {
public:
    /// The kind of input that controls the angle.
    enum class CtrlType : s32 {
        User = 0,
        Water = 1,
    };

    /// Which controller of the angle is active.
    struct CtrlState {
        CtrlType type = CtrlType::User;
        s32 hackFlyerState = -1;
    };

    static_assert(sizeof(CtrlState) == 0x8);

    /// The automatic height following of the hack flyer control.
    struct HackFlyerCtrl {
        void reset() {
            heightOffset = 0.0f;
            heightSpeed = 0.0f;
            waitStep = 0;
        }

        f32 heightOffset = 0.0f;
        f32 heightSpeed = 0.0f;
        s32 waitStep = 0;
        s32 resetStep = 0;
        bool isNeedReset = false;
    };

    static_assert(sizeof(HackFlyerCtrl) == 0x14);

    /// The parameters loaded from the camera parameter file.
    struct Param {
        f32 defaultAngle = 23.0f;
        f32 minAngle = -35.0f;
        f32 maxAngle = 85.0f;
        f32 storedMinAngle = -35.0f;
        f32 storedMaxAngle = 85.0f;
        f32 waterNeutralAngle = 0.0f;
        bool isSetAngleRange = false;
        bool isValidAutoLook = false;
        bool isInvalidHackFlyerCtrl = false;
        bool isInvalidAutoLowAngleReset = false;
    };

    static_assert(sizeof(Param) == 0x1c);

    /// The angle range set by a camera limit rail.
    struct RailRange {
        bool isValid = false;
        f32 minAngle = -35.0f;
        f32 maxAngle = 85.0f;
    };

    static_assert(sizeof(RailRange) == 0xc);

    CameraAngleVerticalCtrl(al::CameraPoser_RS* pPoser);

    static f32 getInitDefaultAngleDegree();

    void loadParam(const al::ByamlIter& rIter);
    void start(const sead::Vector3f& rTargetTrans);
    void startUserCtrl();
    void setHiDegreeLimit(f32 angle);
    void clearHiDegreeLimit();
    void setLowDegreeLimit(f32 angle);
    void clearLowDegreeLimit();
    void update(const CameraAngleUpdateInfo& rInfo);
    void startSnap(f32 angle);
    f32 getDefaultAngleDegree() const;
    void setAngleDegree(f32 angle);
    void setVerticalAngleRange(f32 min, f32 max);
    void storeVerticalAngleRange();
    void restoreVerticalAngleRange();
    void startTargetInterpole(f32 angle);
    void startTargetInterpoleByStep(f32 angle, s32 step);
    void startTargetInterpole(f32 angle, s32 step);
    void startResetInterpole();
    void startResetInterpoleByStep(s32 step);
    void chaseToTargetDegree(f32 angle);
    void chaseToTargetDegreeBySpeed(f32 angle, f32 speed);
    bool isFixInRange() const;
    void setRailAngleDegreeRangeAndInterp(f32 min, f32 max, s32 step);
    void resetRailAngleDegreeRange();
    void startWaterCtrl(s32 step);
    void invalidateAutoResetLowAngleV();
    void endSnap();
    void exeUserCtrl();
    void setIsCameraUnderWater(bool isUnderWater);
    void limitNegativeVerticalAngle(bool isLimit);
    void exeWaterCtrl();
    void exeHackFlyerCtrl();
    void exeInterp();
    void exeSnapStart();
    void exeSnap();
    void exeSnapEnd();

    f32 getAngleDegree() const { return mAngleDegree; }

    f32 getAngleSpeed() const { return mAngleSpeed; }

    bool isWaterCtrl() const { return mIsWaterCtrl; }

    void setIsWaterCtrl(bool isWaterCtrl) { mIsWaterCtrl = isWaterCtrl; }

    /// @note Reads the flag that limitNegativeVerticalAngle() sets.
    bool isUnderWater() const { return mIsLimitNegativeAngle; }

    bool isClimbing() const { return mIsSnapAfterClimb; }

private:
    const al::Nerve* getCtrlNerve() const;
    void setCtrlNerve();
    f32 getRangeMinDegree() const;
    f32 getRangeMaxDegree() const;
    f32 clampAngleDegree(f32 angle) const;
    f32 calcStickRotateDegree() const;
    void updateAngleByTarget();
    bool isStickInputV() const;

    CameraAngleUpdateInfo mInfo;
    CtrlState* mCtrlState = nullptr;
    HackFlyerCtrl* mHackFlyerCtrl = nullptr;
    Param* mParam = nullptr;
    RailRange* mRailRange = nullptr;
    sead::Vector3f mPrevTargetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mUpDir = sead::Vector3f::ey;
    s32 mInterpStep = 0;
    s32 mWaterCtrlStep = 0;
    s32 mAutoResetStep = -1;
    bool mIsCancelAutoReset = false;
    bool mIsMovingAtAutoReset = false;
    f32 mAngleDegree;
    f32 mTargetAngleDegree;
    f32 mHiDegreeLimit;
    f32 mLowDegreeLimit;
    f32 mPrevAngleDegree;
    f32 mPrevTargetAngleDegree;
    f32 mStartAngleDegree = 0.0f;
    f32 mSnapAngleDegree = 0.0f;
    f32 mAngleSpeed = 0.0f;
    bool mIsWaterCtrl = false;
    bool mIsCameraUnderWater = false;
    bool mIsLimitNegativeAngle = false;
    f32 mAutoLookAngleDegree = 0.0f;
    s32 mWaterMoveStep = 0;
    CameraPoserFollowLimit* mPoser;
    bool mIsSnapAfterClimb = false;
    bool mIsValidHiDegreeLimit = false;
    bool mIsValidLowDegreeLimit = false;
};

static_assert(sizeof(CameraAngleVerticalCtrl) == 0x108);
