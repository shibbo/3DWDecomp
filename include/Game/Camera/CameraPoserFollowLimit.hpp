#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadCamera.h>
#include <math/seadVector.h>

#include "Library/Camera/CameraPoser_RS.hpp"

namespace al {
class ByamlIter;
class CameraDistanceCurve;
class CameraLimitRailKeeper;
class CameraTurnInfo;
class CameraVerticalAbsorber2DGalaxy;
struct CameraObjectRequestInfo;
struct CameraStartInfo;
struct PlacementInfo;
}  // namespace al

class CameraAngleVerticalCtrl;
class PlayerWaterSurfaceFinder;

/**
 * @brief Follow camera poser whose movement is limited by the stage, also used as the parallel
 * and 2D parallel cameras.
 */
class CameraPoserFollowLimit : public al::CameraPoser_RS {
public:
    enum class Type : s32 {
        Follow = 0,
        Parallel = 1,
        Parallel2D = 2,
    };

    /// A horizontal angle loaded from the camera parameters, optionally guarded by a flag.
    struct AngleParam {
        AngleParam(al::CameraPoser_RS* pPoser, const char* pKey, const char* pValidKey)
            : poser(pPoser), key(pKey), isValid(pValidKey == nullptr), validKey(pValidKey) {}

        void load(const al::ByamlIter& rIter);
        f32 calcZoneAngle() const;

        al::CameraPoser_RS* poser;
        f32 angle = 0.0f;
        const char* key;
        bool isValid;
        const char* validKey;
    };

    static_assert(sizeof(AngleParam) == 0x28);

    /// How far the camera is pulled towards the target by collisions behind it.
    struct CollideShrink {
        bool isShrink = false;
        f32 rate = 0.0f;
        f32 targetRate = 0.0f;
    };

    static_assert(sizeof(CollideShrink) == 0xc);

    /// The pose of the previous frame, used to interpolate after a target change.
    struct PrevPose {
        sead::LookAtCamera camera;
        s32 requestStep = -1;
        s32 step = -1;
        s32 stepMax = 0;
        sead::Vector3f pos = {0.0f, 0.0f, 0.0f};
        sead::Vector3f at = {0.0f, 0.0f, 0.0f};
        f32 fovy = 0.0f;

        void resetInterpole() {
            requestStep = -1;
            step = -1;
        }

        void requestInterpole(s32 stepNum) {
            requestStep = requestStep > stepNum ? requestStep : stepNum;
        }

        void update(f32 currentFovy, const sead::LookAtCamera& rCamera) {
            if (requestStep > 0) {
                pos = camera.getPos();
                at = camera.getAt();
                fovy = currentFovy;
                stepMax = requestStep;
                step = requestStep;
                requestStep = -1;
            }

            step--;
            camera = rCamera;
        }
    };

    static_assert(sizeof(PrevPose) == 0x88);

    /// The interpolation of the distance after a distance curve change.
    struct DistanceInterpole {
        f32 startDistance = 0.0f;
        s32 step = 0;
        s32 stepMax = -1;
        f32 requestDistance = -1.0f;

        void start(f32 distance) {
            startDistance = distance;
            step = 0;
            stepMax = 60;
            requestDistance = -1.0f;
        }

        void reset() {
            startDistance = 0.0f;
            step = 0;
            stepMax = -1;
        }
    };

    static_assert(sizeof(DistanceInterpole) == 0x10);

    /// The distance used while the target is in water.
    struct WaterDistance {
        bool isValid = false;
        bool isUnderWater = false;
        bool _2 = false;
        f32 distance = 0.0f;
        f32 targetDistance = 0.0f;
        f32 _c = 0.0f;
        f32 defaultDistance = 0.0f;
        f32 _14 = 0.0f;

        void start(f32 startDistance) {
            isValid = true;
            _2 = false;
            distance = startDistance;
            targetDistance = startDistance;
            defaultDistance = startDistance;
        }
    };

    static_assert(sizeof(WaterDistance) == 0x18);

    /// Finds the water surface around the camera and the look at position.
    struct WaterSurface {
        const al::IUseAreaObj* areaUser;
        PlayerWaterSurfaceFinder* cameraFinder = nullptr;
        PlayerWaterSurfaceFinder* lookAtFinder = nullptr;
        sead::Vector3f _18 = {0.0f, 0.0f, 0.0f};
        sead::Vector3f surfacePos = {0.0f, 0.0f, 0.0f};
        sead::Vector3f _30 = {0.0f, 0.0f, 0.0f};
        bool isFound = false;
    };

    static_assert(sizeof(WaterSurface) == 0x40);

    /// The look at position and distance kept from the previous camera.
    struct KeepPreCamera {
        bool isValid = false;
        sead::Vector3f lookAtPos = {0.0f, 0.0f, 0.0f};
        f32 angleH = 0.0f;
        f32 distance = 0.0f;
    };

