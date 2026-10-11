#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ChikaChikaBlockSynchronizer;
class ChikaChikaBlock : public al::LiveActor {
public:
    ChikaChikaBlock(const char*);
    ~ChikaChikaBlock() override;
    void init(const al::ActorInitInfo&) override;
    void exeSwitchOn();
    void exeSwitchOffSign();
    void exeSwitchOff();
    void exeSyncBgm();
    int getSwitchTime() const;
    void onBgmSync();
    void startSwitchOffSign();
    void startSwitch();
    void startSwitchOn();
    void setBgmMute(bool mute) { mIsBgmMute = mute; }
    void disableSpeedMode() { mIsSpeedModeDisabled = true; }
private:
    ChikaChikaBlockSynchronizer* mSynchronizer;
    bool mIsSpeedModeDisabled = false;
    int mPhase = 0;
    bool mIsBgmMute = false;
    bool mIsBgmSync = false;
    int mLastFrame = 0;
};
