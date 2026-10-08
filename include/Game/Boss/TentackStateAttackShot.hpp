#pragma once
#include <math/seadVector.h>
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}
class ITentackSwingTentacleHolder;
class TentackHead;
class TentackTentacle;
class TentackTentacleGroup;

/** @brief State where a head breathes fire while its tentacle group retreats. */
class TentackStateAttackShot : public al::ActorStateBase {
public:
    TentackStateAttackShot(TentackHead* pHead, const al::ActorInitInfo& rInfo,
                           const ITentackSwingTentacleHolder* pSwingHolder);
    void receiveDamage();
    void registerTentacleGroup(TentackTentacleGroup* pGroup);
    void registerTentacle(TentackTentacle* pTentacle);
    const sead::Vector3f& getFirstTarget() const;

    /** @brief Forgets all registered targets. */
    void clearTarget() { mTargetNum = 0; }

    /** @brief Checks whether any target is registered. @return True if a target exists. */
    bool isExistTarget() const { return mTargetNum != 0; }

private:
    unsigned char mUnknown20[0x8];
    s32 mTargetNum;  // 0x28 number of registered targets
    unsigned char mUnknown2C[0x14];

public:
    f32 _40;  // 0x40
};
static_assert(sizeof(TentackStateAttackShot) == 0x48);
