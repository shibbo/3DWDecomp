#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class AreaObjGroup; }

class CameraInSwitchOnAreaWatcher : public al::LiveActor {
public:
    explicit CameraInSwitchOnAreaWatcher(const char* pName);
    ~CameraInSwitchOnAreaWatcher() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void kill() override;

private:
    al::AreaObjGroup* mAreas = nullptr;
};
