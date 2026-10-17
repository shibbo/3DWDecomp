#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; class RumbleCalculatorCosMultLinear; class ComboCounter; }
class BlockBrickBig : public al::LiveActor {
public:
    explicit BlockBrickBig(const char*);
    ~BlockBrickBig() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeReaction();
    void exeCrakReaction();
    void exeBreak();
private:
    int mControlUserId = -1;
    int mDurability = 5;
    int mReactionCount = 0;
    float mFrameRate = 0.0f;
    al::MtxConnector* mConnector = nullptr;
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;
    al::ComboCounter* mComboCounter;
};
static_assert(sizeof(BlockBrickBig) == 0x170);
