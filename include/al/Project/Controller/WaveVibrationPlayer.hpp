#pragma once

#include <nn/hid/hid_VibrationPlayer.h>

namespace al {
struct WaveVibrationData {
    const void* data;
    s32 size;
    const char* name;
    bool operator<(const WaveVibrationData& rOther) const;
};

class WaveVibrationPlayer {
public:
    WaveVibrationPlayer(nn::hid::VibrationMixer* pLeft, nn::hid::VibrationMixer* pRight);
    void update();
    void startOneTime(const WaveVibrationData*, f32, f32, f32, f32, s32, s32, bool);
    void startLoop(const WaveVibrationData*, f32, f32, f32, f32, s32, s32, bool);
    void start(const WaveVibrationData*, f32, f32, f32, f32, bool);
    void stop();
    void pause(bool locked);
    void endPause(bool locked);
    void changeVolume(f32 left, f32 right);
    void changePitch(f32 left, f32 right);
    void changeVolumeAndPitch(f32 leftVolume, f32 rightVolume, f32 leftPitch, f32 rightPitch);
    bool isPlaying() const;
    bool isLoop() const;

    bool isPaused() const { return mPaused; }

    bool isPauseLocked() const { return mPauseLocked; }

private:
    nn::hid::VibrationPlayer* mPlayer = nullptr;
    nn::hid::VibrationNodeConnection* mLeft = nullptr;
    nn::hid::VibrationNodeConnection* mRight = nullptr;
    s32 mFrame = 0;
    const WaveVibrationData* mData = nullptr;
    s32 mPriority = -1;
    s32 mDuration = -1;
    s32 mElapsed = -1;
    bool mPaused = false;
    bool mPauseLocked = false;
    bool mSyncToFrames = false;
};
}  // namespace al
