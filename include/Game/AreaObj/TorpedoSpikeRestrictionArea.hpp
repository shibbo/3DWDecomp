#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class TorpedoSpikeRestrictionArea : public al::AreaObj {
public:
    TorpedoSpikeRestrictionArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    bool isExplode() const;
    bool isSink() const;

    bool mExplode = false;
    bool mSink = false;
};
