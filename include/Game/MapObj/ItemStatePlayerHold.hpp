#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
namespace al { class HitSensor; class SensorMsg; }
class ItemStatePlayerHoldParam;
class HoldColliderControl;
class ItemStatePlayerHold : public al::ActorStateBase {
public:
    ItemStatePlayerHold(al::LiveActor*, const ItemStatePlayerHoldParam*, bool, bool);
    void init() override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool tryStartCarryUp(const al::SensorMsg*, al::HitSensor*, bool);
    bool tryStartCarryFront(const al::SensorMsg*, al::HitSensor*, bool);
    void initColliderControl();
    void updateCollider(al::HitSensor*);
    void exeHold();

    /** @brief Sets the flag at 0x23 (Gorobon enables it on gatekeeper stages). */
    void setFlag23(bool isEnable) { _23 = isEnable; }

private:
    bool mInvalidateSensors;
    bool _21;
    bool _22;
    bool _23;
    al::HitSensor* mHolderSensor;
    const ItemStatePlayerHoldParam* mParam;
    HoldColliderControl* mColliderControl;
    sead::Vector3f _40;
    sead::Vector3f _4c;
};
static_assert(sizeof(ItemStatePlayerHold) == 0x58);
