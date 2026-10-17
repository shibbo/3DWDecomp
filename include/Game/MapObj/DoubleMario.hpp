#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ItemBubble;
class ItemStatePopUpFront;
namespace al { class RumbleCalculatorCosMultLinear; }

class DoubleMario : public al::LiveActor {
public:
    static s32 calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo) { return 1; }
    explicit DoubleMario(const char* pName, ItemBubble* = nullptr, bool = false);

    ~DoubleMario() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void appearPopUpFront();
    void setSensorRadius(float);
    void restoreSensorRadius();
    void exeWait();
    void exeAttachBubble();
    void exePopUpFront();
    void exeLand();
    void exeLandWait();
    void exeItemGetAfterWait();
private:
    ItemStatePopUpFront* mPopUpFront = nullptr;
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemBubble* mBubble;
    float mSensorRadius = 75.0f;
    bool mDelayedKill;
};
static_assert(sizeof(DoubleMario) == 0x168);
