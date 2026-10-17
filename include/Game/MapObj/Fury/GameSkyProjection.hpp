#pragma once

#include "Library/Obj/SkyProjection.hpp"

class GameSkyProjection : public al::SkyProjection {
public:
    explicit GameSkyProjection(const char* pName);
    ~GameSkyProjection() override;
};
