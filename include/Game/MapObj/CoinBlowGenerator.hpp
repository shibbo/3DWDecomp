#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class CoinBlow;
class CoinBlowGenerator : public al::LiveActor {
public:
    struct InitParam {
        int coinCount;
        float blowSpeed;
        float randomSpeed;
        float radialSpeed;
    };
    explicit CoinBlowGenerator(const char*);
    ~CoinBlowGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void initWithParam(const InitParam&, const al::ActorInitInfo&);
    void exeBlow();
    /** @brief Makes the next blow spread the coins concentrically. */
    void setConcentric() { mIsConcentric = true; }
private:
    bool mIsConcentric = false;
    al::DeriveActorGroup<CoinBlow>* mCoins = nullptr;
    int mCoinCount = 0;
    int mLifeTime = 600;
    float mBlowSpeed = 35.0f;
    float mRandomSpeed = 4.0f;
    float mRadialSpeed = 10.0f;
};
static_assert(sizeof(CoinBlowGenerator) == 0x168);
