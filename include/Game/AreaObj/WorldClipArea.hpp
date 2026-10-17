#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class WorldClipArea : public al::AreaObj {
public:
    WorldClipArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;

    u32 mActiveWorldMask = 0;
};
