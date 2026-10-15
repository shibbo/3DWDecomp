#include "Library/Camera/CameraPoserFunction.hpp"

#include <gfx/seadCamera.h>

#include "Library/Camera/CameraFlagCtrl.hpp"
#include "Library/Camera/CameraGyroCtrl.hpp"
#include "Library/Camera/CameraInputHolder.hpp"
#include "Library/Camera/CameraLimitRailKeeper.hpp"
#include "Library/Camera/CameraParamMoveLimit.hpp"
#include "Library/Camera/CameraPoserFlag.hpp"
#include "Library/Camera/CameraPoserSceneInfo_RS.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraStartInfo.hpp"
#include "Library/Camera/CameraSubTargetBase.hpp"
#include "Library/Camera/CameraTargetBase.hpp"
#include "Library/Camera/CameraTargetCollideInfoHolder.hpp"
#include "Library/Camera/CameraTargetHolder.hpp"
#include "Library/Camera/CameraTriangleFilter.hpp"
#include "Library/Camera/ICameraInput.hpp"
#include "Library/Camera/SnapShotCameraCtrl.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Play/Camera/CameraArrowCollider.hpp"
#include "Library/Play/Camera/CameraPoserFix.hpp"
#include "Library/Play/Camera/CameraRailHolder_RS.hpp"
#include "Library/Play/Camera/CameraVerticalAbsorber.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Projection/Projection.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/CameraAngleCtrlInfo.hpp"
#include "Project/Camera/CameraAngleSwingInfo.hpp"
#include "Project/Camera/CameraCollisionPartsFilter.hpp"
#include "Project/Camera/CameraObjectRequestInfo.hpp"
#include "Project/Camera/CameraOffsetCtrlPreset.hpp"
#include "Project/Camera/CameraSubTargetTurnParam.hpp"
#include "Project/Camera/Holder/CameraRequestParamHolder.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace alCameraPoserFunction {
static al::CameraCollisionPartsFilter sPartsFilter;
static al::CameraTriangleFilter sTriangleFilter;
static al::CameraTriangleFilterOnlyCeiling sCeilFilter;
static sead::Vector3f sMtxX = {-1.0f, 0.0f, 0.0f};
static sead::Vector3f sMtxY = {0.0f, 1.0f, 0.0f};
static sead::Vector3f sMtxZ = {0.0f, 0.0f, -1.0f};

static inline al::CameraTargetBase* getTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->getViewTarget(getViewIndex(pPoser));
}

static inline al::CameraTargetBase* tryGetTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->tryGetViewTarget(getViewIndex(pPoser));
}

static inline al::CameraVerticalAbsorber* getVerticalAbsorber(const al::CameraPoser_RS* pPoser) {
    return pPoser->getCameraVerticalAbsorber();
}

static inline al::CameraTargetCollideInfoHolder*
getTargetCollision(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->targetCollideInfoHolder;
}

static inline al::ICameraInput* getCameraInput(const al::CameraPoser_RS* pPoser) {
    return pPoser->getInputHolder()->getInput(getViewIndex(pPoser));
}

static inline al::CameraSubTargetBase* getTopSubTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->getTopSubTarget();
}

static inline const al::CameraSubTargetTurnParam*
getSubTargetTurnParam(const al::CameraPoser_RS* pPoser) {
    return getTopSubTarget(pPoser)->getSubTargetTurnParam();
}

s32 getViewIndex(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getIndex();
}

const sead::LookAtCamera& getLookAtCamera(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getLookAtCam();
}

const al::Projection& getProjection(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getProjection();
}

const sead::Matrix44f& getProjectionMtx(const al::CameraPoser_RS* pPoser) {
    return *pPoser->getViewInfo()->getProjMtx();
}

f32 getNear(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getNear();
}

f32 getFar(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getFar();
}

f32 getAspect(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->getAspect();
}

const sead::Vector3f& getPreCameraPos(const al::CameraPoser_RS* pPoser) {
    return getLookAtCamera(pPoser).getPos();
}

const sead::Vector3f& getPreLookAtPos(const al::CameraPoser_RS* pPoser) {
    return getLookAtCamera(pPoser).getAt();
}

const sead::Vector3f& getPreUpDir(const al::CameraPoser_RS* pPoser) {
    return getLookAtCamera(pPoser).getUp();
}

f32 getPreFovyDegree(const al::CameraPoser_RS* pPoser) {
    return sead::Mathf::rad2deg(getPreFovyRadian(pPoser));
}

f32 getPreFovyRadian(const al::CameraPoser_RS* pPoser) {
    return getProjection(pPoser).getFovy();
}

bool isPrePriorityDemo(const al::CameraStartInfo& rInfo) {
    return rInfo.prePriorityType == al::CameraTicket::Priority_Demo;
}

bool isPrePriorityDemo2(const al::CameraStartInfo& rInfo) {
    return rInfo.prePriorityType == al::CameraTicket::Priority_Demo2;
}

bool isPrePriorityDemoTalk(const al::CameraStartInfo& rInfo) {
    return rInfo.prePriorityType == al::CameraTicket::Priority_DemoTalk;
}

bool isPrePriorityDemoAll(const al::CameraStartInfo& rInfo) {
    return isPrePriorityDemo(rInfo) || isPrePriorityDemo2(rInfo);
}

bool isPrePriorityEntranceAll(const al::CameraStartInfo& rInfo) {
    return rInfo.prePriorityType == al::CameraTicket::Priority_Entrance ||
           rInfo.prePriorityType == al::CameraTicket::Priority_EntranceSub;
}

bool isPrePriorityPlayer(const al::CameraStartInfo& rInfo) {
    return rInfo.prePriorityType == al::CameraTicket::Priority_Player;
}

bool isEqualPreCameraName(const al::CameraStartInfo& rInfo, const char* pName) {
    if (rInfo.preCameraName == nullptr)
        return false;
    return al::isEqualString(pName, rInfo.preCameraName);
}

bool isPreCameraFixAbsolute(const al::CameraStartInfo& rInfo) {
    return isEqualPreCameraName(rInfo, al::CameraPoserFix::getFixAbsoluteCameraName());
}

