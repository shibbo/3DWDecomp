#pragma once

#include "Player/IUsePlayerFigureChangeObserver.hpp"

/// Remembers that the player's figure changed this frame.
class PlayerFigureChangeObserver : public IUsePlayerFigureChangeObserver {
public:
    /**
     * @brief Marks the figure as changed.
     * @param figure The new figure.
     * @param nextFigure The requested next figure.
     * @param oldFigure The previous figure.
     */
    void notifyFigureChange(u32 figure, u32 nextFigure, u32 oldFigure) override {
        mIsChanged = true;
    }

    bool isChanged() const { return mIsChanged; }

    void clear() { mIsChanged = false; }

private:
    bool mIsChanged = false;  // 0x8
};
