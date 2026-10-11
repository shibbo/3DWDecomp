#pragma once
#include "Library/MapObj/FixMapParts.hpp"
#include "MapObj/Fury/DisasterAnimPart.hpp"
class DisasterFixMapParts : public al::FixMapParts, public DisasterAnimPart {
public:
    explicit DisasterFixMapParts(const char*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void startFarLod() override;
    void endFarLod() override;
    void onDisasterModeStateChange(DisasterModeController::State) override;
};
static_assert(sizeof(DisasterFixMapParts) == 0x2a0);
