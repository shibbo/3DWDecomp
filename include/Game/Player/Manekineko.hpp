#pragma once

#include <math/seadMatrix.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
    class ActorInitInfo;
    class HitSensor;
}  // namespace al

class IUsePlayerCollision;
class PlayerModelHolder;

/// The lucky-cat statue the player turns into; drops coins while it falls.
class Manekineko : public al::LiveActor {
public:
    Manekineko(const al::ActorInitInfo& rInfo, const char* pModelName,
               const PlayerModelHolder* pModelHolder, const IUsePlayerCollision* pCollision,
               const al::HitSensor* pSensor);

    void appear() override;
    void control() override;

    void init(const al::ActorInitInfo& rInfo) override {}

private:
    const PlayerModelHolder* mModelHolder;      // 0x148
    const IUsePlayerCollision* mCollision;      // 0x150
    const al::HitSensor* mSensor;               // 0x158
    const sead::Matrix34f* mBaseMtx = nullptr;  // 0x160
    f32 mCoinDropY = 0.0f;                      // 0x168
};

static_assert(sizeof(Manekineko) == 0x170);
