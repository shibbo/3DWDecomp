#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; class MtxConnector; }
class ItemBubble;
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
class KinokoStateRunaway;
class KinokoTreasure : public al::LiveActor {
public:
    explicit KinokoTreasure(const char*, ItemBubble* = nullptr);
    ~KinokoTreasure() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void appearPopUpFront();
    void appearPopUpFrontNoRunaway();
    void appearPopUpAbove();
    void appearItemTakeOut();
    void appearPopUpAboveConnectToCollision();
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void exeWait();
    void exeAttachBubble();
    void exePopUpFront();
    void exePopUpAbove();
    void exeRunaway();
    void exeLand();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStatePopUpFront* mPopUpFront = nullptr;
    KinokoStateRunaway* mRunaway = nullptr;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    al::MtxConnector* mConnector = nullptr;
    bool mConnectToCollision = false;
    bool mIsTakenOut = false;
    bool mCanRunaway = true;
    float mColliderRadius = 0.0f;
    bool mAttachDuringPopUp = false;
    bool mValidateClippingOnWait = false;
};
static_assert(sizeof(KinokoTreasure) == 0x188);
