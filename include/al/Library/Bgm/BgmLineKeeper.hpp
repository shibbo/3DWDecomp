#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <container/seadSafeArray.h>

#include "Project/Audio/AudioInfoList.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"

namespace al {
class AudioSystemInfo;
class BgmDataBase;
class BgmLine;
class BgmLineInfo;

using BgmLineArray = sead::PtrArray<BgmLine>;

class BgmLineKeeper {
public:
    static constexpr s32 cIslandNum = 12;

    BgmLineKeeper(f32 bpmRate);

    void init(AudioSystemInfo* pInfo, const char* pCombinedLineName, const char* pStageName);
    void update(bool isDisableStart);
    void startBgm(const BgmPlayingRequest& rRequest);
    s32 checkIfIslandBgm(const BgmPlayingRequest& rRequest);
    s32 checkPhaseBgm(const BgmPlayingRequest& rRequest);
    s32 startIslandBgm(const BgmPlayingRequest& rRequest, BgmLine* pLine, s32 islandIndex);
    void prepareBgm(const BgmPlayingRequest& rRequest);
    void startPreparedBgm(const char* pName);
    void stopBgm(const char* pName, s32 fadeFrames, s32 resumeFadeFrames);
    void pauseBgm(const char* pName, s32 fadeFrames);
    void resumeBgm(const char* pName, s32 fadeFrames);
    void pauseActiveBgmLine(s32 fadeFrames);
    void resumeActiveBgmLine(s32 fadeFrames);
    void pauseIslandBgm(s32 fadeFrames);
    BgmLine* getBgmLineByLineName(const char* pName) const;
    void resumeIslandBgm(s32 fadeFrames);
    void pauseOceanBgm(s32 fadeFrames);
    void resumeOceanBgm(s32 fadeFrames);
    bool isPauseActiveBgmLine();
    void stopAllBgmLine(s32 fadeFrames);
    void tryStopAllBgmLine(s32 fadeFrames);
    void tryPauseBgmIfNotPlaying(const char* pName, s32 fadeFrames);
    void changeSituation(const char* pName);
    BgmLine* getActiveBgmLine() const;
    void changeLineAutoStopMode(const char* pName, bool isDisableAutoStop);
    void setActiveBgmPitch(f32 pitch);
    void clearIslandList(u32 islandIndex);
    void changeActiveBgmVolume(f32 volume, s32 fadeFrames);
    s32 getBgmSamplePos(const char* pName);

    void setIslandMapBgmVolume(f32 volume1, f32 volume2, bool isEnable) {
        mIsIslandMapBgmVolume = isEnable;
        mIslandMapBgmVolume[1] = volume1;
        mIslandMapBgmVolume[0] = volume2;
    }

    void setIsDisableLineChange(bool isDisable) { mIsDisableLineChange = isDisable; }

private:
    BgmLine* getCurLine() const { return mLines->unsafeAt(mActiveLineIndex); }

    const BgmLineInfo* findLineInfo(const char* pPlayName) const;
    const BgmLineInfo* findLineInfoByIndex(u32 index) const;
    void clearWaitingRequest();

    BgmLineArray* mLines = nullptr;
    BgmDataBase* mDataBase = nullptr;
    AudioInfoList<BgmLineInfo>* mLineInfoList = nullptr;
    u32 mActiveLineIndex = 0;
    bool _1c;
    void* _20;
    bool _28 = false;
    sead::SafeArray<s32, cIslandNum> mIslandSamplePos;
    BgmPlayingRequest mWaitingRequest = {"Dummy"};
    s32 mWaitingTimer = 0;
    bool mIsStartingWaitingBgm = false;
    s32 mResumeFadeFrames = -1;
    f32 mBpmRate;
    s32 mIslandMapBgmVolume[2] = {-1, -1};
    bool mIsIslandMapBgmVolume = false;
    bool mIsDisableLineChange = false;
};

static_assert(sizeof(BgmLineKeeper) == 0xa0);
}  // namespace al