bool isInvalidCollidePreCamera(const al::CameraStartInfo& rInfo) {
    return rInfo.isInvalidCollidePreCamera;
}

bool isInvalidKeepPreCameraDistance(const al::CameraStartInfo& rInfo) {
    return rInfo.isInvalidKeepPreCameraDistance;
}

bool isInvalidKeepPreCameraDistanceIfNoCollide(const al::CameraStartInfo& rInfo) {
    return rInfo.isInvalidKeepPreCameraDistanceIfNoCollide;
}

bool isValidResetPreCameraPose(const al::CameraStartInfo& rInfo) {
    return rInfo.isValidResetPreCameraPose;
}

bool isValidKeepPreSelfCameraPose(const al::CameraStartInfo& rInfo) {
    return rInfo.isValidKeepPreSelfCameraPose;
}

f32 getPreCameraSwingAngleH(const al::CameraStartInfo& rInfo) {
    return rInfo.preCameraSwingAngleH;
}

f32 getPreCameraSwingAngleV(const al::CameraStartInfo& rInfo) {
    return rInfo.preCameraSwingAngleV;
}

f32 getPreCameraMaxSwingAngleH(const al::CameraStartInfo& rInfo) {
    return rInfo.preCameraMaxSwingAngleH;
}

f32 getPreCameraMaxSwingAngleV(const al::CameraStartInfo& rInfo) {
    return rInfo.preCameraMaxSwingAngleV;
}

bool isExistAreaAngleH(const al::CameraStartInfo& rInfo) {
    return rInfo.isExistAreaAngleH;
}

bool isExistAreaAngleV(const al::CameraStartInfo& rInfo) {
    return rInfo.isExistAreaAngleV;
}

f32 getAreaAngleH(const al::CameraStartInfo& rInfo) {
    return rInfo.areaAngleH;
}

f32 getAreaAngleV(const al::CameraStartInfo& rInfo) {
    return rInfo.areaAngleV;
}

bool isExistNextPoseByPreCamera(const al::CameraStartInfo& rInfo) {
    return rInfo.isExistNextPoseByPreCamera;
}

f32 getNextAngleHByPreCamera(const al::CameraStartInfo& rInfo) {
    return rInfo.nextAngleHByPreCamera;
}

f32 getNextAngleVByPreCamera(const al::CameraStartInfo& rInfo) {
    return rInfo.nextAngleVByPreCamera;
}

void calcCameraPose(sead::Quatf* pPose, const al::CameraPoser_RS* pPoser) {
    sead::Vector3f lookDir;
    calcLookDir(&lookDir, pPoser);
    al::makeQuatFrontUp(pPose, lookDir, pPoser->getUp());
}

void calcLookDir(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    calcCameraDir(pDir, pPoser);
    pDir->negate();
}

void calcCameraDir(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    pDir->setSub(pPoser->getEye(), pPoser->getAt());
    al::normalize(pDir);
}

bool calcCameraDirH(sead::Vector3f* pDirH, const al::CameraPoser_RS* pPoser) {
    pDirH->setSub(pPoser->getEye(), pPoser->getAt());
    al::verticalizeVec(pDirH, pPoser->getUp(), *pDirH);
    return al::tryNormalizeOrZero(pDirH);
}

bool calcLookDirH(sead::Vector3f* pDirH, const al::CameraPoser_RS* pPoser) {
    bool isValid = calcCameraDirH(pDirH, pPoser);
    pDirH->negate();
    return isValid;
}

void calcSideDir(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    sead::Vector3f facingDir = pPoser->getEye() - pPoser->getAt();
    al::normalize(&facingDir);
    pDir->setCross(facingDir, pPoser->getUp());
    al::normalize(pDir);
}

void calcPreCameraDir(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    const sead::LookAtCamera& lookAtCam = getLookAtCamera(pPoser);
    pDir->setSub(lookAtCam.getPos(), lookAtCam.getAt());
    al::normalize(pDir);
}

void calcPreCameraDirH(sead::Vector3f* pDirH, const al::CameraPoser_RS* pPoser) {
    const sead::LookAtCamera& lookAtCam = getLookAtCamera(pPoser);
    pDirH->setSub(lookAtCam.getPos(), lookAtCam.getAt());
    al::verticalizeVec(pDirH, sead::Vector3f::ey, *pDirH);
    al::tryNormalizeOrZero(pDirH);
}

void calcPreLookDir(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    calcPreCameraDir(pDir, pPoser);
    pDir->negate();
}

void calcPreLookDirH(sead::Vector3f* pDirH, const al::CameraPoser_RS* pPoser) {
    const sead::LookAtCamera& lookAtCam = getLookAtCamera(pPoser);
    pDirH->set(lookAtCam.getAt() - lookAtCam.getPos());
    al::verticalizeVec(pDirH, getPreUpDir(pPoser), *pDirH);
    al::tryNormalizeOrZero(pDirH);
}

f32 calcPreCameraAngleH(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f cameraDirH = {0.0f, 0.0f, 0.0f};
    calcPreCameraDirH(&cameraDirH, pPoser);
    return sead::Mathf::rad2deg(sead::Mathf::atan2(cameraDirH.x, cameraDirH.z));
}

f32 calcPreCameraAngleV(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f cameraDir = {0.0f, 0.0f, 0.0f};
    calcPreCameraDir(&cameraDir, pPoser);
    al::parallelizeVec(&cameraDir, sead::Vector3f::ey, cameraDir);
    return sead::Mathf::rad2deg(sead::Mathf::asin(cameraDir.length()));
}

/**
 * @brief Places the look-at point at the current camera target.
 * @param pPoser Camera poser whose target supplies the position.
 */
void setLookAtPosToTarget(al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    calcTargetTrans(&targetTrans, pPoser);
    pPoser->setAt(targetTrans);
}

void calcTargetTrans(sead::Vector3f* pTrans, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcTrans(pTrans);
}

void setLookAtPosToTargetAddOffset(al::CameraPoser_RS* pPoser, const sead::Vector3f& offset) {
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    calcTargetTrans(&targetTrans, pPoser);
    pPoser->setAt(targetTrans + offset);
}

/**
 * @brief Places the camera eye at the current camera target.
 * @param pPoser Camera poser whose target supplies the position.
 */
