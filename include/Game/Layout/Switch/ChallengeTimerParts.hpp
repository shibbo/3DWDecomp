#pragma once

#include "Layout/TextBoxTextInfo.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/** @brief HUD countdown timer (seconds) shown during challenges. */
class ChallengeTimerParts : public al::LayoutActor {
public:
    ChallengeTimerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                        const char* pPartsName, al::LayoutActor* pParent);

    void control() override;
    void setTimer(s32 frames);
    void DisplayCounter(s32 count);
    void startTimer();
    void pauseTimer();
    void unpauseTimer();
    void setTimerRed();
    void appear() override;
    void hide(bool isForce);
    void show();
    bool isCountingDown() const;
    bool isVisible() const;
    void displayTime();
    void startDemo();
    void endDemo();

    void exeAppear();
    void exeCountDown();
    void exeEnd();
    void exeHide();
    void exeDisplay();

private:
    void updateTimerPanes(s32 frames);

    s32 _124 = 0;
    s32 mTimeFrames = 0;
    bool mIsPaused = true;
    bool mIsRed = false;
    bool mIsDemo = false;
    fix::TextBoxTextInfo mTimerTextInfo;
    fix::TextBoxTextInfo mTimerShadowTextInfo;
};

static_assert(sizeof(ChallengeTimerParts) == 0x170);
