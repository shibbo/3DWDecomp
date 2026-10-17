#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class DisasterLightning : public al::LiveActor {
public:
    DisasterLightning(const char* pName);
    ~DisasterLightning() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear(int level);
    void exeStrike();
private:
    const char* mEffectName = "DisasterModeLightning";
};
