#include "Camera/CameraPoserFollowLimit.hpp"

#include <attributes.h>
#include <cmath>
#include <gfx/seadCamera.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Camera/CameraAngleUpdateInfo.hpp"
#include "Camera/CameraAngleVerticalCtrl.hpp"
#include "Player/Normal/PlayerWaterSurfaceFinder.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Library/Camera/CameraDistanceCurve.hpp"
#include "Library/Camera/CameraLimitRailKeeper.hpp"
#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Camera/CameraStartInfo.hpp"
#include "Library/Camera/CameraTurnInfo.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Camera/SnapShotCameraCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Camera/CameraArrowCollider.hpp"
#include "Library/Play/Camera/CameraVerticalAbsorber2DGalaxy.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {

// The nerve keeper of a camera poser holds the poser itself, not its IUseNerve base.
#define POSER_NERVE_DECL_(Class, Action, Func)                                                     \
    class Class##Nrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            static_cast<Class*>(static_cast<void*>(pKeeper->getParent<al::IUseNerve>()))           \
                ->exe##Func();                                                                     \
        }                                                                                          \
    };

#define POSER_NERVE_DECL(Class, Action) POSER_NERVE_DECL_(Class, Action, Action)

POSER_NERVE_DECL(CameraPoserFollowLimit, Follow)
POSER_NERVE_DECL(CameraPoserFollowLimit, Water)
POSER_NERVE_DECL(CameraPoserFollowLimit, WaterRadicon)
POSER_NERVE_DECL(CameraPoserFollowLimit, ResetAngle)
POSER_NERVE_DECL_(CameraPoserFollowLimit, ResetAngleWater, ResetAngle)
POSER_NERVE_DECL(CameraPoserFollowLimit, FollowRail)
POSER_NERVE_DECL(CameraPoserFollowLimit, FollowRailUserCtrl)

NERVES_MAKE_NOSTRUCT(CameraPoserFollowLimit, Follow, Water, WaterRadicon, ResetAngle,
                     ResetAngleWater, FollowRail, FollowRailUserCtrl)

/// The follow camera that is currently in the plessie (dinosaur ride) mode.
CameraPoserFollowLimit* sPlessieCamera = nullptr;

/**
 * Checks whether the camera is in one of the water states.
 * @param pPoser The camera.
 * @return True if the camera follows a target in water.
 */
inline bool isWaterNerve(const CameraPoserFollowLimit* pPoser) {
    return al::isNerve(pPoser, &NrvCameraPoserFollowLimitWater) ||
           al::isNerve(pPoser, &NrvCameraPoserFollowLimitWaterRadicon) ||
           al::isNerve(pPoser, &NrvCameraPoserFollowLimitResetAngleWater);
}

/**
 * Starts resetting the angle when the reset button is pressed without tilting the stick.
 * @param pPoser The camera.
 * @return True if the angle reset started.
 */
inline bool tryStartResetAngleByTrigger(CameraPoserFollowLimit* pPoser) {
    if (alCameraPoserFunction::isNoCameraReset(pPoser) ||
        !al::isNearZero(alCameraPoserFunction::calcCameraRotateStickPower(pPoser),
                        CameraPoserFollowLimitFunction::getRotateStickThreshold()) ||
        !alCameraPoserFunction::isTriggerCameraResetRotate(pPoser)) {
        return false;
    }

    al::setNerve(pPoser, &NrvCameraPoserFollowLimitResetAngle);
    return true;
}

/**
 * Calculates a direction from horizontal and vertical angles.
 * @param pDir Where the direction is written.
 * @param angleH Horizontal angle in degrees.
 * @param angleV Vertical angle in degrees.
 */
inline void calcDirByAngleHV(sead::Vector3f* pDir, f32 angleH, f32 angleV) {
    f32 radH = sead::Mathf::deg2rad(angleH);
    f32 radV = sead::Mathf::deg2rad(angleV);
    pDir->set(sinf(radH) * cosf(radV), sinf(radV), cosf(radH) * cosf(radV));
}

/**
 * Calculates the stick input that turns the camera, rotated by the snapshot roll.
 * @param pStick Where the stick input is written.
 * @param pPoser The camera.
 */
void calcRotateStick(sead::Vector2f* pStick, const CameraPoserFollowLimit* pPoser) {
    if (pPoser->getRequestTurnDisablePlayerInput()) {
        pStick->x = sead::Vector2f::zero.x;
        pStick->y = sead::Vector2f::zero.y;
        return;
    }

    alCameraPoserFunction::calcCameraRotateStick(pStick, pPoser);
    pStick->x = -pStick->x;

    if (alCameraPoserFunction::isSnapShotMode(pPoser)) {
        sead::Vector2f dir = *pStick;
        if (al::tryNormalizeOrZero(&dir)) {
            f32 angle = sead::Mathf::rad2deg(atan2f(dir.y, dir.x));
            f32 rad = sead::Mathf::deg2rad(
                al::wrapAngle(angle + alCameraPoserFunction::getSnapShotRollDegree(pPoser)));
            f32 cosRad = cosf(rad);
            f32 sinRad = sinf(rad);
            f32 length = pStick->length();
            pStick->x = cosRad * length;
            pStick->y = sinRad * length;
        }
    }
}

/**
 * Interpolates a camera from the pose kept before a target change.
 * @param pPrevPose The kept pose.
 * @param pCamera The camera to interpolate.
 */
void applyPrevPoseInterpole(const CameraPoserFollowLimit::PrevPose* pPrevPose,
                            sead::LookAtCamera* pCamera) {
    if (pPrevPose->step < 1) {
        return;
    }

    f32 rate = al::hermiteRate(
        al::normalize((f32)(pPrevPose->stepMax - pPrevPose->step), 0.0f, (f32)pPrevPose->stepMax),
        1.5f, 0.0f);
    sead::Vector3f pos = pPrevPose->pos;
    sead::Vector3f at = pPrevPose->at;
    al::lerpVec(&pos, pos, pCamera->getPos(), rate);
    al::lerpVec(&at, at, pCamera->getAt(), rate);
    pCamera->setPos(pos);
    pCamera->setAt(at);
}

/**
 * Calculates the horizontal rotation speed from the stick, slower when looking from above.
 * @param pPoser The camera.
 * @param angleV The current vertical angle in degrees.
 * @return The rotation speed in degrees per frame.
 */
f32 calcRotateSpeedH(const CameraPoserFollowLimit* pPoser, f32 angleV) {
    sead::Vector2f stick = {0.0f, 0.0f};
    calcRotateStick(&stick, pPoser);

    if (al::isNearZero(stick.x, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        return 0.0f;
    }

    f32 rate = al::normalizeAbs(stick.x, CameraPoserFollowLimitFunction::getRotateStickThreshold(),
                                1.0f);
    f32 angleRate;
    if (angleV < 60.0f) {
        angleRate = 1.0f;
    } else if (angleV > 75.0f) {
        angleRate = 0.0f;
    } else {
        angleRate = 1.0f - al::normalize(angleV, 60.0f, 75.0f);
    }

    f32 scale = al::lerpValue(angleRate, 0.75f, 1.0f);
    f32 speed = CameraPoserFollowLimitFunction::calcRotateSpeedDegree(
        alCameraPoserFunction::getStickSensitivityLevel(pPoser));
    return rate * scale * speed;
}

/**
 * Clamps a horizontal angle into the range of a limit, wrapping it once if needed.
 * @param pLimit The limit.
 * @param angle The angle in degrees.
 * @return The clamped angle.
 */
f32 clampAngleH(const CameraPoserFollowLimit::AngleHLimit* pLimit, f32 angle) {
    f32 min;
    f32 max;
    if (pLimit->isClampAngle) {
        min = pLimit->minAngle;
        max = pLimit->maxAngle;
    } else if (pLimit->isClampOffset) {
        min = pLimit->baseAngle + pLimit->minOffset;
        max = pLimit->baseAngle + pLimit->maxOffset;
    } else {
        return angle;
    }

    if (al::isInRange(angle, min, max)) {
        return angle;
    }

    f32 wrapped = angle + al::sign(angle) * -360.0f;
    if (al::isInRange(wrapped, min, max)) {
        return wrapped;
    }

    f32 diffMin = al::diffNearAngleDegree(angle, min);
    f32 diffMax = al::diffNearAngleDegree(angle, max);
    return sead::Mathf::abs(diffMin) < sead::Mathf::abs(diffMax) ? min : max;
}

}  // namespace

/**
 * Loads the angle if its flag is set (or if it has no flag).
 * @param rIter The camera parameters.
 */
inline void CameraPoserFollowLimit::AngleParam::load(const al::ByamlIter& rIter) {
    if (validKey == nullptr || (al::tryGetByamlBool(&isValid, rIter, validKey) && isValid)) {
        al::tryGetByamlF32(&angle, rIter, key);
    }
}

/**
 * Converts the angle from the zone to the root coordinates.
 * @return The angle in degrees.
 */
inline f32 CameraPoserFollowLimit::AngleParam::calcZoneAngle() const {
    return alCameraPoserFunction::calcZoneRotateAngleH(angle, poser);
}

/**
 * Constructs the camera, the type being given by its name.
 * @param pName "Parallel", "Parallel2D" or any follow camera name.
 */
CameraPoserFollowLimit::CameraPoserFollowLimit(const char* pName)
    : CameraPoser_RS(pName),
      mType(al::isEqualString(pName, "Parallel")   ? Type::Parallel :
            al::isEqualString(pName, "Parallel2D") ? Type::Parallel2D :
                                                     Type::Follow),
      mStartAngleV(CameraAngleVerticalCtrl::getInitDefaultAngleDegree()),
      mOutWaterResetAngleV(CameraAngleVerticalCtrl::getInitDefaultAngleDegree()),
      mClimbPoleAngleV(CameraAngleVerticalCtrl::getInitDefaultAngleDegree()),
      mPreClimbPoleAngleV(CameraAngleVerticalCtrl::getInitDefaultAngleDegree()) {
    sPlessieCamera = nullptr;
}

/**
 * Creates the start and reset horizontal angle parameters if they are missing.
 */
inline void CameraPoserFollowLimit::createStartAngleParam() {
    if (mStartAngleHParam == nullptr) {
        Type type = mType;
        mStartAngleHParam = new AngleParam(this, "StartAngleDegreeH",
                                           type == Type::Parallel2D ? nullptr : "IsSetAngleH");
    }
}

/**
 * Creates the horizontal angle parameter used by the angle reset if it is missing.
 */
inline void CameraPoserFollowLimit::createResetAngleParam() {
    if (mResetAngleHParam == nullptr) {
        mResetAngleHParam = new AngleParam(this, "ResetAngleDegreeH", "IsSetResetAngleH");
    }
}

/**
 * Initializes the controllers of the camera.
 */
void CameraPoserFollowLimit::init() {
    initNerve(&NrvCameraPoserFollowLimitFollow, 0);
    alCameraPoserFunction::initCameraOffsetCtrlPreset(this);
    alCameraPoserFunction::initCameraVerticalAbsorber(this);
    alCameraPoserFunction::validateVerticalAbsorbKeepInFrame(this);
    alCameraPoserFunction::setVerticalAbsorbKeepInFrameScreenOffsetUp(this, -240.0f);
    alCameraPoserFunction::setVerticalAbsorbKeepInFrameScreenOffsetDown(this, 120.0f);
    alCameraPoserFunction::initSnapShotCameraCtrlZoomRollMove(this, false, false);
    alCameraPoserFunction::setSnapShotMaxZoomOutFovyDegree(this, 60.0f);
    alCameraPoserFunction::initCameraMoveLimit(this);
    alCameraPoserFunction::validateCollider(this);

    if (mAngleVerticalCtrl == nullptr) {
        mAngleVerticalCtrl = new CameraAngleVerticalCtrl(this);
    }

    mArrowCollider = new al::CameraArrowCollider(getCollisionDirector());
    mPrevPose = new PrevPose;
    al::CameraTurnInfo* turnInfo = new al::CameraTurnInfo;
    turnInfo->mRequesterName = nullptr;
    turnInfo->mDir = {0.0f, 0.0f, 0.0f};
    turnInfo->_14 = 0.0f;
    turnInfo->_18 = 0.0f;
    turnInfo->_1c = false;
    turnInfo->_1d = false;
    mTurnInfo = turnInfo;
    mKeepPreCamera = new KeepPreCamera;
    mDistanceInterpole = new DistanceInterpole;
    mWaterDistance = new WaterDistance;

    WaterSurface* waterSurface = new WaterSurface;
    mWaterSurface = waterSurface;
    waterSurface->areaUser = this;
    waterSurface->cameraFinder = new PlayerWaterSurfaceFinder(this);
    waterSurface->lookAtFinder = new PlayerWaterSurfaceFinder(waterSurface->areaUser);

    if (isUseDistanceCurve() && mDistanceCurve == nullptr) {
        mDistanceCurve = al::CameraDistanceCurve::getDefaultCurve();
    }

    if (mType == Type::Parallel2D) {
        mIsSetAngleV = true;
        initOrthoProjectionParam();
        mAngleHLimit = new AngleHLimit(true);
    } else {
        mCollideShrink = new CollideShrink;
        if (mType == Type::Parallel) {
            mAngleHLimit = new AngleHLimit(false);
        }
    }

    mResetAngle = new ResetAngle;
    mDefaultFovy = getFovyDegree();
    createStartAngleParam();
    createResetAngleParam();
}

