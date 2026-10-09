#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
class KeyPoseKeeper;
class LiveActor;
}  // namespace al

class KuriboTower;

/**
 * @brief Keeps track of the key pose steps a Bros enemy moves along (Goomba towers, route pipes,
 * fixed poses).
 */
class BrosMoveStepKeeper {
public:
    BrosMoveStepKeeper(al::LiveActor* pHost, al::KeyPoseKeeper* pKeyPoseKeeper,
                       const al::ActorInitInfo& rInfo);

    void endInit();
    void reset();
    void killKuriboTower(bool isBlowDown);
    void update();
    bool isCurrentStepKuribo() const;
    bool isCurrentStepRouteDokan() const;
    bool isCurrentStepPoseFixed() const;
    bool isNextStepKuribo() const;
    bool isNextStepRouteDokan() const;
    bool isNextStepPoseFixed() const;
    KuriboTower* tryGetStepKuribo(s32 index) const;

private:
    void* _0;
    void* _8;
    void* _10;
};

static_assert(sizeof(BrosMoveStepKeeper) == 0x18);
