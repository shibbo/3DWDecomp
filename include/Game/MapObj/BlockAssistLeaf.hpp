#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class BlockStateItem;

class BlockAssistLeaf : public al::LiveActor {
public:
    BlockAssistLeaf(const char* pName);
    ~BlockAssistLeaf() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void exeInit();
    void exeWait();

private:
    BlockStateItem* mStateItem = nullptr;
    void* _150 = nullptr;
};
