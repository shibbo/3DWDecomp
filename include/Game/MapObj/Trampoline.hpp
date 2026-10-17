#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>

namespace al {
class KeyPoseKeeper;
class MtxConnector;
class PadRumbleKeeper;
}

class Trampoline : public al::LiveActor {
public:
    explicit Trampoline(const char* name);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void tryJumpPlayer();
    void exeReady();
    void exeWait();
    void exeMove();
    void exeStop();

private:
    sead::FixedPtrArray<al::HitSensor, 16> mPlayers;
    float mJumpPower = 30.0f;
    float mHighJumpPower = 40.0f;
    bool mReacting = false;
    al::KeyPoseKeeper* mKeyPose = nullptr;
    bool mFloorTouchStart = false;
    bool mKeepVelocity = false;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mMoveTime = 60;
    al::MtxConnector* mConnector = nullptr;
    int mBoundTimer = 0;
    sead::Vector3f mThroughPos = sead::Vector3f::zero;
    bool mFirstMove = true;
    al::PadRumbleKeeper* mRumble;
    int mLastBoundTimer = 0;
    al::HitSensor* mLastBound = nullptr;
};

static_assert(sizeof(Trampoline) == 0x240);
