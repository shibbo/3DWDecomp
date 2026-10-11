#pragma once

#include <basis/seadTypes.h>

class DashPanelSeTriggerChecker {
public:
    DashPanelSeTriggerChecker();
    void init(int entryCount, int cooldownFrames, bool checkSuppression);
    void update();
    bool tryTrigger(int id, bool isSuppressed);

private:
    struct Entry {
        int id;
        int remainingFrames;
    };

    Entry* mEntries = nullptr;
    u32 mEntryCount = 0;
    int mCooldownFrames = 0;
    bool mCheckSuppression = false;
};

static_assert(sizeof(DashPanelSeTriggerChecker) == 0x18);
