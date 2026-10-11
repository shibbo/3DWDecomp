#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "NPC/GhostPlayerFunction.hpp"

class CheckpointFlag;
class GameDataHolder;
class GameDataHolderAccessor;

namespace al {
class IUseSceneObjHolder;
class LiveActor;
class NetworkSystem;
class PlacementId;
class PlayerHolder;
struct SceneInitInfo;
}  // namespace al

/** @brief One recorded frame of the player (pose, animation frame and action). */
struct GhostPlayData {
    sead::Vector3f mTrans;
    f32 mSklAnimFrame;
    sead::Vector3<s16> mRotate;
    u16 mActionIndex;
};

static_assert(sizeof(GhostPlayData) == 0x18);

/** @brief A marker (event position) stored in recorded ghost play data. */
struct GhostMarkerData {
    s32 mPosX;
    s32 mPosY;
    s32 mPosZ;
    s32 mFrame;
    s32 mType;
};

static_assert(sizeof(GhostMarkerData) == 0x14);

/** @brief Header written in front of the recorded ghost play data. */
struct GhostPlayDataHeader {
    s32 mVersion;
    s32 mWarpObjNum;
    s32 mActionNameNum;
    s32 mPlayDataNum;
    u32 mWarpObjOffset;
    u32 mActionNameOffset;
    u32 mPlayDataOffset;
    s32 mMarkerNum;
    u32 mMarkerOffset;
};

static_assert(sizeof(GhostPlayDataHeader) == 0x24);

/** @brief Records the player's movement during a stage so it can be replayed as a ghost. */
class GhostPlayerRecorder {
public:
    /** @brief Maximum number of recorded warp objects (checkpoints). */
    static constexpr s32 cWarpObjNumMax = 64;
    /** @brief Maximum number of recorded frames. */
    static constexpr u32 cPlayDataNumMax = 15000;
    /** @brief Maximum number of distinct recorded action names. */
    static constexpr s32 cActionNameNumMax = 256;
    /** @brief Maximum number of recorded markers. */
    static constexpr s32 cMarkerNumMax = 256;
    /** @brief Size of one recorded action name. */
    static constexpr s32 cActionNameLength = 32;

    typedef char ActionName[cActionNameLength];

    static u32 getRecorderBufferSize();

    GhostPlayerRecorder(const al::IUseSceneObjHolder* pHolder, const al::SceneInitInfo& rInfo,
                        const GameDataHolder* pGameDataHolder, al::PlayerHolder* pPlayerHolder);

    virtual bool tryStartRecord();
    virtual s32 endRecord();
    virtual bool saveRecord();
    virtual void recordPosRotData(s32 frame, const sead::Vector3f& rTrans,
                                  const sead::Vector3<s16>& rRotate);
    virtual bool tryCancelRecording();
    virtual s32 getHeaderVersion() const;

    bool isAvailableDataStore() const;
    void clearRecordedData();
    void update();
    void cancelRecord();
    void setClearTime(s32 time);
    bool isUploadableToDataStore() const;
    s32 writeGhostData();
    void setPlacementIdObj(const char* pName, bool isRestartPoint);
    void setFlagShakeAfter(const al::PlacementId* pPlacementId);
    void setPlayerRTByUser(const sead::Vector3f& rRotate, const sead::Vector3f& rTrans);
    void resetPlayerRTByUser();
    void recordMarker(s32 type, const sead::Vector3f& rPos, bool arg);

    /**
     * @brief Check whether a recording is in progress.
     * @return True while recording.
     */
    bool isRecording() const { return mIsRecording; }

    /**
     * @brief Check whether a checkpoint flag was shaken during the recording.
     * @return True after a checkpoint flag was shaken.
     */
    bool isFlagShakeAfter() const { return mIsFlagShakeAfter; }

    /**
     * @brief Placement id of the last shaken checkpoint flag.
     * @return The placement id, or nullptr if none was shaken.
     */
    const al::PlacementId* getFlagPlacementId() const { return mFlagPlacementId; }

