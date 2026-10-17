#include "Library/Camera/CameraUtil.hpp"

#include <gfx/seadCamera.h>

#include "Library/Camera/CameraDistanceCurve.hpp"
#include "Library/Camera/CameraFlagCtrl.hpp"
#include "Library/Camera/CameraPoseInfo.hpp"
#include "Library/Camera/CameraPoseUpdater.hpp"
#include "Library/Camera/CameraPoserFlag.hpp"
#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTargetHolder.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraTicketId.hpp"
#include "Library/Camera/ICameraInput.hpp"
#include "Library/Camera/SceneCameraCtrl.hpp"
#include "Library/Camera/SimpleCameraInput.hpp"
#include "Library/Camera/SnapShotCameraCtrl.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Play/Camera/ActorCameraSubTarget.hpp"
#include "Library/Play/Camera/ActorCameraTarget.hpp"
#include "Library/Play/Camera/CameraPoserActorRailParallel.hpp"
#include "Library/Play/Camera/CameraPoserAnim_RS.hpp"
#include "Library/Play/Camera/CameraPoserBossBattle.hpp"
#include "Library/Play/Camera/CameraPoserCart.hpp"
#include "Library/Play/Camera/CameraPoserEntrance_RS.hpp"
#include "Library/Play/Camera/CameraPoserFix.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Camera/CameraPoserFixLook.hpp"
#include "Library/Play/Camera/CameraPoserFixPoint.hpp"
#include "Library/Play/Camera/CameraPoserFollowSimple.hpp"
#include "Library/Play/Camera/CameraPoserInnerTower.hpp"
#include "Library/Play/Camera/CameraPoserKinopioBrigade_RS.hpp"
#include "Library/Play/Camera/CameraPoserLookDown.hpp"
#include "Library/Play/Camera/CameraPoserParallelSimple.hpp"
#include "Library/Play/Camera/CameraPoserProgramable_RS.hpp"
#include "Library/Play/Camera/CameraPoserQuickTurn.hpp"
#include "Library/Play/Camera/CameraPoserRace.hpp"
#include "Library/Play/Camera/CameraPoserShooterSingle.hpp"
#include "Library/Play/Camera/CameraPoserSubjective_RS.hpp"
#include "Library/Play/Camera/CameraPoserTower_RS.hpp"
#include "Library/Play/Camera/CameraVerticalAbsorber.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Projection/Projection.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/CameraAngleSwingInfo.hpp"
#include "Project/Camera/CameraObjectRequestInfo.hpp"
#include "Project/Camera/Holder/CameraRequestParamHolder.hpp"
#include "Project/Camera/Holder/CameraShaker_RS.hpp"
#include "Project/Camera/Holder/CameraSwitchRequester.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"

