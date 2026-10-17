#pragma once

#include "Scene/SingleModeScene.hpp"

/** An exploration phase of Bowser's Fury and its transition to the next phase. */
class PhaseScene : public SingleModeScene {
public:
    PhaseScene();
    explicit PhaseScene(const char* pName);
    ~PhaseScene() override;

    void initIslandMap(const al::LayoutInitInfo& rLayoutInfo,
                       const al::ActorInitInfo& rActorInfo) override;
    void preInitPlacement(const al::ActorInitInfo& rInfo) override;
    void initAreaObj(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void handlePhaseEnd() override;
    bool isChangePhase() const override;
    bool isPhaseEnd() const override;
    bool isGameEnd() const override;
    bool doZoneIDCheck() const override;
    void exePhaseEnd();
    void exeGameEnd();

private:
    s32 mStartingPhase = 0; // 0x37c
};
static_assert(sizeof(PhaseScene) == 0x380);