/**
 * Checks whether the distance comes from a distance curve instead of the user distance.
 * @return True if a distance curve is used.
 */
bool CameraPoserFollowLimit::isUseDistanceCurve() const {
    if (mIsValidCtrlDistance) {
        return false;
    }

    return mType != Type::Parallel2D;
}

/**
 * Initializes the area limitter from the placement.
 * @param rInfo The placement of the camera.
 */
void CameraPoserFollowLimit::initByPlacementObj(const al::PlacementInfo& rInfo) {
    tryInitAreaLimitter(rInfo);
}

/**
 * Loads the camera parameters.
 * @param rIter The camera parameters.
 */
void CameraPoserFollowLimit::loadParam(const al::ByamlIter& rIter) {
    if (!isUseDistanceCurve() ||
        (al::tryGetByamlBool(&mIsValidCtrlDistance, rIter, "IsValidCtrlDistance") &&
         mIsValidCtrlDistance)) {
        al::tryGetByamlF32(&mDistanceByUser, rIter, "DistanceByUser");
    }

    al::tryGetByamlBool(&mIsBlendFollowAndParallel, rIter, "IsBlendFollowAndParallel");
    al::tryGetByamlBool(&mIsInvalidInWater, rIter, "IsInvalidInWater");
    al::tryGetByamlBool(&mIsInvalidRequestSetAngleV, rIter, "IsInvalidRequestSetAngleV");
    al::tryGetByamlBool(&mIsInvalidSubTargetTurn, rIter, "IsInvalidSubTargetTurn");

    if (al::tryGetByamlBool(&mIsInvalidSearchCollisionParts, rIter,
                            "IsInvalidSearchCollisionParts") &&
        mIsInvalidSearchCollisionParts) {
        mArrowCollider->setIsInvalidSearchCollisionParts(true);
    }

    createStartAngleParam();
    mStartAngleHParam->load(rIter);

    if (!mStartAngleHParam->isValid) {
        al::tryGetByamlBool(&mIsSetTargetBackAngleH, rIter, "IsSetTargetBackAngleH");

        al::ByamlIter multiIter;
        if (rIter.tryGetIterByKey(&multiIter, "MultiStartAngleDegreeH")) {
            mIsMultiStartAngleH = true;
            mMultiStartAngleHParams = new AngleParam*[2];
            for (s32 i = 0; i < 2; i++) {
                al::ByamlIter iter;
                multiIter.tryGetIterByIndex(&iter, i);
                mMultiStartAngleHParams[i] = new AngleParam(this, "StartAngleDegreeH", nullptr);
                mMultiStartAngleHParams[i]->load(iter);
            }
        }
    }

    createResetAngleParam();
    mResetAngleHParam->load(rIter);

    al::tryGetByamlBool(&mIsSetAngleV, rIter, "IsSetAngleV");
    if (mIsSetAngleV) {
        al::tryGetByamlF32(&mStartAngleV, rIter, "StartAngleDegreeV");
    }

    AngleHLimit* limit = mAngleHLimit;
    if (limit != nullptr) {
        if (limit->isOffset) {
            if (al::tryGetByamlBool(&limit->isClampOffset, rIter, "IsClampAngleOffsetH") &&
                limit->isClampOffset) {
                limit->minOffset = al::getByamlKeyFloat(rIter, "MinAngleOffsetH");
                limit->maxOffset = al::getByamlKeyFloat(rIter, "MaxAngleOffsetH");
            } else if (al::tryGetByamlBool(&limit->isClampOffset, rIter,
                                           "IsClampAngleOffsetH2D") &&
                       limit->isClampOffset) {
                limit->minOffset = al::getByamlKeyFloat(rIter, "MinAngleOffsetH2D");
                limit->maxOffset = al::getByamlKeyFloat(rIter, "MaxAngleOffsetH2D");
            }
        } else if (al::tryGetByamlBool(&limit->isClampAngle, rIter, "IsClampAngleH") &&
                   limit->isClampAngle) {
            limit->minAngle = al::getByamlKeyFloat(rIter, "MinAngleH");
            limit->maxAngle = al::getByamlKeyFloat(rIter, "MaxAngleH");
        }
    }

    if (mAngleVerticalCtrl == nullptr) {
        mAngleVerticalCtrl = new CameraAngleVerticalCtrl(this);
    }

    mAngleVerticalCtrl->loadParam(rIter);

    if (!al::tryGetByamlBool(&mIsInvalidOutWaterResetAngleV, rIter,
                             "IsInvalidOutWaterResetAngleV") ||
        !mIsInvalidOutWaterResetAngleV) {
        al::tryGetByamlF32(&mOutWaterResetAngleV, rIter, "OutWaterResetAngleV");
    }

    al::tryGetByamlBool(&mIsInvalidWaterAutoCtrlAngleV, rIter, "IsInvalidWaterAutoCtrlAngleV");

    if (mType == Type::Follow) {
        al::tryGetByamlBool(&mIsInvalidSlopeSnapAngleV, rIter, "IsInvalidSlopeSnapAngleV");
    }

    if (mType == Type::Parallel2D) {
        al::tryGetByamlBool(&mIsApplyStartMovingDirOffset, rIter, "IsApplyStartMovingDirOffset");
        if (al::tryGetByamlBool(&mIsValid2DGalaxy, rIter, "IsValid2DGalaxy") &&
            mIsValid2DGalaxy) {
            mVerticalAbsorber2DGalaxy = new al::CameraVerticalAbsorber2DGalaxy();
            alCameraPoserFunction::offVerticalAbsorb(this);
        }
    } else {
        al::tryGetByamlBool(&mIsInvalidDistanceChaser, rIter, "IsInvalidDistanceChaser");
    }

    if (al::tryGetByamlBool(&mIsSetClimbPoleAngleV, rIter, "IsSetClimbPoleAngleV") &&
        mIsSetClimbPoleAngleV) {
        mClimbPoleAngleV = al::getByamlKeyFloat(rIter, "ClimbPoleAngleV");
    }

    al::tryGetByamlF32(&mMaxMovingDirOffset, rIter, "MaxMovingDirOffset");
    al::tryGetByamlF32(&mMovingDirOffsetChaseRate, rIter, "MovingDirOffsetChaseRate");

    if (isUseDistanceCurve()) {
        mDistanceCurve = al::CameraDistanceCurve::findOrDefaultCurve(rIter);
    }
}

/**
 * Calculates the offset of the look at position from the target.
 * @param isRotateByTargetPose Whether the offset is rotated by the pose of the target.
 * @return The offset.
 */
inline ALWAYS_INLINE sead::Vector3f
CameraPoserFollowLimit::calcLookAtOffset(bool isRotateByTargetPose) const {
    sead::Vector3f offset = alCameraPoserFunction::getOffset(this);
    offset.y += mWaterDistance->isValid && !mWaterDistance->isUnderWater ? 50.0f : 0.0f;

    if (isRotateByTargetPose) {
        sead::Quatf pose = sead::Quatf::unit;
        alCameraPoserFunction::calcTargetPose(&pose, this);
        offset.rotate(pose);
    }

    return offset;
}

/**
 * Calculates the horizontal angle from the camera direction to the back of the target.
 * @return The angle in degrees, or 0 if the target faces up or down.
 */
inline f32 CameraPoserFollowLimit::calcAngleToTargetBackH() const {
    sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcCameraDirH(&dirH, this);
    sead::Vector3f back = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetFront(&back, this);
    back.negate();
    al::verticalizeVec(&back, mUp, back);

    if (!al::tryNormalizeOrZero(&back)) {
        return 0.0f;
    }

    return al::calcAngleOnPlaneDegree(dirH, back, mUp);
}

/**
 * Keeps the distance of the previous camera if it is closer than the distance of this camera.
 * @param rInfo Information about the previous camera.
 * @return True if the distance of the previous camera is kept.
 */
inline ALWAYS_INLINE bool
CameraPoserFollowLimit::tryKeepPreCameraDistance(const al::CameraStartInfo& rInfo) {
    KeepPreCamera* keepPreCamera = mKeepPreCamera;
    f32 distance = calcDistanceRaw();
    f32 angleH = mAngleH;
    f32 angleV = mAngleVerticalCtrl->getAngleDegree();
    if (alCameraPoserFunction::isInvalidKeepPreCameraDistance(rInfo) ||
        mType == Type::Parallel2D || alCameraPoserFunction::isSceneCameraFirstCalc(this) ||
        alCameraPoserFunction::isPrePriorityDemoAll(rInfo) ||
        alCameraPoserFunction::isPrePriorityPlayer(rInfo)) {
        return false;
    }

    sead::Vector3f preDir =
        alCameraPoserFunction::getPreLookAtPos(this) - alCameraPoserFunction::getPreCameraPos(this);
    if (!al::tryNormalizeOrZero(&preDir)) {
        return false;
    }

    sead::Vector3f lookAtDiff = alCameraPoserFunction::getPreLookAtPos(this) - mAt;
    al::verticalizeVec(&lookAtDiff, preDir, lookAtDiff);
    sead::Vector3f lookAtPos = mAt + lookAtDiff;
    f32 preDistance = (alCameraPoserFunction::getPreCameraPos(this) - lookAtPos).length();
    if (preDistance >= distance) {
        return false;
    }

    if (alCameraPoserFunction::isInvalidKeepPreCameraDistanceIfNoCollide(rInfo)) {
        sead::Vector3f dir;
        calcDirByAngleHV(&dir, angleH, angleV);
        if (al::tryNormalizeOrZero(&dir)) {
            sead::Vector3f arrow = distance * dir;
            if (!alCameraPoserFunction::checkFirstCameraCollisionArrow(nullptr, nullptr, this,
                                                                       lookAtPos, arrow)) {
                return false;
            }
        }
    }

    keepPreCamera->lookAtPos = mAt;
    keepPreCamera->distance = preDistance;
    keepPreCamera->angleH = al::wrapAngle(angleH);
    keepPreCamera->isValid = true;
    return true;
}

/**
 * Places the camera behind the target when it becomes active, keeping the previous camera
 * pose when possible.
 * @param rInfo Information about the previous camera.
 */
