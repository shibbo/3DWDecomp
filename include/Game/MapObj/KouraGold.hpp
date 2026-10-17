#pragma once
#include "MapObj/Koura.hpp"
class KouraGold : public Koura {
public:
    explicit KouraGold(const char*);
    void init(const al::ActorInitInfo&) override;
    void appearStart();
    void killSwitch();
    const char* getArchiveName() const override;
    void onHitWall() override;
    void incCoin();
    void control() override;
private:
    sead::Vector3f mLastCoinPos = {0.0f, 0.0f, 0.0f};
    int mCoinNum = 0;
    int mCoinsSpawned = 0;
    int mWallHitCooldown = 0;
    int mBreakDelay = 5;
};
static_assert(sizeof(KouraGold) == 0x290);
