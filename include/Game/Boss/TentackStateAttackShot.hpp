#pragma once
#include <math/seadVector.h>
#include <container/seadPtrArray.h>
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
template <class T> class DeriveActorGroup;
}
class ITentackSwingTentacleHolder;
class TentackHeadAttackBall;
class TentackHead;
class TentackTentacle;
class TentackTentacleGroup;

/** @brief State where a head breathes fire while its tentacle group retreats. */
class TentackStateAttackShot : public al::HostStateBase<TentackHead> {
public:
    TentackStateAttackShot(TentackHead* pHead, const al::ActorInitInfo& rInfo,
                           const ITentackSwingTentacleHolder* pSwingHolder);
    ~TentackStateAttackShot() override;
    void appear() override;
    void kill() override;
    void exeTurn();
    void exeShotSign();
    void exeShot();
    void receiveDamage();
    void registerTentacleGroup(TentackTentacleGroup* pGroup);
    void registerTentacle(TentackTentacle* pTentacle);
    const sead::Vector3f& getFirstTarget() const;

    /** @brief Forgets all registered targets. */
    void clearTarget() { mTargets.clear(); }

    /** @brief Checks whether any target is registered. @return True if a target exists. */
    bool isExistTarget() const { return !mTargets.isEmpty(); }

private:
    const ITentackSwingTentacleHolder* mSwingHolder;
    sead::PtrArray<TentackTentacle> mTargets;
    al::DeriveActorGroup<TentackHeadAttackBall>* mBalls;

public:
    f32 _40;  // 0x40
};
static_assert(sizeof(TentackStateAttackShot) == 0x48);
