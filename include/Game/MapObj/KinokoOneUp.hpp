#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class RumbleCalculatorCosMultLinear; class MtxConnector; }
class ItemBubble;
class ItemStatePopUpFront;
class ItemStatePopUpAbove;
class ItemStatePopUpFrontParam;
class KinokoOneUp : public al::LiveActor {
public:
    explicit KinokoOneUp(const char*, ItemBubble* = nullptr);
    ~KinokoOneUp() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    bool isEnableMsgItemGet(const al::SensorMsg*) const;
    void appearPopUpFront();
    void appearWait();
    void appearPopUpAbove();
    void appearItemTakeOut();
    void appearForceGet(const al::HitSensor*);
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setInRouteDokan() { mIsInRouteDokan = true; }
    void setPopUpOnCollide() { mCollideOnPopUp = true; }
    void exeWait();
    void exeAttachBubble();
    void exeAppearWait();
    void exePopUpFront();
    void exePopUpAbove();
    void exeRunaway();
    void exeForceGet();
private:
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    al::MtxConnector* mConnector = nullptr;
    const al::HitSensor* mForceGetSensor = nullptr;
    ItemStatePopUpFront* mPopUpFront = nullptr;
    ItemStatePopUpAbove* mPopUpAbove = nullptr;
    ItemBubble* mBubble;
    float mForceGetHeight = 0.0f;
    float mForceGetSpeed = 0.0f;
    bool mIsInRouteDokan = false;
    bool _181;
    bool mCollideOnPopUp = false;
};
static_assert(sizeof(KinokoOneUp) == 0x188);
