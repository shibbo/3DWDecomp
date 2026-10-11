#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class FlashingCtrl; }
class ItemBubble;
class ItemStateAssistRotate;
class CoinAttach : public al::LiveActor {
public:
    CoinAttach(const char*, ItemBubble*);
    ~CoinAttach() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeAttach();
    void rotate();
    void exeAttachBubble();
    void exePopUpFront();
    void exeLand();
    void exeSpinDrc();
private:
    ItemStateAssistRotate* mAssistRotate = nullptr;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    int mBounceCount = 0;
    bool mCheckWaterEntryOnly = false;
};
static_assert(sizeof(CoinAttach) == 0x178);
