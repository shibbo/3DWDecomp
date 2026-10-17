#pragma once

#include <basis/seadTypes.h>

/// Switches the player's model between figures (implemented by PlayerModelHolder).
class IUsePlayerModelChange {
public:
    virtual void change(s32 index) = 0;
};
