#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class SensorMsg; class HitSensor; }
class ItemStatePopUpFrontParam;
class ItemStateLeaf : public al::ActorStateBase {
public:
    ItemStateLeaf(al::LiveActor*, const ItemStatePopUpFrontParam*);
    void init() override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void exePopUp();
    void exeTakeOut();
    void exeFallLeaf();
    void exeLandLeaf();
    void setTakeOut(bool value) { mIsTakeOut = value; }
    void setParam(const ItemStatePopUpFrontParam* param) { mParam = param; }
private:
    const ItemStatePopUpFrontParam* mParam;
    bool mIsInWater = false;
    bool mIsTakeOut = false;
};
static_assert(sizeof(ItemStateLeaf) == 0x30);
