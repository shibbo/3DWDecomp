#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerKeyConfig.hpp"

/// Button mapping of the controller bound to one pad port.
class PlayerKeyConfig : public IUsePlayerKeyConfig {
public:
    PlayerKeyConfig(s32 port);

    static bool isPadTriggerDecide(s32 port, bool isIgnoreAssist);
    void update();

    s32 getPort() const override;
    bool isPadTriggerPlayerJump() const override;
    bool isPadHoldPlayerJump() const override;
    bool isPadTriggerPlayerSquat() const override;
    bool isPadHoldPlayerSquat() const override;
    bool isPadTriggerPlayerHipDrop() const override;
    bool isPadHoldPlayerHipDrop() const override;
    bool isPadTriggerPlayerDash() const override;
    bool isPadHoldPlayerDash() const override;
    bool isPadReleasePlayerDash() const override;
    bool isPadHoldPlayerStatue() const override;
    bool isPadTriggerPlayerStatue() const override;
    bool isPadTriggerPlayerRolling() const override;
    bool isPadHoldPlayerRolling() const override;
    bool isPadTriggerPlayerBubble() const override;
    bool isPadHoldPlayerHold() const override;
    bool isPadShakePlayerHold() const override;
    bool isPadTriggerPlayerHold() const override;
    bool isPadTriggerPlayerRelease() const override;
    bool isPadTriggerDecide() const override;
    bool isPadHoldDecide() const override;
    bool isPadTriggerCancel() const override;
    bool isPadHoldCancel() const override;
    bool isPadTriggerGyroCursor() const override;
    bool isPadHoldGyroCursor() const override;
    void calcLeftStick(sead::Vector2f* pStick) const override;

private:
    s32 mPort;
    u8 _c[0x12];
};
static_assert(sizeof(PlayerKeyConfig) == 0x20);