namespace al {
namespace {
inline CameraDirector_RS* getCameraDirector(const IUseCamera_RS* pUser) {
    return pUser->getCameraDirector_RS();
}

inline const sead::LookAtCamera& getLookAtCameraImpl(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getLookAtCam();
}
}  // namespace

SceneCameraInfo* getSceneCameraInfo(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getSceneCameraInfo();
}

s32 getViewNumMax(const IUseCamera_RS* pUser) {
    return getViewNumMax(getSceneCameraInfo(pUser));
}

s32 getViewNumMax(const SceneCameraInfo* pInfo) {
    return pInfo->getViewNumMax();
}

bool isValidView(const IUseCamera_RS* pUser, s32 viewIdx) {
    return isValidView(getSceneCameraInfo(pUser), viewIdx);
}

bool isValidView(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->isValid();
}

const char* getViewName(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getViewName(getSceneCameraInfo(pUser), viewIdx);
}

const char* getViewName(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewName(viewIdx);
}

const sead::Matrix34f& getViewMtx_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getViewMtx(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Matrix34f& getViewMtx(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return getLookAtCameraImpl(pInfo, viewIdx).getMatrix();
}

const sead::Matrix34f* getViewMtxPtr(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getViewMtxPtr(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Matrix34f* getViewMtxPtr(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return &getViewMtx(pInfo, viewIdx);
}

const sead::Matrix44f& getProjectionMtx_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getProjectionMtx(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Matrix44f& getProjectionMtx(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return *pInfo->getViewAt(viewIdx)->getProjMtx();
}

const sead::Matrix44f* getProjectionMtxPtr(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getProjectionMtxPtr(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Matrix44f* getProjectionMtxPtr(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getProjMtx();
}

const sead::LookAtCamera& getLookAtCamera(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getLookAtCamera(getSceneCameraInfo(pUser), viewIdx);
}

const sead::LookAtCamera& getLookAtCamera(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return getLookAtCameraImpl(pInfo, viewIdx);
}

const sead::Projection& getProjectionSead(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getProjectionSead_RS(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Projection& getProjectionSead_RS(const SceneCameraInfo* pInfo, s32 viewIdx) {
    const CameraViewInfo* view = pInfo->getViewAt(viewIdx);
    return view->getProjectionSead();
}

const Projection& getProjection(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getProjection(getSceneCameraInfo(pUser), viewIdx);
}

const Projection& getProjection(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getProjection();
}

const sead::Vector3f& getCameraPos_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getCameraPos_RS(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Vector3f& getCameraPos_RS(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return getLookAtCamera(pInfo, viewIdx).getPos();
}

const sead::Vector3f& getCameraAt_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getCameraAt_RS(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Vector3f& getCameraAt_RS(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return getLookAtCamera(pInfo, viewIdx).getAt();
}

const sead::Vector3f& getCameraUp_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getCameraUp_RS(getSceneCameraInfo(pUser), viewIdx);
}

const sead::Vector3f& getCameraUp_RS(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return getLookAtCamera(pInfo, viewIdx).getUp();
}

f32 getFovyDegree_RS(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getFovyDegree_RS(getSceneCameraInfo(pUser), viewIdx);
}

f32 getFovyDegree_RS(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return sead::Mathf::rad2deg(getFovy(pInfo, viewIdx));
}

f32 getFovy(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getFovy(getSceneCameraInfo(pUser), viewIdx);
}

f32 getFovy(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getProjection().getFovy();
}

f32 getNear(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getNear(getSceneCameraInfo(pUser), viewIdx);
}

f32 getNear(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getNear();
}

f32 getFar(const IUseCamera_RS* pUser, s32 viewIdx) {
    return getFar(getSceneCameraInfo(pUser), viewIdx);
}

f32 getFar(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->getFar();
}

void setCameraReset(const IUseCamera_RS* pUser, bool isReset) {
    getCameraDirector(pUser)->getCurrentTicket()->getPoser()->startCameraReset(isReset);
}

f32 getCameraVerticalAngle(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getCurrentTicket()->getPoser()->getVerticalAngle();
}

f32 calcCameraDistance(const IUseCamera_RS* pUser, s32 viewIdx) {
    return (getCameraPos_RS(pUser, viewIdx) - getCameraAt_RS(pUser, viewIdx)).length();
}

f32 calcFovxDegree(const IUseCamera_RS* pUser, s32 viewIdx) {
    f32 aspect = getSceneCameraInfo(pUser)->getViewAt(viewIdx)->getAspect();
    return aspect * getFovyDegree_RS(pUser, viewIdx);
}

f32 calcCurrentFovyRate(const IUseCamera_RS* pUser, s32 viewIdx) {
    f32 fovy = getFovyDegree_RS(pUser, viewIdx);
    f32 sceneFovy = getCameraDirector(pUser)->getSceneFovyDegree();

    if (isNearZero(fovy, 0.001f) || isNearZero(sceneFovy, 0.001f)) {
        return 0.0f;
    }

    return fovy / sceneFovy;
}

void calcCameraFront(sead::Vector3f* pFront, const IUseCamera_RS* pUser, s32 viewIdx) {
    pFront->set(getCameraAt_RS(pUser, viewIdx) - getCameraPos_RS(pUser, viewIdx));
    normalize(pFront);
}

void setNearClipDistance(const IUseCamera_RS* pUser, f32 distance, s32 updaterIdx) {
    getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->setNearClipDistance(distance);
}

void setFarClipDistance(const IUseCamera_RS* pUser, f32 distance, s32 updaterIdx) {
    getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->setFarClipDistance(distance);
}

void setCurrentCameraPose(CameraPoseInfo* pPoseInfo, const IUseCamera_RS* pUser) {
    pPoseInfo->pos.set(getCameraPos_RS(pUser, 0));
    pPoseInfo->at.set(getCameraAt_RS(pUser, 0));
    pPoseInfo->up.set(getCameraUp_RS(pUser, 0));
}

void calcCameraDir_RS(sead::Vector3f* pDir, const IUseCamera_RS* pUser, s32 viewIdx) {
    const sead::Matrix34f& mtx = getLookAtCamera(pUser, viewIdx).getMatrix();
    pDir->set(mtx(2, 0), mtx(2, 1), mtx(2, 2));
}

void calcCameraLookDir(sead::Vector3f* pDir, const IUseCamera_RS* pUser, s32 viewIdx) {
    calcCameraDir_RS(pDir, pUser, viewIdx);
    pDir->negate();
}

void calcCameraSideDir(sead::Vector3f* pDir, const IUseCamera_RS* pUser, s32 viewIdx) {
    getLookAtCamera(pUser, viewIdx).getRightVectorByMatrix(pDir);
}

bool tryCalcCameraDir(sead::Vector3f* pDir, const SceneCameraInfo* pInfo, s32 viewIdx) {
    const sead::Matrix34f& mtx = getLookAtCamera(pInfo, viewIdx).getMatrix();
    pDir->set(mtx(2, 0), mtx(2, 1), mtx(2, 2));
    return tryNormalizeOrZero(pDir);
}

bool tryCalcCameraDirH(sead::Vector3f* pDir, const SceneCameraInfo* pInfo,
                       const sead::Vector3f& rUp, s32 viewIdx) {
    const sead::Matrix34f& mtx = getLookAtCamera(pInfo, viewIdx).getMatrix();
    sead::Vector3f dir(mtx(2, 0), mtx(2, 1), mtx(2, 2));
    verticalizeVec(pDir, rUp, dir);
    return tryNormalizeOrZero(pDir);
}

bool tryCalcCameraLookDirH(sead::Vector3f* pDir, const SceneCameraInfo* pInfo,
                           const sead::Vector3f& rUp, s32 viewIdx) {
    const sead::Matrix34f& mtx = getLookAtCamera(pInfo, viewIdx).getMatrix();
    sead::Vector3f dir(mtx(2, 0), mtx(2, 1), mtx(2, 2));
    verticalizeVec(pDir, rUp, dir);

    if (!tryNormalizeOrZero(pDir)) {
        return false;
    }

    pDir->negate();
    return true;
}

void startCamera_RS(const IUseCamera_RS* pUser, CameraTicket* pTicket, s32 interpoleStep) {
    getCameraDirector(pUser)->getSceneCameraCtrl()->getViewCtrl(0)->getRequester()->requestStart(
        pTicket, interpoleStep);
}

void startCameraSub(const IUseCamera_RS* pUser, CameraTicket* pTicket, s32 interpoleStep) {
    getCameraDirector(pUser)->getSceneCameraCtrl()->getViewCtrl(1)->getRequester()->requestStart(
        pTicket, interpoleStep);
}

void startAnimCamera_RS(const IUseCamera_RS* pUser, CameraTicket* pTicket, const char* pAnimName,
                        s32 interpoleStep) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->setAnim(pAnimName, -1, -1, -1);
    startCamera_RS(pUser, pTicket, interpoleStep);
}

void startAnimCameraAnim(CameraTicket* pTicket, const char* pAnimName, s32 startStep, s32 endStep,
                         s32 playStep) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())
        ->setAnim(pAnimName, startStep, endStep, playStep);
}

void startAnimCameraWithStartStepAndEndStepAndPlayStep(const IUseCamera_RS* pUser,
                                                       CameraTicket* pTicket,
                                                       const char* pAnimName, s32 startStep,
                                                       s32 endStep, s32 playStep,
                                                       s32 interpoleStep) {
    startAnimCameraAnim(pTicket, pAnimName, startStep, endStep, playStep);
    startCamera_RS(pUser, pTicket, interpoleStep);
}

void endAnimCamera_RS(const IUseCamera_RS* pUser, CameraTicket* pTicket) {
    CameraPoserAnim_RS* poser = static_cast<CameraPoserAnim_RS*>(pTicket->getPoser());
    poser->setAnimEnd();
    poser->end();
}

void endCamera_RS(const IUseCamera_RS* pUser, CameraTicket* pTicket, s32 interpoleStep,
                  bool isKeepPose) {
    getCameraDirector(pUser)->getSceneCameraCtrl()->getViewCtrl(0)->getRequester()->requestEnd(
        pTicket, interpoleStep, isKeepPose);
}

void endCameraWithNextCameraPose(const IUseCamera_RS* pUser, CameraTicket* pTicket,
                                 const CameraPoseInfo* pPoseInfo, s32 interpoleStep) {
    getCameraDirector(pUser)
        ->getSceneCameraCtrl()
        ->getViewCtrl(0)
        ->getRequester()
        ->requestEndWithNextCameraPose(pTicket, pPoseInfo, interpoleStep);
}

void endCameraSub(const IUseCamera_RS* pUser, CameraTicket* pTicket, s32 interpoleStep) {
    getCameraDirector(pUser)->getSceneCameraCtrl()->getViewCtrl(1)->getRequester()->requestEnd(
        pTicket, interpoleStep, false);
}

bool isActiveCamera(const CameraTicket* pTicket) {
    return pTicket->isActiveCamera();
}

CameraTicket* initObjectCamera_RS(const IUseCamera_RS* pUser, const PlacementInfo& rInfo,
                                  const char* pSuffix, const char* pPoserName) {
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    tryGetZoneMatrixTR(&zoneMtx, rInfo);
    CameraDirector_RS* director = getCameraDirector(pUser);
    PlacementId* placementId = new PlacementId();
    placementId->init(rInfo);
    return director->createObjectCamera(placementId, pSuffix, pPoserName,
                                        CameraTicket::Priority_Object, zoneMtx);
}

void initCameraTicket(CameraTicket* pTicket, const IUseCamera_RS* pUser) {
    getCameraDirector(pUser)->initCameraPoser(pTicket->getPoser());
    getCameraDirector(pUser)->registerCameraTicket(pTicket);
}

CameraTicket* initObjectCamera_RS(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                  const char* pSuffix) {
    const PlacementInfo& placementInfo = getPlacementInfo(rInfo);
    PlacementId* placementId = new PlacementId();
    placementId->init(placementInfo);
    CameraTicketId* ticketId = new CameraTicketId(placementId, pSuffix);
    CameraTicket* ticket =
        getCameraDirector(pUser)->initCreateObjectCamera(ticketId, &getPlacementInfo(rInfo));
    initCameraTicket(ticket, pUser);
    return ticket;
}

CameraTicket* initObjectCameraManual_RS(const IUseCamera_RS* pUser, const char* pPoserName,
                                        const ActorInitInfo& rInfo) {
    const PlacementInfo& placementInfo = getPlacementInfo(rInfo);
    PlacementId* placementId = new PlacementId();
    placementId->init(placementInfo);
    CameraTicketId* ticketId = new CameraTicketId(placementId, nullptr);
    CameraTicket* ticket = getCameraDirector(pUser)->initCreateObjectCameraManual(
        ticketId, pPoserName, &getPlacementInfo(rInfo));
    initCameraTicket(ticket, pUser);
    return ticket;
}

CameraTicket* tryInitObjectCamera_RS(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                     const char* pSuffix) {
    if (!getCameraDirector(pUser)->isObjectCameraExist(rInfo.getPlacementInfo())) {
        return nullptr;
    }

    const PlacementInfo& placementInfo = getPlacementInfo(rInfo);
    PlacementId* placementId = new PlacementId();
    placementId->init(placementInfo);
    CameraTicketId* ticketId = new CameraTicketId(placementId, pSuffix);
    CameraTicket* ticket =
        getCameraDirector(pUser)->initCreateObjectCamera(ticketId, &getPlacementInfo(rInfo));
    initCameraTicket(ticket, pUser);
    return ticket;
}

CameraTicket* initObjectCameraNoPlacementInfo(const IUseCamera_RS* pUser, const char* pSuffix,
                                              const char* pPoserName) {
    return getCameraDirector(pUser)->createObjectCamera(
        nullptr, pSuffix, pPoserName, CameraTicket::Priority_Object, sead::Matrix34f::ident);
}

CameraTicket* initFixCamera(const IUseCamera_RS* pUser, const char* pSuffix,
                            const sead::Vector3f& rCameraPos, const sead::Vector3f& rLookAtPos) {
    CameraPoserFix* poser = new CameraPoserFix("固定");
    CameraTicket* ticket = getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
    poser->initCameraPosAndLookAtPos(rCameraPos, rLookAtPos);
    return ticket;
}

}  // namespace al

namespace alCameraFunction {

al::CameraTicket* initCameraNoPlacementInfoNoSave(al::CameraPoser_RS* pPoser,
                                                  const al::IUseCamera_RS* pUser,
                                                  const al::PlacementId* pPlacementId,
                                                  const char* pSuffix, s32 priority,
                                                  const sead::Matrix34f& rZoneMtx) {
    return pUser->getCameraDirector_RS()->createCamera(pPoser, pPlacementId, pSuffix, priority,
                                                       rZoneMtx, true);
}

}  // namespace alCameraFunction

namespace al {

CameraTicket* initFixDoorwayCamera(const IUseCamera_RS* pUser, const char* pSuffix,
                                   const sead::Vector3f& rCameraPos,
                                   const sead::Vector3f& rLookAtPos) {
    CameraPoserFix* poser = new CameraPoserFix(CameraPoserFix::getFixDoorwayCameraName());
    CameraTicket* ticket = getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
    poser->initCameraPosAndLookAtPos(rCameraPos, rLookAtPos);
    return ticket;
}

CameraTicket* initFixActorCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                 const char* pSuffix, const sead::Vector3f& rOffset, f32 distance,
                                 f32 angleH, f32 angleV, bool isCalcNearestAtFromPreAt) {
    CameraPoserFixActor* poser = new CameraPoserFixActor(pActor);
    poser->mOffset = rOffset;
    poser->mDistance = distance;
    poser->mAngleH = angleH;
    poser->mAngleV = angleV;

    if (isCalcNearestAtFromPreAt) {
        poser->mIsCalcNearestAtFromPreAt = true;
    }

    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

}  // namespace al

namespace alCameraFunction {

al::CameraTicket* initCameraNoSave(al::CameraPoser_RS* pPoser, const al::IUseCamera_RS* pUser,
                                   const al::ActorInitInfo& rInfo, const char* pSuffix,
                                   s32 priority) {
    return initCamera(pPoser, pUser, al::getPlacementInfo(rInfo), pSuffix, priority);
}

}  // namespace alCameraFunction

namespace al {

CameraTicket* initFixLookCamera(LiveActor* pActor, const ActorInitInfo& rInfo,
                                const char* pSuffix) {
    CameraPoserFixLook* poser = new CameraPoserFixLook(pSuffix);
    poser->mTargetTrans = getTransPtr(pActor);
    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initFixTalkCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                const char* pSuffix, const sead::Vector3f& rOffset, f32 distance,
                                f32 angleH, f32 angleV, bool isFlag) {
    CameraPoserFixTalk* poser = new CameraPoserFixTalk(pActor);
    poser->mOffset = rOffset;
    poser->mDistance = distance;
    poser->mTalkAngleH = angleH;
    poser->mAngleV = angleV;

    if (isFlag) {
        poser->_170 = true;
    }

    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initFixFishingCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                   const char* pSuffix, const sead::Vector3f& rOffset,
                                   const sead::Vector3f& rTarget, f32 distance, f32 angleH,
                                   f32 angleV, bool isFlag) {
    CameraPoserFixFishing* poser = new CameraPoserFixFishing(pActor);
    poser->mDistance = distance;
    poser->mAngleV = angleV;
    poser->initParam(angleH, rOffset, rTarget);

    if (isFlag) {
        poser->_170 = true;
    }

    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initFixPointCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                 const char* pSuffix, bool isUsePreCameraPos) {
    CameraPoserFixPoint* poser = new CameraPoserFixPoint("定点");

    if (isUsePreCameraPos) {
        poser->validateUsePreCameraPos();
    }

    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

}  // namespace al

namespace alCameraFunction {

al::CameraTicket* initCamera(al::CameraPoser_RS* pPoser, const al::IUseCamera_RS* pUser,
                             const al::ActorInitInfo& rInfo, const char* pSuffix, s32 priority) {
    return initCamera(pPoser, pUser, al::getPlacementInfo(rInfo), pSuffix, priority);
}

}  // namespace alCameraFunction

namespace al {

CameraTicket* initLookDownCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                 const char* pSuffix) {
    CameraPoserLookDown* poser = new CameraPoserLookDown("見下ろし");
    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initProgramableCamera_RS(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                       const char* pSuffix, const sead::Vector3f* pPos,
                                       const sead::Vector3f* pAt, const sead::Vector3f* pUp) {
    CameraPoserProgramable_RS* poser = new CameraPoserProgramable_RS(pPos, pAt, pUp);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initProgramableCamera(const IUseCamera_RS* pUser, const char* pSuffix,
                                    const sead::Vector3f* pPos, const sead::Vector3f* pAt,
                                    const sead::Vector3f* pUp) {
    CameraPoserProgramable_RS* poser = new CameraPoserProgramable_RS(pPos, pAt, pUp);
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
}

CameraTicket* initProgramableCameraWithCollider(const IUseCamera_RS* pUser,
                                                const ActorInitInfo& rInfo, const char* pSuffix,
                                                const sead::Vector3f* pPos,
                                                const sead::Vector3f* pAt,
                                                const sead::Vector3f* pUp) {
    CameraPoserProgramable_RS* poser = new CameraPoserProgramable_RS(pPos, pAt, pUp);
    CameraTicket* ticket = alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    alCameraPoserFunction::initCameraArrowCollider(poser);
    return ticket;
}

CameraTicket* initProgramableAngleCamera(const IUseCamera_RS* pUser, const PlacementInfo& rInfo,
                                         const char* pSuffix, const sead::Vector3f* pAt,
                                         const f32* pAngleH, const f32* pAngleV,
                                         const f32* pDistance) {
    CameraPoserProgramableAngle* poser =
        new CameraPoserProgramableAngle(pAt, pAngleH, pAngleV, pDistance);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

}  // namespace al

namespace alCameraFunction {

al::CameraTicket* initCameraNoSave(al::CameraPoser_RS* pPoser, const al::IUseCamera_RS* pUser,
                                   const al::PlacementInfo& rInfo, const char* pSuffix,
                                   s32 priority) {
    return initCamera(pPoser, pUser, rInfo, pSuffix, priority);
}

}  // namespace alCameraFunction

namespace al {

CameraTicket* initProgramableCameraKeepColliderPreCamera(const IUseCamera_RS* pUser,
                                                         const ActorInitInfo& rInfo,
                                                         const char* pSuffix,
                                                         const sead::Vector3f* pPos,
                                                         const sead::Vector3f* pAt,
                                                         const sead::Vector3f* pUp) {
    CameraPoserProgramableKeepColliderPreCamera* poser =
        new CameraPoserProgramableKeepColliderPreCamera(pPos, pAt, pUp);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

void setProgramableCameraPos_RS(CameraTicket* pTicket, sead::Vector3f* pPos) {
    static_cast<CameraPoserProgramable_RS*>(pTicket->getPoser())->mPosPtr = pPos;
}

void setProgramableCameraAt_RS(CameraTicket* pTicket, sead::Vector3f* pAt) {
    static_cast<CameraPoserProgramable_RS*>(pTicket->getPoser())->mAtPtr = pAt;
}

CameraTicket* initShooterCameraSingle(const IUseCamera_RS* pUser, const char* pSuffix) {
    CameraPoserShooterSingle* poser = new CameraPoserShooterSingle("シングルシュータカメラ");
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
}

}  // namespace al

namespace alCameraFunction {

al::CameraTicket* initCameraNoPlacementInfo(al::CameraPoser_RS* pPoser,
                                            const al::IUseCamera_RS* pUser,
                                            const al::PlacementId* pPlacementId,
                                            const char* pSuffix, s32 priority,
                                            const sead::Matrix34f& rZoneMtx) {
    return pUser->getCameraDirector_RS()->createCamera(pPoser, pPlacementId, pSuffix, priority,
                                                       rZoneMtx, true);
}

}  // namespace alCameraFunction

namespace al {

CameraTicket* initTowerCameraWithSave(const IUseCamera_RS* pUser, const sead::Vector3f* pPos,
                                      const ActorInitInfo& rInfo, const char* pSuffix) {
    CameraPoserTower_RS* poser = new CameraPoserTower_RS("塔", pPos);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initTowerCamera(const IUseCamera_RS* pUser, const sead::Vector3f* pPos,
                              const ActorInitInfo& rInfo, const char* pSuffix) {
    CameraPoserTower_RS* poser = new CameraPoserTower_RS("塔", pPos);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initBossBattleCamera(const IUseCamera_RS* pUser, const sead::Vector3f* pPos,
                                   const ActorInitInfo& rInfo, const char* pSuffix) {
    CameraPoserBossBattle* poser = new CameraPoserBossBattle("ボス戦カメラ", pPos);
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

void initProgramableCameraAngleSwing(CameraTicket* pTicket) {
    alCameraPoserFunction::initAngleSwing(pTicket->getPoser());
}

CameraTicket* initFollowCameraSimple(const IUseCamera_RS* pUser, const char* pSuffix) {
    CameraPoserFollowSimple* poser = new CameraPoserFollowSimple("フォロー");
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
}

CameraTicket* initFollowCameraSimple(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                     const char* pSuffix) {
    CameraPoserFollowSimple* poser = new CameraPoserFollowSimple("フォロー");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initDemoObjectCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                   const char* pSuffix, const char* pPoserName) {
    const PlacementInfo& placementInfo = getPlacementInfo(rInfo);
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    tryGetZoneMatrixTR(&zoneMtx, placementInfo);
    CameraDirector_RS* director = getCameraDirector(pUser);
    PlacementId* placementId = new PlacementId();
    placementId->init(placementInfo);
    return director->createObjectCamera(placementId, pSuffix, pPoserName,
                                        CameraTicket::Priority_Demo, zoneMtx);
}

CameraTicket* initDemoProgramableCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                        const char* pSuffix, const sead::Vector3f* pPos,
                                        const sead::Vector3f* pAt, const sead::Vector3f* pUp) {
    CameraPoserProgramable_RS* poser = new CameraPoserProgramable_RS(pPos, pAt, pUp);
    CameraTicket* ticket = alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    ticket->setPriority(CameraTicket::Priority_Demo);
    return ticket;
}

CameraTicket* initDemoProgramableCameraKeepColliderPreCamera(
    const IUseCamera_RS* pUser, const ActorInitInfo& rInfo, const char* pSuffix,
    const sead::Vector3f* pPos, const sead::Vector3f* pAt, const sead::Vector3f* pUp) {
    CameraPoserProgramableKeepColliderPreCamera* poser =
        new CameraPoserProgramableKeepColliderPreCamera(pPos, pAt, pUp);
    CameraTicket* ticket = alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    ticket->setPriority(CameraTicket::Priority_Demo);
    return ticket;
}

CameraTicket* initDemoAnimCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                 const Resource* pResource, const sead::Matrix34f* pBaseMtx,
                                 const char* pSuffix, bool isCheckRange) {
    CameraPoserAnim_RS* poser = new CameraPoserAnim_RS();
    poser->mIsCheckRange = isCheckRange;
    CameraTicket* ticket = alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    poser->initAnimResource(pResource, pBaseMtx);
    ticket->setPriority(CameraTicket::Priority_Demo);
    return ticket;
}

CameraTicket* initAnimCamera_RS(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                const Resource* pResource, const sead::Matrix34f* pBaseMtx,
                                const char* pSuffix, bool isCheckRange) {
    CameraPoserAnim_RS* poser = new CameraPoserAnim_RS();
    poser->mIsCheckRange = isCheckRange;
    CameraTicket* ticket = alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    poser->initAnimResource(pResource, pBaseMtx);
    return ticket;
}

CameraTicket* initDemoAnimCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                 const char* pSuffix, bool isCheckRange) {
    const Resource* resource = getAnimResource(pActor);
    const IUseCamera_RS* user = pActor;
    const sead::Matrix34f* baseMtx = pActor->getBaseMtx();
    CameraPoserAnim_RS* poser = new CameraPoserAnim_RS();
    poser->mIsCheckRange = isCheckRange;
    CameraTicket* ticket = alCameraFunction::initCamera(poser, user, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    poser->initAnimResource(resource, baseMtx);
    ticket->setPriority(CameraTicket::Priority_Demo);
    return ticket;
}

void loadActorCameraParam(CameraTicket* pTicket, const LiveActor* pActor, const char* pName,
                          const char* pSuffix) {
    if (isExistModelOrAnimResourceYaml(pActor, pName, pSuffix)) {
        ByamlIter iter(getModelOrAnimResourceYaml(pActor, pName, pSuffix));
        pTicket->getPoser()->load(iter);
    }
}

void loadActorCameraParamInitFile(CameraTicket* pTicket, const LiveActor* pActor,
                                  const char* pSuffix) {
    ByamlIter iter;

    if (tryGetActorInitFileIter(&iter, pActor, "InitCamera", pSuffix)) {
        pTicket->getPoser()->load(iter);
    }
}

void setFixedActor(const CameraTicket* pTicket, const LiveActor* pActor, f32 distance) {
    CameraPoserFixActor* poser = static_cast<CameraPoserFixActor*>(pTicket->getPoser());

    if (isEqualString(poser->getName(), "FixedActor")) {
        poser->mTargetActor = pActor;
        poser->mDistance = distance;
    }
}

void setFixedActor(const CameraTicket* pTicket, const LiveActor* pActor, f32 distance,
                   f32 angleH) {
    CameraPoserFixActor* poser = static_cast<CameraPoserFixActor*>(pTicket->getPoser());

    if (isEqualString(poser->getName(), "FixedActor")) {
        poser->mTargetActor = pActor;
        poser->mDistance = distance;
        poser->mAngleH = angleH;
    }
}

void setFixedActor(const CameraTicket* pTicket, const LiveActor* pActor, f32 distance,
                   f32 angleH, f32 angleV, const sead::Vector3f* pOffset) {
    CameraPoserFixActor* poser = static_cast<CameraPoserFixActor*>(pTicket->getPoser());

    if (isEqualString(poser->getName(), "FixedActor")) {
        poser->mTargetActor = pActor;
        poser->mDistance = distance;
        poser->mAngleH = angleH;
        poser->mAngleV = angleV;

        if (pOffset) {
            poser->mOffset = *pOffset;
        }
    }
}

void setFixActorCameraTarget(CameraTicket* pTicket, const LiveActor* pActor) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setTargetActor(pActor);
}

void setFixActorCameraAngleH(CameraTicket* pTicket, f32 angleH) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setAngleH(angleH);
}

void setFixActorCameraDirectAngle(CameraTicket* pTicket) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setDirectAngle();
}

void setFixActorCameraDirectAngle(CameraTicket* pTicket, sead::Vector3f dir) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setDirectAngle(dir);
}

void setFixActorCameraAngleV(CameraTicket* pTicket, f32 angleV) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setAngleV(angleV);
}

void setFixActorCameraDistance(CameraTicket* pTicket, f32 distance) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setDistance(distance);
}

void setFixActorCameraOffset(CameraTicket* pTicket, sead::Vector3f offset) {
    static_cast<CameraPoserFixActor*>(pTicket->getPoser())->setOffset(offset);
}

void setTowerCameraDistance(CameraTicket* pTicket, f32 distance) {
    static_cast<CameraPoserTower_RS*>(pTicket->getPoser())->mDistance = distance;
}

void setTowerCameraStartAngleV(CameraTicket* pTicket, f32 angleV) {
    alCameraPoserFunction::setCameraStartAngleV(pTicket->getPoser(), angleV);
}

void setTowerCameraUserMarginAngleH(CameraTicket* pTicket, f32 angleH) {
    static_cast<CameraPoserTower_RS*>(pTicket->getPoser())->mUserMarginAngleH = angleH;
}

void resetTowerCameraUserMarginAngleH(CameraTicket* pTicket) {
    static_cast<CameraPoserTower_RS*>(pTicket->getPoser())->mUserMarginAngleH = -1.0f;
}

void resetTowerCameraInputRotate(CameraTicket* pTicket, f32 angle, s32 step) {
    static_cast<CameraPoserTower_RS*>(pTicket->getPoser())->resetInputRotate(angle, step);
}

}  // namespace al

void validateFixPointCameraUsePreCameraPos(al::CameraTicket* pTicket) {
    static_cast<al::CameraPoserFixPoint*>(pTicket->getPoser())->validateUsePreCameraPos();
}

namespace al {

CameraTicket* initSubjectiveCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                   const char* pSuffix) {
    CameraPoserSubjective_RS* poser = new CameraPoserSubjective_RS("主観");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Player);
}

CameraTicket* initSubjectiveCameraNoSave(const IUseCamera_RS* pUser, const char* pSuffix) {
    CameraPoserSubjective_RS* poser = new CameraPoserSubjective_RS("主観");
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Player, sead::Matrix34f::ident, true);
}

f32 getSubjectiveCameraOffsetUp(const CameraTicket* pTicket) {
    return static_cast<CameraPoserSubjective_RS*>(pTicket->getPoser())->mCameraOffsetUp;
}

f32 getSubjectiveCameraOffsetFront() {
    return CameraPoserSubjective_RS::getCameraOffsetFront();
}

void setSubjectiveCameraStartAngleH(const CameraTicket* pTicket, f32 angleH) {
    CameraPoserSubjective_RS* poser = static_cast<CameraPoserSubjective_RS*>(pTicket->getPoser());
    poser->mIsSetStartAngleH = true;
    poser->mStartAngleH = angleH;
}

void validateSubjectiveCameraResetAngleH(CameraTicket* pTicket) {
    static_cast<CameraPoserSubjective_RS*>(pTicket->getPoser())->mIsValidResetAngleH = true;
}

void requestSubjectiveCameraZoomIn(CameraTicket* pTicket) {
    static_cast<CameraPoserSubjective_RS*>(pTicket->getPoser())->mIsRequestZoomIn = true;
}

CameraTicket* initParallelCamera(const IUseCamera_RS* pUser, const char* pSuffix) {
    CameraPoserParallelSimple* poser = new CameraPoserParallelSimple("並行");
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
}

CameraTicket* initParallelCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                 const char* pSuffix) {
    CameraPoserParallelSimple* poser = new CameraPoserParallelSimple("並行");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

void setParallelCameraLookAtOffset(const CameraTicket* pTicket, const sead::Vector3f& rOffset) {
    static_cast<CameraPoserParallelSimple*>(pTicket->getPoser())->mLookAtOffset.set(rOffset);
}

void setParallelCameraDistance(const CameraTicket* pTicket, f32 distance) {
    static_cast<CameraPoserParallelSimple*>(pTicket->getPoser())->mDistance = distance;
}

void setParallelCameraAngleH(const CameraTicket* pTicket, f32 angleH) {
    static_cast<CameraPoserParallelSimple*>(pTicket->getPoser())->mAngleH = angleH;
}

void setParallelCameraAngleV(const CameraTicket* pTicket, f32 angleV) {
    static_cast<CameraPoserParallelSimple*>(pTicket->getPoser())->mAngleV = angleV;
}

CameraTicket* initQuickTurnCamera(const IUseCamera_RS* pUser, const char* pSuffix) {
    CameraPoserQuickTurn* poser = new CameraPoserQuickTurn("くるっとターン");
    return getCameraDirector(pUser)->createCamera(
        poser, nullptr, pSuffix, CameraTicket::Priority_Object, sead::Matrix34f::ident, true);
}

void setQuickTurnCameraFollow(CameraTicket* pTicket) {
    static_cast<CameraPoserQuickTurn*>(pTicket->getPoser())->setFollow();
}

void setQuickTurnCameraRotateFast(CameraTicket* pTicket) {
    static_cast<CameraPoserQuickTurn*>(pTicket->getPoser())->mIsRotateFast = true;
}

CameraTicket* initRaceCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                             const char* pSuffix) {
    CameraPoserRace* poser = new CameraPoserRace("レース");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

void setRaceCameraFrontDirPtr(const CameraTicket* pTicket, const sead::Vector3f* pFrontDir) {
    static_cast<CameraPoserRace*>(pTicket->getPoser())->mFrontDirPtr = pFrontDir;
}

void setRaceCameraDistance(const CameraTicket* pTicket, f32 distance) {
    static_cast<CameraPoserRace*>(pTicket->getPoser())->mDistance = distance;
}

void setRaceCameraOffsetY(const CameraTicket* pTicket, f32 offsetY) {
    static_cast<CameraPoserRace*>(pTicket->getPoser())->mOffsetY = offsetY;
}

void setRaceCameraAngleDegreeV(const CameraTicket* pTicket, f32 angleV) {
    static_cast<CameraPoserRace*>(pTicket->getPoser())->mAngleDegreeV = angleV;
}

CameraTicket* initCartCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                             const char* pSuffix) {
    CameraPoserCart* poser = new CameraPoserCart("カート");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

void stopCartCamera(const CameraTicket* pTicket) {
    static_cast<CameraPoserCart*>(pTicket->getPoser())->stop();
}

void restartCartCamera(const CameraTicket* pTicket) {
    static_cast<CameraPoserCart*>(pTicket->getPoser())->restart();
}

CameraTicket* initActorRailParallelCamera(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                          const char* pSuffix) {
    CameraPoserActorRailParallel* poser =
        new CameraPoserActorRailParallel("アクターレール並行", pActor->getRailKeeper());
    return alCameraFunction::initCamera(poser, pActor, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initKinopioBrigadeCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                       const char* pSuffix) {
    CameraPoserKinopioBrigade_RS* poser = new CameraPoserKinopioBrigade_RS("キノピオ探検隊");
    return alCameraFunction::initCamera(poser, pUser, rInfo, pSuffix,
                                        CameraTicket::Priority_Object);
}

CameraTicket* initAnimCamera_RS(const LiveActor* pActor, const ActorInitInfo& rInfo,
                                const char* pSuffix) {
    const Resource* resource = getAnimResource(pActor);
    const IUseCamera_RS* user = pActor;
    const sead::Matrix34f* baseMtx = pActor->getBaseMtx();
    CameraPoserAnim_RS* poser = new CameraPoserAnim_RS();
    poser->mIsCheckRange = false;
    CameraTicket* ticket = alCameraFunction::initCamera(poser, user, rInfo, pSuffix,
                                                        CameraTicket::Priority_Object);
    poser->initAnimResource(resource, baseMtx);
    return ticket;
}

void validateAnimCameraAngleSwing(CameraTicket* pTicket) {
    pTicket->getPoser()->getAngleSwingInfo()->isInvalidSwing = false;
}

void invalidateAnimCameraAngleSwing(CameraTicket* pTicket) {
    pTicket->getPoser()->getAngleSwingInfo()->isInvalidSwing = true;
}

void setAnimCameraBaseMtxPtr(CameraTicket* pTicket, const sead::Matrix34f* pBaseMtx) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mBaseMtxPtr = pBaseMtx;
}

void setAnimCameraLookAtOffset(CameraTicket* pTicket, sead::Vector3f offset) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mLookAtOffset.set(offset);
}

CameraTicket* initEntranceCamera(const IUseCamera_RS* pUser, const PlacementInfo& rInfo,
                                 const char* pSuffix) {
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    tryGetZoneMatrixTR(&zoneMtx, rInfo);
    CameraDirector_RS* director = getCameraDirector(pUser);
    PlacementId* placementId = new PlacementId();
    placementId->init(rInfo);
    return director->createObjectEntranceCamera(placementId, pSuffix, zoneMtx);
}

CameraTicket* initEntranceCamera(const IUseCamera_RS* pUser, const ActorInitInfo& rInfo,
                                 const char* pSuffix) {
    const PlacementInfo& placementInfo = getPlacementInfo(rInfo);
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    tryGetZoneMatrixTR(&zoneMtx, placementInfo);
    CameraDirector_RS* director = getCameraDirector(pUser);
    PlacementId* placementId = new PlacementId();
    placementId->init(placementInfo);
    return director->createObjectEntranceCamera(placementId, pSuffix, zoneMtx);
}

CameraTicket* initEntranceCameraNoSave(const IUseCamera_RS* pUser, const PlacementInfo& rInfo,
                                       const char* pSuffix) {
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    tryGetZoneMatrixTR(&zoneMtx, rInfo);
    CameraDirector_RS* director = getCameraDirector(pUser);
    PlacementId* placementId = new PlacementId();
    placementId->init(rInfo);
    return director->createObjectEntranceCamera(placementId, pSuffix, zoneMtx);
}

void setEntranceCameraParam(CameraTicket* pTicket, f32 distance, const sead::Vector3f& rCameraPos,
                            const sead::Vector3f& rLookAtPos) {
    static_cast<CameraPoserEntrance_RS*>(pTicket->getPoser())
        ->initParam(distance, rCameraPos, rLookAtPos);
}

void setEntranceCameraLookAt(CameraTicket* pTicket, const sead::Vector3f& rLookAtPos) {
    static_cast<CameraPoserEntrance_RS*>(pTicket->getPoser())->initLookAtPosDirect(rLookAtPos);
}

void invalidateEndEntranceCamera(LiveActor* pActor) {
    invalidateEndEntranceCameraWithName(pActor, pActor->getName());
}

void invalidateEndEntranceCameraWithName(IUseCamera_RS* pUser, const char* pName) {
    getCameraDirector(pUser)->getFlagCtrl()->isInvalidEndEntranceCamera = true;
}

void validateEndEntranceCamera(IUseCamera_RS* pUser) {
    getCameraDirector(pUser)->getFlagCtrl()->isInvalidEndEntranceCamera = false;
}

bool isPlayingEntranceCamera(const IUseCamera_RS* pUser, s32 updaterIdx) {
    if (getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->isCurrentCameraPriority(
            CameraTicket::Priority_Entrance)) {
        return true;
    }

    return getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->isCurrentCameraPriority(
        CameraTicket::Priority_EntranceSub);
}

s32 getCameraInterpoleStep(CameraTicket* pTicket) {
    return pTicket->getPoser()->getInterpoleStep();
}

void setCameraInterpoleStep(CameraTicket* pTicket, s32 step) {
    pTicket->getPoser()->setInterpoleStep(step);
}

void setCameraEndInterpoleStep(CameraTicket* pTicket, s32 step) {
    pTicket->getPoser()->setEndInterpoleStep(step);
}

void setCameraFovyDegree(CameraTicket* pTicket, f32 fovy) {
    pTicket->getPoser()->setFovyDegree(fovy);
}

SimpleCameraInput* createSimpleCameraInput(s32 port) {
    return new SimpleCameraInput(port);
}

void setCameraInput(IUseCamera_RS* pUser, const ICameraInput* pInput) {
    getCameraDirector(pUser)->setCameraInput(pInput);
}

void setViewCameraInput(IUseCamera_RS* pUser, const ICameraInput* pInput, s32 viewIdx) {
    getCameraDirector(pUser)->setViewCameraInput(pInput, viewIdx);
}

bool isExistCameraInputAtDisableTiming(const IUseCamera_RS* pUser, s32 inputIdx) {
    if (getCameraDirector(pUser)->getPoseUpdater(0)->isCurrentCameraEnableRotateByPad()) {
        return false;
    }

    sead::Vector2f stick = {0.0f, 0.0f};
    getCameraDirector(pUser)->getCameraInput(inputIdx)->calcInputStick(&stick);
    return !isNearZero(stick, 0.001f);
}

bool isCurrentCameraEnableRotateByPad(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getPoseUpdater(0)->isCurrentCameraEnableRotateByPad();
}

}  // namespace al

namespace al {

ActorCameraTarget* createActorCameraTarget(const LiveActor* pActor, f32 offsetY) {
    return new ActorCameraTarget(pActor, offsetY, nullptr);
}

ActorCameraTarget* createActorCameraTarget(const LiveActor* pActor,
                                           const sead::Vector3f* pLocalOffset) {
    return new ActorCameraTarget(pActor, 0.0f, pLocalOffset);
}

ActorMatrixCameraTarget* createActorJointCameraTarget(const LiveActor* pActor,
                                                      const char* pJointName) {
    return new ActorMatrixCameraTarget(pActor, getJointMtxPtr(pActor, pJointName));
}

ActorMatrixCameraTarget* createActorMatrixCameraTarget(const LiveActor* pActor,
                                                       const sead::Matrix34f* pMtx) {
    return new ActorMatrixCameraTarget(pActor, pMtx);
}

bool isActiveCameraTarget(const CameraTargetBase* pTarget) {
    return pTarget->isActiveTarget();
}

void setCameraTarget(IUseCamera_RS* pUser, CameraTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->addTarget(pTarget);
}

void resetCameraTarget(IUseCamera_RS* pUser, CameraTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->removeTarget(pTarget);
}

ActorCameraSubTarget* createActorCameraSubTarget(const LiveActor* pActor,
                                                 const sead::Vector3f* pOffset) {
    ActorCameraSubTarget* target = new ActorCameraSubTarget(pActor);

    if (pOffset) {
        target->setOffset(pOffset);
    }

    return target;
}

ActorBackAroundCameraSubTarget* createActorBackAroundCameraSubTarget(
    const LiveActor* pActor, const sead::Vector3f* pOffset) {
    ActorBackAroundCameraSubTarget* target = new ActorBackAroundCameraSubTarget(pActor);

    if (pOffset) {
        target->setOffset(pOffset);
    }

    return target;
}

TransCameraSubTarget* createTransCameraSubTarget(const char* pName,
                                                 const sead::Vector3f* pTrans) {
    return new TransCameraSubTarget(pName, pTrans);
}

void initCameraSubTargetTurnParam(CameraSubTargetBase* pTarget,
                                  const CameraSubTargetTurnParam* pParam) {
    pTarget->setSubTargetTurnParam(pParam);
}

bool isActiveCameraSubTarget(const CameraSubTargetBase* pTarget) {
    return pTarget->isActiveTarget();
}

void setCameraSubTarget(IUseCamera_RS* pUser, CameraSubTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->addSubTarget(pTarget);
}

void resetCameraSubTarget(IUseCamera_RS* pUser, CameraSubTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->removeSubTarget(pTarget);
}

void setCameraPlacementSubTarget(IUseCamera_RS* pUser, CameraSubTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->addPlacementSubTarget(pTarget);
}

void resetCameraPlacementSubTarget(IUseCamera_RS* pUser, CameraSubTargetBase* pTarget) {
    getCameraDirector(pUser)->getTargetHolder()->removePlacementSubTarget(pTarget);
}

const CameraDistanceCurve* getCameraDistanceRocketFlowerCurve() {
    return CameraDistanceCurve::getRocketFlowerCurve();
}

void setViewCameraTarget(IUseCamera_RS* pUser, CameraTargetBase* pTarget, s32 viewIdx) {
    getCameraDirector(pUser)->getTargetHolder()->setViewTarget(pTarget, viewIdx);
}

void startCameraShakeByAction(const LiveActor* pActor, const char* pShakeName,
                              const char* pActionName, s32 steps, s32 viewIdx) {
    const IUseCamera_RS* user = pActor;
    CameraDirector_RS* director = getCameraDirector(user);

    if (viewIdx >= 0) {
        director->getPoseUpdater(viewIdx)->getShaker()->startShakeByAction(
            pShakeName, pActor->getName(), pActionName, steps);
        return;
    }

    s32 viewNum = director->getSceneCameraInfo()->getViewNumMax();

    for (s32 i = 0; i < viewNum; i++) {
        getCameraDirector(user)->getPoseUpdater(i)->getShaker()->startShakeByAction(
            pShakeName, pActor->getName(), pActionName, steps);
    }
}

void startCameraShakeByHitReaction(const IUseCamera_RS* pUser, const char* pShakeName,
                                   const char* pReactionName, const char* pActorName, s32 steps,
                                   s32 viewIdx) {
    CameraDirector_RS* director = getCameraDirector(pUser);

    if (viewIdx >= 0) {
        director->getPoseUpdater(viewIdx)->getShaker()->startShakeByHitReaction(
            pShakeName, pReactionName, pActorName, steps);
        return;
    }

    s32 viewNum = director->getSceneCameraInfo()->getViewNumMax();

    for (s32 i = 0; i < viewNum; i++) {
        getCameraDirector(pUser)->getPoseUpdater(i)->getShaker()->startShakeByHitReaction(
            pShakeName, pReactionName, pActorName, steps);
    }
}

void requestCameraLoopShakeWeak(const IUseCamera_RS* pUser) {
    getCameraDirector(pUser)->getSceneCameraCtrl()->getViewCtrl(0)->setShakeName("弱");
}

}  // namespace al

namespace alCameraFunction {

void requestCameraShakeLoop(const al::IUseCamera_RS* pUser, const char* pShakeName) {
    pUser->getCameraDirector_RS()->getSceneCameraCtrl()->getViewCtrl(0)->setShakeName(pShakeName);
}

}  // namespace alCameraFunction

namespace al {

void cancelCameraShake(const IUseCamera_RS* pUser) {
    s32 viewNum = getCameraDirector(pUser)->getSceneCameraInfo()->getViewNumMax();

    for (s32 i = 0; i < viewNum; i++) {
        getCameraDirector(pUser)->getPoseUpdater(i)->getShaker()->cancelShake();
    }
}

bool isActiveCameraInterpole(const IUseCamera_RS* pUser, s32 updaterIdx) {
    return getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->isActiveInterpole();
}

bool isActiveCameraInterpole(const SceneCameraInfo* pInfo, s32 viewIdx) {
    return pInfo->getViewAt(viewIdx)->isActiveInterpole();
}

void startCameraInterpole(const IUseCamera_RS* pUser, s32 updaterIdx, s32 step) {
    getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->startInterpole(step);
}

void requestCancelCameraInterpole(const IUseCamera_RS* pUser, s32 updaterIdx) {
    getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->requestCancelInterpole();
}

bool tryCalcCameraPoseWithoutInterpole(sead::LookAtCamera* pCamera, const IUseCamera_RS* pUser,
                                       s32 updaterIdx) {
    return getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->calcCameraPoseWithoutInterpole(
        pCamera);
}

void invalidateCameraPoserVerticalAbsorber(CameraTicket* pTicket) {
    CameraVerticalAbsorber* absorber = pTicket->getPoser()->getCameraVerticalAbsorber();

    if (absorber != nullptr) {
        absorber->invalidate();
    }
}

void requestStopCameraVerticalAbsorb(IUseCamera_RS* pUser) {
    CameraObjectRequestInfo info;
    info.isStopVerticalAbsorb = true;
    getCameraDirector(pUser)->getPoseUpdater(0)->tryReceiveCameraRequestFromObject(info);
}

void validateSnapShotCameraZoomFovy(CameraTicket* pTicket) {
    pTicket->getPoser()->getSnapShotCtrl()->setIsValidZoomFovy(true);
}

void validateSnapShotCameraRoll(CameraTicket* pTicket) {
    pTicket->getPoser()->getSnapShotCtrl()->setIsValidRoll(true);
}

bool isSnapShotOrientationRotate90(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getPoseUpdater(0)->isSnapShotOrientationRotate90();
}

bool isSnapShotOrientationRotate270(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getPoseUpdater(0)->isSnapShotOrientationRotate270();
}

bool isValidCameraGyro(const IUseCamera_RS* pUser) {
    return !getCameraDirector(pUser)->getFlagCtrl()->isInvalidCameraGyro;
}

bool isInvalidChangeSubjectiveCamera(const IUseCamera_RS* pUser) {
    return getCameraDirector(pUser)->getPoseUpdater(0)->isInvalidChangeSubjectiveCamera();
}

bool isCurrentCameraZooming(const IUseCamera_RS* pUser, s32 updaterIdx) {
    return getCameraDirector(pUser)->getPoseUpdater(updaterIdx)->isCurrentCameraZooming();
}

void onCameraRideObj(const LiveActor* pActor) {
    getCameraDirector(pActor)->getSceneCameraCtrl()->getRequestParamHolder()->onRideObj(
        pActor, pActor->getName());
}

void offCameraRideObj(const LiveActor* pActor) {
    getCameraDirector(pActor)->getSceneCameraCtrl()->getRequestParamHolder()->offRideObj(
        pActor, pActor->getName());
}

bool isExistAnimCameraData(const CameraTicket* pTicket, const char* pAnimName) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->isExistAnim(pAnimName);
}

bool isEndAnimCamera(const CameraTicket* pTicket) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->isAnimEnd();
}

