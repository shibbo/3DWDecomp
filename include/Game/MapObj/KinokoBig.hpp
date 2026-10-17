#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; }
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
class KinokoStateRunaway;
class KinokoBig : public al::LiveActor {
public:
    explicit KinokoBig(const char*);
    ~KinokoBig() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void appearPopUpFront();
    void appearPopUpFront(const ItemStatePopUpFrontParam&);
    void exePopUpFront();
    void exeRunaway();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStatePopUpFront* mPopUpState = nullptr;
    KinokoStateRunaway* mRunawayState = nullptr;
};
static_assert(sizeof(KinokoBig) == 0x160);
