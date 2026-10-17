#pragma once

#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class KeyPoseKeeper;

class KeyMoveMovement : public HostStateBase<LiveActor> {
public:
    KeyMoveMovement(LiveActor* pHost, const ActorInitInfo& rInfo);

    const sead::Vector3f& getMoveTrans() const { return mTrans; }

    void exeWait();
    void exeMove();
    void exeStop();

private:
    KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTrans;
    s32 mTime = 0;
};

static_assert(sizeof(KeyMoveMovement) == 0x48);

}  // namespace al
