#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/// Watches its linked enemies and turns on its dead switch once all of them are dead.
class AllDeadWatcher : public al::LiveActor {
public:
    AllDeadWatcher(const char* pName);
    /** @brief Destroys the watcher. */
    ~AllDeadWatcher() override {}
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    virtual void exeWatch();
    virtual void exeWait();
    virtual void exeWatchPartial();
    void setWaitNerve();
    void setWatchNerve();

protected:
    al::LiveActor** mActors = nullptr;     // 0x148
    sead::Vector3f* mActorTrans = nullptr;  // 0x150
    int mActorCount = 0;                    // 0x158
    int mSwitchOnDelayStep = 10;            // 0x15C
    bool mIsEnableRespawn = false;          // 0x160
    bool mIsAllDead = false;                // 0x161
    bool mIsKilled = false;                 // 0x162
    bool _163 = false;                      // 0x163
    bool mIsValidSwitchPartialDeadOn = false;  // 0x164
    bool mIsRequireOnGround = false;           // 0x165
    bool mIsKillRemainingLinkedActors = true;  // 0x166
};

static_assert(sizeof(AllDeadWatcher) == 0x168);
