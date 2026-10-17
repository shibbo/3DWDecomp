#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockRouletteState;
class BlockRoulette : public al::LiveActor {
public:
    explicit BlockRoulette(const char*);
    ~BlockRoulette() override;
    void init(const al::ActorInitInfo&) override;
    void respawn() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeAppearItem();
private:
    BlockRouletteState* mState = nullptr;
    al::LiveActor* mInnerModel = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
};
