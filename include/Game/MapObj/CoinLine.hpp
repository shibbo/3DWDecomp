#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RailKeeperGroup; template<class T> class DeriveActorGroup; }
class Coin;
class CoinLine : public al::LiveActor {
public:
    explicit CoinLine(const char*);
    ~CoinLine() override;
    void init(const al::ActorInitInfo&) override;
    int getCoinGroupNum() const;
    void appear() override;
    void control() override;
private:
    al::RailKeeperGroup* mRails = nullptr;
    al::DeriveActorGroup<Coin>** mGroups = nullptr;
    int mFrame = -1;
    int mDelay = 0;
    int mDelayPerCoin = 7;
    bool mInvalidPopUp = false;
};
static_assert(sizeof(CoinLine) == 0x168);
