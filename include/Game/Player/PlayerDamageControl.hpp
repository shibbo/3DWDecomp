#pragma once

#include <basis/seadTypes.h>

class IUsePlayerDamageInvalidCheck;
class IUsePlayerDamageObserver;
class IUsePlayerEffect;
class IUsePlayerEquipment;
class IUsePlayerLifeControl;
class PlayerActionGraph;
class PlayerTrigger;

/// Applies the damage the player got this frame.
class PlayerDamageControl {
public:
    PlayerDamageControl(const PlayerActionGraph* pActionGraph, const PlayerTrigger* pTrigger,
                        const IUsePlayerEquipment* pEquipment,
                        const IUsePlayerDamageInvalidCheck* pDamageInvalidCheck,
                        IUsePlayerEffect* pEffect, IUsePlayerLifeControl* pLifeControl);
    void update();
    void damage();

    void setDamageObserver(IUsePlayerDamageObserver* pObserver) { mDamageObserver = pObserver; }

private:
    u8 _0[0x30];
    IUsePlayerDamageObserver* mDamageObserver;  // 0x30
};
static_assert(sizeof(PlayerDamageControl) == 0x38);
