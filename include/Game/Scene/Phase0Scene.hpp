#pragma once

#include "Scene/SingleModeScene.hpp"

/** The opening phase of Bowser's Fury. */
class Phase0Scene : public SingleModeScene {
public:
    Phase0Scene();
    ~Phase0Scene() override;

    void preInitPlacement(const al::ActorInitInfo& rInfo) override;
    void initAreaObj(const al::ActorInitInfo& rInfo) override;
    bool isChangePhase() const override;
    void kill() override;
    void handleGameOver() override;
    void appear() override;
    bool doZoneIDCheck() const override;

private:
    bool mIsChangePhase = false;  // 0x379
};
static_assert(sizeof(Phase0Scene) == 0x380);
