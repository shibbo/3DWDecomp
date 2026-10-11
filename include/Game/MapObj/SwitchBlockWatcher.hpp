#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"

class SwitchBlock;
class TrampleSwitch;

class SwitchBlockWatcher : public al::LiveActor {
public:
    SwitchBlockWatcher(const char* pName);
    virtual ~SwitchBlockWatcher();
    virtual void init(const al::ActorInitInfo& rInfo);

    void exeWait();
    void exeMove();
    bool isMoving() const;

    int mMoveCount = 0; // 0x144
    bool mMoveRequested = false; // 0x148
    al::DeriveActorGroup<SwitchBlock>* mBlocks = nullptr; // 0x150
    al::DeriveActorGroup<TrampleSwitch>* mSwitches = nullptr; // 0x158
};