void CameraPoserFollowLimit::start(const al::CameraStartInfo& rInfo) {
    mIsRequestTurn = false;
    mIsSubTargetResetAfterTurnV = false;
    mTurnState = 0;
    mSlopeSnapState = 0;
    mRotateSpeedH = 0.0f;
    mMovingDirOffset = 0.0f;
    mPrevPose->resetInterpole();
    mTargetMoveDir = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetTrans(&mTargetTrans, this);
    mPrevTargetTrans = mTargetTrans;

    CameraAngleVerticalCtrl* angleVerticalCtrl = mAngleVerticalCtrl;
    f32 startAngleV = mStartAngleV;
    bool isSetAngleV = mIsSetAngleV;
    if (alCameraPoserFunction::isExistNextPoseByPreCamera(rInfo)) {
        angleVerticalCtrl->setAngleDegree(alCameraPoserFunction::getNextAngleVByPreCamera(rInfo));
    } else if (alCameraPoserFunction::isExistAreaAngleV(rInfo)) {
        angleVerticalCtrl->setAngleDegree(alCameraPoserFunction::getAreaAngleV(rInfo));
    } else if (isSetAngleV) {
        angleVerticalCtrl->setAngleDegree(startAngleV);
    } else if (alCameraPoserFunction::isSceneCameraFirstCalc(this) ||
               alCameraPoserFunction::isValidResetPreCameraPose(rInfo) ||
               alCameraPoserFunction::isPrePriorityDemoAll(rInfo) ||
               alCameraPoserFunction::isPrePriorityEntranceAll(rInfo)) {
        angleVerticalCtrl->setAngleDegree(angleVerticalCtrl->getDefaultAngleDegree());
    } else if (isFirstCalc() || !alCameraPoserFunction::isValidKeepPreSelfCameraPose(rInfo)) {
        const sead::Vector3f& preCameraPos = alCameraPoserFunction::getPreCameraPos(this);
        const sead::Vector3f& preLookAtPos = alCameraPoserFunction::getPreLookAtPos(this);
        if (!al::isNearZero((preCameraPos - preLookAtPos).length(), 0.001f)) {
            sead::Vector3f preDir = {0.0f, 0.0f, 0.0f};
            alCameraPoserFunction::calcPreCameraDir(&preDir, this);
            sead::Vector3f dirV = preDir;
            al::parallelizeVec(&dirV, mUp, dirV);
            f32 length = dirV.length();
            f32 sign;
            if (al::isNearZero(dirV, 0.001f)) {
                sign = 1.0f;
            } else {
                sign = mUp.dot(dirV) < 0.0f ? -1.0f : 1.0f;
            }

            f32 angle = sead::Mathf::rad2deg(
                asinf(sead::Mathf::clamp(length * sign, -0.9999f, 0.9999f)));
            angleVerticalCtrl->setAngleDegree(sead::Mathf::clamp(angle, -85.0f, 85.0f));
        }
    }

    bool isClimbPole = alCameraPoserFunction::isTargetClimbPole(this);
    mIsClimbPole = isClimbPole;
    if (isClimbPole && mIsSetClimbPoleAngleV) {
        mPreClimbPoleAngleV = mAngleVerticalCtrl->getAngleDegree();
        mAngleVerticalCtrl->setAngleDegree(mClimbPoleAngleV);
    }

    mAngleVerticalCtrl->start(mTargetTrans);

    sead::Vector3f offset = calcLookAtOffset(mIsValid2DGalaxy);

    CollideShrink* collideShrink = mCollideShrink;
    if (collideShrink != nullptr) {
        bool isInvalid = isInvalidCollider();
        bool isNoCollide = is140();
        collideShrink->isShrink = false;
        collideShrink->rate = 0.0f;
        collideShrink->targetRate = 0.0f;

        if (!isInvalid && !isNoCollide) {
            sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
            sead::Vector3f arrow = offset * 2.0f;
            if (alCameraPoserFunction::checkFirstCameraCollisionArrow(&hitPos, nullptr, this,
                                                                      mTargetTrans, arrow)) {
                f32 rate = al::easeIn(
                    1.0f -
                    al::normalize((hitPos - mTargetTrans).length(), 0.0f, arrow.length()));
                collideShrink->rate = rate;
                collideShrink->targetRate = rate;
            }
        }

        offset *= 1.0f - mCollideShrink->rate;
    }

    mAt = mTargetTrans + offset;
    mUp = sead::Vector3f::ey;

    if (alCameraPoserFunction::isExistNextPoseByPreCamera(rInfo)) {
        mAngleH = alCameraPoserFunction::getNextAngleHByPreCamera(rInfo);
    } else if (alCameraPoserFunction::isExistAreaAngleH(rInfo)) {
        mAngleH = alCameraPoserFunction::getAreaAngleH(rInfo);
    } else if (mType == Type::Parallel2D) {
        mAngleH = mStartAngleHParam->calcZoneAngle();
    } else if (!alCameraPoserFunction::isPrePriorityPlayer(rInfo) && mStartAngleHParam->isValid) {
        mAngleH = mStartAngleHParam->calcZoneAngle();
    } else if (!alCameraPoserFunction::isPrePriorityPlayer(rInfo) && mIsMultiStartAngleH) {
        f32 preAngleH = alCameraPoserFunction::calcPreCameraAngleH(this);
        f32 minDiff = -1.0f;
        for (s32 i = 0; i < 2; i++) {
            f32 diff = sead::Mathf::abs(
                al::diffNearAngleDegree(preAngleH, mMultiStartAngleHParams[i]->calcZoneAngle()));
            if (minDiff < 0.0f || diff < minDiff) {
                minDiff = diff;
                mAngleH = mMultiStartAngleHParams[i]->calcZoneAngle();
            }
        }
    } else if (mIsSetTargetBackAngleH || alCameraPoserFunction::isSceneCameraFirstCalc(this) ||
               alCameraPoserFunction::isValidResetPreCameraPose(rInfo)) {
        sead::Vector3f back = {0.0f, 0.0f, 0.0f};
        alCameraPoserFunction::calcTargetFront(&back, this);
        back.negate();
        mAngleH = al::calcAngleOnPlaneDegree(sead::Vector3f::ez, back, mUp);
    } else if (isFirstCalc() || !alCameraPoserFunction::isValidKeepPreSelfCameraPose(rInfo)) {
        mAngleH = alCameraPoserFunction::calcPreCameraAngleH(this);
    }

    if (mIsInvalidDistanceChaser || !tryKeepPreCameraDistance(rInfo)) {
        mKeepPreCamera->isValid = false;
    }

    const al::CameraDistanceCurve* requestDistanceCurve =
        alCameraPoserFunction::tryGetBossDistanceCurve(this);
    if (requestDistanceCurve == nullptr) {
        requestDistanceCurve = alCameraPoserFunction::tryGetEquipmentDistanceCurve(this);
    }

    mRequestDistanceCurve = requestDistanceCurve;
    mDistanceInterpole->reset();

    f32 requestDistance = -1.0f;
    if (alCameraPoserFunction::tryGetTargetRequestDistance(&requestDistance, this) &&
        requestDistance > 0.0f) {
        f32 distance = calcDistanceRaw();
        DistanceInterpole* distanceInterpole = mDistanceInterpole;
        if (distanceInterpole->requestDistance < 0.0f) {
            distanceInterpole->requestDistance = distance;
        }

        f32 target = al::lerpValue(0.3f, distanceInterpole->requestDistance, distance);
        distanceInterpole->requestDistance =
            al::lerpValue(0.1f, distanceInterpole->requestDistance, target);
    }

    AngleHLimit* angleHLimit = mAngleHLimit;
    if (angleHLimit != nullptr) {
        angleHLimit->baseAngle = alCameraPoserFunction::calcZoneInvRotateAngleH(mAngleH, mViewMtx);
        angleHLimit->offset = clampAngleH(angleHLimit, angleHLimit->baseAngle);
    }

    if (mVerticalAbsorber2DGalaxy != nullptr) {
        mVerticalAbsorber2DGalaxy->start(this);
    }

    sead::Vector3f dir;
    calcDirByAngleHV(&dir, mAngleH, mAngleVerticalCtrl->getAngleDegree());
    f32 distance = calcDistance();
    mEye = mAt + dir * distance;

    if (mIsBlendFollowAndParallel) {
        sead::Vector3f side = {0.0f, 0.0f, 0.0f};
        alCameraPoserFunction::calcSideDir(&side, this);
        sead::Vector3f lookAtDiff = alCameraPoserFunction::getPreLookAtPos(this) - mAt;
        al::parallelizeVec(&lookAtDiff, side, lookAtDiff);
        f32 movingDirOffset = lookAtDiff.length() * (side.dot(lookAtDiff) < 0.0f ? -1.0f : 1.0f);
        mMovingDirOffset = movingDirOffset;
        sead::Vector3f sideOffset = side * movingDirOffset;
        mAt += sideOffset;
        mEye += sideOffset;
    }

    mCameraPos = mEye;
    mPrevAngleH = mAngleH;
    mDistance = distance;

    if (!isInvalidCollider()) {
        mArrowCollider->start();
    }

    sead::LookAtCamera camera;
    makeLookAtCameraPrev(&camera);
    makeLookAtCameraPost(&camera);
    mPrevPose->camera = camera;
    mLookAtPos = mAt;

    if (!tryStartWater(true)) {
        if (al::isNerve(this, &NrvCameraPoserFollowLimitWater)) {
            alCameraPoserFunction::onVerticalAbsorb(this);
        }

        al::setNerve(this, &NrvCameraPoserFollowLimitFollow);
    }
}

/**
 * Gets the vertical angle of the camera.
 * @return The angle in degrees.
 */
f32 CameraPoserFollowLimit::getAngleDegreeV() const {
    return mAngleVerticalCtrl->getAngleDegree();
}

/**
 * Checks whether the camera ignores the collision, either by the limit rail or by the target.
 * @return True if the collision is ignored.
 */
bool CameraPoserFollowLimit::isInvalidCollider() const {
    if (mLimitRailKeeper != nullptr && mLimitRailKeeper->isInvalidCheckCollision()) {
        return true;
    }

    return alCameraPoserFunction::isInvalidCollider(this);
}

/**
 * Calculates the distance between the camera and the look at position, without the
 * interpolations.
 * @return The distance.
 */
f32 CameraPoserFollowLimit::calcDistanceRaw() {
    f32 distance;
    bool isClimbing = false;
    if (isWaterNerve(this)) {
        distance = mWaterDistance->distance;
    } else if (mRequestDistanceCurve != nullptr) {
        distance = mRequestDistanceCurve->calcDistance(mAngleVerticalCtrl->getAngleDegree());
    } else if (!isUseDistanceCurve()) {
        distance = mDistanceByUser;
    } else {
        distance = mDistanceCurve->calcDistance(mAngleVerticalCtrl->getAngleDegree());
        if (distance < 800.0f &&
            (alCameraPoserFunction::isPlayerClimbing(this) || mAngleVerticalCtrl->isClimbing())) {
            s32 step = mClimbStep < 60 ? mClimbStep : 60;
            mClimbStep++;
            f32 rate = step / 60.0f;
            distance = rate * 800.0f + distance * (1.0f - rate);
            isClimbing = true;
        }
    }

    if (!isClimbing) {
        mClimbStep = 0;
    }

    f32 requestDistance = -1.0f;
    if (alCameraPoserFunction::tryGetTargetRequestDistance(&requestDistance, this) &&
        (distance < requestDistance || mIsPriorRequestDistance)) {
        distance = requestDistance;
    }

    return distance;
}

/**
 * Calculates the distance between the camera and the look at position.
 * @return The distance.
 */
f32 CameraPoserFollowLimit::calcDistance() {
    if (is141()) {
        return mCurrentDistance;
    }

    f32 distance;
    if (mKeepPreCamera->isValid) {
        distance = mKeepPreCamera->distance;
    } else {
        distance = calcDistanceRaw();
    }

    DistanceInterpole* distanceInterpole = mDistanceInterpole;
    if (distanceInterpole->requestDistance > 0.0f) {
        return distanceInterpole->requestDistance;
    }

    if (distanceInterpole->stepMax < 1) {
        return distance;
    }

    return al::lerpValue(al::normalize((f32)distanceInterpole->step, 0.0f,
                                       (f32)distanceInterpole->stepMax),
                         distanceInterpole->startDistance, distance);
}

/**
 * Starts following a target in water.
 * @param isStart Whether the camera is just starting.
 * @return True if the target is in water.
 */
bool CameraPoserFollowLimit::tryStartWater(bool isStart) {
    if (!alCameraPoserFunction::isTargetInWater(this) || mIsInvalidInWater ||
        alCameraPoserFunction::isOnRideObj(this)) {
        return false;
    }

    if (!isStart) {
        mPrevPose->requestInterpole(60);
    }

    if (!mIsInvalidWaterAutoCtrlAngleV) {
        mAngleVerticalCtrl->startWaterCtrl(isStart ? 0 : 60);
    }

    mAngleVerticalCtrl->setIsWaterCtrl(true);

    sead::Vector3f checkPos(mAt.x, mAt.y + -200.0f, mAt.z);
    bool isInWaterNoSink = rc::isInWaterAreaNoSink(this, checkPos);
    WaterDistance* waterDistance = mWaterDistance;
    if (isInWaterNoSink) {
        waterDistance->start(600.0f);
        mWaterDistance->isUnderWater = false;
        mAngleVerticalCtrl->setIsCameraUnderWater(false);
    } else {
        f32 distance;
        if (isUseDistanceCurve()) {
            distance = mDistanceCurve->calcDistance(
                CameraAngleVerticalCtrl::getInitDefaultAngleDegree());
        } else {
            distance = mDistanceByUser;
        }

        waterDistance->start(distance);
        if (isStart) {
            mWaterDistance->isUnderWater = true;
            mAngleVerticalCtrl->setIsCameraUnderWater(true);
        } else {
            mWaterDistance->isUnderWater = false;
            mAngleVerticalCtrl->setIsCameraUnderWater(false);
        }
    }

    al::setNerve(this, &NrvCameraPoserFollowLimitWater);
    return true;
}

