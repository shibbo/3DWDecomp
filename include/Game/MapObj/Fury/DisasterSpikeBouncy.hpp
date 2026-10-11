#pragma once

#include "MapObj/DisasterSpike.hpp"

namespace al {
class AreaObj;
class PadRumbleKeeper;
}  // namespace al

/// Disaster spike that bounces the player up high when they land on it.
class DisasterSpikeBouncy : public DisasterSpike {
public:
    explicit DisasterSpikeBouncy(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void control() override;
    void kill() override;

private:
    al::HitSensor* mBindPlayerSensor = nullptr;  // 0x318
    al::PadRumbleKeeper* mPadRumbleKeeper;       // 0x320
    al::AreaObj* mCameraArea = nullptr;          // 0x328
    al::HitSensor* mPlayerSensor = nullptr;      // 0x330
    bool mIsKeepPlayerVelocity = false;          // 0x338
    bool mIsBouncing = false;                    // 0x339
    bool mIsBounceStarted = false;               // 0x33a
};

static_assert(sizeof(DisasterSpikeBouncy) == 0x340);
