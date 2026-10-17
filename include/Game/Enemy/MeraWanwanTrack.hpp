#pragma once

#include <math/seadVector.h>
#include <math/seadQuat.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CollisionParts;
class MtxConnector;
}  // namespace al

/** @brief Burning track left on the ground behind a rolling MeraWanwan. */
class MeraWanwanTrack : public al::LiveActor {
public:
    explicit MeraWanwanTrack(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeAppear();
    void exeWait();
    void exeDisappear();

    void start(const sead::Vector3f& rTrans, const sead::Vector3f& rNormal,
               const al::CollisionParts* pParts);

private:
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;
};
static_assert(sizeof(MeraWanwanTrack) == 0x170);
