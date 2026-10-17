#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BlockEmptyCourseSelect;
class CoinCountUp;

class BlockQuestionCourseSelect : public al::LiveActor {
public:
    explicit BlockQuestionCourseSelect(const char* pName);
    ~BlockQuestionCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void appear() override;
    void exeWait();
    void exeAppearItem();
    void exeEnd();

private:
    int mObjectId = -1;
    BlockEmptyCourseSelect* mpEmptyBlock = nullptr;
    CoinCountUp* mpCoin = nullptr;
};

static_assert(sizeof(BlockQuestionCourseSelect) == 0x158);