void setCameraPosToTarget(al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    calcTargetTrans(&targetTrans, pPoser);
    pPoser->setEye(targetTrans);
}

void setCameraPosToTargetAddOffset(al::CameraPoser_RS* pPoser, const sead::Vector3f& offset) {
    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    calcTargetTrans(&targetTrans, pPoser);
    pPoser->setEye(targetTrans + offset);
}

bool isTargetFollowExact(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isFollowExact();
}

void calcTargetTransWithOffset(sead::Vector3f* pTrans, const al::CameraPoser_RS* pPoser) {
    calcTargetTrans(pTrans, pPoser);
    al::CameraOffsetCtrlPreset* cameraOffsetCtrlPreset = pPoser->getOffsetCtrlPreset();

    if (cameraOffsetCtrlPreset != nullptr)
        pTrans->add(cameraOffsetCtrlPreset->getOffset());
}

void calcTargetVelocity(sead::Vector3f* pVelocity, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcVelocity(pVelocity);
}

void calcTargetVelocityH(sead::Vector3f* pVelocityH, const al::CameraPoser_RS* pPoser) {
    calcTargetVelocity(pVelocityH, pPoser);
    sead::Vector3f up = sead::Vector3f::ey;
    calcTargetUp(&up, pPoser);
    al::verticalizeVec(pVelocityH, up, *pVelocityH);
}

void calcTargetUp(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcUp(pDir);
}

f32 calcTargetSpeedV(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetVelocity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f targetUp = {0.0f, 0.0f, 0.0f};
    calcTargetVelocity(&targetVelocity, pPoser);
    calcTargetUp(&targetUp, pPoser);
    al::parallelizeVec(&targetVelocity, targetUp, targetVelocity);

    f32 direction = targetVelocity.dot(targetUp);
    f32 speedV = targetVelocity.length();

    if (direction > 0.0f)
        return speedV;
    else
        return -speedV;
}

void calcTargetPose(sead::Quatf* pPose, const al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetUp = {0.0f, 0.0f, 0.0f};
    sead::Vector3f targetFront = {0.0f, 0.0f, 0.0f};
    calcTargetUp(&targetUp, pPoser);
    calcTargetFront(&targetFront, pPoser);
    al::makeQuatFrontUp(pPose, targetFront, targetUp);
}

void calcTargetFront(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcFront(pDir);
}

void calcTargetSide(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcSide(pDir);
}

void calcTargetGravity(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    getTarget(pPoser)->calcGravity(pDir);
}

f32 calcTargetSpeedH(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetGravity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f targetVelocity = {0.0f, 0.0f, 0.0f};
    calcTargetGravity(&targetGravity, pPoser);
    calcTargetVelocity(&targetVelocity, pPoser);

    al::verticalizeVec(&targetVelocity, targetGravity, targetVelocity);
    return targetVelocity.length();
}

f32 calcTargetJumpSpeed(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetGravity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f targetVelocity = {0.0f, 0.0f, 0.0f};
    calcTargetGravity(&targetGravity, pPoser);
    calcTargetVelocity(&targetVelocity, pPoser);

    al::parallelizeVec(&targetVelocity, targetGravity, targetVelocity);

    if (al::isNearZero(targetVelocity) || targetGravity.dot(targetVelocity) > 0.0f)
        return 0.0f;

    return targetVelocity.length();
}

f32 calcTargetFallSpeed(const al::CameraPoser_RS* pPoser) {
    sead::Vector3f targetGravity = {0.0f, 0.0f, 0.0f};
    sead::Vector3f targetVelocity = {0.0f, 0.0f, 0.0f};
    calcTargetGravity(&targetGravity, pPoser);
    calcTargetVelocity(&targetVelocity, pPoser);

    al::parallelizeVec(&targetVelocity, targetGravity, targetVelocity);

    if (al::isNearZero(targetVelocity) || targetGravity.dot(targetVelocity) < 0.0f)
        return 0.0f;

    return targetVelocity.length();
}

bool isChangeTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->isChangeViewTarget(getViewIndex(pPoser));
}

bool isNoCameraReset(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isNoCameraReset();
}

bool tryGetTargetRequestDistance(f32* pDistance, const al::CameraPoser_RS* pPoser) {
    al::CameraTargetBase* target = getTarget(pPoser);

    if (target->getRequestDistance() > 0.0f) {
        *pDistance = target->getRequestDistance();
        return true;
    }

    return false;
}

al::CameraDistanceCurve* tryGetBossDistanceCurve(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->getBossDistanceCurve();
}

al::CameraDistanceCurve* tryGetEquipmentDistanceCurve(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->getEquipmentDistanceCurve();
}

bool isExistCollisionUnderTarget(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->isExistCollisionUnderTarget();
}

const sead::Vector3f& getUnderTargetCollisionPos(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->getTargetCollisionPos();
}

const sead::Vector3f& getUnderTargetCollisionNormal(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->getTargetCollisionNormal();
}

bool isExistSlopeCollisionUnderTarget(const al::CameraPoser_RS* pPoser) {
    return isExistCollisionUnderTarget(pPoser) &&
           getTargetCollision(pPoser)->isExistSlopeCollisionUnderTarget();
}

bool isExistWallCollisionUnderTarget(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->isExistUnderWall();
}

bool tryCalcSlopeCollisionDownFrontDirH(sead::Vector3f* pDirH,
                                        const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->tryCalcSlopeDownFrontDirH(pDirH);
}

f32 getSlopeCollisionUpSpeed(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->getSlopeCollisionUpSpeed();
}

f32 getSlopeCollisionDownSpeed(const al::CameraPoser_RS* pPoser) {
    return getTargetCollision(pPoser)->getSlopeCollisionDownSpeed();
}

bool isExistSubTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->getTopSubTargetInfo().target != nullptr;
}

