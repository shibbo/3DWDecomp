#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class CoinRing : public al::LiveActor {
public:
    explicit CoinRing(const char* name);
    ~CoinRing() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void reappear() override;
    void startKeyMove();
    void show();
    void hide();
    void exeWait();
    void exeMove();
    void exeDelay();
    void exeStop();
    void exeDisappear();
private:
    al::KeyPoseKeeper* mKeyPose = nullptr;
    int mWaitTime = 0;
    int mMoveTime = 0;
    int mDelayTime = 0;
    sead::Vector3f mShadowSize{0.0f, 0.0f, 0.0f};
    float mShadowScale = 0.001f;
    bool mHasKeyMove = false;
    bool mSingleMode = false;
    int mPlacementIndex = -1;
};
static_assert(sizeof(CoinRing) == 0x178);
