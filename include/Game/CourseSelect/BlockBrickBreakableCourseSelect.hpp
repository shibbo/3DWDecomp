#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BreakModel;
}

/**
 * @brief Breakable brick block on the course-select map. Once broken, it stays gone for the rest
 * of the save via the course-select object flags.
 */
class BlockBrickBreakableCourseSelect : public al::LiveActor {
public:
    explicit BlockBrickBreakableCourseSelect(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void appear() override;

    void exeWait();
    void exeReaction();
    void exeBreak();

private:
    al::BreakModel* mBreakModel = nullptr;
    s32 mObjectId = -1;
};

static_assert(sizeof(BlockBrickBreakableCourseSelect) == 0x158);
