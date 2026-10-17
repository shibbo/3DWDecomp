#pragma once

#include "Scene/PhaseScene.hpp"

/** Preview scene using the exploration phase's ocean setup. */
class PrOceanScene : public PhaseScene {
public:
    PrOceanScene();
    ~PrOceanScene() override;
};
static_assert(sizeof(PrOceanScene) == 0x380);
