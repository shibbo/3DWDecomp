#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
class PadRumbleKeeper;
class Resource;

class ActionPadAndCameraData {
public:
    ActionPadAndCameraData();

private:
    void* _0 = nullptr;
    void* _8 = nullptr;
    void* _10 = nullptr;
    void* _18 = nullptr;
    void* _20 = nullptr;
    void* _28 = nullptr;
    s32 _30 = 0;
};

struct ActionPadAndCameraCtrlInfo {
    ActionPadAndCameraCtrlInfo();

    const char* mActionName = nullptr;
    s32 mStartFrame = 0;
    s32 mEndFrame = -1;
    bool mIsUseDemo = false;
    bool mIsUseDeadPlayer = false;
    bool mIsUsePadRumbleKeeper = false;
    const char* mCameraShakeName = nullptr;
    const char* mCameraShakeNameMiddle = nullptr;
    const char* mCameraShakeNameFar = nullptr;
    const char* mPadRumbleName = nullptr;
    const char* mPadRumbleNameMiddle = nullptr;
    const char* mPadRumbleNameFar = nullptr;
    s32 mCameraViewTarget = 0;
    s32 _4c;
    f32 mDistanceNear = -1.0f;
    f32 mDistanceFar = -1.0f;
    f32 mDistanceInvalid = -1.0f;
    bool mIsActive = false;
    bool mIsPlaying = false;
    f32 mLastUpdateFrame = -1.0f;
};

static_assert(sizeof(ActionPadAndCameraCtrlInfo) == 0x68);

class ActionPadAndCameraCtrl {
public:
    static ActionPadAndCameraCtrl* tryCreate(const LiveActor* pActor, const sead::Vector3f* pPos,
                                             const char* pSuffix);
    static ActionPadAndCameraCtrl* tryCreate(const LiveActor* pActor, const sead::Vector3f* pPos,
                                             Resource* pResource);

    ActionPadAndCameraCtrl(const LiveActor* pActor, const sead::Vector3f* pPos,
                           const char* pSuffix);
    ActionPadAndCameraCtrl(const LiveActor* pActor, const sead::Vector3f* pPos,
                           Resource* pResource, const char* pFileName);

    void init(const u8* pYaml, bool isUnused);
    void startAction(const char* pActionName);
    void updatePadAndCamera(const ActionPadAndCameraCtrlInfo* pInfo);
    void update(f32 frame, f32 frameRate);

    void setPadRumbleKeeper(const PadRumbleKeeper* pKeeper) { mPadRumbleKeeper = pKeeper; }

    const PadRumbleKeeper* getPadRumbleKeeper() const { return mPadRumbleKeeper; }

private:
    const LiveActor* mParentActor;
    const sead::Vector3f* mPos;
    const char* mActionName = nullptr;
    s32 mInfoCount = 0;
    ActionPadAndCameraCtrlInfo* mInfos = nullptr;
    const PadRumbleKeeper* mPadRumbleKeeper = nullptr;
};

static_assert(sizeof(ActionPadAndCameraCtrl) == 0x30);
}  // namespace al
