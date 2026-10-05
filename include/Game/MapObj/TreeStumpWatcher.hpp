#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>

class TreeStump;

class TreeStumpWatcher : public al::LiveActor {
public:
    TreeStumpWatcher(const char* pName);
    virtual ~TreeStumpWatcher();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void appear();
    virtual void kill();
    virtual void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    virtual bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget);

    void exeWatch();
    void addStomped();

    sead::PtrArray<TreeStump> mStumps; // 0x148
    int mStompedCount = 0; // 0x158
    bool mIsSwitchOn = false; // 0x15c
};
