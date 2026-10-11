#pragma once

#include "Scene/SingleModeScene.hpp"

/**
 * @brief Scene of a Bowser's Fury exploration phase.
 */
class PhaseScene : public SingleModeScene {
  public:
    PhaseScene();

  private:
    u8 mUnknown379[7]; // Unreconstructed phase state (after SingleModeScene's tail byte).
};
static_assert(sizeof(PhaseScene) == 0x380);
