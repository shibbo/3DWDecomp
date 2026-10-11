#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerInvincibleCheck.hpp"

class IUsePlayerAnimator;
class IUsePlayerDamageInvalidCheck;
class IUsePlayerEffect;
class IUsePlayerEventReceiver;
class PlayerConstParam;
class PlayerFigureDirector;

/// The player's star invincibility.
class PlayerInvincibleState : public IUsePlayerInvincibleCheck {
public:
    PlayerInvincibleState(IUsePlayerAnimator* pAnimator, IUsePlayerEffect* pEffect,
                          IUsePlayerDamageInvalidCheck* pDamageInvalidCheck,
                          const PlayerFigureDirector* pFigureDirector,
                          const PlayerConstParam* pConstParam,
                          IUsePlayerEventReceiver* pEventReceiver);
    void update();

    bool isInvincible() const override;

    void getStar(bool isPlayBgm);
    void endForce(bool isStopBgm, bool isKeepModel);

private:
    u8 _8[0x48 - 0x8];
};
