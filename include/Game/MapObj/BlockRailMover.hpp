#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockRailMover : public al::LiveActor {
public:
    explicit BlockRailMover(const char*);
    ~BlockRailMover() override;
    void init(const al::ActorInitInfo&) override;
    void start();
    void calcBlockClippingCenter(sead::Vector3f*);
    void accompanyToRail();
    void exeStandBy();
    void exeWait();
    void exeStop();
    void exeMove();
private:
    struct Block { al::LiveActor* actor; float coord; u8 isLong; };
    bool mFixedDirection = true;
    int mBlockCount = 0;
    int mMoveType = 0;
    int mMoveTime = 0;
    int mWaitTime = 0;
    int mDelayTime = 0;
    float mSpeed = 3.0f;
    float mBlockLength = 0.0f;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    Block** mBlocks = nullptr;
};
static_assert(sizeof(BlockRailMover) == 0x178);
