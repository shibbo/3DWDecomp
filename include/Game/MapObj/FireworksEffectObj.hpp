#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>

namespace al { class PrePassProjLight; }

class FireworksEffectObj : public al::LiveActor {
public:
    FireworksEffectObj(const char* pName);
    ~FireworksEffectObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    const sead::Matrix34f* getBaseMtx() const override { return &mBaseMtx; }
    void exeWait();
    void exeFire();
    void setLightPower(float power);

private:
    bool mIsOneShot = false;
    bool mHasFired = false;
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;
    al::PrePassProjLight** mLights = nullptr;
    int mLightCount = 0;
};
