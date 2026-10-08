#pragma once

#include <container/seadPtrArray.h>
#include <container/seadSafeArray.h>
#include <math/seadVector.h>

#include "Library/Controller/PadRumbleFunction.hpp"

namespace al {
class CameraDirector;
class CameraDirector_RS;
class LiveActor;
class PlayerHolder;
class WaveVibrationHolder;
class WaveVibrationPlayer;
struct WaveVibrationData;

/**
 * @brief A pad rumble entry of "SystemData/VibrationWaves" that plays a wave vibration file.
 */
struct PadRumbleData {
    const char* name;
    const char* dataName;
    f32 volume;
    s32 priority;
    s32 minLength;
    bool isLoop;
    bool isRecommend;
    bool isShowOR;
};

static_assert(sizeof(PadRumbleData) == 0x20);

/**
 * @brief A pad rumble entry of "SystemData/VibrationWaves" that is played from direct values.
 */
struct PadRumbleDirectData {
    const char* name;
    s32 length;
    f32 lowFreqStart;
    f32 lowFreqEnd;
    f32 highFreqStart;
    f32 highFreqEnd;
    f32 fadePercent;
};

static_assert(sizeof(PadRumbleDirectData) == 0x20);

class PadRumbleDirector {
public:
    typedef sead::SafeArray<WaveVibrationPlayer*, 5> PlayerArray;

    /**
     * @brief A looping rumble that follows a position.
     */
    struct LoopInfo {
        PlayerArray players;
        const sead::Vector3f* pos = nullptr;
        sead::Vector3f prevPos = sead::Vector3f::zero;
        PadRumbleParam param;
        f32 volume = 1.0f;
        s32 port = 0;
        s32 _64;
        const char* name = nullptr;
        const char* dataName = nullptr;
        bool isNo3D = false;
        bool isPaused = false;

        LoopInfo() { clearPlayers(); }

        void clearPlayers() {
            for (s32 i = 0; i < 5; i++) {
                players[i] = nullptr;
            }
        }
    };

    static_assert(sizeof(LoopInfo) == 0x80);

    /**
     * @brief A one time rumble.
     */
    struct OneTimeInfo {
        PlayerArray players;
        sead::Vector3f pos = sead::Vector3f::zero;
        f32 volumeLeft = -1.0f;
        f32 volumeRight = -1.0f;
        s32 port = -1;
        const char* name = nullptr;
        const char* dataName = nullptr;
        bool isNo3D = false;
        s32 timer = -1;

        OneTimeInfo() { clearPlayers(); }

        void clearPlayers() {
            for (s32 i = 0; i < 5; i++) {
                players[i] = nullptr;
            }
        }

        void clear() {
            clearPlayers();
            timer = 0;
        }
    };

    static_assert(sizeof(OneTimeInfo) == 0x58);

    /**
     * @brief A rumble played from the direct values of a PadRumbleDirectData.
     */
    struct DirectPlayInfo {
        PadRumbleDirectData* data = nullptr;
        s32 frame = 0;
        f32 volumeLeft = 0.0f;
        f32 volumeRight = 0.0f;
        s32 port = -1;
        bool isPaused = false;

        void init(PadRumbleDirectData* pData);
    };

    static_assert(sizeof(DirectPlayInfo) == 0x20);

    /**
     * @brief The position the rumbles are heard from.
     */
    struct ListenerInfo {
        sead::Vector3f pos = sead::Vector3f::zero;
        sead::Vector3f prevPos = sead::Vector3f::zero;
        f32 cameraRate = 0.05f;
    };

    PadRumbleDirector(const PlayerHolder* pPlayerHolder, const CameraDirector* pCameraDirector);
    PadRumbleDirector(const PlayerHolder* pPlayerHolder, const CameraDirector_RS* pCameraDirector);

