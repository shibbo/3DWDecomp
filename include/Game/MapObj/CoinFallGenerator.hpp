#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class CoinBlow;
class CoinFallGenerator : public al::LiveActor {
public:
    explicit CoinFallGenerator(const char*);
    ~CoinFallGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void exeStandby();
    void exeFall();
private:
    al::DeriveActorGroup<CoinBlow>* mCoins = nullptr;
    int mCoinCount = 0;
    int mFinishedCount = 0;
    int mLifeTime = 600;
    int* mPositions = nullptr;
    int mFallDelay = 0;
};
static_assert(sizeof(CoinFallGenerator) == 0x170);
