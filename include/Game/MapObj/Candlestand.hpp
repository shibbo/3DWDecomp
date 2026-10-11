#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CandlestandWatcher;
class Candlestand : public al::LiveActor {
public:
    Candlestand(const char*, CandlestandWatcher* = nullptr);
    ~Candlestand() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isLightOff();
    void exeLightOn();
    void exeLightOff();
    void exeLightOnAndShake();
private:
    bool mIsEnableLightPrePass = true;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    CandlestandWatcher* mWatcher;
};
static_assert(sizeof(Candlestand) == 0x180);
