#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
class SpotLightPatroller;
class SpotLightPatrollerHolder : public al::LiveActor, public al::ISceneObj {
public:
    SpotLightPatrollerHolder();
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    ~SpotLightPatrollerHolder() override;
    const char* getSceneObjName() const override { return "スポットライト監視者の管理"; }
    void movement() override;
private:
    sead::FixedPtrArray<SpotLightPatroller, 64> mPatrollers;
    sead::PtrArray<SpotLightPatroller> mAlertPatrollers;
};
