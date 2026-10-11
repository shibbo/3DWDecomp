#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class BlockRailRouteSelecter;
class HitSensor;
class SensorMsg;
}  // namespace al

class Koura;

/**
 * @brief Drives a player riding inside a green shell (Koura). Partial layout.
 */
class KouraBindPuppeteer : public BindPuppeteer {
public:
    KouraBindPuppeteer(Koura* pKoura);

    bool isTargetSensor(al::HitSensor* pSensor);
    bool startEnter(al::HitSensor* pPlayerSensor, bool isSingleMode);
    void startExit();
    void startExitStay();
    void startDizzy();
    void stopBind();
    void endKouraBind();
    void endKouraBindWallHit(const sead::Vector3f& rDir);
    void endBindForce();
    void endBindForceAbyss() override;
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

    /** @brief Gets the sensor of the bound player. @return The player sensor. */
    al::HitSensor* getPlayerSensor() const { return mPlayerSensor; }

private:
    u8 _20[0x30 - 0x20];
    al::HitSensor* mPlayerSensor;  // 0x30
    u8 _38[0x48 - 0x38];
};

static_assert(sizeof(KouraBindPuppeteer) == 0x48);
