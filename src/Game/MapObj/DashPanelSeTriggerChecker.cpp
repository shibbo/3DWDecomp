#include "MapObj/DashPanelSeTriggerChecker.hpp"

DashPanelSeTriggerChecker::DashPanelSeTriggerChecker() {}

void DashPanelSeTriggerChecker::init(int entryCount, int cooldownFrames, bool checkSuppression) {
    mEntryCount = entryCount;
    mCooldownFrames = cooldownFrames;
    mEntries = new Entry[entryCount];
    for (u32 i = 0; i < mEntryCount; ++i) {
        mEntries[i].id = -1;
        mEntries[i].remainingFrames = 0;
    }
    mCheckSuppression = checkSuppression;
}

void DashPanelSeTriggerChecker::update() {
    for (u32 i = 0; i < mEntryCount; ++i) {
        int remaining = mEntries[i].remainingFrames - 1;
        if (remaining >= 0)
            mEntries[i].remainingFrames = remaining;
        else
            mEntries[i].id = -1;
    }
}

bool DashPanelSeTriggerChecker::tryTrigger(int id, bool isSuppressed) {
    if (mCheckSuppression && isSuppressed)
        return false;
    for (u32 i = 0; i < mEntryCount; ++i) {
        if (mEntries[i].id == id)
            return false;
    }
    for (u32 i = 0; i < mEntryCount; ++i) {
        if (mEntries[i].remainingFrames == 0) {
            mEntries[i].id = id;
            mEntries[i].remainingFrames = mCooldownFrames;
            return true;
        }
    }
    return false;
}
