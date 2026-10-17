#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class BreakModel; }

class BlockBrickBreakableCourseSelect : public al::LiveActor {
public:
    explicit BlockBrickBreakableCourseSelect(const char* pName);
    ~BlockBrickBreakableCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    void appear() override;
    void exeWait();
    void exeReaction();
    void exeBreak();

private:
    al::BreakModel* mpBreakModel = nullptr;
    int mObjectId = -1;
};

static_assert(sizeof(BlockBrickBreakableCourseSelect) == 0x158);