/**
 * Updates the target position, smoothing its height in water.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateTargetTrans() {
    mPrevAngleH = mAngleH;
    mPrevTargetTrans = mTargetTrans;

    if (!al::isNerve(this, &NrvCameraPoserFollowLimitWater) || mIsClimbing) {
        alCameraPoserFunction::calcTargetTrans(&mTargetTrans, this);
    } else {
        sead::Vector3f targetTrans;
        alCameraPoserFunction::calcTargetTrans(&targetTrans, this);
        mTargetTrans.x = targetTrans.x;
        mTargetTrans.z = targetTrans.z;
        mTargetTrans.y = al::lerpValue(0.02f, mTargetTrans.y, targetTrans.y);
    }
}

/**
 * Moves the look at position sideways in the moving direction of the target.
 * @param rOffset The offset of the look at position from the target.
 * @param rSide The side direction of the camera.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateMovingDirOffset(const sead::Vector3f& rOffset,
                                                                        const sead::Vector3f& rSide) {
    sead::Vector3f velocity(mTargetTrans.x - mPrevTargetTrans.x, 0.0f,
                            mTargetTrans.z - mPrevTargetTrans.z);
    mTargetMoveDir = (velocity * 5.0f + mTargetMoveDir) * 0.825f;

    sead::Vector3f moveDir = mTargetMoveDir;
    al::parallelizeVec(&moveDir, rSide, moveDir);

    f32 movingDirOffset;
    if (velocity.length() > 5.0f) {
        movingDirOffset = moveDir.length() * (moveDir.dot(rSide) < 0.0f ? -1.0f : 1.0f);
        movingDirOffset =
            sead::Mathf::clamp(movingDirOffset, -mMaxMovingDirOffset, mMaxMovingDirOffset);
    } else {
        movingDirOffset =
            sead::Mathf::clamp(mMovingDirOffset, -mMaxMovingDirOffset, mMaxMovingDirOffset);
    }

    f32 prevOffset = mMovingDirOffset;
    f32 fastRate = mMovingDirOffsetChaseRate * 0.2f;
    f32 slowRate = mMovingDirOffsetChaseRate * 0.05f;
    mMovingDirOffset =
        al::lerpValue(slowRate, prevOffset, al::lerpValue(fastRate, prevOffset, movingDirOffset));

    f32 scale;
    if (mRotateSpeedH != 0.0f) {
        scale = sead::Mathf::clampMax(
            al::normalize(sead::Mathf::abs(mRotateSpeedH), 0.0f, 1.8f) * -0.05f + 1.0f, 1.0f);
    } else {
        scale = 1.0f;
    }

    if (!al::isNearZero(mAngleVerticalCtrl->getAngleSpeed(), 0.001f)) {
        f32 speedScale =
            al::normalize(sead::Mathf::abs(mAngleVerticalCtrl->getAngleSpeed()), 0.0f, 1.8f) *
                -0.05f +
            1.0f;
        scale = sead::Mathf::min(scale, speedScale);
    }

    if (mArrowCollider->isShrink()) {
        scale = std::fmin(scale, 0.76f);
    }

    mMovingDirOffset *= scale;

    if (al::isNearZero(mMovingDirOffset, 0.001f)) {
        return;
    }

    sead::Vector3f lookAtPos = mTargetTrans + rOffset;
    f32 checkOffset = mMovingDirOffset + al::sign(mMovingDirOffset) * 50.0f;
    sead::Vector3f arrow = rSide * checkOffset;
    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
    if (alCameraPoserFunction::checkFirstCameraCollisionArrow(&hitPos, nullptr, this, lookAtPos,
                                                              arrow)) {
        f32 sign = al::sign(mMovingDirOffset);
        mMovingDirOffset =
            sign * sead::Mathf::clampMin((lookAtPos - hitPos).length() + -50.0f, 0.0f);
    }
}

/**
 * Pulls the look at position towards the target when the collision is between them.
 * @param rOffset The offset of the look at position from the target.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateCollideShrink(const sead::Vector3f& rOffset) {
    CollideShrink* collideShrink = mCollideShrink;
    if (isInvalidCollider() || is140()) {
        collideShrink->isShrink = false;
        collideShrink->rate = 0.0f;
        collideShrink->targetRate = 0.0f;
        return;
    }

    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f arrow = rOffset * 2.0f;
    if (!alCameraPoserFunction::checkFirstCameraCollisionArrow(&hitPos, nullptr, this, mTargetTrans,
                                                               arrow)) {
        collideShrink->rate = al::lerpValue(0.015f, collideShrink->rate, 0.0f);
        collideShrink->targetRate = 0.0f;
        collideShrink->isShrink = false;
        return;
    }

    f32 rate = al::easeIn(
        1.0f - al::normalize((hitPos - mTargetTrans).length(), 0.0f, arrow.length()));
    collideShrink->targetRate = rate;
    collideShrink->rate = al::lerpValue(0.2f, collideShrink->rate, rate);
    f32 hitDistance = (mTargetTrans - hitPos).length();
    collideShrink->isShrink = hitDistance < (rOffset * (1.0f - collideShrink->rate)).length();
}

/**
 * Moves the look at position towards the target, slower vertically.
 * @param rLookAtPos The position to look at.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::chaseLookAtPos(const sead::Vector3f& rLookAtPos) {
    if (mIsValid2DGalaxy) {
        sead::Vector3f lookAtPos = rLookAtPos;
        if (mVerticalAbsorber2DGalaxy != nullptr) {
            sead::Vector3f targetTrans = mTargetTrans;
            mVerticalAbsorber2DGalaxy->update(this);
            mVerticalAbsorber2DGalaxy->applyLimit(&targetTrans);
            lookAtPos += targetTrans - mTargetTrans;
        }

        f32 rate = alCameraPoserFunction::isPlayerTypeHighSpeedMove(this) ? 0.6f : 0.1f;
        // the interpolated position is never applied
        al::lerpVec(&lookAtPos, lookAtPos, mAt, rate);
        return;
    }

    sead::Vector3f diff = rLookAtPos - mAt;
    sead::Vector3f diffV = {0.0f, 0.0f, 0.0f};
    sead::Vector3f gravity = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetGravity(&gravity, this);
    al::parallelizeVec(&diffV, gravity, diff);
    sead::Vector3f diffH = diff - diffV;
    sead::Vector3f at = mAt + diffV;
    f32 rate = alCameraPoserFunction::isPlayerTypeHighSpeedMove(this) ? 0.6f : 0.1f;
    mAt = at + diffH * rate;
}

/**
 * Updates the distance kept from the previous camera, stopping when it is not closer anymore.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateKeepPreCamera() {
    KeepPreCamera* keepPreCamera = mKeepPreCamera;
    if (!keepPreCamera->isValid) {
        return;
    }

    f32 angleH = mAngleH;
    f32 distanceRaw = calcDistanceRaw();
    f32 moveDistance = sead::Vector2f(mAt.x - keepPreCamera->lookAtPos.x,
                                      mAt.z - keepPreCamera->lookAtPos.z)
                           .length();
    f32 maxDistance = distanceRaw * sead::Mathf::pi();
    f32 rotateDistance =
        maxDistance *
        al::normalize(sead::Mathf::abs(al::diffNearAngleDegree(keepPreCamera->angleH, angleH)),
                      0.0f, 180.0f);
    f32 chaseDistance =
        rotateDistance *
        (1.0f -
         al::lerpValue(al::normalize(keepPreCamera->distance, 0.0f, distanceRaw), 0.3f, 1.0f));
    f32 distance =
        keepPreCamera->distance + (moveDistance > chaseDistance ? moveDistance : chaseDistance);
    if (distance < distanceRaw) {
        keepPreCamera->angleH = angleH;
        keepPreCamera->distance = distance;
        keepPreCamera->lookAtPos = mAt;
    } else {
        keepPreCamera->isValid = false;
    }
}

/**
 * Updates the horizontal angle from the camera position and the turn requests.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateAngleH() {
    sead::Vector3f cameraDir = mCameraPos - mAt;
    al::verticalizeVec(&cameraDir, mUp, cameraDir);
    if (!al::tryNormalizeOrZero(&cameraDir)) {
        f32 radH = sead::Mathf::deg2rad(mAngleH);
        cameraDir.set(sinf(radH), 0.0f, cosf(sead::Mathf::deg2rad(mAngleH)));
        al::normalize(&cameraDir);
    }

    f32 cameraAngleH = al::calcAngleOnPlaneDegree(sead::Vector3f::ez, cameraDir, mUp);
    mAngleHDiff = 0.0f;

    if (mType == Type::Follow) {
        f32 prevAngleH = mAngleH;
        if (!mIsPlessieFreezeAngleH) {
            f32 angleV = mAngleVerticalCtrl->getAngleDegree();
            f32 rate;
            if (angleV > 70.0f) {
                rate = 0.0f;
            } else if (angleV > 15.0f) {
                rate = 1.0f - al::normalize(angleV, 15.0f, 70.0f);
            } else {
                rate = 1.0f;
            }

            mAngleH = al::lerpDegree(mAngleH, cameraAngleH, mIsPlessieMode ? 0.16f : rate);
        }

        mAngleHDiff = al::diffNearAngleDegree(prevAngleH, mAngleH);

        if (mIsRequestTurn) {
            if (mTurnState == 0) {
                mTurnState = 1;
            }
        } else {
            mTurnState = 0;
        }

        mIsRequestTurn = false;

        if (mTurnState == 1 && !mTurnInfo->_1c &&
            alCameraPoserFunction::calcCameraRotateStickPower(this) > 0.3f) {
            mTurnState = 2;
        }

        if (mTurnState == 1) {
            sead::Vector3f turnDir(-mTurnInfo->mDir.x, 0.0f, -mTurnInfo->mDir.z);
            if (al::tryNormalizeOrZero(&turnDir)) {
                f32 turnAngle = sead::Mathf::rad2deg(atan2f(turnDir.x, turnDir.z));
                mAngleH = al::lerpDegree(
                    mAngleH, al::lerpDegree(mAngleH, turnAngle, mTurnInfo->_14), mTurnInfo->_18);
            }
        }
    } else if ((mType == Type::Parallel || mType == Type::Parallel2D) &&
               mIsBlendFollowAndParallel) {
        mAngleH = al::lerpDegree(mAngleH, cameraAngleH, 0.8f);
    }
}

/**
 * Turns the camera up when the target is too close to a ceiling in water.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::avoidWaterCeiling() {
    sead::LookAtCamera camera;
    makeLookAtCameraPrev(&camera);
    makeLookAtCameraPost(&camera);

    f32 angleV = sead::Mathf::clampMax(mAngleVerticalCtrl->getAngleDegree() + 5.0f, 87.5f);
    f32 radH = sead::Mathf::deg2rad(mAngleH);
    f32 sinH = sinf(radH);
    f32 cosH = cosf(radH);
    f32 radV = sead::Mathf::deg2rad(angleV);
    f32 sinV = sinf(radV);
    f32 cosV = cosf(radV);
    f32 distance = mWaterDistance->distance;
    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f hitNormal = {0.0f, 0.0f, 0.0f};
    sead::Vector3f arrow(sinH * cosV * distance, sinV * distance, cosH * cosV * distance);
    if (!alCameraPoserFunction::checkFirstCameraCollisionArrowOnlyCeiling(
            &hitPos, &hitNormal, this, camera.getAt(), arrow)) {
        return;
    }

    sead::Vector3f dir;
    calcDirByAngleHV(&dir, mAngleH, mAngleVerticalCtrl->getAngleDegree());
    al::verticalizeVec(&dir, hitNormal, dir);
    if (!al::tryNormalizeOrZero(&dir)) {
        return;
    }

    f32 ceilingAngle = al::calcAngleDegree(-mUp, dir);
    sead::Vector3f hitDiff = hitPos - camera.getAt();
    sead::Vector3f hitDiffV = {0.0f, 0.0f, 0.0f};
    al::parallelizeVec(&hitDiffV, mUp, hitDiff);
    f32 angleA = al::calcAngleDegree(-(hitDiff - hitDiffV), -dir);
    f32 angleB = al::calcAngleDegree(-dir, -hitDiff);
    f32 rate = al::normalize(angleB, 0.0f, angleA + angleB);
    f32 length = (hitDiffV * rate).length();
    f32 angle =
        sead::Mathf::rad2deg(asinf(length * sinf(sead::Mathf::deg2rad(ceilingAngle)) / distance));
    mAngleVerticalCtrl->chaseToTargetDegree(
        sead::Mathf::clampMin(ceilingAngle + -90.0f + angle + -5.0f, -87.5f));
}

/**
 * Updates the vertical angle from the stick, the sub target, the poles, the slopes and the
 * ceilings.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateAngleV() {
    sead::Vector2f stick = {0.0f, 0.0f};
    calcRotateStick(&stick, this);

    bool isParallel2D = mType == Type::Parallel2D;
    bool isSnapShotMode = alCameraPoserFunction::isSnapShotMode(this);
    f32 defaultAngleV;
    if (mIsSetAngleV) {
        defaultAngleV = mStartAngleV;
    } else if (isWaterNerve(this)) {
        defaultAngleV = 15.0f;
    } else {
        defaultAngleV = mAngleVerticalCtrl->getDefaultAngleDegree();
    }

    s32 sensitivityLevel = alCameraPoserFunction::getStickSensitivityLevel(this);
    f32 sensitivityScale = alCameraPoserFunction::getStickSensitivityScale(this);
    bool isPlayerTypeFlyer = alCameraPoserFunction::isPlayerTypeFlyer(this);
    bool isOnRideObj = alCameraPoserFunction::isOnRideObj(this);
    bool isOnGround = alCameraPoserFunction::isTargetCollideGround(this) &&
                      alCameraPoserFunction::isExistCollisionUnderTarget(this);
    const sead::Vector3f& groundNormal =
        alCameraPoserFunction::isExistCollisionUnderTarget(this) ?
            alCameraPoserFunction::getUnderTargetCollisionNormal(this) :
            sead::Vector3f::zero;
    CameraAngleUpdateInfo updateInfo(isParallel2D, isSnapShotMode, defaultAngleV, mCameraPos, mAt,
                                     mTargetTrans, mPrevTargetTrans, stick, sensitivityLevel,
                                     sensitivityScale, isPlayerTypeFlyer, isOnRideObj, isOnGround,
                                     groundNormal);

    if (alCameraPoserFunction::isSnapShotMode(this)) {
        updateInfo.setRotationScaler(mSnapShotCtrl->getRotationScaler());
    }

    if (!alCameraPoserFunction::isSnapShotMode(this)) {
        if (!mIsInvalidSubTargetTurn && alCameraPoserFunction::checkValidTurnToSubTarget(this) &&
            alCameraPoserFunction::isValidSubTargetTurnV(this)) {
            sead::Vector3f subTargetTrans = {0.0f, 0.0f, 0.0f};
            alCameraPoserFunction::calcSubTargetTrans(&subTargetTrans, this);
            f32 brakeRate = 0.0f;
            bool isBrake =
                alCameraPoserFunction::tryCalcSubTargetTurnBrakeDistanceRate(&brakeRate, this);
            f32 rate = isBrake ? 1.0f - brakeRate : 1.0f;
            sead::Vector3f subTargetDir = mAt - subTargetTrans;

            if (al::isNearZero(stick.y, 0.3f)) {
                mSubTargetTurnRestartStepV--;
            } else {
                mSubTargetTurnRestartStepV =
                    alCameraPoserFunction::getSubTargetTurnRestartStep(this);
            }

            if (al::tryNormalizeOrZero(&subTargetDir) && mSubTargetTurnRestartStepV <= 0) {
                f32 angle = sead::Mathf::rad2deg(
                    asinf(sead::Mathf::clamp(subTargetDir.y, -0.999f, 0.999f)));
                alCameraPoserFunction::clampAngleSubTargetTurnRangeV(&angle, this);
                updateInfo.chaseToSubTarget(angle, rate);
                mIsSubTargetResetAfterTurnV =
                    alCameraPoserFunction::isValidSubTargetResetAfterTurnV(this);
            }
        } else {
            if (mIsSubTargetResetAfterTurnV) {
                mAngleVerticalCtrl->startTargetInterpole(
                    mIsSetAngleV ? mStartAngleV : mAngleVerticalCtrl->getDefaultAngleDegree());
                mIsSubTargetResetAfterTurnV = false;
            }

            mSubTargetTurnRestartStepV--;
        }
    }

    bool isClimbPole;
    if (mIsClimbPole) {
        isClimbPole = !alCameraPoserFunction::isTargetCollideGround(this);
    } else {
        isClimbPole = alCameraPoserFunction::isTargetClimbPole(this);
    }

    if (mIsSetClimbPoleAngleV && isClimbPole != mIsClimbPole) {
        if (isClimbPole) {
            mPreClimbPoleAngleV = mAngleVerticalCtrl->getAngleDegree();
            mAngleVerticalCtrl->startTargetInterpole(mClimbPoleAngleV);
        } else {
            mAngleVerticalCtrl->startTargetInterpole(mPreClimbPoleAngleV);
        }
    }

    mIsClimbPole = isClimbPole;

    if (mType == Type::Follow && !mIsInvalidSlopeSnapAngleV) {
        if (alCameraPoserFunction::isExistSlopeCollisionUnderTarget(this) &&
            alCameraPoserFunction::isTargetCollideGround(this)) {
            sead::Vector3f slopeDir = {0.0f, 0.0f, 0.0f};
            if (alCameraPoserFunction::tryCalcSlopeCollisionDownFrontDirH(&slopeDir, this)) {
                f32 radH = sead::Mathf::deg2rad(mAngleH);
                sead::Vector3f cameraDirH(sinf(radH), 0.0f, cosf(sead::Mathf::deg2rad(mAngleH)));

                if (mSlopeSnapState != 1 &&
                    alCameraPoserFunction::getSlopeCollisionDownSpeed(this) > 15.0f &&
                    al::calcAngleDegree(-cameraDirH, slopeDir) < 180.0f) {
                    mAngleVerticalCtrl->startSnap(
                        al::calcAngleDegree(
                            sead::Vector3f::ey,
                            alCameraPoserFunction::getUnderTargetCollisionNormal(this)) *
                            0.6f +
                        mAngleVerticalCtrl->getDefaultAngleDegree());
                    mSlopeSnapState = 1;
                }

                if (mSlopeSnapState != 2 &&
                    alCameraPoserFunction::getSlopeCollisionUpSpeed(this) > 12.0f &&
                    al::calcAngleDegree(cameraDirH, slopeDir) < 45.0f) {
                    mAngleVerticalCtrl->startSnap(
                        mAngleVerticalCtrl->getDefaultAngleDegree() +
                        al::calcAngleDegree(
                            sead::Vector3f::ey,
                            alCameraPoserFunction::getUnderTargetCollisionNormal(this)) *
                            -0.8f);
                    mSlopeSnapState = 2;
                }
            }
        } else {
            if (mSlopeSnapState != 0) {
                mAngleVerticalCtrl->endSnap();
            }

            mSlopeSnapState = 0;
        }
    }

    if (isWaterNerve(this) && !mIsInvalidInWater) {
        avoidWaterCeiling();
    }

    bool isClimbing = alCameraPoserFunction::isPlayerClimbing(this);
    updateInfo.setIsFreezeAngle(isClimbing ? false : mIsClimbing);

    sead::Vector3f eye = mEye;
    if (al::isInInk(this, eye, 180.0f)) {
        if (mAngleVerticalCtrl->getAngleDegree() < 12.0f) {
            mAngleVerticalCtrl->setAngleDegree(12.0f);
        }

        updateInfo.setIsInInk(true);
    }

    mAngleVerticalCtrl->update(updateInfo);
    mIsClimbing = isClimbing;
}

/**
 * Updates the interpolation of the distance after a distance curve change or a distance
 * request.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateDistanceInterpole() {
    const al::CameraDistanceCurve* requestDistanceCurve =
        alCameraPoserFunction::tryGetBossDistanceCurve(this);
    if (requestDistanceCurve == nullptr) {
        requestDistanceCurve = alCameraPoserFunction::tryGetEquipmentDistanceCurve(this);
    }

    if (requestDistanceCurve != mRequestDistanceCurve) {
        mRequestDistanceCurve = requestDistanceCurve;
        mDistanceInterpole->start(mDistance);
    }

    DistanceInterpole* distanceInterpole = mDistanceInterpole;
    distanceInterpole->step++;
    if (distanceInterpole->step >= distanceInterpole->stepMax) {
        distanceInterpole->reset();
    }

    f32 requestDistance = -1.0f;
    bool isRequest = alCameraPoserFunction::tryGetTargetRequestDistance(&requestDistance, this);
    distanceInterpole = mDistanceInterpole;
    if (isRequest && requestDistance > 0.0f) {
        f32 distance = calcDistanceRaw();
        if (distanceInterpole->requestDistance < 0.0f) {
            distanceInterpole->requestDistance = mDistance;
        }

        f32 target = al::lerpValue(0.3f, distanceInterpole->requestDistance, distance);
        distanceInterpole->requestDistance =
            al::lerpValue(0.1f, distanceInterpole->requestDistance, target);
    } else if (distanceInterpole->requestDistance > 0.0f) {
        distanceInterpole->start(distanceInterpole->requestDistance);
    }
}

/**
 * Keeps the camera above the water surface when the look at position is above it.
 */
