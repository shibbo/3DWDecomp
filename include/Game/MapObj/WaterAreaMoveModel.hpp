#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class WaterAreaMoveModel : public al::LiveActor {
public:
    explicit WaterAreaMoveModel(const char*);
    ~WaterAreaMoveModel() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void start();
    void updateUbo(al::LiveActor*, bool);
    void setMoveTime();
    void exeMove();
    void exeDelay();
    void exeWait();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPoses = nullptr;
    sead::Vector3f mClippingCenter{0.0f, 0.0f, 0.0f};
    int mMoveTime = 0;
    int mWaitTime = 0;
    int mDelayTime = 0;
    bool mPlayMoveSound = false;
    sead::Vector2f mTexOffsetA{0.0f, 0.0f};
    sead::Vector2f mTexOffsetB{0.0f, 0.0f};
    al::LiveActor* mBackModel = nullptr;
};
static_assert(sizeof(WaterAreaMoveModel) == 0x188);