bool isAnimCameraPlaying(const CameraTicket* pTicket) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mStep >= 0;
}

bool isAnimCameraAnimPlaying(const CameraTicket* pTicket, const char* pAnimName) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->isAnimPlaying(pAnimName);
}

s32 getAnimCameraStepMax(const CameraTicket* pTicket) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mStepMax;
}

s32 getAnimCameraStep(const CameraTicket* pTicket) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mStep;
}

s32 calcAnimCameraAnimStepMax(const CameraTicket* pTicket, const char* pAnimName) {
    return static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->calcStepMax(pAnimName);
}

void setAnimCameraRotateBaseUp(const CameraTicket* pTicket) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mIsRotateBaseUp = true;
}

void setAnimCameraStep(const CameraTicket* pTicket, s32 step) {
    static_cast<CameraPoserAnim_RS*>(pTicket->getPoser())->mStep = step;
}

bool isInInk(const IUseCollision* pCollision, sead::Vector3f& rPos, f32 radius) {
    CollisionPartsFilterOnlySpecialPurpose filter("InkLimit");
    return alCollisionUtil::checkStrikeSphere(pCollision, rPos, radius, &filter, nullptr) != 0;
}

bool isInInk(const IUseCollision* pCollision, sead::Vector3f& rPos, sead::Vector3f& rDir,
             f32* pOutDistance) {
    CollisionPartsFilterOnlySpecialPurpose filter("InkLimit");
    s32 hitNum = alCollisionUtil::checkStrikeArrow(pCollision, rPos, rDir, &filter, nullptr);

    if (hitNum != 0) {
        const HitInfo* hitInfo = alCollisionUtil::getStrikeArrowInfo(pCollision, 0);

        if (pOutDistance) {
            *pOutDistance = hitInfo->_70;
        }
    }

    return hitNum != 0;
}

}  // namespace al

