#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class KoopaFireBall;
class KoopaFireBallGenerator : public al::LiveActor {
public:
    explicit KoopaFireBallGenerator(const char*);
    ~KoopaFireBallGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void exeDelay();
    void exeFall();
    void exeHide();
private:
    KoopaFireBall* mFireBall = nullptr;
    float mDistance = 0.0f;
    int mDelayAppearFrame = 0;
    int mGenerateFrame = 0;
    bool mIsSingleMode = false;
    bool mIsDisasterCameraOn = false;
};
