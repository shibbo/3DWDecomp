#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerLifeControl.hpp"

class IUsePlayerAudio;
class IUsePlayerDamageInvalidCheck;
class IUsePlayerDoubleMarioCheck;
class IUsePlayerEventReceiver;
class PlayerConstParam;
class PlayerFigureDirector;
class PlayerGigaDirector;

/// Takes the player's power-up away on damage and kills it when it has none left.
class PlayerLifeControl : public IUsePlayerLifeControl {
public:
    PlayerLifeControl(PlayerFigureDirector* pFigureDirector,
                      IUsePlayerDamageInvalidCheck* pDamageInvalidCheck, IUsePlayerAudio* pAudio,
                      const PlayerConstParam* pConstParam, IUsePlayerEventReceiver* pEventReceiver,
                      const IUsePlayerDoubleMarioCheck* pDoubleMarioCheck,
                      const PlayerGigaDirector* pGigaDirector);

    void damage() override;
    void forceVanish() override;
    void startDamageInvalidTimer() override;
    bool isDying() const override;
    bool isVanishDying() const override;
    void revive() override;

private:
    u8 _8[0x48 - 0x8];
};
static_assert(sizeof(PlayerLifeControl) == 0x48);
