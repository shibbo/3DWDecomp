#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class IslandArea : public al::AreaObj {
public:
    explicit IslandArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

    /**
     * @brief Check whether disaster mode is forbidden on the island.
     * @return True when disaster mode is forbidden.
     */
    bool isNoDisaster() const { return mNoDisaster; }

private:
    bool mNoDisaster = false;
};