namespace alCameraFunction {

/**
 * Creates a camera ticket for a poser with the placement id and zone of a placement.
 * @param pPoser Camera poser.
 * @param pUser Camera user.
 * @param rInfo Placement info.
 * @param pSuffix Ticket id suffix.
 * @param priority Camera priority.
 * @return Created camera ticket.
 */
al::CameraTicket* initCamera(al::CameraPoser_RS* pPoser, const al::IUseCamera_RS* pUser,
                             const al::PlacementInfo& rInfo, const char* pSuffix, s32 priority) {
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    al::tryGetZoneMatrixTR(&zoneMtx, rInfo);
    al::PlacementId* placementId;
    al::PlacementId id;

    if (al::tryGetPlacementID(&id, rInfo)) {
        placementId = new al::PlacementId();
        placementId->init(rInfo);
    } else {
        placementId = nullptr;
    }

    return pUser->getCameraDirector_RS()->createCamera(pPoser, placementId, pSuffix, priority,
                                                       zoneMtx, true);
}

/**
 * Creates the camera of a camera area.
 * @param pUser Camera user.
 * @param rInfo Placement info of the area.
 * @param isDisaster Whether the ticket is flagged as disaster camera.
 * @param pSuffix Ticket id suffix.
 * @return Created camera ticket.
 */
al::CameraTicket* initAreaCamera(const al::IUseCamera_RS* pUser, const al::PlacementInfo& rInfo,
                                 bool isDisaster, const char* pSuffix) {
    al::CameraPoser_RS* poser = nullptr;

    if (al::isExistLinkChild(rInfo, "TowerCameraAxis", 0)) {
        poser = new al::CameraPoserTower_RS("塔", nullptr);
    } else if (al::isExistLinkChild(rInfo, "InnerTowerCameraAxis", 0)) {
        poser = new al::CameraPoserInnerTower("塔の内側");
    }

    al::CameraTicket* ticket;

    if (poser != nullptr) {
        sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
        al::tryGetZoneMatrixTR(&zoneMtx, rInfo);
        al::CameraDirector_RS* director = pUser->getCameraDirector_RS();
        al::PlacementId* placementId = new al::PlacementId();
        placementId->init(rInfo);
        ticket = director->createCamera(poser, placementId, pSuffix,
                                        al::CameraTicket::Priority_Area, zoneMtx, true);
    } else {
        sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
        al::tryGetZoneMatrixTR(&zoneMtx, rInfo);
        al::CameraDirector_RS* director = pUser->getCameraDirector_RS();
        al::PlacementId* placementId = new al::PlacementId();
        placementId->init(rInfo);
        ticket = director->createObjectCamera(placementId, pSuffix, nullptr,
                                              al::CameraTicket::Priority_Area, zoneMtx);
    }

    if (isDisaster) {
        ticket->setDisaster();
    }

    ticket->getPoser()->initByPlacementObj(rInfo);
    return ticket;
}

/**
 * Creates the camera of a forced camera area.
 * @param pUser Camera user.
 * @param rInfo Placement info of the area.
 * @param pName Disaster flag source.
 * @return Created camera ticket.
 */
al::CameraTicket* initForceAreaCamera(const al::IUseCamera_RS* pUser,
                                      const al::PlacementInfo& rInfo, const char* pName) {
    al::CameraTicket* ticket = initAreaCamera(pUser, rInfo, pName != nullptr, nullptr);
    ticket->setPriority(al::CameraTicket::Priority_ForceArea);
    return ticket;
}

/**
 * Sets the boss field priority.
 * @param pTicket Camera ticket.
 */
void initPriorityBossField(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_BossField);
}

