#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; }
class ItemBubble;
class ItemStateCheckCollision;
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
class WhiteBell : public al::LiveActor {
public:
    explicit WhiteBell(const char*, ItemBubble* = nullptr);
    ~WhiteBell() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void appearPopUpFront();
    void appearItemTakeOut();
    void appearItemHoming(const al::HitSensor*);
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setPopUpOnCollide() { mPopUpOnCollide = true; }
    void exeWait();
    void exeAttachBubble();
    void exePopUpFront();
    void exeWaitAndCheckCollision();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStatePopUpFront* mPopUpFront = nullptr;
    ItemStateCheckCollision* mCollisionState = nullptr;
    ItemBubble* mBubble;
    al::FlashingCtrl* mFlashing = nullptr;
    bool mIsTakeOut = false;
    bool mPopUpOnCollide = false;
    float mColliderRadius = 0.0f;
};
static_assert(sizeof(WhiteBell) == 0x178);