    void sharedInit(bool isUnused);
    void setWaveVibrationHolder(WaveVibrationHolder* pHolder);
    void update();
    void updateInfoListAll();
    void startRumble(const char* pName, const sead::Vector3f& rPos, const PadRumbleParam& rParam,
                     s32 port, bool isIgnorePriority);
    OneTimeInfo* findDeadInfoOneTime();
    void startRumbleNo3D(const char* pName, const PadRumbleParam& rParam, s32 port,
                         bool isIgnorePriority);
    DirectPlayInfo* findDirectPlayInfo(const char* pName);
    void stopPadRumbleDirect(s32 port);
    void stopRumbleDirectValue(s32 port);
    void stopPadRumbleOneTime(const char* pName, s32 port);
    void startRumbleLoop(const char* pName, const sead::Vector3f* pPos,
                         const PadRumbleParam& rParam, s32 port, bool isIgnorePriority);
    LoopInfo* findDeadInfo();
    void startRumbleLoopNo3D(const char* pName, const sead::Vector3f* pPos,
                             const PadRumbleParam& rParam, s32 port, bool isIgnorePriority);
    void stopRumbleLoop(const char* pName, const sead::Vector3f* pPos, s32 port);
    LoopInfo* findInfo(const char* pName, const sead::Vector3f* pPos, s32 port);
    bool checkIsAliveRumbleLoop(const char* pName, const sead::Vector3f* pPos, s32 port);
    void stopAllRumble();
    void clearAllInfoList();
    void pause();
    void endPause();
    void pauseActiveRumbles();
    void clearAllOneTime();
    void resumeActiveRumbles();
    void changeRumbleLoopVolume(const char* pName, const sead::Vector3f* pPos, f32 volumeLeft,
                                f32 volumeRight, s32 port);
    void updateInfoListLoop();
    void changeRumbleLoopPitch(const char* pName, const sead::Vector3f* pPos, f32 pitchLeft,
                               f32 pitchRight, s32 port);
    void startRumbleWithVolume(const char* pName, f32 volumeLeft, f32 volumeRight, s32 port);
    void startRumbleDirectValue(f32 freqLow, f32 freqHigh, f32 ampLow, f32 ampHigh,
                                f32 volumeLeft, f32 volumeRight, s32 port);
    void updateInfoListOneTime();
    void updateDirectPlayInfoList();
    void testStartPadRumbleWithVolumeNoActor(const char* pName, f32 volumeLeft, f32 volumeRight);

    void validate() { mIsValid = true; }

    void invalidate() { mIsValid = false; }

    void setPowerLevel(s32 level) { mPowerLevel = level; }

private:
    WaveVibrationHolder* mWaveVibrationHolder;
    const PlayerHolder* mPlayerHolder;
    const CameraDirector* mCameraDirector;
    const CameraDirector_RS* mCameraDirectorRS;
    LoopInfo* mLoopInfos;
    ListenerInfo* mListener;
    OneTimeInfo* mOneTimeInfos;
    DirectPlayInfo* mDirectPlayInfos;
    bool mIsListenerCamera = false;
    bool mIsValid = true;
    s32 mPauseCount = 0;
    s32 mPowerLevel = 0;
    sead::PtrArray<PadRumbleData> mShowORDatas;
    sead::PtrArray<PadRumbleData> mShowORLoopDatas;
    sead::PtrArray<PadRumbleData> mDatas;
    sead::PtrArray<PadRumbleDirectData> mDirectDatas;
};

static_assert(sizeof(PadRumbleDirector) == 0x90);

class PadRumbleKeeper;

void setPadRumbleKeeper(const LiveActor* pActor, const PadRumbleKeeper* pKeeper);
void trySetPadRumbleKeeper(const LiveActor* pActor, const PadRumbleKeeper* pKeeper);
bool isExistPadRumbleKeeper(const LiveActor* pActor);
void startPadRumbleWeak(const LiveActor* pActor, s32 port);
void startPadRumbleNormal(const LiveActor* pActor, s32 port);
void startPadRumbleStrong(const LiveActor* pActor, s32 port);
void startPadRumbleContinueWeak(const LiveActor* pActor, s32 port);
void startPadRumbleContinueNormal(const LiveActor* pActor, s32 port);
void startPadRumbleContinueStrong(const LiveActor* pActor, s32 port);
void validatePadRumbleAll(const LiveActor* pActor);
void invalidatePadRumbleAll(const LiveActor* pActor);
bool isEnablePadWaveRumble(const PadRumbleDirector* pDirector);
bool isEnablePadWaveRumble(const LiveActor* pActor);
}  // namespace al
