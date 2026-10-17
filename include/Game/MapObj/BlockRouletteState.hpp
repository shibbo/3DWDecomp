#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class ActorInitInfo; class SensorMsg; class HitSensor; class ScreenPointer; class ScreenPointTarget; }
class BlockRouletteState : public al::ActorStateBase {
public:
    BlockRouletteState(al::LiveActor*, const al::ActorInitInfo&);
    int getNextType() const;
    void appearItem();
    void exeWait();
    void exeAppearItem();
    void killDummyModel();
    void reset();
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*);
private:
    int mCurrentType = 1;
    al::LiveActor** mDummyModels = nullptr;
    int mUserId = -1;
    float* mScales;
    int mSoundFadeFrames = 80;
};
static_assert(sizeof(BlockRouletteState) == 0x48);
