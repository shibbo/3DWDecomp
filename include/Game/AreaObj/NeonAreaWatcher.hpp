#pragma once

#include "Project/AreaObj/IUseAreaObj.hpp"

namespace al {
class PlayerHolder;
}

class NeonAreaWatcher : public al::IUseAreaObj {
public:
    NeonAreaWatcher(al::AreaObjDirector* pDirector, const al::PlayerHolder* pPlayerHolder);
    bool isInPlayer();

    /**
     * @brief Returns the area's director.
     * @return Director supplied at construction.
     */
    al::AreaObjDirector* getAreaObjDirector() const override { return mAreaObjDirector; }

    const al::PlayerHolder* mPlayerHolder;
    al::AreaObjDirector* mAreaObjDirector;
};
