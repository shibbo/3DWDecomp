#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerGlideInhibitor.hpp"

class PlayerTrigger;

/// Blocks gliding for a while (e.g. after a punch).
class PlayerGlideInhibitor : public IUsePlayerGlideInhibitor {
public:
    PlayerGlideInhibitor();
    void update(const PlayerTrigger* pTrigger);

    void requestInhibit(s32 frame) override;
    bool isInhibit() const override;

private:
    s32 mInhibitFrame;  // 0x8
};
static_assert(sizeof(PlayerGlideInhibitor) == 0x10);
