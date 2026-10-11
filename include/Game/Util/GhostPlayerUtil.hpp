#pragma once
namespace al { class IUseSceneObjHolder; }
namespace rc {
/** @brief Built-in ghost owner entry (staff ghosts shipped in ROM). */
struct GhostPlayerUserInfo {
    const char* mKey;
    const char* mUserName;
    bool mIsMii;
};

const GhostPlayerUserInfo* findGhostPlayerUserInfo(const char* pKey);
void endRecordGhostPlayerRecorder(const al::IUseSceneObjHolder*, bool);
void notifyGoalKillGhostPlayerRecorder(const al::IUseSceneObjHolder* pHolder);
void stopAndHideGhostPlayerAll(const al::IUseSceneObjHolder* pHolder);
void restartGhostPlayerAll(const al::IUseSceneObjHolder* pHolder);
}
