#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
class SensorMsg;
}  // namespace al

/** @brief Chameleon-like model actor of a Gamane that switches between transparent and real. */
class GamaneChameleon : public al::LiveActor {
public:
    GamaneChameleon(const char* pName, al::LiveActor* pHost);

    bool isInvisible() const;
    bool isTransparent() const;
    void requestDirectHit();
    void requestHipDropReaction(const al::SensorMsg* pMsg, al::HitSensor* pSensor);
    void requestMicReaction();
    void requestTouchReaction();
    void requestDisappear();

private:
    u8 mUnreconstructed144[0x2C];
};

static_assert(sizeof(GamaneChameleon) == 0x170);
