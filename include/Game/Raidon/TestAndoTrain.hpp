#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class RailKeeper;
class RailRider;
}  // namespace al

class IUsePlayerPuppet;

/// Test train: a locomotive with three carriages that follows a rail and carries up to four
/// players, who can drive it with the stick and make the carriages hop in a wave.
class TestAndoTrain : public al::LiveActor {
public:
    /// What the controlling player is currently doing with the stick.
    enum class DriveState : s32 {
        None = 0,
        Accel = 1,
        Brake = 2,
    };

    explicit TestAndoTrain(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void syncNearestRailPos();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void exeWait();
    void exeMove();
    s32 findControlPlayerIndex() const;
    void updateRailRider(s32 playerIndex);
    void updateCarriage(s32 playerIndex);
    void updatePuppet();
    void updateEffect();
    al::RailRider* getRailRider();

private:
    static constexpr s32 cCarNum = 4;
    static constexpr s32 cRailNumMax = 64;

    sead::Matrix34f mCarMtx[cCarNum];                  // 0x144
    al::LiveActor* mCars[cCarNum];                     // 0x208
    IUsePlayerPuppet* mPuppets[cCarNum];               // 0x228
    s32 mRailNum;                                      // 0x248
    al::RailKeeper* mRailKeepers[cRailNumMax];         // 0x250
    sead::Vector3f mEffectTrans;                       // 0x450
    sead::Matrix34f mPuppetMtx[cCarNum];               // 0x45c
    s32 mRailIndex;                                    // 0x51c
    f32 mSpeed;                                        // 0x520
    s32 mJumpFrame[cCarNum];                           // 0x524
    f32 mJumpHeight[cCarNum];                          // 0x534
    f32 mJumpSpeed[cCarNum];                           // 0x544
    DriveState mDriveState;                            // 0x554
};

static_assert(sizeof(TestAndoTrain) == 0x558);
