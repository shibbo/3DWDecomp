#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BgmRegionChanger : public al::LiveActor {
public:
    BgmRegionChanger(const char* pName);
    virtual ~BgmRegionChanger();
    virtual void init(const al::ActorInitInfo& rInfo);

    void changeRegion();
    void exeWait();

    const char* mBgmSituationName = nullptr;  // 0x148
};
