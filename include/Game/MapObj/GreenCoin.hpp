#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
namespace al { class FlashingCtrl; class MtxConnector; }
class GreenRing;
class ItemStateAssistRotate;
class GreenCoin : public al::LiveActor {
public:
    GreenCoin(const char*);
    ~GreenCoin() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void acquirerCoinGreen();
    int getCountDownStep() const;
    void setHost(GreenRing* host) { mHost = host; }
    static int getTimerFrame();
    void exeAppear();
    void rotate(float);
    void exeCountDown();
    void exeSpin();
private:
    al::FlashingCtrl* mFlashing = nullptr;
    al::MtxConnector* mConnector = nullptr;
    sead::Vector3f mItemDirection = sead::Vector3f::ez;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    float mRotateY = 0.0f;
    ItemStateAssistRotate* mAssistRotate = nullptr;
    int mCollectedFrame = 0;
    GreenRing* mHost = nullptr;
};
static_assert(sizeof(GreenCoin) == 0x190);
