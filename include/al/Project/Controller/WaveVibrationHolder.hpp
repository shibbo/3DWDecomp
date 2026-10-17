#pragma once

#include <aal/components/aalAudioFrameProcessMgr.h>
#include <container/seadPtrArray.h>
#include "Project/Controller/WaveVibrationPlayer.hpp"

namespace al {
class GamePadSystem;

/**
 * @brief Holds the wave vibration players, updated from the audio frame.
 */
class WaveVibrationHolder : public aal::IAudioFrameProcess {
public:
    WaveVibrationHolder(const GamePadSystem* pGamePadSystem);

    void audioFrameProcess_() override;

    void loadData();
    void update();
    const WaveVibrationData* findWaveVibrationData(const char* pName) const;
    WaveVibrationPlayer* findPlayableVibrationPlayer(s32 port) const;
    s32 findId(s32 port) const;
    bool getPlayingPriority(s32 port, s32* pPriority) const;
    void stopAll();
    void stopAllOneShot();
    s32 getUsePadNum() const;
    void pause();
    void endPause();
    void startVibrationDirectValue(s32 port, f32 lowFrequency, f32 highFrequency,
                                   f32 lowAmplitude, f32 highAmplitude,
                                   f32 leftVolume, f32 rightVolume);
    void stopVibrationDirectValue(s32 port);

private:
    struct OutputInfo {
        const nn::hid::VibrationDeviceHandle* pLeftHandle = nullptr;
        const nn::hid::VibrationDeviceHandle* pRightHandle = nullptr;
        nn::hid::VibrationTarget* pLeftTarget = nullptr;
        nn::hid::VibrationTarget* pRightTarget = nullptr;
        nn::hid::VibrationMixer* pLeftMixer = nullptr;
        nn::hid::VibrationMixer* pRightMixer = nullptr;
        nn::hid::VibrationMixer* pMonoMixer = nullptr;
        nn::hid::VibrationNodeConnection* pLeftOutput = nullptr;
        nn::hid::VibrationNodeConnection* pRightOutput = nullptr;
        nn::hid::VibrationNodeConnection* pLeftToMono = nullptr;
        nn::hid::VibrationNodeConnection* pRightToMono = nullptr;
        nn::hid::VibrationNodeConnection* pMonoOutput = nullptr;
        s32 deviceNum = 0;
    };
    struct DirectPlayer {
        nn::hid::VibrationWriter* pWriter;
        nn::hid::VibrationNodeConnection* pLeft;
        nn::hid::VibrationNodeConnection* pRight;
    };
    const GamePadSystem* mGamePadSystem;
    WaveVibrationPlayer*** mPlayers = nullptr;
    sead::FixedPtrArray<OutputInfo, 8> mOutputs;
    sead::PtrArray<WaveVibrationData> mData;
    sead::FixedPtrArray<DirectPlayer, 8> mDirectPlayers;
    s32 mPortCount = 5;
    s32 mMaxPort = 4;
    s32* mStyles = nullptr;
};

static_assert(sizeof(WaveVibrationHolder) == 0xe8);
}  // namespace al
