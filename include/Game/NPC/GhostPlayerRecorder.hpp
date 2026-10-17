#pragma once

namespace al {
class IUseSceneObjHolder;
class PlacementId;
}  // namespace al

#include <basis/seadTypes.h>
#include <math/seadVector.h>

class GameDataHolder;
class GameDataHolderAccessor;
class CheckpointFlag;
class PlayerActor;
namespace al {
struct SceneInitInfo;
class PlayerHolder;
class LiveActor;
class NetworkSystem;
}
namespace GhostPlayerFunction { struct WarpObjData; }
struct GhostRecorderActionData { char name[32]; };
struct GhostRecorderPoseData {
    sead::Vector3f trans;
    float animFrame;
    sead::Vector3<s16> rotate;
    u16 actionIndex;
};
struct GhostRecorderMarkerData {
    sead::Vector3<int> trans;
    int poseIndex;
    int markerId;
};
static_assert(sizeof(GhostRecorderPoseData) == 24);
static_assert(sizeof(GhostRecorderMarkerData) == 20);
static_assert(sizeof(GhostRecorderActionData) == 32);

class GhostPlayerRecorder {
public:
    static u32 getRecorderBufferSize();
    GhostPlayerRecorder(const al::IUseSceneObjHolder*, const al::SceneInitInfo&,
                        const GameDataHolder*, al::PlayerHolder*);
    virtual bool tryStartRecord();
    virtual int endRecord();
    virtual bool saveRecord();
    virtual void recordPosRotData(int, const sead::Vector3f&, const sead::Vector3<s16>&);
    virtual bool tryCancelRecording();
    virtual int getHeaderVersion() const;
    bool isAvailableDataStore() const;
    void clearRecordedData();
    void update();
    void cancelRecord();
    void setClearTime(int);
    bool isUploadableToDataStore() const;
    u32 writeGhostData();
    void setPlacementIdObj(const char*, bool);
    void setFlagShakeAfter(const al::PlacementId*);
    void setPlayerRTByUser(const sead::Vector3f&, const sead::Vector3f&);
    void resetPlayerRTByUser();
    void recordMarker(int, const sead::Vector3f&, bool);
    void notifyGoalKill() { mIsGoalKilled = true; }
    const char* getStageName() const { return mStageName; }
    bool isFlagShakeAfter() const { return mIsFlagShakeAfter; }
    const al::PlacementId* getCheckpointId() const { return mCheckpointId; }
    bool isRecording() const { return mIsRecording; }

private:
    const al::IUseSceneObjHolder* mSceneUser;
    const GameDataHolder* mGameData;
    GhostPlayerFunction::WarpObjData* mWarpData = nullptr;
    GhostRecorderActionData* mActionData = nullptr;
    GhostRecorderPoseData* mPoseData = nullptr;
    al::PlayerHolder* mPlayerHolder;
    al::LiveActor* mPlayer = nullptr;
    bool mIsRecording = false;
    bool mIsFlagShakeAfter = false;
    int mPoseCount = 0;
    int mFrame = 0;
    int mWarpCount = 0;
    u16 mActionCount = 0;
    const char* mStageName;
    const al::PlacementId* mCheckpointId = nullptr;
    u8* mBuffer = nullptr;
    const u8* mSavedData = nullptr;
    u32 mSavedSize = 0;
    al::NetworkSystem* mNetworkSystem = nullptr;
    int mClearTime = 0;
    bool mIsGoalKilled = false;
    bool mHasUserPose = false;
    sead::Vector3<s16> mUserRotate = sead::Vector3<s16>(0, 0, 0);
    sead::Vector3f mUserTrans = sead::Vector3f(0, 0, 0);
    int mMarkerCount = 0;
    GhostRecorderMarkerData* mMarkerData = nullptr;
};
static_assert(sizeof(GhostPlayerRecorder) == 0xb0);

namespace rc {
bool isCourseAppearGhostPlayer(GameDataHolderAccessor, int course);
void endRecordGhostPlayerRecorder(const al::IUseSceneObjHolder*, bool useBaseTime);
void cancelRecordGhostPlayerRecorder(const al::IUseSceneObjHolder*);
void notifyGoalKillGhostPlayerRecorder(const al::IUseSceneObjHolder*);
bool tryRequestDownloadGhostData(al::NetworkSystem*, const GameDataHolder*, const char*, int, bool);
bool tryCalcEntryGhostId(int*, int*, int*, bool*, al::NetworkSystem*, GameDataHolderAccessor, int);
bool isExistTimeAttackRivalGhost(al::NetworkSystem*, GameDataHolderAccessor, int);
bool tryCreateGhostMiiIcon(al::NetworkSystem*, GameDataHolderAccessor, int);
const char* getStageNameGhostPlayerRecorder(const al::IUseSceneObjHolder*);
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder*, const char*, bool isStart);
void setFlagShakeAfterGhostPlayerRecorder(const CheckpointFlag*);
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder*, const char*);
void stopAndHideGhostPlayerAll(const al::IUseSceneObjHolder*);
void restartGhostPlayerAll(const al::IUseSceneObjHolder*);
void setPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder*,
                                        const sead::Vector3f& rotate, const sead::Vector3f& trans);
void resetPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder*);
void notifyPlayerStartWarpToGhostPlayer(const al::IUseSceneObjHolder*);
bool isFlagShakeAfterGhostPlayerRecorder(const al::IUseSceneObjHolder*);
const al::PlacementId* tryGetPlacementIdFlagGhostPlayerRecorder(const al::IUseSceneObjHolder*);
bool isExistGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser);
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                          const al::PlacementId* pPlacementId, bool isStart);
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                        const al::PlacementId* pPlacementId);
}  // namespace rc
