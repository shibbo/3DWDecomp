#pragma once
#include <math/seadVector.h>
#include <container/seadPtrArray.h>
class BlockHardLaserOnly;
class DisasterBlockDirector {
public:
    DisasterBlockDirector();
    void registerDisasterBlock(BlockHardLaserOnly*);
    void notifyBreak(BlockHardLaserOnly*);
    bool averageDisasterBlockPosition(sead::Vector3f&, int, bool);
    bool isClusterDestroyed(float);
    void setGlowSoundPlayer(BlockHardLaserOnly*);
    BlockHardLaserOnly* getGlowSoundPlayer();
private:
    s32 mBlockNum = 0;
    sead::PtrArray<BlockHardLaserOnly> mBlocks;
    BlockHardLaserOnly* mGlowSoundPlayer = nullptr;
};
static_assert(sizeof(DisasterBlockDirector) == 0x20);
