#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class CoinStackBase;
class CoinStackMoving : public al::LiveActor {
public:
    CoinStackMoving(const char*);
    ~CoinStackMoving() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void exeWait();
    void updateLinkedTrans(const sead::Vector3f&) override;
    void appear() override;
    void kill() override;
    void makeActorDead() override;
private:
    CoinStackBase** mCoins = nullptr;
    int mCoinCount = 0;
    int mCollectedCount = 0;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    al::MtxConnector* mConnector = nullptr;
};
static_assert(sizeof(CoinStackMoving) == 0x170);
