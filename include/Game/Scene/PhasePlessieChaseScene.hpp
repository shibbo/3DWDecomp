#pragma once

#include "Scene/PhaseScene.hpp"

/** Plessie's chase at the end of a Bowser's Fury boss phase. */
class PhasePlessieChaseScene : public PhaseScene {
public:
    PhasePlessieChaseScene();
    ~PhasePlessieChaseScene() override;
    void init(const al::SceneInitInfo& rInfo) override;
    void appear() override;

private:
    s32 mUnknown380 = 0;
};
static_assert(sizeof(PhasePlessieChaseScene) == 0x388);
