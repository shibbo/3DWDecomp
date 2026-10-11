#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/// Plessie, the dinosaur the player rides in Bowser's Fury.
class RaidonActor : public al::LiveActor {
public:
    RaidonActor(const char* pName, const char* pSuffix);

    virtual void init(const al::ActorInitInfo& rInfo, const char* pName);
    virtual void startPuppetActionAll(const char* pActionName);
    virtual void setPuppetInputBlendAnimWeight();
    virtual void setInputBlendAnimWeight();
    virtual bool isOnGroundRaidon() const;
    virtual bool isInWater() const;
    virtual bool isOnGroundOrWaterRaidon() const;

    void convergeHeadRotate(f32, f32);
    void convergeNeckRotate(f32, f32);
    void convergeSpine1Rotate(f32, f32);
    void convergeSpine2Rotate(f32, f32);
    void turnHead(const sead::Vector3f& rTarget);
    void backHead();
    void addSpringControlRate(f32 rate);
    void subSpringControlRate(f32 rate);
    void offSpringControl();

private:
    u8 _144[0x170 - 0x144];
};
