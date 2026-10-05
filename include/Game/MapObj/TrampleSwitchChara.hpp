#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class MtxConnector; }

class TrampleSwitchChara : public al::LiveActor {
public:
    TrampleSwitchChara(const char* pName);
    virtual ~TrampleSwitchChara();
    virtual void init(const al::ActorInitInfo& rInfo);
    virtual void initAfterPlacement();
    virtual void control();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver);

    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeReaction();

    al::MtxConnector* mConnector = nullptr; // 0x148
    int mCharaType = 0;                    // 0x150
    int mReactionFrames = 0;               // 0x154
};