    static_assert(sizeof(KeepPreCamera) == 0x18);

    /// The range the horizontal angle is clamped to.
    struct AngleHLimit {
        bool isOffset;
        bool isClampAngle = false;
        f32 minAngle = -360.0f;
        f32 maxAngle = 360.0f;
        bool isClampOffset = false;
        f32 minOffset = -45.0f;
        f32 maxOffset = 45.0f;
        f32 baseAngle = 0.0f;
        f32 offset = 0.0f;

        AngleHLimit(bool isOffsetMode) : isOffset(isOffsetMode) {}

        bool isClamp() const { return isClampAngle || isClampOffset; }
    };

    static_assert(sizeof(AngleHLimit) == 0x20);

    /// The interpolation of the horizontal angle when the angle is reset.
    struct ResetAngle {
        s32 step = 0;
        f32 angle = 0.0f;
        f32 startAngle;
        f32 movingDirOffset = 0.0f;
    };

    static_assert(sizeof(ResetAngle) == 0x10);

    CameraPoserFollowLimit(const char* pName);

    void init() override;
    bool isUseDistanceCurve() const;
    void initByPlacementObj(const al::PlacementInfo& rInfo) override;
    void loadParam(const al::ByamlIter& rIter) override;
    void start(const al::CameraStartInfo& rInfo) override;
    f32 getAngleDegreeV() const;
    bool isInvalidCollider() const;
    f32 calcDistanceRaw();
    f32 calcDistance();
    bool tryStartWater(bool isStart);
    void movement() override;
    void update() override;
    void calcCameraPose(sead::LookAtCamera* pCamera) const override;
    bool receiveRequestFromObject(const al::CameraObjectRequestInfo& rInfo) override;
    void startSnapShotMode() override;
    void endSnapShotMode() override;
    bool isEnableRotateByPad() const override;
    void startCameraReset(bool isReset) override;
    f32 getVerticalAngle() override;
    void setPlessieCameraOn(f32 min, f32 max);
    void setPlessieMode(bool isPlessie);
    void setVerticalAngleRange(f32 min, f32 max);
    void setPlessieCameraOff();
    void exeFollow();
    bool trySwitchLimitObj();
    void updateInputOrSubTargetTurnH();
    void exeResetAngle();
    void endWater();
    void exeFollowRail();
    void exeFollowRailUserCtrl();
    void updateRotateSpeedInputH();
    void exeWater();
    void exeWaterRadicon();
    void startTurnBrake(s32 step);
    void limitNegativeVerticalAngle(bool isLimit);
    bool getRequestTurnDisablePlayerInput() const;
    void setTargetAngleV(f32 angle);
    void setTargetAngleV(f32 angle, s32 step);
    void setHiDegreeLimit(f32 angle);
    void clearHiDegreeLimit();
    void setLowDegreeLimit(f32 angle);
    void clearLowDegreeLimit();
    void setIsFreezeCamPos(bool isFreeze);
    f32 getDistanceWithAngle(f32 angle);
    bool requestTurnToDirection(const al::CameraTurnInfo* pInfo) override;
    void invalidateAutoResetLowAngleV();

    Type getType() const { return mType; }

    f32 getAngleH() const { return mAngleH; }

    f32 getRotateSpeedH() const { return mRotateSpeedH; }

    bool isSnapShotRollFollow() const { return mIsSnapShotRollFollow; }

    const sead::Vector3f& getTargetTrans() const { return mTargetTrans; }

    const sead::Vector3f& getPrevTargetTrans() const { return mPrevTargetTrans; }

    s32 getTurnState() const { return mTurnState; }

    /** @param isPrior Whether the requested distance wins over the computed one. */
    void setPriorRequestDistance(bool isPrior) { mIsPriorRequestDistance = isPrior; }

    /** @param isFreeze Whether the horizontal angle is frozen in Plessie mode. */
    void setPlessieFreezeAngleH(bool isFreeze) { mIsPlessieFreezeAngleH = isFreeze; }

    /** @param isFollow Whether the snapshot roll follows the target. */
    void setSnapShotRollFollow(bool isFollow) { mIsSnapShotRollFollow = isFollow; }

private:
    void createStartAngleParam();
    void createResetAngleParam();
    f32 calcAngleToTargetBackH() const;
    sead::Vector3f calcLookAtOffset(bool isRotateByTargetPose) const;
    bool tryKeepPreCameraDistance(const al::CameraStartInfo& rInfo);
    void updateTargetTrans();
    void updateMovingDirOffset(const sead::Vector3f& rOffset, const sead::Vector3f& rSide);
    void updateCollideShrink(const sead::Vector3f& rOffset);
    void chaseLookAtPos(const sead::Vector3f& rLookAtPos);
    void updateKeepPreCamera();
    void updateAngleH();
    void avoidWaterCeiling();
    void updateAngleV();
    void updateDistanceInterpole();
    void updateWaterSurface();

