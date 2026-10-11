#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/IUseRedCoin.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
namespace al { template <class T> class DeriveActorGroup; }
class CoinRed;
class CollectNumber;
class CoinRedRing : public al::LiveActor, public IUseRedCoin {
public:
    CoinRedRing(const char*);
    ~CoinRedRing() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appearItemCoinRed();
    void exeWait();
    void exeCountDown();
    int getSeParamNum() override;
    int getMaxCoinNum() override;
private:
    al::DeriveActorGroup<CoinRed>* mCoins = nullptr;
    sead::PtrArray<CollectNumber> mNumbers;
    int mCollectedCount = 0;
    int mItemType = 8;
    al::HitSensor* mCollector = nullptr;
    sead::Vector3f mClippingOffset{0.0f, 0.0f, 0.0f};
};
static_assert(sizeof(CoinRedRing) == 0x188);
