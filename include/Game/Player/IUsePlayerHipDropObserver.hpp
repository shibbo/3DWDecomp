#pragma once

/// Gets told about the player's hip drops.
class IUsePlayerHipDropObserver {
public:
    /// Called on the frame the hip drop hits the floor.
    virtual void notifyOnFloorTrig() = 0;
};
