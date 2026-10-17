#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IGoalObj.hpp"
class BlockSlot;
namespace al { class SimpleLayoutAppearWaitEnd; class SimpleLayoutAppearWait; }
class GoalBonusGameBlockSlot : public al::LiveActor, public IGoalObj {
public:
    explicit GoalBonusGameBlockSlot(const char*);
    ~GoalBonusGameBlockSlot() override;
    void init(const al::ActorInitInfo&) override;
    void exeWait();
    void exeRolling();
    void startSlot();
    bool updateSameId();
    bool isStopSlotAll() const;
    void exeResult();
    int calcResultCount() const;
    void exeSplashCoin();
    void exeSplashCoinEnd();
    void exeHead();
    void exeEnd();
    bool isGoal() const override;
    bool isEndGoalDemo() const override;
    bool isUseResult() const override;
private:
    BlockSlot** mSlots = nullptr;
    al::SimpleLayoutAppearWaitEnd* mCounter = nullptr;
    al::SimpleLayoutAppearWait* mResult = nullptr;
    int mSlotCount = 3;
    int mReward = 0;
    int mCoinsRemaining = 0;
    int mExtraLives = 0;
    int mFirstMatch = -1;
};
static_assert(sizeof(GoalBonusGameBlockSlot) == 0x180);
