#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; }
class BlockStateItem;
class BlockBrick : public al::LiveActor {
public:
    explicit BlockBrick(const char*);
    void init(const al::ActorInitInfo&) override;
    void reappear() override;
    void respawn() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void tryAppearBreakModel();
    BlockStateItem* getBlockStateItem() const;
    void onConnectRailBlock();
    void exeState();
    void exeEmpty();
private:
    al::BreakModel* mBreakModel = nullptr;
    bool mConnectedRail = false;
    BlockStateItem* mState = nullptr;
    void* _160 = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    bool mSingleMode = false;
};
static_assert(sizeof(BlockBrick) == 0x178);