bool checkValidTurnToSubTarget(const al::CameraPoser_RS* pPoser) {
    if (!isExistSubTarget(pPoser))
        return false;

    const al::CameraSubTargetTurnParam* subTargetTurnParam = getSubTargetTurnParam(pPoser);

    if (subTargetTurnParam->validTurnDegreeRangeH < 0.0f &&
        subTargetTurnParam->validFaceDegreeRangeH != 0.0f) {
        return true;
    }

    sead::Vector3f lookDirH = {0.0f, 0.0f, 0.0f};

    if (!calcLookDirH(&lookDirH, pPoser))
        return false;

    if (subTargetTurnParam->validFaceDegreeRangeH >= 0.0f) {
        sead::Vector3f targetBack = {0.0f, 0.0f, 0.0f};
        calcSubTargetBack(&targetBack, pPoser);

        if (subTargetTurnParam->validFaceDegreeRangeH < al::calcAngleDegree(targetBack, lookDirH))
            return false;
    }

    if (subTargetTurnParam->validTurnDegreeRangeH >= 0.0f) {
        sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
        sead::Vector3f lookDir = {0.0f, 0.0f, 0.0f};
        calcSubTargetTrans(&targetTrans, pPoser);
        lookDir = targetTrans - pPoser->getEye();
        al::verticalizeVec(&lookDir, pPoser->getUp(), lookDir);

        if (!al::tryNormalizeOrZero(&lookDir))
            return false;
        if (subTargetTurnParam->validTurnDegreeRangeH / 2.0f <
            al::calcAngleDegree(lookDirH, lookDir))
            return false;
    }

    return true;
}

void calcSubTargetBack(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    calcSubTargetFront(pDir, pPoser);
    pDir->negate();
}

void calcSubTargetTrans(sead::Vector3f* pTrans, const al::CameraPoser_RS* pPoser) {
    getTopSubTarget(pPoser)->calcTrans(pTrans);
}

bool isChangeSubTarget(const al::CameraPoser_RS* pPoser) {
    return pPoser->getTargetHolder()->getTopSubTargetInfo().hasTargetChanged;
}

void calcSubTargetFront(sead::Vector3f* pDir, const al::CameraPoser_RS* pPoser) {
    getTopSubTarget(pPoser)->calcFront(pDir);
}

f32 getSubTargetRequestDistance(const al::CameraPoser_RS* pPoser) {
    return getTopSubTarget(pPoser)->getRequestDistance();
}

f32 getSubTargetTurnSpeedRate1(const al::CameraPoser_RS* pPoser) {
    return getSubTargetTurnParam(pPoser)->turnSpeedRate1;
}

f32 getSubTargetTurnSpeedRate2(const al::CameraPoser_RS* pPoser) {
    return getSubTargetTurnParam(pPoser)->turnSpeedRate2;
}

s32 getSubTargetTurnRestartStep(const al::CameraPoser_RS* pPoser) {
    return getSubTargetTurnParam(pPoser)->targetTurnRestartStep;
}

bool tryCalcSubTargetTurnBrakeDistanceRate(f32* pRate,
                                           const al::CameraPoser_RS* pPoser) {
    const al::CameraSubTargetTurnParam* turnParam = getSubTargetTurnParam(pPoser);

    sead::Vector3f targetTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f subTargetTrans = {0.0f, 0.0f, 0.0f};
    calcTargetTrans(&targetTrans, pPoser);
    calcSubTargetTrans(&subTargetTrans, pPoser);

    f32 distance = sead::Mathf::sqrt(sead::Mathf::pow(targetTrans.x - subTargetTrans.x, 2.0f) +
                                     sead::Mathf::pow(targetTrans.z - subTargetTrans.z, 2.0f));

    if (turnParam->turnBrakeEndDistance > 0.0f &&
        turnParam->turnBrakeEndDistance < turnParam->turnBrakeStartDistance &&
        distance < turnParam->turnBrakeStartDistance) {
        *pRate = 1.0f - al::normalize(distance, turnParam->turnBrakeEndDistance,
                                                turnParam->turnBrakeStartDistance);
        return true;
    }

    if (turnParam->turnStopStartDistance > 0.0f &&
        turnParam->turnStopStartDistance < turnParam->turnStopEndDistance &&
        turnParam->turnStopStartDistance < distance) {
        *pRate = al::normalize(distance, turnParam->turnStopStartDistance,
                                         turnParam->turnStopEndDistance);
        return true;
    }

    return false;
}

bool isValidSubTargetTurnV(const al::CameraPoser_RS* pPoser) {
    return getSubTargetTurnParam(pPoser)->isTurnV;
}

bool isValidSubTargetResetAfterTurnV(const al::CameraPoser_RS* pPoser) {
    return getSubTargetTurnParam(pPoser)->isResetAfterTurnV;
}

void clampAngleSubTargetTurnRangeV(f32* pAngle, const al::CameraPoser_RS* pPoser) {
    const al::CameraSubTargetTurnParam* param = getSubTargetTurnParam(pPoser);
    *pAngle = sead::Mathf::clamp(*pAngle, param->minTurnDegreeV, param->maxTurnDegreeV);
}

void initCameraVerticalAbsorber(al::CameraPoser_RS* pPoser) {
    pPoser->setVerticalAbsorber(new al::CameraVerticalAbsorber(pPoser, false));
}

void initCameraVerticalAbsorberNoCameraPosAbsorb(al::CameraPoser_RS* pPoser) {
    pPoser->setVerticalAbsorber(new al::CameraVerticalAbsorber(pPoser, true));
}

f32 getCameraVerticalAbsorbPosUp(const al::CameraPoser_RS* pPoser) {
    return getVerticalAbsorber(pPoser)->getAbsorbScreenPosUp();
}

f32 getCameraVerticalAbsorbPosDown(const al::CameraPoser_RS* pPoser) {
    return getVerticalAbsorber(pPoser)->getAbsorbScreenPosDown();
}

void liberateVerticalAbsorb(al::CameraPoser_RS* pPoser) {
    getVerticalAbsorber(pPoser)->liberateAbsorb();
}

void stopUpdateVerticalAbsorb(al::CameraPoser_RS* pPoser) {
    getVerticalAbsorber(pPoser)->setIsStopUpdate(true);
}

void stopUpdateVerticalAbsorbForSnapShotMode(al::CameraPoser_RS* pPoser,
                                             const sead::Vector3f& absorbVec) {
    stopUpdateVerticalAbsorb(pPoser);
    getVerticalAbsorber(pPoser)->tryResetAbsorbVecIfInCollision(absorbVec);
}

