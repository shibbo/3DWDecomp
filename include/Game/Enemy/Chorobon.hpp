#pragma once
#include "Enemy/ChorobonNode.hpp"

namespace al { class RumbleCalculatorCosMultLinear; }

/** @brief A Fuzzy that follows its holder's rail. */
class Chorobon : public ChorobonNode {
public:
    Chorobon(const char* pName, const f32* pSpeed);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void killBySwitch();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    bool tryReaction(const al::SensorMsg* pMsg);
    void exeMove();
    void exeReaction();
    void exeVanish();
private:
    sead::Vector3f mClippingCenter = sead::Vector3f(0.0f, 0.0f, 0.0f);
    al::RumbleCalculatorCosMultLinear* mRumble;
};
static_assert(sizeof(Chorobon) == 0x168);
