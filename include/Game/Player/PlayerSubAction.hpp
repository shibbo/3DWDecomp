#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerSubAction.hpp"

class IUsePlayerAnimator;
class IUsePlayerAttack;
class IUsePlayerAudio;
class IUsePlayerCeilingCheck;
class IUsePlayerCharaQuery;
class IUsePlayerCollision;
class IUsePlayerEffect;
class IUsePlayerEquipment;
class IUsePlayerEventReceiver;
class IUsePlayerFireBallLauncher;
class IUsePlayerFlag;
class IUsePlayerInput;
class PlayerActor;
class PlayerConstParam;
class PlayerFigureDirector;
class PlayerGiantDirector;
class PlayerGigaDirector;

/// Actions that run on top of the main one (throwing, the tail attack, ...).
class PlayerSubAction : public IUsePlayerSubAction {
public:
    PlayerSubAction(const IUsePlayerInput* pInput, IUsePlayerAnimator* pAnimator,
                    IUsePlayerAudio* pAudio, IUsePlayerEffect* pEffect,
                    PlayerFigureDirector* pFigureDirector, const IUsePlayerCharaQuery* pCharaQuery,
                    IUsePlayerFireBallLauncher* pFireBallLauncher,
                    IUsePlayerFireBallLauncher* pBoomerangLauncher, IUsePlayerAttack* pAttack,
                    const IUsePlayerCeilingCheck* pCeilingCheck,
                    const IUsePlayerEquipment* pEquipment, const IUsePlayerFlag* pFlag,
                    const IUsePlayerCollision* pCollision, const PlayerConstParam* pConstParam,
                    IUsePlayerEventReceiver* pEventReceiver,
                    const PlayerGiantDirector* pGiantDirector,
                    const PlayerGigaDirector* pGigaDirector, PlayerActor* pActor);
    void update();

    void validateAll() override;
    void invalidateAll() override;
    void validate(u32) override;
    void invalidate(u32) override;
    bool isValid(u32) const override;
    void setThrowAnimCancel(bool) override;
    void setMainAnimAfterThrow(const char*) override;
    void setMainAnimAfterTailAttack(const char*) override;
    void setIgnoreFloorCondition(bool) override;
    bool isRunning() const override;
    void forceEnd() override;

private:
    u8 _8[0x58 - 0x8];
};
static_assert(sizeof(PlayerSubAction) == 0x58);
