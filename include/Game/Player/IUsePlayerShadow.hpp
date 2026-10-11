#pragma once

/// Shows and hides the player's shadow (implemented by PlayerModelHolder).
class IUsePlayerShadow {
public:
    virtual void showShadow() = 0;
    virtual void hideShadow() = 0;
    virtual bool isShadowHidden() const = 0;
};
