#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class TimerGate;

/**
 * @brief Bowser's Fury HUD countdown shown while a timer gate (Plessie ring course) is running.
 * @note Only what reconstructed code needs is declared so far.
 */
class CounterTimerGate : public al::LayoutActor {
public:
    CounterTimerGate(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                     al::LayoutActor* pParent);

    void setTimer(s32 frames, s32 maxFrames);
    void startTimer();
    void pauseTimer();
    void unpauseTimer();
    void setTimerRed();
    void appear() override;
    void hide(bool isForce);
    void show();
    void resetTimerGate();
    bool isCountingDown() const;
    bool isVisible() const;
    void setTimerGate(TimerGate* pTimerGate);

    void exeAppear();
    void exeCountDown();
    void exeEnd();
    void exeHide();

private:
    u8 _121[0x140 - 0x121];
};

static_assert(sizeof(CounterTimerGate) == 0x140);
