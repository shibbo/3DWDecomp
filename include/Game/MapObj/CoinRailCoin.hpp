#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RailKeeper; }
class ItemStateAssistRotate;
class CoinRailCoin : public al::LiveActor {
public:
    CoinRailCoin(const char*, int, int, float, float);
    ~CoinRailCoin() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    bool isEnableMsgItemGet(const al::SensorMsg*) const;
    void exeStop();
    void exeStandby();
    void exeMove();
    void moveCoinRail();
    void exeSpinDrc();
    void setStop();
    void setMove();
    al::RailKeeper* getCoinRailKeeper() const { return mCoinRailKeeper; }
    void setFollowOffset(const sead::Vector3f* offset) { mFollowOffset = offset; }
private:
    al::RailKeeper* mCoinRailKeeper = nullptr;
    bool mInRouteDokan = false;
    ItemStateAssistRotate* mAssistRotate;
    int mCoinIndex;
    int mDelay;
    int mSection = 0;
    float mSpeed;
    float mSpacing;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    const sead::Vector3f* mFollowOffset = nullptr;
};
static_assert(sizeof(CoinRailCoin) == 0x198);
