#pragma once

/// Gets told when the player starts and ends a mid-air turn (implemented by PlayerAirTurnChecker).
class IUsePlayerAirTurnObserver {
public:
    virtual void startAirTurn() = 0;
    virtual void endAirTurn() = 0;
};
