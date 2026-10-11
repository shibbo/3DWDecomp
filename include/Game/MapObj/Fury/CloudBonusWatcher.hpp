#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/Scene/ISceneObj.hpp"

namespace al {
class LiveActor;
}

/// Scene object that tracks the cloud bonus stages of Bowser's Fury.
class CloudBonusWatcher : public al::ISceneObj {
public:
    /// Actors registered for one cloud bonus stage (identified by its island area file ID).
    struct Entry {
        s32 mFileID = -1;
        sead::FixedPtrArray<al::LiveActor, 128> mActors;
    };

    typedef sead::FixedPtrArray<Entry, 8> EntryArray;

    CloudBonusWatcher();

    bool tryRegisterActor(al::LiveActor* pActor, s32 fileID);
    void tryRegisterCloudBonusLauncher(al::LiveActor* pActor, sead::Vector3f pos);
    s32 tryFindIslandAreaFileID(al::LiveActor* pActor, sead::Vector3f pos);
    bool tryUpdateLastCloudBonusFileID(al::LiveActor* pActor);
    bool tryResetLastCloudBonusStage();
    bool isPlayerInCloudBonusStage() const;
    void setPlayerInCloudBonusStage(bool isInStage);

private:
    EntryArray mEntries;
    s32 mLastFileID = -1;
    bool mIsPlayerInCloudBonusStage = false;
};

static_assert(sizeof(CloudBonusWatcher::Entry) == 0x418);
static_assert(sizeof(CloudBonusWatcher) == 0x60);