/**
 * Sets the capture priority.
 * @param pTicket Camera ticket.
 */
void initPriorityCapture(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_Capture);
}

/**
 * Sets the object priority.
 * @param pTicket Camera ticket.
 */
void initPriorityObject(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_Object);
}

/**
 * Sets the safety point recovery priority.
 * @param pTicket Camera ticket.
 */
void initPrioritySafetyPointRecovery(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_SafetyPointRecovery);
}

/**
 * Sets the demo talk priority.
 * @param pTicket Camera ticket.
 */
void initPriorityDemoTalk(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_DemoTalk);
}

/**
 * Sets the demo priority.
 * @param pTicket Camera ticket.
 */
void initPriorityDemo(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_Demo);
}

/**
 * Sets the second demo priority.
 * @param pTicket Camera ticket.
 */
void initPriorityDemo2(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_Demo2);
}

/**
 * Sets the pipe priority.
 * @param pTicket Camera ticket.
 */
void initPriorityDokan(al::CameraTicket* pTicket) {
    pTicket->setPriority(al::CameraTicket::Priority_Dokan);
}

/**
 * Returns whether the current camera of a view has the player priority.
 * @param pUser Camera user.
 * @param viewIndex View index.
 * @return Whether the current camera has the player priority.
 */
bool isCurrentCameraPriorityPlayer(const al::IUseCamera_RS* pUser, s32 viewIndex) {
    return al::getCameraDirector(pUser)->getPoseUpdater(viewIndex)->isCurrentCameraPriority(
        al::CameraTicket::Priority_Player);
}

