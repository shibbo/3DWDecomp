#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CoinBlow : public al::LiveActor {
public:
    explicit CoinBlow(const char*);
    void setOffCollide(int);
    void setOffSensor(int);
    void appearWithHitReaction();
    void setHideModel(int);
    void setWaiting();
    void setLifeTime(int frames) { mLifeTime = frames; }
    bool isCounted() const { return mCounted; }
    void setCounted() { mCounted = true; }
    void disableCollider() { mUseCollider = false; }
private:
    bool mCounted;
    u8 _145[2];
    bool mUseCollider;
    int mLifeTime;
    u8 mUnreconstructed14C[0x34];
};
static_assert(sizeof(CoinBlow) == 0x180);
