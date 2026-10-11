#pragma once

#include <basis/seadTypes.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class BlockRailRouteSelecter;
class HitSensor;
class SensorMsg;
}  // namespace al

class KouraSurf;

/**
 * @brief Drives a player riding inside a surfing shell (KouraSurf).
 */
class KouraSurfBindPuppeteer : public BindPuppeteer {
public:
    KouraSurfBindPuppeteer(KouraSurf* pKoura);

    bool isTargetSensor(al::HitSensor* pSensor);
    bool startEnter(al::HitSensor* pPlayerSensor);
    void startExit();
    void startExitStay();
    void startDizzy();
    void stopBind();
    void endKouraBind();
    void endBindForce();
    void endBindForceAbyss();
    void update();
    al::BlockRailRouteSelecter* getRouteSelecter() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    void exeWait();
    void exeBindWait();
    void exeEnter();
    void exeHide();
    void exeExit();
    void exeDizzyStart();
    void exeDizzyLoop();
    void exeDizzyEnd();

private:
    u8 _20[0x40 - 0x20];
};

static_assert(sizeof(KouraSurfBindPuppeteer) == 0x40);