    Type mType;
    al::CameraLimitRailKeeper* mLimitRailKeeper = nullptr;
    CameraAngleVerticalCtrl* mAngleVerticalCtrl = nullptr;
    const al::CameraDistanceCurve* mDistanceCurve = nullptr;
    const al::CameraDistanceCurve* mRequestDistanceCurve = nullptr;
    CollideShrink* mCollideShrink = nullptr;
    al::CameraVerticalAbsorber2DGalaxy* mVerticalAbsorber2DGalaxy = nullptr;
    PrevPose* mPrevPose = nullptr;
    DistanceInterpole* mDistanceInterpole = nullptr;
    WaterDistance* mWaterDistance = nullptr;
    WaterSurface* mWaterSurface = nullptr;
    al::CameraTurnInfo* mTurnInfo = nullptr;
    KeepPreCamera* mKeepPreCamera = nullptr;
    AngleHLimit* mAngleHLimit = nullptr;
    ResetAngle* mResetAngle = nullptr;
    f32 mAngleH = 0.0f;
    f32 mPrevAngleH = 0.0f;
    f32 mAngleHDiff = 0.0f;
    AngleParam* mStartAngleHParam = nullptr;
    AngleParam** mMultiStartAngleHParams = nullptr;
    AngleParam* mResetAngleHParam = nullptr;
    f32 mStartAngleV;
    f32 mRotateSpeedH = 0.0f;
    f32 mRotateSpeedInputH = 0.0f;
    f32 mRotateSpeedRailH = 0.0f;
    f32 mOutWaterResetAngleV;
    sead::Vector3f mLookAtPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mCameraPos = {0.0f, 0.0f, 100.0f};
    f32 mDistance = 0.0f;
    bool mIsBlendFollowAndParallel = false;
    bool mIsSetTargetBackAngleH = false;
    bool mIsMultiStartAngleH = false;
    bool mIsSetAngleV = false;
    bool mIsValidCtrlDistance = false;
    bool mIsInvalidSearchCollisionParts = false;
    bool mIsInvalidDistanceChaser = false;
    bool mIsInvalidInWater = false;
    bool mIsInvalidOutWaterResetAngleV = false;
    bool mIsInvalidWaterAutoCtrlAngleV = false;
    bool mIsInvalidRequestSetAngleV = false;
    bool mIsInvalidSlopeSnapAngleV = false;
    bool mIsValid2DGalaxy = false;
    bool mIsRequestTurn = false;
    bool mIsPriorRequestDistance = false;
    s32 mTurnState = 0;
    s32 mSlopeSnapState = 0;
    f32 mDistanceByUser = 1400.0f;
    sead::Vector3f mTargetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mPrevTargetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTargetMoveDir = {0.0f, 0.0f, 0.0f};
    f32 mMovingDirOffset = 0.0f;
    f32 mCurrentDistance;
    bool mIsApplyStartMovingDirOffset = false;
    f32 mMaxMovingDirOffset = 350.0f;
    f32 mMovingDirOffsetChaseRate = 1.0f;
    f32 mWaterRotateSpeedH = 0.0f;
    f32 mWaterRotateRate = 0.0f;
    f32 mDefaultFovy = 0.0f;
    bool mIsClimbPole = false;
    bool mIsSetClimbPoleAngleV = false;
    f32 mClimbPoleAngleV;
    f32 mPreClimbPoleAngleV;
    bool mIsInvalidSubTargetTurn = false;
    f32 mSubTargetTurnRate = 0.0f;
    s32 mSubTargetTurnRestartStep = 0;
    bool mIsSubTargetResetAfterTurnV = false;
    s32 mSubTargetTurnRestartStepV = 0;
    bool mIsTurnBrake = false;
    bool mIsResetByRequest = false;
    f32 mTurnBrakeSpeed = 0.0f;
    s32 mTurnBrakeStep = 0;
    s32 mTurnBrakeStepMax = 0;
    s32 _2a0;
    s32 mPlessieStep;
    bool mIsPlessieMode = false;
    bool mIsPlessieFreezeAngleH = false;
    bool mIsClimbing = false;
    bool mIsFreezeCamPos = false;
    bool mIsSnapShotRollFollow = false;
    bool mIsResetByTrigger = false;
    s32 mClimbStep = 0;
};

static_assert(sizeof(CameraPoserFollowLimit) == 0x2b8);

namespace CameraFunction {
al::CameraPoser_RS* createFollowLimitCamera();
al::CameraPoser_RS* createParallel2DCamera();
}  // namespace CameraFunction

namespace CameraPoserFollowLimitFunction {
f32 calcRotateSpeedDegree(s32 sensitivityLevel);
f32 getRotateStickThreshold();
bool isPreCameraFollowOrParallel(const al::CameraStartInfo& rInfo);
}  // namespace CameraPoserFollowLimitFunction
