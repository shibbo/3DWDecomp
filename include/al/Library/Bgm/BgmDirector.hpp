#pragma once

#include <basis/seadTypes.h>

#include "Library/Bgm/IUseActiveBgmLine.hpp"

namespace al {
class AudioSystemInfo;
class BgmDataBase;
class BgmLine;
class BgmLineKeeper;
class BgmRhythmCtrl;
class SeadAudioPlayer;
struct BgmPlayingRequest;

class BgmDirector : public IUseActiveBgmLine {
public:
    BgmDirector();

    void init(AudioSystemInfo* pInfo, const char* pStageName, const char* pScenarioName, f32 volume);
    void update();
    void startBgm(const BgmPlayingRequest& rRequest);
    void prepareBgm(const BgmPlayingRequest& rRequest);
    void startPreparedBgm(const char* pName);
    void pauseBgm(const char* pName);
    void stopBgm(const char* pName, s32 fadeFrames, s32 unk);
    void pauseBgm(const char* pName, s32 fadeFrames);
    void resumeBgm(const char* pName, s32 fadeFrames);
    void pauseActiveBgm(s32 fadeFrames);
    void resumeActiveBgm(s32 fadeFrames);
    void pauseIslandBgm(s32 fadeFrames);
    void resumeIslandBgm(s32 fadeFrames);
    void pauseOceanBgm(s32 fadeFrames);
    void resumeOceanBgm(s32 fadeFrames);
    bool isPauseActiveBgm();
    bool isPauseBgm(const char* pName);
    bool pauseActiveBgmById(u32 id, s32 fadeFrames);
    bool resumeActiveBgmById(u32 id, s32 fadeFrames);
    void stopAllBgm(s32 fadeFrames);
    void tryStopAllBgm(s32 fadeFrames);
    void tryPauseBgmIfNotPlaying(const char* pName, s32 fadeFrames);
    void changeSituation(const char* pName);
    const char* getBgmLineSituationName(const char* pLineName) const;
    void changeLineAutoStopMode(const char* pName, bool isAutoStop);
    void disableLineChange(bool isDisable);
    void changeBgmVolume(f32 volume, s32 fadeFrames);
    void changeIslandMapBgmVolume(s32 unk1, s32 unk2, bool unk3);
    s32 getBgmSamplePos(const char* pName);
    bool isBgmCurrentlyPlaying(const char* pName);
    BgmLine* getActiveBgmLine() const override;
    void setActiveBgmPitch(f32 pitch);

    BgmDataBase* getBgmDataBase() const { return mBgmDataBase; }
    SeadAudioPlayer* getAudioPlayer() const { return mAudioPlayer; }
    BgmLineKeeper* getBgmLineKeeper() const { return mBgmLineKeeper; }
    BgmRhythmCtrl* getBgmRhythmCtrl() const { return mBgmRhythmCtrl; }
    void setIsDisableBgmStart(bool isDisable) { mIsDisableBgmStart = isDisable; }
    void setIsDisableChangeSituation(bool isDisable) { mIsDisableChangeSituation = isDisable; }
    void setIsDisableVolumeChange(bool isDisable) { mIsDisableVolumeChange = isDisable; }

private:
    BgmDataBase* mBgmDataBase = nullptr;
    SeadAudioPlayer* mAudioPlayer = nullptr;
    BgmLineKeeper* mBgmLineKeeper = nullptr;
    BgmRhythmCtrl* mBgmRhythmCtrl = nullptr;
    f32 mVolume = 1.0f;
    s32 _2c = 0;
    s32 _30 = 0;
    u32 mPauseIdFlags = 0;
    bool mIsDisableBgmStart = false;
    bool mIsDisableChangeSituation = false;
    bool mIsDisableVolumeChange = false;
};

static_assert(sizeof(BgmDirector) == 0x40);
}  // namespace al
