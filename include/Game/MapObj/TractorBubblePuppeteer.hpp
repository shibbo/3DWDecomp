#pragma once

#include <basis/seadTypes.h>

#include "MapObj/BindPuppeteer.hpp"

namespace al {
class HitSensor;
class LiveActor;
class SensorMsg;
}  // namespace al

class TractorBubble;

/**
 * @brief Drives the player carried inside a TractorBubble.
 */
class TractorBubblePuppeteer : public BindPuppeteer {
public:
    TractorBubblePuppeteer(TractorBubble* pBubble);

    void update();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther);
    void startBubbleWithActivate(al::LiveActor* pPlayer);
    void startBubbleWithScreenOut(al::LiveActor* pPlayer);
    void startBubbleWithInput(al::LiveActor* pPlayer);
    void endBubble();
    void endBubblePunch();

    /** @brief Gets the player carried by the bubble. @return The player actor, or nullptr. */
    al::LiveActor* getPlayerActor() const { return mPlayerActor; }

private:
    TractorBubble* mBubble;  // 0x20
    al::LiveActor* mPlayerActor;  // 0x28
};

static_assert(sizeof(TractorBubblePuppeteer) == 0x30);
