#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; class MtxConnector; }
class ItemBubble;
class ItemStatePopUpFront;
class ItemStatePopUpAbove;
class ItemStatePopUpFrontParam;
class KinokoStateRunaway;
class KinokoSuper : public al::LiveActor {
public:
    explicit KinokoSuper(const char*, ItemBubble* = nullptr);
    ~KinokoSuper() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void appearPopUpFront();
    void appearPopUpFrontNoRunaway();
    void appearPopUpAbove();
    void appearItemTakeOut();
    void appearItemHoming(const al::HitSensor*);
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setPopUpOnCollide() { mCollideOnPopUp = true; }
    void exeWait();
    void exeAttachBubble();
    void exePopUpFront();
    void exePopUpAbove();
    void exeRunaway();
    void exeLand();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStatePopUpFront* mPopUpFront = nullptr;
    ItemStatePopUpAbove* mPopUpAbove = nullptr;
    KinokoStateRunaway* mRunaway = nullptr;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    al::MtxConnector* mConnector = nullptr;
    bool mIsTakenOut = false;
    bool mCanRunaway = true;
    bool mCollideOnPopUp = false;
    float mColliderRadius = 0.0f;
};
static_assert(sizeof(KinokoSuper) == 0x188);
