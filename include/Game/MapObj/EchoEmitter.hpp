#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class EchoEmitter : public al::LiveActor {
public:
    EchoEmitter(const char* pName);
    ~EchoEmitter() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start(const sead::Vector3f& rPosition, float radius, int life);
    void startKeep(const sead::Vector3f& rPosition, float radius, int life);
    void exeWait();
    void exeKeep();
    void exeStop();
    int getLife() const;

private:
    int mLife = 0;
    float mDistance = 0.0f;
    float mRadius = 10.0f;
    float mIntensity = 1.0f;
    float mKeepRadius = 0.0f;
    float mStartDistance = -300.0f;
    float mEndDistance = 600.0f;
    float mStartRadius = 300.0f;
    float mEndRadius = 300.0f;
    const void* mUser = nullptr;
};
