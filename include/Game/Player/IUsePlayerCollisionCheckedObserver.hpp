#pragma once

/// Gets told after the player's collision was checked this frame.
class IUsePlayerCollisionCheckedObserver {
public:
    virtual void notifyCollisionChecked() = 0;
};
