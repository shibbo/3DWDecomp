#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; }
class ItemStateLeaf;
class ItemBubble;
class ItemStatePopUpFront;
class ItemStateCheckCollision;
class ItemStatePopUpAbove;
class ItemStatePopUpFrontParam;
class SuperLeaf : public al::LiveActor {
public:
    explicit SuperLeaf(const char*, ItemBubble* = nullptr);
    ~SuperLeaf() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void appearPopUpAbove();
    void appearPopUpFront();
    void appearItemTakeOut();
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setPopUpOnCollide() { mDisablePopUpCollision = true; }
    void appearItemHoming(const al::HitSensor*);
    void exeHoming();
    void exeAttachBubble();
    void exeWait();
    void exePopUpAbove();
    void exePopUpFront();
    void exeWaitAndCheckCollision();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStateCheckCollision* mCollisionState = nullptr;
    ItemStatePopUpAbove* mPopUpAbove = nullptr;
    ItemStateLeaf* mLeaf = nullptr;
    ItemStatePopUpFront* mHoming = nullptr;
    ItemStatePopUpFrontParam* mCustomParam = nullptr;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    bool mIsTakeOut = false;
    bool mDisablePopUpCollision = false;
    float mColliderRadius = 0.0f;
};
static_assert(sizeof(SuperLeaf) == 0x190);