void restartUpdateVerticalAbsorb(al::CameraPoser_RS* pPoser) {
    getVerticalAbsorber(pPoser)->setIsStopUpdate(false);
}

void validateVerticalAbsorbKeepInFrame(al::CameraPoser_RS* pPoser) {
    getVerticalAbsorber(pPoser)->setIsKeepInFrame(true);
}

void invalidateVerticalAbsorbKeepInFrame(al::CameraPoser_RS* pPoser) {
    getVerticalAbsorber(pPoser)->setIsKeepInFrame(false);
}

void setVerticalAbsorbKeepInFrameScreenOffsetUp(al::CameraPoser_RS* pPoser,
                                                f32 keepInFrameOffsetUp) {
    getVerticalAbsorber(pPoser)->setKeepInFrameOffsetUp(keepInFrameOffsetUp);
}

void setVerticalAbsorbKeepInFrameScreenOffsetDown(al::CameraPoser_RS* pPoser,
                                                  f32 keepInFrameOffsetDown) {
    getVerticalAbsorber(pPoser)->setKeepInFrameOffsetDown(keepInFrameOffsetDown);
}

void initCameraArrowCollider(al::CameraPoser_RS* pPoser) {
    al::CameraArrowCollider* cameraArrowCollider =
        new al::CameraArrowCollider(pPoser->getCollisionDirector());
    pPoser->initArrowCollider(cameraArrowCollider);
}

void initCameraArrowColliderWithoutThroughPassCollision(al::CameraPoser_RS* pPoser) {
    al::CameraArrowCollider* cameraArrowCollider =
        new al::CameraArrowCollider(pPoser->getCollisionDirector());
    cameraArrowCollider->setIsInvalidThroughPassCollision(true);
    pPoser->initArrowCollider(cameraArrowCollider);
}

void initCameraMoveLimit(al::CameraPoser_RS* pPoser) {
    pPoser->setParamMoveLimit(al::CameraParamMoveLimit::create(pPoser));
}

void initCameraAngleCtrl(al::CameraPoser_RS* pPoser) {
    pPoser->setAngleCtrlInfo(new al::CameraAngleCtrlInfo());
}

void initCameraAngleCtrlWithRelativeH(al::CameraPoser_RS* pPoser) {
    pPoser->setAngleCtrlInfo(al::CameraAngleCtrlInfo::createWithRelativeH());
}

void initCameraDefaultAngleRangeV(al::CameraPoser_RS* pPoser, f32 min, f32 max) {
    pPoser->getAngleCtrlInfo()->setDefaultAngleV(min, max);
}

void setCameraStartAngleV(al::CameraPoser_RS* pPoser, f32 angle) {
    pPoser->getAngleCtrlInfo()->setStartAngleV(angle);
}

void setCameraAngleV(al::CameraPoser_RS* pPoser, f32 angle) {
    pPoser->getAngleCtrlInfo()->setAngleV(angle);
}

f32 getCameraAngleH(const al::CameraPoser_RS* pPoser) {
    return pPoser->getAngleCtrlInfo()->getAngleH();
}

f32 getCameraAngleV(const al::CameraPoser_RS* pPoser) {
    return pPoser->getAngleCtrlInfo()->getAngleV();
}

void initAngleSwing(al::CameraPoser_RS* pPoser) {
    pPoser->setAngleSwingInfo(new al::CameraAngleSwingInfo());
}

bool isValidAngleSwing(const al::CameraPoser_RS* pPoser) {
    return !pPoser->getAngleSwingInfo()->isInvalidSwing;
}

void initCameraOffsetCtrlPreset(al::CameraPoser_RS* pPoser) {
    pPoser->setOffsetCtrlPreset(new al::CameraOffsetCtrlPreset());
}

const sead::Vector3f& getOffset(const al::CameraPoser_RS* pPoser) {
    return pPoser->getOffsetCtrlPreset()->getOffset();
}

void initGyroCameraCtrl(al::CameraPoser_RS* pPoser) {
    pPoser->setGyroCtrl(new al::GyroCameraCtrl());
}

void resetGyro(al::CameraPoser_RS* pPoser) {
    sead::Vector3f pSide;
    sead::Vector3f pUp;
    sead::Vector3f pFront;
    calcCameraGyroPose(pPoser, &pSide, &pUp, &pFront);
    pPoser->getGyroCtrl()->reset(pSide, pUp, pFront);
}

void calcCameraGyroPose(const al::CameraPoser_RS* pPoser, sead::Vector3f* pSide,
                        sead::Vector3f* pUp, sead::Vector3f* pFront) {
    getCameraInput(pPoser)->calcGyroPose(pSide, pUp, pFront);
}

const sead::Vector3f& getGyroFront(al::CameraPoser_RS* pPoser) {
    return pPoser->getGyroCtrl()->getFront();
}

f32 getGyroAngleV(al::CameraPoser_RS* pPoser) {
    return pPoser->getGyroCtrl()->getAngleV();
}

f32 getGyroAngleH(al::CameraPoser_RS* pPoser) {
    return pPoser->getGyroCtrl()->getAngleH();
}

void setGyroLimitAngleV(al::CameraPoser_RS* pPoser, f32 min, f32 max) {
    pPoser->getGyroCtrl()->setLimitAngleV(min, max);
}

void setGyroSensitivity(al::CameraPoser_RS* pPoser, f32 min, f32 max) {
    pPoser->getGyroCtrl()->setSensitivity(min, max);
}

void reduceGyroSencitivity(al::CameraPoser_RS* pPoser) {
    pPoser->getGyroCtrl()->reduceSensitivity();
}

void stopUpdateGyro(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isStopUpdateGyro = true;
}

void restartUpdateGyro(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isStopUpdateGyro = false;
}

bool isStopUpdateGyro(const al::CameraPoser_RS* pPoser) {
    return pPoser->getPoserFlag()->isStopUpdateGyro;
}

bool isTargetCollideGround(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isCollideGround();
}

bool isTargetInWater(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isInWater();
}

bool isTargetInMoonGravity(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isInMoonGravity();
}

bool isTargetClimbPole(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isClimbPole();
}

