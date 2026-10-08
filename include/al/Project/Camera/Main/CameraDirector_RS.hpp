#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadCamera.h>
#include <gfx/seadProjection.h>
#include <math/seadMatrix.h>

#include "Library/Execute/IUseExecutor.hpp"
#include "Project/Camera/Core/IUseCameraDirector.hpp"

namespace al {
class AreaObj;
class AreaObjDirector;
class CameraAngleVerticalRequester;
class CameraPoser_RS;
class CameraRailHolder_RS;
class CameraTargetBase;
class CameraTicket;
class CameraTicketId;
class ICameraInput;
class IUseAudioKeeper;
class PlacementId;
class PlayerHolder;
struct NameToCameraParamTransferFunc;
struct PlacementInfo;
struct CameraFlagCtrl;
class CameraInputHolder;
class CameraInSwitchOnAreaDirector;
class CameraParamTransfer;
class CameraPoserFactory_RS;
struct CameraPoserSceneInfo_RS;
class CameraPoseUpdater;
class CameraResourceHolder;
class CameraStartParamCtrl;
class CameraStopJudge;
class CameraTargetCollideInfoHolder;
class CameraTargetHolder;
class CameraTicketHolder;
class ClippingDirectorBase;
class SceneCameraCtrl;
class SpecialCameraHolder;

class CameraDirector_RS : public IUseExecutor, public IUseCamera {
public:
    CameraDirector_RS(s32 viewNum);

    virtual void execute();
    virtual void draw() const;
    virtual ~CameraDirector_RS();
    virtual SceneCameraInfo* getSceneCameraInfo() const;

    void init(CameraPoserSceneInfo_RS* pSceneInfo, const CameraPoserFactory_RS* pFactory,
              SceneCameraInfo* pSceneCameraInfo);
    CameraPoseUpdater* getPoseUpdater(s32 index) const;
    const sead::LookAtCamera& getLookAtMain() const;
    void endInit(const PlayerHolder* pPlayerHolder, s32 playerNum, bool isValid);
    CameraTicket* createCameraFromFactory(const char* pPoserName, const PlacementId* pPlacementId,
                                          const char* pSuffix, s32 priority,
                                          const sead::Matrix34f& rZoneMtx);
    CameraTicket* createCamera(CameraPoser_RS* pPoser, const PlacementId* pPlacementId,
                               const char* pSuffix, s32 priority, s32 viewIndex,
                               const sead::Matrix34f& rZoneMtx, bool isSave);
    void setupCameraAreaObjDirector(AreaObjDirector* pDirector);
    void storeCamera();
    void restoreCamera();
    void clearStoredCamera();

    /** @brief Whether a camera pose is stored. @return The flag. */
    bool isStoredCamera() const { return mIsStoredCamera; }

    /** @brief Position of the stored camera pose. @return The position. */
    const sead::Vector3f& getStoredCameraPos() const { return mStoredCameraPos; }

