#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Coin;
class CoinCirclePlacement : public al::LiveActor {
public:
    explicit CoinCirclePlacement(const char*);
    void init(const al::ActorInitInfo&) override;
    void exeWatch();
private:
    Coin** mCoins = nullptr;
    int mCoinCount = 1;
    float mSpeed = 0.0f;
    float mAngle = 0.0f;
    float mSideRadius = 0.0f;
    float mFrontRadius = 0.0f;
    sead::Vector3f mSide = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mUp = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mFront = {0.0f, 0.0f, 0.0f};
};
static_assert(sizeof(CoinCirclePlacement) == 0x188);
