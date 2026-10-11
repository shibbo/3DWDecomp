#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

namespace sead {
class LookAtCamera;
class PerspectiveProjection;
class Viewport;
}  // namespace sead

namespace al {
class ActorInitInfo;
class Resource;
class AreaObjDirector;
class CollisionDirector;
class IUseAudioKeeper;
class PlayerHolder;
class SceneCameraInfo;

class CameraDirector {
public:
    CameraDirector(s32 viewNum, AreaObjDirector* pAreaObjDirector,
                   CollisionDirector* pCollisionDirector);

    void init(const PlayerHolder* pPlayerHolder);
    void initCameraCreator(s32 resourceNum, bool isStageOneResource);
    void setCameraResource(const Resource* pResource, s32 index);
    void initAudioKeeper(ActorInitInfo& rInfo);
    void setCameraAspect(const sead::Viewport* pMainViewport, const sead::Viewport* pSubViewport);
    void setStageName(const char* pStageName);
    void update(bool isPaused);
    s32 getCameraMode() const;
    void setCameraMode(s32 mode);
    void setSnapShotAudioKeeper(IUseAudioKeeper* pAudioKeeper);
    void validUserCameraControlByPortOnly(s32 port);
    void validUserCameraControlByPort(s32 port);
    void invalidUserCameraControlByPort(s32 port);
    void setSingleJoyconCameraValid(bool isValid);
    void setGoalPosPtr(const sead::Vector3f* pGoalPos);
    void reviseCameraInfo(SceneCameraInfo* pSceneCameraInfo);

    SceneCameraInfo* getSceneCameraInfo() const { return mSceneCameraInfo; }
    sead::LookAtCamera* getLookAtCamera() const { return mLookAtCamera; }
    sead::PerspectiveProjection* getProjection() const { return mProjection; }
    void setMainPlayerIndex(s32 index) { mMainPlayerIndex = index; }
    void setReverseHorizontal(bool isReverse) { mIsReverseHorizontal = isReverse; }
    void setReverseVertical(bool isReverse) { mIsReverseVertical = isReverse; }
    void setKinopioBrigadeReverseHorizontal(bool isReverse) {
        mIsKinopioBrigadeReverseHorizontal = isReverse;
    }
    void setKinopioBrigadeReverseVertical(bool isReverse) {
        mIsKinopioBrigadeReverseVertical = isReverse;
    }

    u8 _0[0x8];
    sead::LookAtCamera* mLookAtCamera;
    u8 _10[0x8];
    sead::PerspectiveProjection* mProjection;
    u8 _20[0x8];
    SceneCameraInfo* mSceneCameraInfo;
    u8 _30[0xa0 - 0x30];
    sead::Matrix34f mMainViewMtx;  // 0xa0
    sead::Matrix34f mSubViewMtx;   // 0xd0
    u8 _100[0x108 - 0x100];
    bool* _108;  // 0x108, shared with the camera observer
    bool* _110;  // 0x110, shared with the camera observer
    bool* _118;  // 0x118, shared with the camera observer
    bool _120;   // 0x120, shared with the camera observer
    u8 _121[0x123 - 0x121];
    bool _123;  // 0x123, set when the stage starts from a checkpoint
    u8 _124[0x154 - 0x124];
    bool _154;  // 0x154, whether the gyro camera is enabled
    u8 _155[0x158 - 0x155];
    s32 _158;  // 0x158, shared with the camera observer
    bool mIsReverseHorizontal;  // 0x15c
    bool mIsReverseVertical;  // 0x15d
    bool mIsKinopioBrigadeReverseHorizontal;  // 0x15e
    bool mIsKinopioBrigadeReverseVertical;  // 0x15f
    u8 _160[0x161 - 0x160];
    bool mIsTitleFlag;  // 0x161, cleared by the title scene (meaning unknown)
    bool _162;          // 0x162, set while the camera is rotated by a player
    u8 _163[0x1c8 - 0x163];
    s32 mMainPlayerIndex;  // 0x1c8
    u8 _1cc[0x1f0 - 0x1cc];
};
}  // namespace al
