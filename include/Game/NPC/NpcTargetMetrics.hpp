#pragma once
#include "NPC/NpcTargetFunction.hpp"
#include <math/seadVector.h>
#include <cfloat>
namespace al {
class LiveActor;
}
namespace npc {
struct TargetMetrics {
    bool mUnknown0 = true;
    const al::LiveActor* mBestTarget = nullptr;
    float mBestDistance = FLT_MAX;
    unsigned char mBestPriority = 0;
    bool mBestInSight = false;
    bool mHasCandidate = false;
    NpcFindTargetType mBestType = NpcFindTargetType::None;
    sead::Vector3f delta;
    float distance;
    float horizontalCos;
    float verticalCos;
    bool mUnknown34;
    /** @brief Clears the current best candidate while retaining calculated geometry. */
    void reset() {
        mUnknown0 = true;
        mBestTarget = nullptr;
        mBestDistance = FLT_MAX;
        mHasCandidate = false;
        mBestPriority = 0;
        mBestInSight = false;
        mBestType = NpcFindTargetType::None;
        mUnknown34 = false;
    }
};
bool calcTargetMetrics(TargetMetrics*, const al::LiveActor*, const sead::Vector3f&,
                       const sead::Vector3f&, const al::LiveActor*);
} // namespace npc
