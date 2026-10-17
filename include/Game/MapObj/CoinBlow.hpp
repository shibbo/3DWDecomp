#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
namespace al { class FlashingCtrl; class CollisionPartsConnector; }
class ItemStateAssistRotate;
class CoinBlow : public al::LiveActor {
public:
    explicit CoinBlow(const char*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void rotate();
    void exeBlow();
    void exeWait();
    void exeSpinDrc();
    void setBound(bool);
    void setBoundOnArea();
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
    bool mCounted = false;
    bool mSpinFromWait = false;
    bool mConnected = false;
    bool mUseCollider = true;
    int mLifeTime = 0;
    int mOffCollideStep = 0;
    int mOffSensorStep = 28;
    int mHideModelStep = 0;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    al::FlashingCtrl* mFlashing = nullptr;
    ItemStateAssistRotate* mAssistRotate = nullptr;
    al::CollisionPartsConnector* mConnector;
};
static_assert(sizeof(CoinBlow) == 0x180);
