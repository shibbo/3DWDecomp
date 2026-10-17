#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class CoinStackBase;
class CoinStack : public al::LiveActor {
public:
    CoinStack(const char*);
    ~CoinStack() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void exeWait();
private:
    CoinStackBase** mCoins = nullptr;
    int mCoinCount = 0;
    int mCollectedCount = 0;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    al::MtxConnector* mConnector = nullptr;
};
static_assert(sizeof(CoinStack) == 0x170);
