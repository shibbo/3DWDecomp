#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class KeyMoveLoopLift : public al::LiveActor {
public:
    explicit KeyMoveLoopLift(const char*);
    ~KeyMoveLoopLift() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void start();
    void startStandBy();
    void exeStandBy();
    void exeAppear();
    void exeStartSign();
    void exeWait();
    void exeMoveSign();
    void exeMove();
    void exeStopSign();
    void exeStop();
    void startAppearAndStandBy();
    void startAppear();
    bool isStandByEnd() const;
    const al::KeyPoseKeeper* getKeyPoseKeeper() const { return mKeyPoseKeeper; }
private:
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mWaitTime = 0;
    int mMoveTime = 0;
};
static_assert(sizeof(KeyMoveLoopLift) == 0x168);
