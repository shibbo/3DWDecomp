#pragma once

namespace al {
class HitSensor;
class SensorMsg;
}  // namespace al

class NekoNormal;

/**
 * @brief Moves a regular cat (NekoNormal) around while it is bound to (ridden by or carried in) an
 * other actor.
 * @note Only what reconstructed code needs is declared so far.
 */
class NekoBindPuppeteer {
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
    void endBindForceAbyss();
    void update();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);

    /** @return Whether the puppeteer currently binds the cat to an actor. */
    bool isBinding() const { return mBindSensor != nullptr; }

private:
    unsigned char _0[0x10];
    al::HitSensor* mBindSensor;  // 0x10
    unsigned char _18[0x40 - 0x18];
};

static_assert(sizeof(NekoBindPuppeteer) == 0x40);
