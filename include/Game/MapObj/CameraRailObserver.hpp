#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CameraRail;
class CameraRailHolder;
}

class CameraRailObserver : public al::LiveActor {
public:
    CameraRailObserver(const char* pName);
    ~CameraRailObserver() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void stop();
    void control() override;

private:
    int mFirstRailIndex = -1;
    int mRailCount = 0;
    al::CameraRail** mRails = nullptr;
    al::CameraRailHolder* mRailHolder = nullptr;
};
