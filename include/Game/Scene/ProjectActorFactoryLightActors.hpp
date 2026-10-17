#pragma once

#include "Library/Light/LppActor.hpp"
#include "Library/Light/PrePassLineLight.hpp"

namespace al {

class PrePassPointLight : public PrePassLightPlacementBase<LppPoint> {
public:
    explicit PrePassPointLight(const char* pName) : PrePassLightPlacementBase(pName) {}
};

class PrePassSpotLight : public PrePassLightPlacementBase<LppSpot> {
public:
    explicit PrePassSpotLight(const char* pName) : PrePassLightPlacementBase(pName) {}
};

class PrePassProjLight : public PrePassLightPlacementBase<LppProj> {
public:
    explicit PrePassProjLight(const char* pName) : PrePassLightPlacementBase(pName) {}
};

class PrePassProjOrthoLight : public PrePassLightPlacementBase<LppProjOrtho> {
public:
    explicit PrePassProjOrthoLight(const char* pName) : PrePassLightPlacementBase(pName) {}
    void init(const ActorInitInfo& rInfo) override {
        PrePassLightPlacementBase::init(rInfo);
        if (mLight->mParam.mIsUseParentYRotation) {
            LppFunction::recalculateRotation(this, rInfo);
        }
    }
};

}  // namespace al