/**
 * Sets the near clip distance of a camera poser.
 * @param pTicket Camera ticket.
 * @param distance Near clip distance.
 */
void setPoserNearClipDistance(al::CameraTicket* pTicket, f32 distance) {
    pTicket->getPoser()->setNearClipDistance(distance);
}

/**
 * Returns the near clip distance of a view.
 * @param pUser Camera user.
 * @param viewIndex View index.
 * @return Near clip distance.
 */
f32 getNearClipDistance(const al::IUseCamera_RS* pUser, s32 viewIndex) {
    return al::getCameraDirector(pUser)->getPoseUpdater(viewIndex)->getNearClipDistance();
}

/**
 * Enables the 2D camera areas.
 * @param pUser Camera user.
 */
void validateCameraArea2D(al::IUseCamera_RS* pUser) {
    al::getCameraDirector(pUser)->validateCameraArea2D();
}

/**
 * Disables the 2D camera areas.
 * @param pUser Camera user.
 */
void invalidateCameraArea2D(al::IUseCamera_RS* pUser) {
    al::getCameraDirector(pUser)->invalidateCameraArea2D();
}

/**
 * Enables the kids camera areas.
 * @param pUser Camera user.
 */
void validateCameraAreaKids(al::IUseCamera_RS* pUser) {
    al::getCameraDirector(pUser)->getFlagCtrl()->isValidCameraAreaKids = true;
}

