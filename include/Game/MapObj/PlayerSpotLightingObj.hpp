#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <gfx/seadColor.h>


class PlayerSpotLightingObj : public al::LiveActor {
public:
    PlayerSpotLightingObj(const char* pName);
    ~PlayerSpotLightingObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void movementPaused(bool isPaused) override;

private:
    sead::Vector3f mOffset = {0.0f, 1600.0f, 0.0f};
    sead::Vector3f mRotateOffset = {0.0f, 0.0f, 0.0f};
    sead::Color4f mColor;
    float mSpotLightDegree = 10.0f;
    float mSpotLightLength = 3000.0f;
    float mLightDistDamp = 0.0f;
    float mSpotLightAngleDamp = 0.5f;
};
