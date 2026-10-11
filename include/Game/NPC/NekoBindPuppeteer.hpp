#pragma once

#include <attributes.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class HitSensor;
class Nerve;
class SensorMsg;
}  // namespace al

class NekoNormal;
class PuppetStickRouteSelecter;

/**
 * @brief Drives a player bound to a regular cat (NekoNormal): the player jumps into the cat, rides
 * hidden inside it and gets thrown out (or dizzy) when the binding ends.
 */
class NekoBindPuppeteer : public BindPuppeteer {
public:
    NekoBindPuppeteer(NekoNormal* pNeko);

    bool isTargetSensor(al::HitSensor* pSensor);
    bool startEnter(al::HitSensor* pSensor);
    void startExit();
    void startExitStay();
    void startDizzy();
    void stopBind();
    void endNekoBind();
    void endBindForce();
    void endBindForceAbyss() override;
    void update();
    PuppetStickRouteSelecter* getRouteSelecter() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    void exeWait();
    void exeBindWait();
    void exeEnter();
    void exeHide();
    void exeExit();
    void exeDizzyStart();
    void exeDizzyLoop();
    void exeDizzyEnd();

    /** @brief Does nothing, the player has left the cat. */
    void exeEnd() {}

    /** @return Whether the puppeteer currently binds a player to the cat. */
    bool isBinding() const { return isBind(); }

private:
    ALWAYS_INLINE void restorePuppet();
    ALWAYS_INLINE bool tryEndDizzy();

    NekoNormal* mNeko;                                    // 0x20
    const al::Nerve* mEnterNerve = nullptr;               // 0x28
    al::HitSensor* mPlayerSensor = nullptr;               // 0x30
    PuppetStickRouteSelecter* mRouteSelecter = nullptr;  // 0x38
};

static_assert(sizeof(NekoBindPuppeteer) == 0x40);
