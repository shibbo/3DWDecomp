#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class MtxConnector; }
class GuideBalloon;

class TrampleSwitchGoalItem : public al::LiveActor {
public:
    TrampleSwitchGoalItem(const char* pName);
    virtual ~TrampleSwitchGoalItem();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void initAfterPlacement();
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    void appearBySwitch();
    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeReaction();

    al::MtxConnector* mMtxConnector = nullptr; // 0x148
    GuideBalloon* mGuide = nullptr; // 0x150
    int mGoalItemsNeeded = 0; // 0x158
    int mReactionFrames = 0; // 0x15c
};
