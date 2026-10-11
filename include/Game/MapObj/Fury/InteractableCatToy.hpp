#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class CoinBlow;
class InteractableCatToy : public al::LiveActor {
public:
    explicit InteractableCatToy(const char*);
    ~InteractableCatToy() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void Push(al::HitSensor*, al::HitSensor*, float);
    void TryDropCoin(al::HitSensor*, al::HitSensor*);
    void exeWait();
private:
    al::LiveActor* mAttachment = nullptr;
    float mRestLength = 0.0f;
    sead::PtrArray<CoinBlow> mCoins;
    int mCoinCount = 20;
    int mCoinsDropped = 0;
    int mAttackCooldown = 0;
};
static_assert(sizeof(InteractableCatToy) == 0x178);
