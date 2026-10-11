#pragma once

#include "MapObj/BindPuppeteer.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al { struct ActorInitInfo; }
class BindWarpEffect;

class RouteDokanLauncherPuppeteer : public BindPuppeteer {
public:
    RouteDokanLauncherPuppeteer(const char* pName, const al::ActorInitInfo& rInfo);
    void startBindForce(al::HitSensor* pPlayer, al::HitSensor* pBinder,
                        const sead::Quatf& rQuat, const sead::Vector3f& rTrans);
    void startBindInStart(al::HitSensor* pPlayer, al::HitSensor* pBinder,
                          const sead::Quatf& rQuat, const sead::Vector3f& rTrans);
    bool isEnableStart() const;
    void exeBindForce();
    void exeInWait();

private:
    BindWarpEffect* mWarpEffect = nullptr;
};
