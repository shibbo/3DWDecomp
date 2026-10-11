#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>

namespace al { class AreaObj; }

class EffectObjBreathBubble : public al::LiveActor {
public:
    EffectObjBreathBubble(const char* pName);
    ~EffectObjBreathBubble() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void exeRise();
    void exeBurst();

private:
    u8 _144[0xc];
    sead::Vector3f mEffectPosition = sead::Vector3f::zero;
    al::AreaObj* mWaterArea = nullptr;
};
