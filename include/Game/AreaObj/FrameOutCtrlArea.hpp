#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class FrameOutCtrlArea : public al::AreaObj {
public:
    FrameOutCtrlArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;

    bool mEnableHorizontalCheck = false;
    bool mEnableVerticalCheck = false;
    bool mShowIconAtDisable = false;
};
