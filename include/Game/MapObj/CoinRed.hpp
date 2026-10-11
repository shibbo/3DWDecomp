#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
namespace al { class FlashingCtrl; class MtxConnector; }
class IUseRedCoin;
class ItemStateAssistRotate;
class CoinRed : public al::LiveActor {
public:
    CoinRed(const char*, IUseRedCoin*);
    ~CoinRed() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void acquirerCoinRed();
    void disableCountdown();
    static int getTimerFrame();
    void exeAppear();
    void rotate(float);
    void exeCountDown();
    void exeSpin();
    void exeStop();
    const sead::Vector3f& getItemDirection() const { return mItemDirection; }
    al::HitSensor* getCollector() const { return mCollector; }
    int getCollectedFrame() const { return mCollectedFrame; }
private:
    al::FlashingCtrl* mFlashing = nullptr;
    al::MtxConnector* mConnector = nullptr;
    sead::Vector3f mItemDirection = sead::Vector3f::ez;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    float mRotateY = 0.0f;
    ItemStateAssistRotate* mAssistRotate = nullptr;
    al::HitSensor* mCollector = nullptr;
    int mCollectedFrame = 0;
    IUseRedCoin* mHost;
};
static_assert(sizeof(CoinRed) == 0x198);