/**
 * Returns whether the kids camera areas are enabled.
 * @param pFlagCtrl Camera flags.
 * @return Whether the kids camera areas are enabled.
 */
bool isValidCameraAreaKids(const al::CameraFlagCtrl* pFlagCtrl) {
    return pFlagCtrl->isValidCameraAreaKids || pFlagCtrl->isSeparatePlayMode;
}

/**
 * Turns on the separate play mode.
 * @param pUser Camera user.
 */
void onSeparatePlayMode(al::IUseCamera_RS* pUser) {
    al::getCameraDirector(pUser)->getFlagCtrl()->isSeparatePlayMode = true;
}

/**
 * Turns off the separate play mode.
 * @param pUser Camera user.
 */
void offSeparatePlayMode(al::IUseCamera_RS* pUser) {
    al::getCameraDirector(pUser)->getFlagCtrl()->isSeparatePlayMode = false;
}

/**
 * Makes the next camera reset the pose of this camera.
 * @param pTicket Camera ticket.
 */
void validateResetPoseNextCamera(al::CameraTicket* pTicket) {
    pTicket->getPoser()->getPoserFlag()->_3 = true;
}

/**
 * Makes the next camera keep the pose of this camera.
 * @param pTicket Camera ticket.
 */
void validateKeepPreSelfPoseNextCamera(al::CameraTicket* pTicket) {
    pTicket->getPoser()->getPoserFlag()->isOverWriteProgram = true;
}