inline ALWAYS_INLINE void CameraPoserFollowLimit::updateWaterSurface() {
    WaterSurface* waterSurface = mWaterSurface;
    waterSurface->isFound = false;
    waterSurface->cameraFinder->update(mEye, mUp);
    waterSurface->lookAtFinder->update(mAt, mUp);
    if (!waterSurface->cameraFinder->isWaterSurfaceExist()) {
        return;
    }

    sead::Vector3f checkPos(mEye.x, mEye.y + -50.0f, mEye.z);
    if (rc::isInWaterArea(waterSurface->areaUser, checkPos)) {
        waterSurface->surfacePos = waterSurface->cameraFinder->getWaterSurfacePosition() +
                                   sead::Vector3f(0.0f, 50.0f, 0.0f);
        waterSurface->isFound = true;
        return;
    }

    sead::Vector3f deepCheckPos(mEye.x, mEye.y + -150.0f, mEye.z);
    if (rc::isInWaterArea(waterSurface->areaUser, deepCheckPos)) {
        if (mEye.y > 0.0f) {
            waterSurface->surfacePos = mEye;
        } else {
            waterSurface->surfacePos = waterSurface->cameraFinder->getWaterSurfacePosition() +
                                       sead::Vector3f(0.0f, 50.0f, 0.0f);
        }

        waterSurface->isFound = true;
    }
}

/**
 * Moves the look at position and the camera after the target.
 */
void CameraPoserFollowLimit::movement() {
    if (isCalcEndAfterInterpole() && mType == Type::Parallel2D) {
        return;
    }

    updateTargetTrans();

    bool isFollowExact = alCameraPoserFunction::isTargetFollowExact(this);
    sead::Vector3f offset = calcLookAtOffset(mIsValid2DGalaxy);
    sead::Vector3f side = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcSideDir(&side, this);

    if (!alCameraPoserFunction::isSnapShotMode(this) &&
        !al::isNerve(this, &NrvCameraPoserFollowLimitResetAngle) &&
        !(al::isNerve(this, &NrvCameraPoserFollowLimitResetAngleWater) || isFollowExact)) {
        updateMovingDirOffset(offset, side);
    }

    if (mCollideShrink != nullptr) {
        updateCollideShrink(offset);
    }

    sead::Vector3f sideOffset = side * mMovingDirOffset;
    f32 offsetRate = mCollideShrink != nullptr ? 1.0f - mCollideShrink->rate : 1.0f;
    mLookAtPos = mTargetTrans + offset * offsetRate;
    sead::Vector3f lookAtPos = sideOffset + mLookAtPos;

    if (isFollowExact || alCameraPoserFunction::isSnapShotMode(this)) {
        mAt = lookAtPos;
    } else {
        chaseLookAtPos(lookAtPos);
    }

    updateKeepPreCamera();
    updateAngleH();

    if (mLimitRailKeeper != nullptr) {
        mLimitRailKeeper->updateRider(this);
    }

    CameraPoser_RS::movement();
    updateAngleV();

    if (!is141()) {
        updateDistanceInterpole();
    }

    bool isWater = al::isNerve(this, &NrvCameraPoserFollowLimitWater);
    sead::Vector3f dir;
    calcDirByAngleHV(&dir, mAngleH, mAngleVerticalCtrl->getAngleDegree());
    f32 distance;
    if (isWater) {
        distance = al::lerpValue(0.02f, mDistance, calcDistance());
    } else {
        distance = calcDistance();
    }

    mCurrentDistance = distance;
    mEye = mAt + dir * distance;
    updateWaterSurface();

    if (mWaterSurface->isFound) {
        mEye = mWaterSurface->surfacePos;
    }

    mDistance = mCurrentDistance;
    mCameraPos = mEye;

    sead::LookAtCamera camera;
    makeLookAtCameraPrev(&camera);
    makeLookAtCameraPost(&camera);
    sead::LookAtCamera interpoleCamera = camera;
    applyPrevPoseInterpole(mPrevPose, &interpoleCamera);

    if (alCameraPoserFunction::isChangeTarget(this)) {
        mPrevPose->requestInterpole(30);
    }

    mPrevPose->update(getFovyDegree(), interpoleCamera);

    PrevPose* prevPose = mPrevPose;
    if (prevPose->step >= 1) {
        f32 defaultFovy = mDefaultFovy;
        mFovyDegree = al::lerpValue(
            al::normalize((f32)(prevPose->stepMax - prevPose->step), 0.0f, (f32)prevPose->stepMax),
            prevPose->fovy, defaultFovy);
    }

    if (!isInvalidCollider() && !isCalcEndAfterInterpole()) {
        applyPrevPoseInterpole(mPrevPose, &camera);
        makeLookAtCameraLast(&camera);
        mArrowCollider->update(camera.getPos(), camera.getAt(), camera.getUp());

        sead::Vector3f inkCheckPos = mEye;
        sead::Vector3f inkCheckArrow = {0.0f, -180.0f, 0.0f};
        f32 inkDepth;
        // the position raised out of the ink is never applied
        if (al::isInInk(this, inkCheckPos, inkCheckArrow, &inkDepth)) {
            inkCheckPos.y += inkDepth;
        }
    }
}

