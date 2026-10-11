#include "MapObj/Fury/FieryRotateParts.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

FieryRotateParts::FieryRotateParts(const char* pName) : al::RotateMapParts(pName) {
    mScenarioRotateSpeeds.fill(-1.0f);
}

FieryRotateParts::~FieryRotateParts() {}

void FieryRotateParts::init(const al::ActorInitInfo& rInfo) {
    al::RotateMapParts::init(rInfo);
    al::tryGetArg(&mScenarioRotateSpeeds[0], rInfo, "RotateSpeedScenario1");
    if (mScenarioRotateSpeeds[0] == -1.0f)
        mScenarioRotateSpeeds[0] = mRotateSpeed;
    al::tryGetArg(&mScenarioRotateSpeeds[1], rInfo, "RotateSpeedScenario2");
    if (mScenarioRotateSpeeds[1] == -1.0f)
        mScenarioRotateSpeeds[1] = mRotateSpeed;
    al::tryGetArg(&mScenarioRotateSpeeds[2], rInfo, "RotateSpeedScenario3");
    if (mScenarioRotateSpeeds[2] == -1.0f)
        mScenarioRotateSpeeds[2] = mRotateSpeed;
    al::tryGetArg(&mScenarioRotateSpeeds[3], rInfo, "RotateSpeedScenarioX");
    if (mScenarioRotateSpeeds[3] == -1.0f)
        mScenarioRotateSpeeds[3] = mRotateSpeed;
    al::tryGetArg(&mScenarioRotateSpeeds[4], rInfo, "RotateSpeedScenarioX");
    if (mScenarioRotateSpeeds[4] == -1.0f)
        mScenarioRotateSpeeds[4] = mRotateSpeed;
}

void FieryRotateParts::changeScenarioID(int scenarioId, bool) {
    if (scenarioId < mScenarioRotateSpeeds.size())
        mRotateSpeed = mScenarioRotateSpeeds[scenarioId];
    else
        mRotateSpeed = mScenarioRotateSpeeds.back();
}
