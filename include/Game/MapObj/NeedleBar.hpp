#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NeedleBar : public al::LiveActor {
public:
    explicit NeedleBar(const char*);
    ~NeedleBar() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void setRotateY(float, bool);
    bool isNerveSupportFreeze() const;
    bool isStop() const;
    void onSyncSupportFreeze();
    void offSyncSupportFreeze();
    void exeWait();
    void exeSupportFreeze();
    void exeSupportFreezeSync();
private:
    sead::Quatf mInitialQuat = sead::Quatf::unit;
    sead::Vector3f mCenter = sead::Vector3f::zero;
    sead::Vector3f mSide = sead::Vector3f::ex;
    float mOrbitRadius = 0.0f;
    float mRotateX = 0.0f;
    float mRotateY = 0.0f;
    float mSensorRadius = 400.0f;
    int mFreezeTime = 0;
};
static_assert(sizeof(NeedleBar) == 0x180);
