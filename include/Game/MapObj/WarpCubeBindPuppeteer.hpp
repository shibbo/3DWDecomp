#pragma once
#include "MapObj/BindPuppeteer.hpp"
#include <math/seadVector.h>
namespace al { class ActorInitInfo; class SensorMsg; }
class BindWarpEffect;
class WarpCubeBindPuppeteer : public BindPuppeteer {
public:
    WarpCubeBindPuppeteer(const char*, const al::ActorInitInfo&);
    void startBindForce(al::HitSensor*, al::HitSensor*, const sead::Vector3f&);
    void startBindInStart(al::HitSensor*, al::HitSensor*);
    bool tryCancelBind(const al::SensorMsg*, al::HitSensor*);
    void endBind(const PlayerBindEndParam*) override;
    void endBindOnGround() override;
    void endBindSquat() override;
    void exeBindForce();
    void exeInWait();
    bool isEnableStart() const;
private:
    BindWarpEffect* mWarpEffect;
};
