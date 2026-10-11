#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class FlowerCat : public al::LiveActor {
public:
    explicit FlowerCat(const char*);
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void tryAppearItem(const al::SensorMsg*, const al::HitSensor*, bool direct, bool indirect);
    void tryAppearItemScreenPointer(const al::SensorMsg*, const al::ScreenPointer*);
    void exeWait();
    void exeReaction();
private:
    int mReactionCooldown = 0;
    const char* mItemTiming = nullptr;
    bool mCottonGone = true;
};
static_assert(sizeof(FlowerCat) == 0x158);
