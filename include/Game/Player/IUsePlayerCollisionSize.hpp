#pragma once

/// Changes the size of the player's collision (implemented by PlayerCollisionSize).
class IUsePlayerCollisionSize {
public:
    virtual void changeShort() = 0;
    virtual void changeSuper() = 0;
    virtual void squat() = 0;
    virtual void standUp() = 0;
    virtual void changeClimbShort() = 0;
    virtual void changeClimbSuper() = 0;
    virtual bool isShort() const = 0;
};
