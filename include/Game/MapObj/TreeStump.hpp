#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TreeStumpWatcher;

class TreeStump : public al::LiveActor {
public:
    TreeStump(const char* pName);
    virtual ~TreeStump();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void appear();
    virtual void kill();
    virtual void makeActorDead();
    virtual void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    virtual bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget);

    bool isReaction();
    void exeWait();
    void exeReactionStart();
    void exeReaction();
    void exeReactionEnd();

    TreeStumpWatcher* mWatcher = nullptr; // 0x148
    bool mIsStomped = false; // 0x150
};
