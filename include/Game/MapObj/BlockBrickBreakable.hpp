#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; class MtxConnector; class ComboCounter; }
class SnowCover;
class BlockBrickBreakable : public al::LiveActor {
public:
    BlockBrickBreakable(const char*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void respawn() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void killBySwitch();
    bool trySendMsgToUpperLowerObj(al::HitSensor*, al::HitSensor*);
    bool isBreakable(const al::SensorMsg*, al::HitSensor*) const;
    void doBreak(bool);
    void exeWait();
    void exeReaction();
    void exeBreak();
private:
    al::BreakModel* mBreakModel = nullptr;
    int mControlUserId = -1;
    al::MtxConnector* mConnector = nullptr;
    SnowCover* mSnowCover = nullptr;
    bool mSendToBothSides = false;
    bool mAppearBreakModel = true;
    sead::Vector3f mClippingCenter;
    al::ComboCounter* mComboCounter;
};
static_assert(sizeof(BlockBrickBreakable) == 0x180);
