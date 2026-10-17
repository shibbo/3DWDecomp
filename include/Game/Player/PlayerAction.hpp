#pragma once

#include <prim/seadRuntimeTypeInfo.h>

/// Base class of every action the player's action graph can run.
class PlayerAction {
    SEAD_RTTI_BASE(PlayerAction)

public:
    virtual ~PlayerAction() {}

    virtual void move() = 0;
    virtual void update() = 0;
    virtual void setup();
    virtual void teardown();
};
