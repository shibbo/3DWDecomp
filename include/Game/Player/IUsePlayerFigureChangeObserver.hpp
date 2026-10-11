#pragma once

#include <basis/seadTypes.h>

/// Gets told when the player's figure (power-up) changes.
class IUsePlayerFigureChangeObserver {
public:
    virtual void notifyFigureChange(u32 figure, u32 nextFigure, u32 oldFigure) = 0;
};
