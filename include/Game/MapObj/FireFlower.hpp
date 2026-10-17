#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; }
class ItemBubble;
class ItemStateCheckCollision;
class ItemStatePopUpAbove;
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
class FireFlower : public al::LiveActor {
public:
    explicit FireFlower(const char*, ItemBubble* = nullptr);
    ~FireFlower() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void appearPopUpAbove();
    void appearPopUpFront();
    void appearItemTakeOut();
    void appearItemHoming(const al::HitSensor*);
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setPopUpOnCollide() { mPopUpOnCollide = true; }
    void exeWait();
    void exeAttachBubble();
    void exePopUpAbove();
    void exePopUpFront();
    void exeWaitAndCheckCollision();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStateCheckCollision* mCollisionState = nullptr;
    ItemStatePopUpAbove* mPopUpAbove = nullptr;
    ItemStatePopUpFront* mPopUpFront = nullptr;
    bool mIsTakeOut = false;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    float mColliderRadius = 0.0f;
    bool mPopUpOnCollide = false;
};
static_assert(sizeof(FireFlower) == 0x188);
