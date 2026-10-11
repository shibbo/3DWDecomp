#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BlockEmptyCourseSelect;
class CoinCountUp;

/**
 * @brief Question block on the course-select map. Punching it from below pops out a coin and
 * turns it into an empty block, which stays for the rest of the save via the course-select
 * object flags.
 */
class BlockQuestionCourseSelect : public al::LiveActor {
public:
    explicit BlockQuestionCourseSelect(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void appear() override;

    void exeWait();
    void exeAppearItem();
    void exeEnd();

private:
    s32 mObjectId = -1;
    BlockEmptyCourseSelect* mEmptyBlock = nullptr;
    CoinCountUp* mCoin = nullptr;
};

static_assert(sizeof(BlockQuestionCourseSelect) == 0x158);