    /** @brief Look-at point of the stored camera pose. @return The look-at point. */
    const sead::Vector3f& getStoredLookAtPos() const { return mStoredLookAtPos; }
    bool isTargetOnGround();
    bool isTargetInWater();
    bool isTargetDummy();
    void setClippingDirector(ClippingDirectorBase* pDirector);
    void setFreezeDistance();
    void initCameraPoser(CameraPoser_RS* pPoser) const;
    void registerCameraTicket(CameraTicket* pTicket);
    void AddCameraTarget(CameraTargetBase* pTarget);
    void RemoveCameraTarget(CameraTargetBase* pTarget);
    void setReverseRightAndLeftFlag(bool isReverse);
    void setReverseUpAndDownFlag(bool isReverse);
    bool isDisasterCameraOn() const;
    void update(bool isPaused);
    void executePaused();
    CameraTicket* createCamera(CameraPoser_RS* pPoser, const PlacementId* pPlacementId,
                               const char* pSuffix, s32 priority, const sead::Matrix34f& rZoneMtx,
                               bool isSave);
    bool isObjectCameraExist(const PlacementInfo& rInfo);
    CameraTicket* initCreateObjectCamera(const CameraTicketId* pTicketId,
                                         const PlacementInfo* pInfo);
    CameraTicket* initCreateObjectCameraManual(const CameraTicketId* pTicketId,
                                               const char* pPoserName, const PlacementInfo* pInfo);
    CameraTicket* createObjectCamera(const PlacementId* pPlacementId, const char* pSuffix,
                                     const char* pPoserName, s32 priority,
                                     const sead::Matrix34f& rZoneMtx);
    CameraTicket* createObjectEntranceCamera(const PlacementId* pPlacementId, const char* pSuffix,
                                             const sead::Matrix34f& rZoneMtx);
    CameraTicket* createMirrorObjectCamera(const PlacementId* pPlacementId, const char* pSuffix,
                                           s32 priority, const sead::Matrix34f& rZoneMtx);
    void initAreaCameraSwitcherMultiForPrototype(AreaObjDirector* pDirector);
    const ICameraInput* getCameraInput(s32 index) const;
    void setCameraInput(const ICameraInput* pInput);
    void setViewCameraInput(const ICameraInput* pInput, s32 index);
    s32 getActiveInputNum() const;
    void setActiveInputNum(s32 num);
    void freezeCameraInput(bool isFreeze);
    bool isCameraInputFrozen();
    void initAreaCameraSwitcherSingle();
    void initResourceHolder(const CameraResourceHolder* pHolder);
    void registerCameraRailHolder(CameraRailHolder_RS* pHolder);
    void initSceneFovyDegree(f32 fovy);
    void setCameraParamTransferFuncTable(const NameToCameraParamTransferFunc* pTable, s32 num);
    void initSettingCloudSea(f32 height);
    void initSnapShotCameraAudioKeeper(IUseAudioKeeper* pAudioKeeper);
    void initAndCreatePauseCameraCtrl(f32 fovy);
    f32 getSceneFovyDegree() const;
    void validateCameraArea2D();
    void invalidateCameraArea2D();
    void stopByDeathPlayer();
    void restartByDeathPlayer();
    void startInvalidStopJudgeByDemo();
    void endInvalidStopJudgeByDemo();
    bool isCameraStop();
    void startSnapShotMode(bool isLock);
    void enableSnapShotRoll(bool isEnable);
    void endSnapShotMode();
    const sead::Projection& getProjectionMain() const;
    CameraTicket* getCurrentTicket();
    CameraTicket* findCameraAreaTicket(AreaObj* pArea);
    void setDisasterAreaCheck(bool isCheck);
    void setJustWarped();

    SceneCameraCtrl* getSceneCameraCtrl() const { return mSceneCameraCtrl; }

    /** @param isPlessie Whether the camera currently follows Plessie. */
    void setPlessieCamera(bool isPlessie) { mIsPlessieCamera = isPlessie; }

    CameraPoserSceneInfo_RS* getSceneInfo() const { return mSceneInfo; }

    CameraTicketHolder* getTicketHolder() const { return mTicketHolder; }

    SpecialCameraHolder* getSpecialCameraHolder() const { return mSpecialCameraHolder; }

    CameraTargetHolder* getTargetHolder() const { return mTargetHolder; }

    CameraInputHolder* getInputHolder() const { return mInputHolder; }

    CameraStopJudge* getStopJudge() const { return mStopJudge; }

    CameraFlagCtrl* getFlagCtrl() const { return mCameraFlagCtrl; }

    s32 _10;
    s32 _14;
    SceneCameraInfo* mSceneCameraInfo;
    SceneCameraCtrl* mSceneCameraCtrl;
    CameraPoseUpdater** mPoseUpdaters;
    CameraPoserFactory_RS* mFactory;
    CameraPoserSceneInfo_RS* mSceneInfo;
    CameraTicketHolder* mTicketHolder;
    SpecialCameraHolder* mSpecialCameraHolder;
    CameraTargetCollideInfoHolder* mCollideInfoHolder;
    CameraTargetHolder* mTargetHolder;
    CameraInputHolder* mInputHolder;
    CameraAngleVerticalRequester* mCameraAngleVerticalReq;
    CameraStartParamCtrl* mStartParamCtrl;
    CameraStopJudge* mStopJudge;
    CameraParamTransfer* mParamTransfer;
    CameraResourceHolder* mResourceHolder;
    CameraFlagCtrl* mCameraFlagCtrl;
    u64 _98;
    CameraInSwitchOnAreaDirector* mCameraSwitchOnArea;
    u64 _a8;
    ClippingDirectorBase* mClippingDirectorBase;
    sead::Vector3f mStoredCameraPos;  // 0xb8
    sead::Vector3f mStoredLookAtPos;  // 0xc4
    sead::Matrix34f mViewMtx;  // 0xd0
    u8 _100;
    u8 _101;
    u8 _102;
    u8 _103;
    bool mIsStoredCamera;  // 0x104
    bool mIsPlessieCamera;  // 0x105
};
}  // namespace al