    /**
     * @brief Name of the recorded stage.
     * @return The stage name.
     */
    const char* getStageName() const { return mStageName; }

    /** @brief Notify that the player was killed by reaching the goal. */
    void notifyGoalKill() { mIsGoalKill = true; }

private:
    /** @brief Stop the recording and drop the recorded frames, when there is a data buffer. */
    void resetRecording() {
        if (mDataBuffer != nullptr) {
            mIsRecording = false;
            mRecordFrame = 0;
        }
    }

    const al::IUseSceneObjHolder* mSceneObjHolder;
    const GameDataHolder* mGameDataHolder;
    GhostWarpObjData* mWarpObjData = nullptr;
    ActionName* mActionNames = nullptr;
    GhostPlayData* mPlayData = nullptr;
    al::PlayerHolder* mPlayerHolder;
    al::LiveActor* mPlayer = nullptr;
    bool mIsRecording = false;
    bool mIsFlagShakeAfter = false;
    s32 mPlayDataNum = 0;
    s32 mRecordFrame = 0;
    s32 mWarpObjNum = 0;
    u16 mActionNameNum = 0;
    const char* mStageName;
    const al::PlacementId* mFlagPlacementId = nullptr;
    void* mDataBuffer = nullptr;
    void* mSaveData = nullptr;
    s32 mSaveDataSize = 0;
    al::NetworkSystem* mNetworkSystem = nullptr;
    s32 mClearTime = 0;
    bool mIsGoalKill = false;
    bool mIsPlayerRTByUser = false;
    sead::Vector3<s16> mUserRotate = {0, 0, 0};
    sead::Vector3f mUserTrans = {0.0f, 0.0f, 0.0f};
    s32 mMarkerNum = 0;
    GhostMarkerData* mMarkers = nullptr;
};

static_assert(sizeof(GhostPlayerRecorder) == 0xb0);

namespace GhostPlayerFunction {
void getTimeAttackRank(sead::BufferedSafeString* pOut, GameDataHolderAccessor accessor,
                       const char* pStageName, s32 time);
void getTimeAttackRivalRank(sead::BufferedSafeString* pOut, GameDataHolderAccessor accessor,
                            const char* pStageName, s32 time);
}  // namespace GhostPlayerFunction

namespace rc {
bool isCourseAppearGhostPlayer(GameDataHolderAccessor accessor, s32 courseId);
bool isExistGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
void cancelRecordGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
bool tryRequestDownloadGhostData(al::NetworkSystem* pNetworkSystem,
                                 const GameDataHolder* pGameDataHolder, const char* pStageName,
                                 s32 arg, bool arg2);
bool tryCalcEntryGhostId(s32* pOutA, s32* pOutB, s32* pOutC, bool* pOutD,
                         al::NetworkSystem* pNetworkSystem, GameDataHolderAccessor accessor,
                         s32 courseId);
bool isExistTimeAttackRivalGhost(al::NetworkSystem* pNetworkSystem,
                                 GameDataHolderAccessor accessor, s32 courseId);
bool tryCreateGhostMiiIcon(al::NetworkSystem* pNetworkSystem, GameDataHolderAccessor accessor,
                           s32 courseId);
const char* getStageNameGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
void setFlagShakeAfterGhostPlayerRecorder(const CheckpointFlag* flag);
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                          const al::PlacementId* pPlacementId, bool isStart);
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                        const al::PlacementId* pPlacementId);
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser, const char* pId,
                                          bool isStart);
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser, const char* pId);
bool isFlagShakeAfterGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
const al::PlacementId* tryGetPlacementIdFlagGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
void setPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                          const sead::Vector3f& rRotate,
                                          const sead::Vector3f& rTrans);
void resetPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
void notifyPlayerStartWarpToGhostPlayer(const al::IUseSceneObjHolder* pUser);
}  // namespace rc