bool isTargetGrabCeil(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isGrabCeil();
}

bool isTargetInvalidMoveByInput(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isInvalidMoveByInput();
}

bool isTargetEnableEndAfterInterpole(const al::CameraPoser_RS* pPoser) {
    return (tryGetTarget(pPoser) != nullptr) && getTarget(pPoser)->isEnableEndAfterInterpole();
}

bool isTargetWallCatch(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isWallCatch();
}

bool isSnapShotMode(const al::CameraPoser_RS* pPoser) {
    return pPoser->getFlagCtrl()->isSnapShotModeRunning;
}

void initSnapShotCameraCtrl(al::CameraPoser_RS* pPoser) {
    al::SnapShotCameraCtrl* snapShotCtrl =
        new al::SnapShotCameraCtrl(pPoser, pPoser->getSceneInfo()->snapShotCameraSceneInfo, false);
    pPoser->setSnapShotCtrl(snapShotCtrl);
}

void initSnapShotCameraCtrlZoomAutoReset(al::CameraPoser_RS* pPoser) {
    al::SnapShotCameraCtrl* snapShotCtrl =
        new al::SnapShotCameraCtrl(pPoser, pPoser->getSceneInfo()->snapShotCameraSceneInfo, false);
    snapShotCtrl->setIsValidZoomFovy(true);
    snapShotCtrl->setIsZoomAutoReset(true);
    pPoser->setSnapShotCtrl(snapShotCtrl);
}

void initSnapShotCameraCtrlZoomRollMove(al::CameraPoser_RS* pPoser, bool isValidMove,
                                        bool isValidZoomFovy) {
    al::SnapShotCameraCtrl* snapShotCtrl = new al::SnapShotCameraCtrl(
        pPoser, pPoser->getSceneInfo()->snapShotCameraSceneInfo, isValidZoomFovy);
    snapShotCtrl->setIsValidZoomFovy(true);
    snapShotCtrl->setIsValidRoll(true);
    snapShotCtrl->setIsValidLookAtOffset(true);

    if (isValidMove) {
        snapShotCtrl->setIsValidMove(true);
    }

    pPoser->setSnapShotCtrl(snapShotCtrl);
}

void validateSnapShotCameraLookAtOffset(al::CameraPoser_RS* pPoser) {
    pPoser->getSnapShotCtrl()->setIsValidLookAtOffset(true);
}

void validateSnapShotCameraZoomFovy(al::CameraPoser_RS* pPoser) {
    pPoser->getSnapShotCtrl()->setIsValidZoomFovy(true);
}

void validateSnapShotCameraRoll(al::CameraPoser_RS* pPoser) {
    pPoser->getSnapShotCtrl()->setIsValidRoll(true);
}

void updateSnapShotCameraCtrl(al::CameraPoser_RS* pPoser) {
    sead::LookAtCamera camera;
    pPoser->makeLookAtCameraPrev(&camera);

    pPoser->getSnapShotCtrl()->update(camera, pPoser, getCameraInput(pPoser));
}

void startResetSnapShotCameraCtrl(al::CameraPoser_RS* pPoser, s32 value) {
    pPoser->getSnapShotCtrl()->startReset(value);
}

void setSnapShotMaxZoomOutFovyDegree(al::CameraPoser_RS* pPoser, f32 value) {
    pPoser->getSnapShotCtrl()->setMaxZoomOutFovyDegree(value);
}

f32 getSnapShotRollDegree(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSnapShotCtrl()->getRollDegree();
}

const sead::Vector3f& getSnapShotLookAtOffset(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSnapShotCtrl()->getLookAtOffset();
}

bool isOffVerticalAbsorb(const al::CameraPoser_RS* pPoser) {
    return pPoser->getPoserFlag()->isOffVerticalAbsorb;
}

void onVerticalAbsorb(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isOffVerticalAbsorb = false;
}

void offVerticalAbsorb(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isOffVerticalAbsorb = true;
}

void invalidateCameraBlur(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidCameraBlur = true;
}

bool isRequestStopVerticalAbsorb(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isStopVerticalAbsorb;
}

bool isRequestResetPosition(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isResetPosition;
}

bool isRequestResetAngleV(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isResetAngleV;
}

bool isRequestDownToDefaultAngleBySpeed(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isDownToDefaultAngleBySpeed;
}

bool isRequestUpToTargetAngleBySpeed(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isUpToTargetAngleBySpeed;
}

f32 getRequestTargetAngleV(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.targetAngleV;
}

f32 getRequestAngleSpeed(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.angleSpeed;
}

bool isRequestMoveDownAngleV(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isMoveDownAngle;
}

bool isRequestSetAngleV(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.isSetAngleV;
}

f32 getRequestAngleV(const al::CameraObjectRequestInfo& rInfo) {
    return rInfo.angleV;
}

bool isInvalidCollider(const al::CameraPoser_RS* pPoser) {
    return pPoser->getPoserFlag()->isInvalidCollider;
}

void validateCollider(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidCollider = false;
}

void invalidateCollider(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidCollider = true;
}

void validateCtrlSubjective(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isValidCtrlSubjective = true;
}

void invalidateChangeSubjective(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidChangeSubjective = true;
}

void invalidateKeepDistanceNextCamera(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidKeepDistanceNextCamera = true;
}

void invalidateKeepDistanceNextCameraIfNoCollide(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidKeepDistanceNextCameraIfNoCollide = true;
}

void invalidatePreCameraEndAfterInterpole(al::CameraPoser_RS* pPoser) {
    pPoser->getPoserFlag()->isInvalidPreCameraEndAfterInterpole = true;
}

bool isInvalidPreCameraEndAfterInterpole(const al::CameraPoser_RS* pPoser) {
    return pPoser->getPoserFlag()->isInvalidPreCameraEndAfterInterpole;
}

bool isSceneCameraFirstCalc(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->isFirstCalc();
}

bool isActiveInterpole(const al::CameraPoser_RS* pPoser) {
    return pPoser->getViewInfo()->isActiveInterpole();
}

bool isInvalidEndEntranceCamera(const al::CameraPoser_RS* pPoser) {
    return pPoser->getFlagCtrl()->isInvalidEndEntranceCamera;
}

