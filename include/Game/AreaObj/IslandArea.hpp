#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class IslandArea : public al::AreaObj {
public:
    explicit IslandArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

private:
    bool mNoDisaster = false;
};