/**
 * Turns the camera horizontally with the stick, or clamps the angle of a parallel camera.
 */
void CameraPoserFollowLimit::update() {
    AngleHLimit* angleHLimit = mAngleHLimit;
    if (angleHLimit != nullptr && angleHLimit->isClamp()) {
        f32 rotateSpeedH = mRotateSpeedH;
        const sead::Matrix34f& viewMtx = mViewMtx;
        bool isResetAngle = al::isNerve(this, &NrvCameraPoserFollowLimitResetAngle) ||
                            al::isNerve(this, &NrvCameraPoserFollowLimitResetAngleWater);
        f32 offset = rotateSpeedH + angleHLimit->offset;
        if (!isResetAngle) {
            offset = al::lerpValue(0.6f, angleHLimit->offset, offset);
        }

        angleHLimit->offset = offset;
        angleHLimit->offset = clampAngleH(angleHLimit, angleHLimit->offset);
        f32 baseAngle = clampAngleH(
            angleHLimit, alCameraPoserFunction::calcZoneInvRotateAngleH(mAngleH, viewMtx));
        mAngleH = alCameraPoserFunction::calcZoneRotateAngleH(
            al::lerpValue(0.2f, baseAngle, angleHLimit->offset), viewMtx);
        return;
    }

    if (alCameraPoserFunction::calcCameraRotateStickPower(this) >
            CameraPoserFollowLimitFunction::getRotateStickThreshold() &&
        mAngleHDiff * mRotateSpeedH < 0.0f) {
        f32 diff = al::diffNearAngleDegree(mPrevAngleH, mAngleH);
        f32 back;
        if (diff * mAngleHDiff < 0.0f) {
            back = 0.0f;
        } else {
            back = (mAngleHDiff * mAngleHDiff < diff * diff ? mAngleHDiff : diff) * 0.8f;
        }

        mAngleH = al::wrapAngle(mAngleH - back);
    }

    mAngleH = al::wrapAngle(mAngleH + mRotateSpeedH);
}

/**
 * Makes the camera from the current pose, interpolated from the previous pose and pushed by
 * the collision.
 * @param pCamera The camera to make.
 */
void CameraPoserFollowLimit::calcCameraPose(sead::LookAtCamera* pCamera) const {
    makeLookAtCameraPrev(pCamera);
    makeLookAtCameraPost(pCamera);
    applyPrevPoseInterpole(mPrevPose, pCamera);

    if (!isInvalidCollider()) {
        mArrowCollider->makeLookAtCamera(pCamera);
    }

    makeLookAtCameraLast(pCamera);
}

/**
 * Receives a vertical angle or reset request from an object.
 * @param rInfo The request.
 * @return True if the request was handled.
 */
bool CameraPoserFollowLimit::receiveRequestFromObject(const al::CameraObjectRequestInfo& rInfo) {
    if (alCameraPoserFunction::isRequestSetAngleV(rInfo)) {
        if (mIsInvalidRequestSetAngleV) {
            return false;
        }

        if (alCameraPoserFunction::isExistSubTarget(this) &&
            alCameraPoserFunction::isValidSubTargetTurnV(this)) {
            return false;
        }

        mAngleVerticalCtrl->startTargetInterpole(alCameraPoserFunction::getRequestAngleV(rInfo));
        return true;
    }

    if (alCameraPoserFunction::isRequestResetPosition(rInfo)) {
        al::CameraStartInfo startInfo;
        startInfo.isValidResetPreCameraPose = true;
        appear(startInfo);
        return true;
    }

    if (alCameraPoserFunction::isRequestResetAngleV(rInfo)) {
        mAngleVerticalCtrl->startResetInterpole();
        return true;
    }

    if (alCameraPoserFunction::isRequestDownToDefaultAngleBySpeed(rInfo) &&
        mAngleVerticalCtrl->getDefaultAngleDegree() < mAngleVerticalCtrl->getAngleDegree()) {
        CameraAngleVerticalCtrl* angleVerticalCtrl = mAngleVerticalCtrl;
        f32 speed = alCameraPoserFunction::getRequestAngleSpeed(rInfo);
        angleVerticalCtrl->chaseToTargetDegreeBySpeed(angleVerticalCtrl->getDefaultAngleDegree(),
                                                      speed);
        return true;
    }

    if (alCameraPoserFunction::isRequestUpToTargetAngleBySpeed(rInfo) &&
        mAngleVerticalCtrl->getAngleDegree() < alCameraPoserFunction::getRequestTargetAngleV(rInfo)) {
        mAngleVerticalCtrl->chaseToTargetDegreeBySpeed(
            alCameraPoserFunction::getRequestTargetAngleV(rInfo),
            alCameraPoserFunction::getRequestAngleSpeed(rInfo));
        return true;
    }

    if (alCameraPoserFunction::isRequestMoveDownAngleV(rInfo)) {
        CameraAngleVerticalCtrl* angleVerticalCtrl = mAngleVerticalCtrl;
        if (mAngleVerticalCtrl->getDefaultAngleDegree() < mAngleVerticalCtrl->getAngleDegree()) {
            angleVerticalCtrl = mAngleVerticalCtrl;
            angleVerticalCtrl->chaseToTargetDegree(angleVerticalCtrl->getDefaultAngleDegree());
        }

        return true;
    }

    return false;
}

/**
 * Freezes the look at position and the vertical absorb for the snapshot mode.
 */
void CameraPoserFollowLimit::startSnapShotMode() {
    mAt = mLookAtPos;
    mMovingDirOffset = 0.0f;

    sead::LookAtCamera camera;
    calcCameraPose(&camera);
    alCameraPoserFunction::stopUpdateVerticalAbsorbForSnapShotMode(this, camera.getAt());
}

/**
 * Restarts the vertical absorb, interpolating back if the look at position was not moved.
 */
void CameraPoserFollowLimit::endSnapShotMode() {
    alCameraPoserFunction::restartUpdateVerticalAbsorb(this);

    sead::LookAtCamera camera;
    calcCameraPose(&camera);

    if (al::isNearZero(alCameraPoserFunction::getSnapShotLookAtOffset(this), 0.001f)) {
        mPrevPose->requestInterpole(30);
    }
}

/**
 * Checks whether the stick can rotate the camera, which a fixed parallel camera forbids.
 * @return True if the camera can be rotated.
 */
bool CameraPoserFollowLimit::isEnableRotateByPad() const {
    if ((mType == Type::Parallel || mType == Type::Parallel2D) &&
        mAngleVerticalCtrl->isFixInRange() && mAngleHLimit->isClamp()) {
        if (mAngleHLimit->isOffset) {
            if (al::isNear(mAngleHLimit->minOffset, mAngleHLimit->maxOffset, 0.001f)) {
                return false;
            }
        } else if (al::isNear(mAngleHLimit->minAngle, mAngleHLimit->maxAngle, 0.001f)) {
            return false;
        }
    }

    return true;
}

/**
 * Starts resetting the camera angle.
 * @param isReset Whether the angle is set at once instead of being interpolated.
 */
void CameraPoserFollowLimit::startCameraReset(bool isReset) {
    if (al::isNerve(this, &NrvCameraPoserFollowLimitResetAngle)) {
        return;
    }

    al::setNerve(this, &NrvCameraPoserFollowLimitResetAngle);
    mIsResetByRequest = isReset;
}

/**
 * Gets the vertical angle of the camera.
 * @return The angle in degrees.
 */
f32 CameraPoserFollowLimit::getVerticalAngle() {
    return mAngleVerticalCtrl->getAngleDegree();
}

/**
 * Turns the plessie mode on for the plessie camera and limits its vertical angle.
 * @param min Minimum vertical angle in degrees.
 * @param max Maximum vertical angle in degrees.
 */
void CameraPoserFollowLimit::setPlessieCameraOn(f32 min, f32 max) {
    if (sPlessieCamera == nullptr) {
        sPlessieCamera = this;
    }

    sPlessieCamera->setPlessieMode(true);
    sPlessieCamera->setVerticalAngleRange(min, max);
}

/**
 * Turns the plessie mode on or off, keeping the vertical angle range to restore it later.
 * @param isPlessie Whether the plessie mode is on.
 */
void CameraPoserFollowLimit::setPlessieMode(bool isPlessie) {
    if (mIsPlessieMode == isPlessie) {
        return;
    }

    mIsPlessieMode = isPlessie;
    if (isPlessie) {
        mAngleVerticalCtrl->storeVerticalAngleRange();
    } else {
        mType = Type::Follow;
        mAngleVerticalCtrl->restoreVerticalAngleRange();
    }
}

/**
 * Sets the range of the vertical angle.
 * @param min Minimum vertical angle in degrees.
 * @param max Maximum vertical angle in degrees.
 */
void CameraPoserFollowLimit::setVerticalAngleRange(f32 min, f32 max) {
    mAngleVerticalCtrl->setVerticalAngleRange(min, max);
}

/**
 * Turns the plessie mode off for the plessie camera.
 */
void CameraPoserFollowLimit::setPlessieCameraOff() {
    if (sPlessieCamera != nullptr) {
        sPlessieCamera->setPlessieMode(false);
    }
}

/**
 * Follows the target, turning with the stick.
 */
void CameraPoserFollowLimit::exeFollow() {
    if (al::isFirstStep(this)) {
        mRotateSpeedInputH = 0.0f;
        mSubTargetTurnRate = 0.0f;
    }

    if (!al::isNerve(this, &NrvCameraPoserFollowLimitResetAngle) &&
        tryStartResetAngleByTrigger(this)) {
        mIsResetByTrigger = true;
        return;
    }

    if (trySwitchLimitObj()) {
        return;
    }

    if (tryStartWater(false)) {
        return;
    }

    updateInputOrSubTargetTurnH();
}

/**
 * Switches to the nearest limit rail, or back to the plain follow.
 * @return True if the limit rail changed.
 */
bool CameraPoserFollowLimit::trySwitchLimitObj() {
    al::CameraLimitRailKeeper* limitRailKeeper =
        alCameraPoserFunction::tryFindNearestLimitRailKeeper(this, mTargetTrans);
    if (limitRailKeeper == mLimitRailKeeper) {
        return false;
    }

    if (limitRailKeeper != nullptr && limitRailKeeper->isApplyAngleElevation()) {
        if (limitRailKeeper->isFixedAngle()) {
            mAngleVerticalCtrl->setRailAngleDegreeRangeAndInterp(
                limitRailKeeper->getAngleElevation(), limitRailKeeper->getAngleElevation2(),
                limitRailKeeper->getAngleElevationIterpStep());
        } else {
            mAngleVerticalCtrl->startTargetInterpoleByStep(
                limitRailKeeper->getAngleElevation(),
                limitRailKeeper->getAngleElevationIterpStep());
        }
    } else {
        mAngleVerticalCtrl->resetRailAngleDegreeRange();
        al::CameraLimitRailKeeper* prevLimitRailKeeper = mLimitRailKeeper;
        if (prevLimitRailKeeper != nullptr && prevLimitRailKeeper->isResetAngleElevation()) {
            CameraAngleVerticalCtrl* angleVerticalCtrl = mAngleVerticalCtrl;
            if (mIsSetAngleV) {
                angleVerticalCtrl->startTargetInterpoleByStep(
                    mStartAngleV, prevLimitRailKeeper->getAngleElevationResetStep());
            } else {
                s32 step = prevLimitRailKeeper->getAngleElevationResetStep();
                angleVerticalCtrl->startTargetInterpoleByStep(
                    angleVerticalCtrl->getDefaultAngleDegree(), step);
            }
        }
    }

    mLimitRailKeeper = limitRailKeeper;
    if (limitRailKeeper != nullptr) {
        al::setNerve(this, &NrvCameraPoserFollowLimitFollowRail);
        return true;
    }

    if (al::isNerve(this, &NrvCameraPoserFollowLimitFollow)) {
        return false;
    }

    if (!al::isNerve(this, &NrvCameraPoserFollowLimitFollowRailUserCtrl)) {
        startTurnBrake(30);
    }

    al::setNerve(this, &NrvCameraPoserFollowLimitFollow);
    return true;
}

