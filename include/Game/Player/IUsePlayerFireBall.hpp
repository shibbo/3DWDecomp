#pragma once

/// A projectile thrown by the player (fire ball) that its launcher can shoot and end.
class IUsePlayerFireBall {
public:
    virtual void shoot() = 0;
    virtual bool isVanished() const = 0;
    virtual void forceEnd() = 0;
};
