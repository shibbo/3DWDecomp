#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
namespace al { class FlashingCtrl; class MtxConnector; }
class ItemStateAssistRotate;
class TimerCoin : public al::LiveActor {
public:
    TimerCoin(const char*);
    void setTimerFrame(int frames) { mTimerFrame = frames; }
    bool isCounted() const { return mIsCounted; }
    void setCounted() { mIsCounted = true; }
    al::HitSensor* getCollectSensor() const { return mCollectSensor; }
    ~TimerCoin() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void rotate(float);
    void exeAppear();
    void exeCountDown();
    void exeSpin();
private:
    al::FlashingCtrl* mFlashing = nullptr;
    al::MtxConnector* mConnector = nullptr;
    al::HitSensor* mCollectSensor = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    float mRotateDegree = 0.0f;
    int mTimerFrame = 0;
    bool mIsCounted = false;
    ItemStateAssistRotate* mAssistRotate = nullptr;
};
