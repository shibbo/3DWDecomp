#pragma once

#include "Library/MapObj/KeyMoveMapParts.hpp"
#include "Util/AttachObjectList.hpp"

class IslandKeyMoveMapParts : public al::KeyMoveMapParts {
public:
    IslandKeyMoveMapParts(const char* pName);
    ~IslandKeyMoveMapParts() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void startClipped() override;
    void endClipped() override;
    void changeScenarioID(int id, bool immediate) override;
    void exeWait() override;
    void exeMove() override;

private:
    rc::AttachObjectList mAttachedObjects;
    int mDesiredScenario = 0;
    int mCurrentScenario = 0;
    sead::Vector3f mInitialPosition;
};
