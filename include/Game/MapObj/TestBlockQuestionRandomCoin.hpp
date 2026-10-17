#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockEmpty;
class TestBlockQuestionRandomCoin : public al::LiveActor {
public:
    explicit TestBlockQuestionRandomCoin(const char*);
    ~TestBlockQuestionRandomCoin() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void control() override;
    void exeWait();
    void exeAppearCoin();
    void exeEmpty();
    void exeReaction();
    void exeHipDropReaction();
private:
    int mCoinTimer = 0;
    BlockEmpty* mEmptyBlock;
};
static_assert(sizeof(TestBlockQuestionRandomCoin) == 0x150);
