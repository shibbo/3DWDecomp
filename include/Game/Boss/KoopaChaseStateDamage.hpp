#pragma once
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class HitSensor;
class SensorMsg;
}  // namespace al
class KoopaChase;
class KoopaChaseStateDamage : public al::NerveStateBase {
public:
    KoopaChaseStateDamage(KoopaChase* pHost);
    bool receiveMsg(bool* pIsDamage, const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf, bool isJumpOrWarp);
    bool isDamageFinish() const;

    int getDamageCount() const { return mDamageCount; }

    KoopaChase* mHost;
    int mDamageCount;
    unsigned char mStateData24[0xc];
};
