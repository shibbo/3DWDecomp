#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class TimerGate;

/**
 * @brief Bowser's Fury HUD countdown shown while a timer gate (Plessie ring course) is running.
 */
class CounterTimerGate : public al::LayoutActor {
public:
    CounterTimerGate(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                     al::LayoutActor* pParent);

    void setTimer(s32 frames, s32 warningFrames);
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
    s32 _124 = 0;
    s32 mFrames = 0;
    s32 mWarningFrames = 0;
    bool mIsPaused = true;
    bool mIsRed = false;
    TimerGate* mTimerGate = nullptr;
};

static_assert(sizeof(CounterTimerGate) == 0x140);
