#pragma once

#include <container/seadSafeArray.h>
#include "Library/MapObj/RotateMapParts.hpp"

class FieryRotateParts : public al::RotateMapParts {
public:
    FieryRotateParts(const char* pName);
    ~FieryRotateParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void changeScenarioID(int scenarioId, bool) override;

private:
    sead::SafeArray<float, 5> mScenarioRotateSpeeds;
};
