#pragma once

#include <container/seadObjArray.h>
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class LiveActor;
class ScreenPointDirector;
class ScreenPointTarget;

struct ScreenPointTargetHitInfo {
    ScreenPointTarget* mTarget = nullptr;
    f32 mDistance = 0.0f;
    sead::Vector3f mHitPos = sead::Vector3f::zero;
    sead::Vector3f mHitNormal = sead::Vector3f::zero;
};

using ScreenPointTargetHitInfoArray = sead::ObjArray<ScreenPointTargetHitInfo>;

class ScreenPointer {
public:
    ScreenPointer(const ActorInitInfo& rInfo, const LiveActor* pHost, const sead::Vector3f* pPos);

    bool hitCheckSegment(const sead::Vector3f& rStart, const sead::Vector3f& rEnd);
    bool hitCheckScreenCircle(const sead::Vector2f& rPos, f32 radius);
    ScreenPointTarget* getHitTargetAndSetPosAndNormal(s32 index);

    s32 getHitTargetNum() const { return mHitInfos.size(); }

    const LiveActor* getHost() const { return mHost; }
    const sead::Vector3f& getHitPos() const { return mHitPos; }
    const sead::Vector3f& getHitNormal() const { return mHitNormal; }

private:
    const LiveActor* mHost;
    sead::Vector3f mHitPos = sead::Vector3f::zero;
    sead::Vector3f mHitNormal = sead::Vector3f::zero;
    const sead::Vector3f* mPos;
    ScreenPointDirector* mDirector = nullptr;
    ScreenPointTargetHitInfoArray mHitInfos;
};
}  // namespace al
