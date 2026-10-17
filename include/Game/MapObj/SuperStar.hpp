#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ItemBubble;
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
namespace al { class RumbleCalculatorCosMultLinear; class OccludedEffectRequestInfo; }
class SuperStar : public al::LiveActor {
public:
    explicit SuperStar(const char*, ItemBubble* = nullptr);
    ~SuperStar() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void reappear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    bool isEnableMsgItemGet(const al::SensorMsg*) const;
    void appearPopUpFront();
    void appearItemTakeOut();
    void appearItemHoming(const al::HitSensor*);
    void setPopUpFrontParam(const ItemStatePopUpFrontParam*);
    void setPopUpOnCollide() { mPopUpOnCollide = true; }
    void exeWait();
    void exeAttachBubble();
    void exePopUpFront();
    void exeRunaway();
private:
    ItemStatePopUpFront* mPopUpFront = nullptr;
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    bool mIsInRouteDokan = false;
    bool mUsingOccludedEffect = true;
    ItemBubble* mBubble;
    bool mPopUpOnCollide = false;
    bool mIsSingleMode = false;
    s32 mPlacementIndex = -1;
    al::OccludedEffectRequestInfo* mOccludedEffect = nullptr;
};
static_assert(sizeof(SuperStar) == 0x178);
