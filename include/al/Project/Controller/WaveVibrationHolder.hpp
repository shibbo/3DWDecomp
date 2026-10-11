#pragma once

#include <aal/components/aalAudioFrameProcessMgr.h>

namespace al {
class GamePadSystem;
class WaveVibrationPlayer;
struct WaveVibrationData;

/**
 * @brief Holds the wave vibration players, updated from the audio frame (partially reconstructed).
 */
class WaveVibrationHolder : public aal::IAudioFrameProcess {
public:
    WaveVibrationHolder(const GamePadSystem* pGamePadSystem);

    void audioFrameProcess() override;

    void update();
    void pause();
    void endPause();
    void stopAll();
    void stopAllOneShot();
    s32 getUsePadNum() const;
    WaveVibrationPlayer* findPlayableVibrationPlayer(s32 port) const;
    const WaveVibrationData* findWaveVibrationData(const char* pName) const;
    void startVibrationDirectValue(s32 port, f32 freqLow, f32 freqHigh, f32 ampLow, f32 ampHigh,
                                   f32 volumeLeft, f32 volumeRight);
    void stopVibrationDirectValue(s32 port);

private:
    unsigned char _18[0xe8 - 0x18];
};

static_assert(sizeof(WaveVibrationHolder) == 0xe8);
}  // namespace al
