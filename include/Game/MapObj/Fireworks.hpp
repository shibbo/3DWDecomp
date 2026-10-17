#pragma once
#include "Library/LiveActor/LiveActor.hpp"

namespace al { class PrePassProjLight; }

class Fireworks : public al::LiveActor {
public:
    Fireworks(const char* pName);
    ~Fireworks() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
    void exeFire();
    void setLightPower(float power);

private:
    al::PrePassProjLight** mLights = nullptr;
    int mLightCount = 0;
    int mWaitTime = 60;
};
