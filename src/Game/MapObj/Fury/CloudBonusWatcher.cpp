#include "MapObj/Fury/CloudBonusWatcher.hpp"

#include <attributes.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/**
 * @brief Finds the island area containing a position and returns its file ID.
 * @param pActor Actor used to reach the area objects.
 * @param rPos Position to test.
 * @return The file ID of the island area, or -1 if none contains the position.
 */
ALWAYS_INLINE s32 findIslandAreaFileID(al::LiveActor* pActor, const sead::Vector3f& rPos) {
    al::AreaObjGroup* group = rc::tryFindAreaObjGroup(pActor, rc::AreaObjType::IslandArea);
    if (group == nullptr) {
        return -1;
    }

    for (s32 i = 0; i < group->getSize(); i++) {
        al::AreaObj* area = group->getAreaObj(i);
        if (area->isInVolumeCheck(rPos)) {
            return area->getPlacementInfo()._28;
        }
    }

    return -1;
}
}  // namespace

/**
 * @brief Constructs the watcher and allocates every stage entry.
 */
CloudBonusWatcher::CloudBonusWatcher() {
    for (s32 i = 0; i < mEntries.capacity(); i++) {
        mEntries.pushBack(new Entry());
    }
}

/**
 * @brief Registers an actor to the entry of a cloud bonus stage.
 * @param pActor Actor to register.
 * @param fileID File ID of the stage's island area.
 * @return False if the stage's entry is full, true otherwise.
 */
bool CloudBonusWatcher::tryRegisterActor(al::LiveActor* pActor, s32 fileID) {
    for (s32 i = 0; i < mEntries.size(); i++) {
        Entry* entry = mEntries.unsafeAt(i);
        if (entry->mFileID != -1 && entry->mFileID == fileID) {
            if (entry->mActors.size() == entry->mActors.capacity()) {
                return false;
            }

            entry->mActors.pushBack(pActor);
            return true;
        }
    }

    return true;
}

/**
 * @brief Assigns the island area containing a cloud bonus launcher to a free stage entry.
 * @param pActor The launcher actor.
 * @param pos Position of the launcher.
 */
void CloudBonusWatcher::tryRegisterCloudBonusLauncher(al::LiveActor* pActor, sead::Vector3f pos) {
    s32 fileID = findIslandAreaFileID(pActor, pos);
    if (fileID == -1) {
        return;
    }

    for (s32 i = 0; i < mEntries.size(); i++) {
        Entry* entry = mEntries.unsafeAt(i);
        if (entry->mFileID == -1) {
            entry->mFileID = fileID;
            return;
        }
    }
}

/**
 * @brief Finds the file ID of the island area containing a position.
 * @param pActor Actor used to reach the area objects.
 * @param pos Position to test.
 * @return The file ID of the island area, or -1 if none contains the position.
 */
s32 CloudBonusWatcher::tryFindIslandAreaFileID(al::LiveActor* pActor, sead::Vector3f pos) {
    return findIslandAreaFileID(pActor, pos);
}

/**
 * @brief Remembers the cloud bonus stage whose island area contains the active player.
 * @param pActor Actor used to reach the player.
 * @return True if the player is in a registered cloud bonus stage.
 */
bool CloudBonusWatcher::tryUpdateLastCloudBonusFileID(al::LiveActor* pActor) {
    sead::Vector3f pos = al::getTrans(rc::getActivePlayer(pActor));
    s32 fileID = findIslandAreaFileID(pActor, pos);
    if (fileID == -1) {
        return false;
    }

    for (s32 i = 0; i < mEntries.size(); i++) {
        s32 entryFileID = mEntries.unsafeAt(i)->mFileID;
        if (entryFileID == -1) {
            return false;
        }

        if (entryFileID == fileID) {
            mLastFileID = fileID;
            return true;
        }
    }

    return false;
}

/**
 * @brief Revives every dead actor of the last cloud bonus stage.
 * @return True if the last stage's entry was found.
 */
bool CloudBonusWatcher::tryResetLastCloudBonusStage() {
    if (mLastFileID == -1) {
        return false;
    }

    for (s32 i = 0; i < mEntries.size(); i++) {
        if (mEntries.at(i)->mFileID == mLastFileID) {
            for (s32 j = 0; j < mEntries.at(i)->mActors.size(); j++) {
                al::LiveActor* actor = mEntries.at(i)->mActors.at(j);
                if (al::isDead(actor)) {
                    actor->reappear();
                }
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief Whether the player is in a cloud bonus stage.
 * @return True if the player is in a cloud bonus stage.
 */
bool CloudBonusWatcher::isPlayerInCloudBonusStage() const {
    return mIsPlayerInCloudBonusStage;
}

/**
 * @brief Sets whether the player is in a cloud bonus stage.
 * @param isInStage True if the player entered a cloud bonus stage.
 */
void CloudBonusWatcher::setPlayerInCloudBonusStage(bool isInStage) {
    mIsPlayerInCloudBonusStage = isInStage;
}