/**
 * Updates the horizontal rotation speed from the stick, or turns towards the sub target.
 */
void CameraPoserFollowLimit::updateInputOrSubTargetTurnH() {
    if (getRequestTurnDisablePlayerInput()) {
        return;
    }

    if (alCameraPoserFunction::isSnapShotMode(this) || mIsInvalidSubTargetTurn) {
        mSubTargetTurnRate = 0.0f;
        updateRotateSpeedInputH();
        return;
    }

    bool isValidTurn = alCameraPoserFunction::checkValidTurnToSubTarget(this);
    if (!al::isNearZero(alCameraPoserFunction::calcCameraRotateStickH(this),
                        CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        if (isValidTurn && mSubTargetTurnRestartStep <= 0) {
            mRotateSpeedH = 0.0f;
            mSubTargetTurnRestartStep = alCameraPoserFunction::getSubTargetTurnRestartStep(this);
        }

        mSubTargetTurnRate = 0.0f;
        mIsTurnBrake = false;
    }

    if (isCalcEndAfterInterpole()) {
        updateRotateSpeedInputH();
        return;
    }

    if (alCameraPoserFunction::isChangeSubTarget(this) && !isValidTurn) {
        startTurnBrake(30);
        mSubTargetTurnRate = 0.0f;
        return;
    }

    if (isValidTurn && mSubTargetTurnRestartStep <= 0) {
        f32 turnRate = mSubTargetTurnRate;
        mIsTurnBrake = false;
        mSubTargetTurnRate = al::lerpValue(
            0.1f, turnRate,
            al::lerpValue(CameraPoserFollowLimitFunction::getRotateStickThreshold(), turnRate,
                          1.0f));

        sead::Vector3f subTargetTrans = {0.0f, 0.0f, 0.0f};
        alCameraPoserFunction::calcSubTargetTrans(&subTargetTrans, this);
        sead::Vector3f subTargetDirH = subTargetTrans - mTargetTrans;
        al::verticalizeVec(&subTargetDirH, mUp, subTargetDirH);
        sead::Vector3f subTargetDir = subTargetDirH;

        if (al::tryNormalizeOrZero(&subTargetDir)) {
            f32 brakeRate = 0.0f;
            alCameraPoserFunction::tryCalcSubTargetTurnBrakeDistanceRate(&brakeRate, this);
            f32 diff = al::diffNearAngleDegree(
                mAngleH, sead::Mathf::rad2deg(atan2f(-subTargetDir.x, -subTargetDir.z)));
            f32 speed = sead::Mathf::clamp(
                            diff * alCameraPoserFunction::getSubTargetTurnSpeedRate1(this), -10.0f,
                            10.0f) *
                        mSubTargetTurnRate;
            mRotateSpeedH = speed * alCameraPoserFunction::getSubTargetTurnSpeedRate2(this) *
                            (1.0f - brakeRate);
        } else {
            mRotateSpeedH *= 0.9f;
        }

        return;
    }

    if (mIsTurnBrake) {
        f32 brakeSpeed = mTurnBrakeSpeed;
        f32 rate =
            1.0f - al::normalize((f32)mTurnBrakeStep, 0.0f, (f32)mTurnBrakeStepMax);
        mTurnBrakeStep++;
        mRotateSpeedH = brakeSpeed * rate;
        if (mTurnBrakeStep >= mTurnBrakeStepMax) {
            mIsTurnBrake = false;
        }

        return;
    }

    updateRotateSpeedInputH();
    if (!alCameraPoserFunction::isPause(this) && mSubTargetTurnRestartStep > 0) {
        mSubTargetTurnRestartStep--;
    }
}

/**
 * Turns the camera to its reset angle.
 */
void CameraPoserFollowLimit::exeResetAngle() {
    if (al::isFirstStep(this)) {
        mTargetMoveDir = {0.0f, 0.0f, 0.0f};

        f32 diff;
        if (mLimitRailKeeper != nullptr) {
            sead::Vector3f dirH = {0.0f, 0.0f, 0.0f};
            sead::Vector3f railDirH = {0.0f, 0.0f, 0.0f};
            alCameraPoserFunction::calcCameraDirH(&dirH, this);
            mLimitRailKeeper->calcCameraDirH(&railDirH, this);
            diff = al::calcAngleOnPlaneDegree(dirH, railDirH, mUp);
        } else {
            if (mResetAngleHParam->isValid) {
                diff = al::diffNearAngleDegree(mAngleH, mResetAngleHParam->calcZoneAngle());
            } else if (mType == Type::Parallel2D ||
                       (mAngleHLimit != nullptr && mAngleHLimit->isClamp() &&
                        mStartAngleHParam->isValid)) {
                diff = al::diffNearAngleDegree(mAngleH, mStartAngleHParam->calcZoneAngle());
            } else {
                diff = calcAngleToTargetBackH();
            }
        }

        f32 absDiff = diff < 0.0f ? -diff : diff;
        s32 step = absDiff < 8.0f ? 0 : (s32)(absDiff * 0.125f);
        step = step > 12 ? step : 12;
        if (mIsResetByRequest) {
            mRotateSpeedH = diff;
            step = 1;
            mIsResetByRequest = false;
        } else if (mIsSnapShotRollFollow) {
            step = (s32)((f32)step + (f32)step);
        }

        ResetAngle* resetAngle = mResetAngle;
        resetAngle->step = step;
        resetAngle->angle = diff;
        resetAngle->startAngle = diff;
        resetAngle->movingDirOffset = mMovingDirOffset;

        if (alCameraPoserFunction::isSnapShotMode(this)) {
            alCameraPoserFunction::startResetSnapShotCameraCtrl(this, step);
        }
    }

    if (mIsResetByTrigger && mIsSnapShotRollFollow &&
        getNerveKeeper()->getCurrentStep() == 10) {
        f32 angle = calcAngleToTargetBackH();
        if (!al::isNearZero(angle - mResetAngle->startAngle, 10.0f)) {
            al::setNerve(this, &NrvCameraPoserFollowLimitResetAngle);
            return;
        }
    }

    ResetAngle* resetAngle = mResetAngle;
    s32 step = al::getNerveStep(this);
    if (step >= 0 && resetAngle->step > step + 1) {
        f32 prevRate = al::squareOut(al::normalize((f32)step, 0.0f, (f32)resetAngle->step));
        f32 rate = al::squareOut(al::normalize((f32)(step + 1), 0.0f, (f32)resetAngle->step));
        mRotateSpeedH = rate * resetAngle->angle - prevRate * resetAngle->angle;
        mMovingDirOffset = (1.0f - rate) * resetAngle->movingDirOffset;
    }

    if (!al::isGreaterEqualStep(this, mResetAngle->step - 1) &&
        al::isNearZero(calcRotateSpeedH(this, mAngleVerticalCtrl->getAngleDegree()), 0.001f)) {
        return;
    }

    if (al::isNerve(this, &NrvCameraPoserFollowLimitResetAngleWater)) {
        bool isInWater = alCameraPoserFunction::isTargetInWater(this);
        mIsResetByTrigger = false;
        if (isInWater) {
            al::setNerve(this, &NrvCameraPoserFollowLimitWater);
        } else {
            endWater();
        }

        return;
    }

    if (tryStartWater(false)) {
        return;
    }

    if (trySwitchLimitObj()) {
        return;
    }

    if (mLimitRailKeeper != nullptr) {
        mIsResetByTrigger = false;
        al::setNerve(this, &NrvCameraPoserFollowLimitFollowRail);
        return;
    }

    // the angle to the back of the target is calculated but not used
    calcAngleToTargetBackH();
    mIsResetByTrigger = false;
    al::setNerve(this, &NrvCameraPoserFollowLimitFollow);
}

/**
 * Stops following the target in water.
 */
void CameraPoserFollowLimit::endWater() {
    alCameraPoserFunction::onVerticalAbsorb(this);
    mPrevPose->requestInterpole(60);
    mAngleVerticalCtrl->setIsWaterCtrl(false);

    if (!mIsInvalidWaterAutoCtrlAngleV) {
        mAngleVerticalCtrl->startUserCtrl();
    }

    if (!mIsInvalidOutWaterResetAngleV) {
        mAngleVerticalCtrl->startTargetInterpoleByStep(mOutWaterResetAngleV, 60);
    }

    al::setNerve(this, &NrvCameraPoserFollowLimitFollow);
    mAngleVerticalCtrl->setIsCameraUnderWater(false);
    mWaterDistance->isValid = false;
}

/**
 * Follows the target along the limit rail, turning towards the rail direction.
 */
void CameraPoserFollowLimit::exeFollowRail() {
    if (al::isFirstStep(this)) {
        mRotateSpeedH = 0.0f;
        mRotateSpeedInputH = 0.0f;
        mRotateSpeedRailH = 0.0f;
    }

    if (tryStartResetAngleByTrigger(this)) {
        return;
    }

    if (trySwitchLimitObj()) {
        return;
    }

    al::CameraLimitRailKeeper* limitRailKeeper = mLimitRailKeeper;
    sead::Vector3f dirH = sead::Vector3f::ez;
    alCameraPoserFunction::calcCameraDirH(&dirH, this);
    sead::Vector3f railDirH = sead::Vector3f::ez;
    limitRailKeeper->calcCameraDirH(&railDirH, this);
    f32 angle = al::calcAngleOnPlaneDegree(dirH, railDirH, mUp);

    f32 margin = mLimitRailKeeper->getDegreeMargin() + 0.0f;
    f32 speed;
    if (margin >= angle && angle >= -margin) {
        speed = al::lerpValue(0.015f, mRotateSpeedH, 0.0f);
    } else {
        f32 sign = al::sign(angle);
        f32 rate = al::lerpValue(
            al::normalize(angle * sign - mLimitRailKeeper->getDegreeMargin(), 0.0f, 40.0f),
            0.025f, 1.5f);
        if (al::isNearZero(rate, 0.001f)) {
            speed = al::lerpValue(0.015f, mRotateSpeedRailH, 0.0f);
        } else {
            speed = al::converge(mRotateSpeedRailH, sign * rate, 0.1f);
        }
    }

    mRotateSpeedRailH = speed;
    mRotateSpeedH = speed;

    f32 inputSpeed = calcRotateSpeedH(this, mAngleVerticalCtrl->getAngleDegree());
    if (!al::isNearZero(inputSpeed, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        if (inputSpeed * mRotateSpeedH < 0.0f) {
            mRotateSpeedH = 0.0f;
        }

        al::setNerve(this, &NrvCameraPoserFollowLimitFollowRailUserCtrl);
    }
}

/**
 * Follows the target along the limit rail, turning with the stick.
 */
void CameraPoserFollowLimit::exeFollowRailUserCtrl() {
    if (al::isFirstStep(this)) {
        mRotateSpeedInputH = 0.0f;
    }

    if (tryStartResetAngleByTrigger(this)) {
        return;
    }

    if (trySwitchLimitObj()) {
        return;
    }

    al::CameraLimitRailKeeper* limitRailKeeper = mLimitRailKeeper;
    if (!limitRailKeeper->isFixedAngle()) {
        updateRotateSpeedInputH();
        return;
    }

    f32 margin = limitRailKeeper->getDegreeMargin();
    sead::Vector3f dirH = sead::Vector3f::ez;
    alCameraPoserFunction::calcCameraDirH(&dirH, this);
    sead::Vector3f railDirH = sead::Vector3f::ez;
    limitRailKeeper->calcCameraDirH(&railDirH, this);
    f32 angle = al::calcAngleOnPlaneDegree(dirH, railDirH, mUp);
    f32 sign = al::sign(angle);
    f32 rate = al::normalize(angle * sign - margin, 0.0f, 40.0f) * 1.5f;
    if (al::isNearZero(rate, 0.001f)) {
        mRotateSpeedRailH = al::lerpValue(0.015f, mRotateSpeedRailH, 0.0f);
    } else {
        mRotateSpeedRailH = al::converge(mRotateSpeedRailH, sign * rate, 0.1f);
    }

    f32 inputSpeed = calcRotateSpeedH(this, mAngleVerticalCtrl->getAngleDegree());
    f32 rotateSpeedInputH;
    if (al::isNearZero(inputSpeed, CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        rotateSpeedInputH = mRotateSpeedInputH * 0.9f;
    } else {
        if (angle * inputSpeed < 0.0f) {
            inputSpeed = inputSpeed * 1.5f *
                         (1.0f - al::normalize(sead::Mathf::abs(angle), 0.0f, margin));
        }

        rotateSpeedInputH = inputSpeed + mRotateSpeedInputH;
    }

    mRotateSpeedInputH = rotateSpeedInputH;
    f32 speed = rotateSpeedInputH * 0.2f * 0.5f;
    mRotateSpeedInputH = rotateSpeedInputH - speed;

    if (sead::Mathf::abs(speed) < sead::Mathf::abs(mRotateSpeedRailH)) {
        mRotateSpeedH = mRotateSpeedRailH;
    } else {
        mRotateSpeedH = speed;
    }
}

/**
 * Updates the horizontal rotation speed from the stick.
 */
void CameraPoserFollowLimit::updateRotateSpeedInputH() {
    f32 inputSpeed = calcRotateSpeedH(this, mAngleVerticalCtrl->getAngleDegree());
    if (al::isNearZero(inputSpeed, 0.001f)) {
        mRotateSpeedInputH *= 0.9f;
        if (mIsPlessieMode && mType == Type::Parallel) {
            mPlessieStep--;
            if (mPlessieStep == 0) {
                mType = Type::Follow;
            }
        }
    } else {
        if (mAngleHLimit != nullptr && mAngleHLimit->isClamp()) {
            inputSpeed *= 1.5f;
        }

        if (alCameraPoserFunction::isSnapShotMode(this)) {
            inputSpeed *= mSnapShotCtrl->getRotationScaler();
        }

        mRotateSpeedInputH = inputSpeed + mRotateSpeedInputH;
        if (mIsPlessieMode) {
            mPlessieStep = 20;
            mType = Type::Parallel;
        }
    }

    f32 rotateSpeedInputH = mRotateSpeedInputH;
    f32 speed = rotateSpeedInputH * 0.2f * 0.5f;
    mRotateSpeedH = speed;
    mRotateSpeedInputH = rotateSpeedInputH - speed;
}

/**
 * Follows the target in water, keeping the camera behind it.
 */
void CameraPoserFollowLimit::exeWater() {
    if (al::isFirstStep(this)) {
        mWaterRotateSpeedH = 0.0f;
        mWaterRotateRate = 0.0f;
    }

    sead::Vector3f velocity = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetVelocity(&velocity, this);
    f32 requestDistance = -1.0f;
    alCameraPoserFunction::tryGetTargetRequestDistance(&requestDistance, this);
    f32 angleV = mAngleVerticalCtrl->getAngleDegree();
    bool isUnderWater = mAngleVerticalCtrl->isUnderWater();
    WaterDistance* waterDistance = mWaterDistance;
    f32 speedH = sead::Vector2f(velocity.x, velocity.z).length();

    if (requestDistance > 0.0f) {
        waterDistance->distance = requestDistance;
    } else {
        f32 speedRate = al::normalize(speedH, 1.0f, 50.0f);
        f32 farDistance = al::lerpValue(speedRate, 600.0f, 1500.0f);
        if (isUnderWater) {
            f32 distance = (mAt - mCameraPos).length();
            f32 clamped = sead::Mathf::clamp(
                distance, sead::Mathf::max(waterDistance->defaultDistance + -250.0f, 1.0f),
                waterDistance->defaultDistance + 250.0f);
            waterDistance->targetDistance = al::lerpValue(0.9f, 1000.0f, clamped);
            waterDistance->distance =
                al::lerpValue(0.8f, waterDistance->distance, waterDistance->targetDistance);
        } else if (angleV > 50.0f) {
            waterDistance->distance = farDistance;
        } else {
            f32 nearDistance = al::lerpValue(speedRate, 450.0f, 900.0f);
            if (angleV > 5.0f) {
                waterDistance->distance =
                    al::lerpValue(al::normalize(angleV, 5.0f, 50.0f), nearDistance, farDistance);
            } else {
                waterDistance->distance =
                    al::lerpValue(al::normalize(angleV, -15.0f, 5.0f), 430.0f, nearDistance);
            }
        }
    }

    updateInputOrSubTargetTurnH();

    f32 radH = sead::Mathf::deg2rad(mAngleH);
    sead::Vector3f cameraDirH(sinf(radH), 0.0f, cosf(sead::Mathf::deg2rad(mAngleH)));
    sead::Vector3f back = {0.0f, 0.0f, 0.0f};
    alCameraPoserFunction::calcTargetFront(&back, this);
    back.negate();
    f32 angle = al::calcAngleOnPlaneDegree(cameraDirH, back, mUp);

    if (sead::Mathf::abs(angle) > 30.0f) {
        mWaterRotateSpeedH += al::sign(angle) * 0.02f;
    }

    mWaterRotateSpeedH *= 0.95f;

    f32 moveRateH = al::normalize(sead::Vector2f(mPrevTargetTrans.x - mTargetTrans.x,
                                                 mPrevTargetTrans.z - mTargetTrans.z)
                                      .length(),
                                  0.0f, 5.0f);
    f32 moveRateV = 0.0f;
    f32 moveV = mTargetTrans.y - mPrevTargetTrans.y;
    if (moveV > 0.0f) {
        moveRateV = al::normalize(moveV, 0.0f, 5.0f);
    }

    mWaterRotateRate =
        al::lerpValue(0.2f, mWaterRotateRate, moveRateH > moveRateV ? moveRateH : moveRateV);
    f32 maxSpeed = angle < 0.0f ? -angle : angle;
    mWaterRotateSpeedH =
        sead::Mathf::clamp(mWaterRotateRate * mWaterRotateSpeedH, -maxSpeed, maxSpeed);
    mAngleH = al::wrapAngle(mWaterRotateSpeedH + mAngleH);

    if (al::isNearZero(alCameraPoserFunction::calcCameraRotateStickPower(this),
                       CameraPoserFollowLimitFunction::getRotateStickThreshold())) {
        bool isTrigger = alCameraPoserFunction::isTriggerCameraResetRotate(this);
        bool isInWater = alCameraPoserFunction::isTargetInWater(this);
        if (isTrigger) {
            if (isInWater) {
                al::setNerve(this, &NrvCameraPoserFollowLimitResetAngleWater);
            } else {
                endWater();
                al::setNerve(this, &NrvCameraPoserFollowLimitResetAngle);
            }

            return;
        }

        if (!isInWater && alCameraPoserFunction::isTargetCollideGround(this)) {
            endWater();
            return;
        }
    } else if (!alCameraPoserFunction::isTargetInWater(this) &&
               alCameraPoserFunction::isTargetCollideGround(this)) {
        endWater();
        return;
    }

    WaterDistance* waterDistanceAfter = mWaterDistance;
    if (waterDistanceAfter->_2) {
        waterDistanceAfter->_2 = false;
        waterDistanceAfter->_14 = 0.0f;
    }
}

/**
 * Follows the target in water while it is remote controlled.
 */
void CameraPoserFollowLimit::exeWaterRadicon() {
    updateRotateSpeedInputH();

    if (!alCameraPoserFunction::isTargetInWater(this)) {
        endWater();
    }
}

/**
 * Slows the horizontal rotation down over some steps.
 * @param step Number of steps until the rotation stops.
 */
void CameraPoserFollowLimit::startTurnBrake(s32 step) {
    mIsTurnBrake = true;
    mTurnBrakeSpeed = mRotateSpeedH;
    mTurnBrakeStep = 0;
    mTurnBrakeStepMax = step;
}

/**
 * Forbids or allows negative vertical angles.
 * @param isLimit Whether negative angles are forbidden.
 */
void CameraPoserFollowLimit::limitNegativeVerticalAngle(bool isLimit) {
    mAngleVerticalCtrl->limitNegativeVerticalAngle(isLimit);
}

/**
 * Checks whether a turn request currently disables the stick.
 * @return True if the stick is ignored.
 */
bool CameraPoserFollowLimit::getRequestTurnDisablePlayerInput() const {
    if (mTurnState != 1) {
        return false;
    }

    return mTurnInfo->_1d;
}

/**
 * Interpolates the vertical angle to an angle.
 * @param angle The angle in degrees.
 */
void CameraPoserFollowLimit::setTargetAngleV(f32 angle) {
    mAngleVerticalCtrl->startTargetInterpole(angle);
}

/**
 * Interpolates the vertical angle to an angle over some steps.
 * @param angle The angle in degrees.
 * @param step Number of steps of the interpolation.
 */
void CameraPoserFollowLimit::setTargetAngleV(f32 angle, s32 step) {
    mAngleVerticalCtrl->startTargetInterpole(angle, step);
}

/**
 * Limits the vertical angle from above, moving the camera down if needed.
 * @param angle The highest angle in degrees.
 */
void CameraPoserFollowLimit::setHiDegreeLimit(f32 angle) {
    mAngleVerticalCtrl->setHiDegreeLimit(angle);
    if (mAngleVerticalCtrl->getAngleDegree() > angle) {
        mAngleVerticalCtrl->startTargetInterpole(angle);
    }
}

/**
 * Removes the upper limit of the vertical angle.
 */
void CameraPoserFollowLimit::clearHiDegreeLimit() {
    mAngleVerticalCtrl->clearHiDegreeLimit();
}

/**
 * Limits the vertical angle from below, moving the camera up if needed.
 * @param angle The lowest angle in degrees.
 */
void CameraPoserFollowLimit::setLowDegreeLimit(f32 angle) {
    mAngleVerticalCtrl->setLowDegreeLimit(angle);
    if (mAngleVerticalCtrl->getAngleDegree() < angle) {
        mAngleVerticalCtrl->startTargetInterpole(angle);
    }
}

/**
 * Removes the lower limit of the vertical angle.
 */
void CameraPoserFollowLimit::clearLowDegreeLimit() {
    mAngleVerticalCtrl->clearLowDegreeLimit();
}

/**
 * Freezes or unfreezes the camera position.
 * @param isFreeze Whether the camera position is frozen.
 */
void CameraPoserFollowLimit::setIsFreezeCamPos(bool isFreeze) {
    mIsFreezeCamPos = isFreeze;
}

/**
 * Calculates the distance of the camera at a vertical angle.
 * @param angle The vertical angle in degrees.
 * @return The distance.
 */
f32 CameraPoserFollowLimit::getDistanceWithAngle(f32 angle) {
    return mDistanceCurve->calcDistance(angle);
}

/**
 * Requests the camera to turn towards a direction.
 * @param pInfo The turn request.
 * @return Always true.
 */
bool CameraPoserFollowLimit::requestTurnToDirection(const al::CameraTurnInfo* pInfo) {
    al::CameraTurnInfo* turnInfo = mTurnInfo;
    turnInfo->mDir = pInfo->mDir;
    turnInfo->mRequesterName = pInfo->mRequesterName;
    turnInfo->_14 = pInfo->_14;
    turnInfo->_18 = pInfo->_18;
    turnInfo->_1c = pInfo->_1c;
    turnInfo->_1d = pInfo->_1d;
    mIsRequestTurn = true;
    return true;
}

/**
 * Stops the vertical angle from going back up automatically after looking from below.
 */
void CameraPoserFollowLimit::invalidateAutoResetLowAngleV() {
    mAngleVerticalCtrl->invalidateAutoResetLowAngleV();
}

namespace CameraFunction {

/**
 * Creates a follow camera.
 * @return The camera.
 */
al::CameraPoser_RS* createFollowLimitCamera() {
    return new CameraPoserFollowLimit("Follow");
}

/**
 * Creates a 2D parallel camera.
 * @return The camera.
 */
al::CameraPoser_RS* createParallel2DCamera() {
    return new CameraPoserFollowLimit("Parallel2D");
}

}  // namespace CameraFunction

namespace CameraPoserFollowLimitFunction {

/**
 * Gets the horizontal rotation speed at full tilt for a stick sensitivity level.
 * @param sensitivityLevel The stick sensitivity level, from -2 to 2.
 * @return The rotation speed in degrees per frame.
 */
f32 calcRotateSpeedDegree(s32 sensitivityLevel) {
    if (sensitivityLevel < -1) {
        return 0.8f;
    }

    switch (sensitivityLevel) {
    case -1:
        return 1.3f;
    case 0:
        return 1.8f;
    case 1:
        return 2.3f;
    default:
        return 2.8f;
    }
}

/**
 * Gets the stick tilt below which the stick is ignored.
 * @return The threshold.
 */
f32 getRotateStickThreshold() {
    return 0.3f;
}

/**
 * Checks whether the previous camera was a follow or a parallel camera.
 * @param rInfo Information about the previous camera.
 * @return True if the previous camera was a follow or a parallel camera.
 */
bool isPreCameraFollowOrParallel(const al::CameraStartInfo& rInfo) {
    return alCameraPoserFunction::isEqualPreCameraName(rInfo, "Follow") ||
           alCameraPoserFunction::isEqualPreCameraName(rInfo, "Parallel");
}

}  // namespace CameraPoserFollowLimitFunction
