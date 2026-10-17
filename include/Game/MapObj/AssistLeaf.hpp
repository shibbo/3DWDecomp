#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class FlashingCtrl; }
class ItemStateLeaf;
class ItemStateCheckCollision;
class ItemStatePopUpAbove;
class ItemStatePopUpFrontParam;
class AssistLeaf : public al::LiveActor {
public:
    explicit AssistLeaf(const char*);
    ~AssistLeaf() override;
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
    void exeWait();
    void exePopUpAbove();
    void exePopUpFront();
    void exeWaitAndCheckCollision();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    ItemStateCheckCollision* mCollisionState = nullptr;
    ItemStatePopUpAbove* mPopUpAbove = nullptr;
    ItemStateLeaf* mLeaf = nullptr;
    ItemStatePopUpFrontParam* mCustomParam = nullptr;
    al::FlashingCtrl* mFlashing = nullptr;
    bool mIsTakeOut = false;
    float mColliderRadius = 0.0f;
};
static_assert(sizeof(AssistLeaf) == 0x180);