bool isPause(const al::CameraPoser_RS* pPoser) {
    return false;
}

bool checkFirstCameraCollisionArrow(sead::Vector3f* pHitPos, sead::Vector3f* pNormal,
                                    const al::IUseCollision* collision, const sead::Vector3f& pos,
                                    const sead::Vector3f& dir) {
    CameraCollisionHitResult result;

    if (!checkFirstCameraCollisionArrow(&result, collision, pos, dir))
        return false;

    if (pHitPos)
        pHitPos->set(result.hitPos);
    if (pNormal)
        pNormal->set(result.normal);

    return true;
}

bool checkFirstCameraCollisionArrow(CameraCollisionHitResult* pResult,
                                    const al::IUseCollision* collision, const sead::Vector3f& pos,
                                    const sead::Vector3f& dir) {
    const al::ArrowHitInfo* hitInfo = nullptr;

    if (!alCollisionUtil::getFirstPolyOnArrow(collision, &hitInfo, pos, dir, &sPartsFilter,
                                              &sTriangleFilter)) {
        return false;
    }

    pResult->hitPos.set(hitInfo->mPos);
    pResult->normal.set(*hitInfo->mTriangle.getNormal(0));

    CameraCollisionLocation location = CameraCollisionLocation::Default;

    if (hitInfo->isCollisionAtFace()) {
        location = CameraCollisionLocation::Face;
    } else if (hitInfo->isCollisionAtEdge()) {
        location = CameraCollisionLocation::Edge;
    }

    pResult->location = location;

    return true;
}

bool checkFirstCameraCollisionArrowOnlyCeiling(sead::Vector3f* pHitPos, sead::Vector3f* pNormal,
                                               const al::IUseCollision* collision,
                                               const sead::Vector3f& pos,
                                               const sead::Vector3f& dir) {
    al::Triangle triangle;
    sead::Vector3f hitPos = {0.0f, 0.0f, 0.0f};

    if (!alCollisionUtil::getFirstPolyOnArrow(collision, &hitPos, &triangle, pos, dir, nullptr,
                                              &sCeilFilter)) {
        return false;
    }

    if (pHitPos)
        pHitPos->set(hitPos);

    if (pNormal)
        pNormal->set(*triangle.getNormal(0));
    return true;
}

f32 calcZoneRotateAngleH(f32 angle, const al::CameraPoser_RS* pPoser) {
    return calcZoneRotateAngleH(angle, pPoser->getViewMtx());
}

f32 calcZoneRotateAngleH(f32 angle, const sead::Matrix34f& mtx) {
    f32 sin = sead::Mathf::sin(sead::Mathf::deg2rad(angle));
    f32 cos = sead::Mathf::cos(sead::Mathf::deg2rad(angle));

    f32 rotateAngle =
        sead::Mathf::atan2(sin * mtx.m[0][0] + 0.0f * mtx.m[0][1] + cos * mtx.m[0][2],
                           sin * mtx.m[2][0] + 0.0f * mtx.m[2][1] + cos * mtx.m[2][2]);

    return sead::Mathf::rad2deg(rotateAngle);
}

f32 calcZoneInvRotateAngleH(f32 angle, const sead::Matrix34f& mtx) {
    f32 sin = sead::Mathf::sin(sead::Mathf::deg2rad(angle));
    f32 cos = sead::Mathf::cos(sead::Mathf::deg2rad(angle));

    f32 rotateAngle =
        sead::Mathf::atan2(sin * mtx.m[0][0] + 0.0f * mtx.m[1][0] + cos * mtx.m[2][0],
                           sin * mtx.m[0][2] + 0.0f * mtx.m[1][2] + cos * mtx.m[2][2]);

    return sead::Mathf::rad2deg(rotateAngle);
}

void multVecZone(sead::Vector3f* pOut, const sead::Vector3f& vec,
                 const al::CameraPoser_RS* pPoser) {
    pOut->setMul(pPoser->getViewMtx(), vec);
}

void multVecInvZone(sead::Vector3f* pOut, const sead::Vector3f& vec,
                    const al::CameraPoser_RS* pPoser) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    mtx.setInverse(pPoser->getViewMtx());
    pOut->setMul(mtx, vec);
}

void rotateVecZone(sead::Vector3f* pOut, const sead::Vector3f& vec,
                   const al::CameraPoser_RS* pPoser) {
    pOut->setRotated(pPoser->getViewMtx(), vec);
}

bool makeCameraKeepInFrameV(sead::LookAtCamera* camera, const sead::Vector3f& vec,
                            const al::CameraPoser_RS* pPoser, f32 a, f32 b) {
    sead::Vector3f offset = {0.0f, 0.0f, 0.0f};

    if (!calcOffsetCameraKeepInFrameV(&offset, camera, vec, pPoser, a, b))
        return false;

    camera->setAt(camera->getAt() + offset);
    camera->setPos(camera->getPos() + offset);
    return true;
}

void initCameraRail(al::CameraPoser_RS* pPoser, const al::PlacementInfo& info, const char* name) {
    al::PlacementInfo placementInfo;
    al::tryGetLinksInfo(&placementInfo, info, name);
    pPoser->initRail(placementInfo);
}

bool tryGetCameraRailArg(f32* pArg, const al::PlacementInfo& info, const char* argName,
                         const char* name) {
    al::PlacementInfo placementInfo;

    if (!al::tryGetLinksInfo(&placementInfo, info, name))
        return false;

    return al::tryGetArg(pArg, placementInfo, argName);
}

const char* getCameraRailPointObjId(const al::CameraPoser_RS* pPoser, s32 index) {
    al::PlacementInfo* railPointInfo = al::getRailPointInfo(pPoser, index);

    al::PlacementId placementId;
    placementId.init(*railPointInfo);
    return placementId.mPlacementID;
}

