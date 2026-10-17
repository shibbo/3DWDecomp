#pragma once

#include "Project/AreaObj/AreaObj.hpp"

/// Area that makes the torpedo spikes Fury Bowser shoots sink or explode when they enter it.
class TorpedoSpikeRestrictionArea : public al::AreaObj {
public:
    TorpedoSpikeRestrictionArea(const char* pName);

    void init(const al::AreaInitInfo& rInfo) override;

    bool isExplode() const;
    bool isSink() const;

private:
    bool mIsExplode = false;  // 0x84
    bool mIsSink = false;     // 0x85
};
