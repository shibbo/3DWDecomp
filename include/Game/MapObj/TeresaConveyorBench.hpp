#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class TeresaConveyorBench : public al::LiveActor {
public:
    TeresaConveyorBench(const char*);
    ~TeresaConveyorBench() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void start();
    void exeStandBy();
    void exeMoveSign();
    void exeWait();
    void exeMove();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mWaitTime = 30;
    int mMoveTime = 0;
};
