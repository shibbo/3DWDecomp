#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class DisasterModeArea : public al::AreaObj {
public:
    DisasterModeArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

    s32 mSpawnLocationId = -1;
    bool _88 = false;
};