/**
 * Makes the interpolation to this camera ease out.
 * @param pTicket Camera ticket.
 */
void validateCameraInterpoleEaseOut(al::CameraTicket* pTicket) {
    pTicket->getPoser()->setInterpoleEaseOut();
}

/**
 * Forces collision checks at the start of the interpolation to this camera.
 * @param pTicket Camera ticket.
 */
void onForceCollideAtStartInterpole(al::CameraTicket* pTicket) {
    pTicket->getPoser()->getPoserFlag()->_c = true;
}

/**
 * Initializes the cloud sea camera settings.
 * @param pUser Camera user.
 * @param height Cloud sea height.
 */
void initCameraSettingCloudSea(al::IUseCamera_RS* pUser, f32 height) {
    al::getCameraDirector(pUser)->initSettingCloudSea(height);
}

/**
 * Creates the camera of a mirror camera area.
 * @param pUser Camera user.
 * @param rInfo Placement info of the area.
 * @param pSuffix Ticket id suffix.
 * @return Created camera ticket.
 */
al::CameraTicket* initMirrorAreaCamera(const al::IUseCamera_RS* pUser,
                                       const al::PlacementInfo& rInfo, const char* pSuffix) {
    sead::Matrix34f zoneMtx = sead::Matrix34f::ident;
    al::tryGetZoneMatrixTR(&zoneMtx, rInfo);
    al::CameraDirector_RS* director = pUser->getCameraDirector_RS();
    al::PlacementId* placementId = new al::PlacementId();
    placementId->init(rInfo);
    return director->createMirrorObjectCamera(placementId, pSuffix,
                                              al::CameraTicket::Priority_Area, zoneMtx);
}

}  // namespace alCameraFunction
