#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class HitSensor;
class LayoutActor;
class LiveActor;
class Resource;

struct HitReactionInfo {
    HitReactionInfo();

    const char* mReactionName = nullptr;
    const char* mEffectName = nullptr;
    f32 mEffectPosOffsetBetweenSensors = 0.0f;
    const char* mSoundName = nullptr;
    const char* mSeName = nullptr;
    const char* mOceanWaveName = nullptr;
    const char* mPadRumbleType = "全員";
    f32 mPadRumbleDistance = 300.0f;
    const char* mPadRumbleName = nullptr;
    const char* mCameraShakeType = "常に振動";
    f32 mCameraShakeDistance = 1000.0f;
    const char* mCameraShakeName = nullptr;
    s32 mStopSceneFrame = -1;
    bool mIsStopSceneForHitEffect = true;
    bool mIsStopScenePlayers = false;
    s32 mRadialBlurFrame = 0;
    f32 mRadialBlurRadiusBegin = 0.0f;
    f32 mRadialBlurRadiusEnd = 0.0f;
};

static_assert(sizeof(HitReactionInfo) == 0x78);

class PadRumbleKeeper;

class HitReactionKeeper {
public:
    static HitReactionKeeper* tryCreate(LiveActor* pActor, const Resource* pResource,
                                        const char* pName);
    static HitReactionKeeper* tryCreate(LayoutActor* pActor, const Resource* pResource,
                                        const char* pName);

    HitReactionKeeper(LiveActor* pActor, const Resource* pResource, const char* pName);
    HitReactionKeeper(LayoutActor* pActor, const Resource* pResource, const char* pName);

    void start(const char* pName, const sead::Vector3f* pPos, const HitSensor* pSensor1,
               const HitSensor* pSensor2);

    void setPadRumbleKeeper(PadRumbleKeeper* pKeeper) {
        mPadRumblePort = reinterpret_cast<const s32*>(pKeeper);
    }

    const s32* getPadRumblePort() const { return mPadRumblePort; }

private:
    LiveActor* mActor = nullptr;
    LayoutActor* mLayoutActor = nullptr;
    s32 mReactionNum = 0;
    HitReactionInfo* mReactionInfos = nullptr;
    const s32* mPadRumblePort = nullptr;
};

static_assert(sizeof(HitReactionKeeper) == 0x28);
}  // namespace al
