#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; class LiveActor; }

class GuideFrameOutSingleMode : public al::LayoutActor {
public:
    GuideFrameOutSingleMode(const al::LayoutInitInfo& rInfo, const al::LiveActor* pActor,
                            const char* pCharacterAction);
    void appear() override;
    void control() override;
    void startDemo();
    void endDemo();
    void startPause();
    void endPause();
    void updateVisibility(bool isPaneOnly);
    void setEnable();
    void setDisable();
    void setEnableBubble(bool enabled);
    void setEnableWarpGuide(bool enabled);
    bool isEnable() const;
    bool isScreenOut() const;
    s32 getScreenOutFrame() const;
    void updateWarpGuide(f32 angle);
    void disappearImmediate();
    void initIcon();
    void exeStartAppear();
    void exeAppear();
    void exeStartDisappear();
    void exeDisappear();
    void setUnk148(f32 value) { mDisappearRate = value; }
    void onUnk142() { mUnknown142 = true; }
    void onUnk143() { mDelayAppear = true; }
    void onUnk145() { mForcePosition = true; }
    void setAerial(bool isAerial) { mCenterHorizontally = isAerial; }
private:
    const al::LiveActor* mpActor;
    s32 mCharacterType = 0;
    s32 mScreenOutFrame = 0;
    s32 mUnknown138;
    bool mEnabled = false;
    bool mBubbleEnabled = true;
    bool mShowing = false;
    bool mInDemo = false;
    bool mPaused = false;
    bool mFixedIcon = true;
    bool mUnknown142 = false;
    bool mDelayAppear = false;
    bool mCenterHorizontally = false;
    bool mForcePosition = false;
    f32 mDisappearRate = 1.0f;
    s32 mWarpGuideState = 0;
};

static_assert(sizeof(GuideFrameOutSingleMode) == 0x150);
