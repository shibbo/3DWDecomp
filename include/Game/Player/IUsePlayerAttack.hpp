#pragma once

/// Starts and ends the player's attacks (e.g. the raccoon dog tail attack).
class IUsePlayerAttack {
public:
    virtual void update() = 0;  // name unknown
    virtual void startTailAttack() = 0;
    virtual void endTailAttack() = 0;
};
