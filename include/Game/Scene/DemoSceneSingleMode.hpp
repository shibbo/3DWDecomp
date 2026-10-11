#pragma once
#include "Scene/DemoScene.hpp"
/** @brief Demo scene for Bowser's Fury; only the constructor used by sequence states is declared. */
class DemoSceneSingleMode : public DemoScene {
public:
    DemoSceneSingleMode(bool isOpening, bool isNoBgm);
};
static_assert(sizeof(DemoSceneSingleMode) == 0x170);
