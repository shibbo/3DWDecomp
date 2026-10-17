#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <gfx/seadColor.h>

namespace al { class LppPointParam; }

class PlayerPointLightingObj : public al::LiveActor {
public:
    PlayerPointLightingObj(const char* pName);
    ~PlayerPointLightingObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void control() override;
    void movementPaused(bool isPaused) override;

private:
    sead::Vector3f mOffset = {0.0f, 0.0f, 0.0f};
    sead::Color4f mColor = sead::Color4f::cWhite;
    al::LppPointParam* mParam;
    bool mIsEnableSpecular = false;
};