al::CameraLimitRailKeeper* tryFindNearestLimitRailKeeper(const al::CameraPoser_RS* pPoser,
                                                         const sead::Vector3f& pos) {
    f32 minDistance = -1.0f;
    al::CameraLimitRailKeeper* nearestRailKeeper = nullptr;

    for (s32 i = 0; i < pPoser->getSceneInfo()->railHolderNum; i++) {
        al::CameraRailHolder_RS* railHolder = pPoser->getSceneInfo()->railHolders[i];

        if (!railHolder->isActive()) {
            continue;
        }

        for (s32 j = 0; j < railHolder->getRailCount(); j++) {
            al::CameraLimitRailKeeper* railKeeper = railHolder->getRail(j);

            sead::Vector3f nearestPos = {0.0f, 0.0f, 0.0f};
            railKeeper->calcNearestRailPos(&nearestPos, pos);
            f32 distance = (nearestPos - pos).length();

            if (railKeeper->getActivateDistance() < distance)
                continue;

            if (minDistance < 0.0f || distance < minDistance) {
                minDistance = distance;
                nearestRailKeeper = railKeeper;
            }
        }
    }

    return nearestRailKeeper;
}

void calcCameraRotateStick(sead::Vector2f* pStick, const al::CameraPoser_RS* pPoser) {
    sead::Vector2f stick = sead::Vector2f::zero;

    for (s32 i = 0; i < pPoser->getInputHolder()->getInputNum(); i++) {
        sead::Vector2f inputStick;
        pPoser->getInputHolder()->getInput(i)->calcInputStick(&inputStick);
        stick += inputStick;
    }

    pStick->set(stick);

    if (pPoser->getFlagCtrl()->isCameraReverseInputH)
        pStick->x = -pStick->x;
    if (pPoser->getFlagCtrl()->isCameraReverseInputV)
        pStick->y = -pStick->y;
}

void disableInput(const al::CameraPoser_RS* pPoser, bool isDisable) {
    for (s32 i = 0; i < pPoser->getInputHolder()->getInputNum(); i++) {
        pPoser->getInputHolder()->getInput(i)->setDisableInput(isDisable);
    }
}

void calcCameraRolledRotateStick(sead::Vector2f* pStick, const al::CameraPoser_RS* pPoser) {
    calcCameraRotateStick(pStick, pPoser);

    if (!isSnapShotMode(pPoser) || pPoser->getSnapShotCtrl() == nullptr) {
        return;
    }

    sead::Vector2f dir = *pStick;

    if (!al::tryNormalizeOrZero(&dir)) {
        return;
    }

    f32 degree = sead::Mathf::rad2deg(sead::Mathf::atan2(dir.y, dir.x));
    degree = al::modf(degree - pPoser->getSnapShotCtrl()->getRollDegree() + 360.0f, 360.0f) + 0.0f;
    f32 radian = sead::Mathf::deg2rad(degree);
    f32 cos = sead::Mathf::cos(radian);
    f32 sin = sead::Mathf::sin(radian);
    f32 length = pStick->length();
    pStick->set(cos * length, sin * length);
}

f32 calcCameraRotateStickH(const al::CameraPoser_RS* pPoser) {
    sead::Vector2f stick = {0.0f, 0.0f};
    calcCameraRotateStick(&stick, pPoser);
    return stick.x;
}

f32 calcCameraRotateStickV(const al::CameraPoser_RS* pPoser) {
    sead::Vector2f stick = {0.0f, 0.0f};
    calcCameraRotateStick(&stick, pPoser);
    return stick.y;
}

f32 calcCameraRotateStickPower(const al::CameraPoser_RS* pPoser) {
    sead::Vector2f stick = {0.0f, 0.0f};
    calcCameraRotateStick(&stick, pPoser);
    return stick.length();
}

s32 getStickSensitivityLevel(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->getStickSensitivityLevel();
}

f32 getStickSensitivityScale(const al::CameraPoser_RS* pPoser) {
    switch (getStickSensitivityLevel(pPoser)) {
    case -2:
        return 0.44f;
    case -1:
        return 0.72f;
    case 0:
        return 1.6f;
    case 1:
        return 1.27f;
    case 2:
        return 1.55f;
    default:
        return 1.0f;
    }
}

bool isValidGyro(const al::CameraPoser_RS* pPoser) {
    return !pPoser->getFlagCtrl()->isInvalidCameraGyro;
}

s32 getGyroSensitivityLevel(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->getGyroSensitivityLevel();
}

f32 getGyroSensitivityScale(const al::CameraPoser_RS* pPoser) {
    switch (getGyroSensitivityLevel(pPoser)) {
    case -1:
        return 0.625f;
    case 0:
        return 1.0f;
    case 1:
        return 1.6f;
    default:
        return 1.0f;
    }
}

bool isTriggerCameraResetRotate(const al::CameraPoser_RS* pPoser) {
    if (isSnapShotMode(pPoser)) {
        return false;
    }

    for (s32 i = 0; i < pPoser->getInputHolder()->getInputNum(); i++) {
        if (pPoser->getInputHolder()->getInput(i)->isTriggerReset()) {
            return true;
        }
    }

    return false;
}

bool isHoldCameraZoom(const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->isHoldZoom();
}

bool isHoldCameraSnapShotZoomIn(const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->isHoldSnapShotZoomIn();
}

bool isHoldCameraSnapShotZoomOut(const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->isHoldSnapShotZoomOut();
}

bool isHoldCameraSnapShotRollLeft(const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->isHoldSnapShotRollLeft();
}

bool isHoldCameraSnapShotRollRight(const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->isHoldSnapShotRollRight();
}

bool tryCalcCameraSnapShotMoveStick(sead::Vector2f* pStick, const al::CameraPoser_RS* pPoser) {
    return getCameraInput(pPoser)->tryCalcSnapShotMoveStick(pStick);
}

bool isPlayerTypeFlyer(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->isPlayerTypeFlyer();
}

bool isPlayerTypeHighSpeedMove(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->isPlayerTypeHighSpeedMove();
}

bool isPlayerTypeHighJump(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->isPlayerTypeHighJump();
}

bool isPlayerTypeNotTouchGround(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->isPlayerTypeNotTouchGround();
}

bool isOnRideObj(const al::CameraPoser_RS* pPoser) {
    return pPoser->getSceneInfo()->requestParamHolder->isOnRideObj();
}

bool isPlayerClimbing(const al::CameraPoser_RS* pPoser) {
    return getTarget(pPoser)->isClimbing();
}

}  // namespace alCameraPoserFunction

