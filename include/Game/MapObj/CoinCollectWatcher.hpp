#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "MapObj/IUseRedCoin.hpp"
#include <container/seadPtrArray.h>
class CoinRed;
class CollectNumber;
class CoinCollectWatcher : public al::LiveActor, public IUseRedCoin {
public:
    CoinCollectWatcher(const char*);
    ~CoinCollectWatcher() override;
    void init(const al::ActorInitInfo&) override;
    int getSeParamNum() override;
    int getMaxCoinNum() override;
    void exeWatch();
private:
    al::DeriveActorGroup<CoinRed>* mCoins = nullptr;
    sead::PtrArray<CollectNumber> mNumbers;
    int mCollectedCount = 0;
};
