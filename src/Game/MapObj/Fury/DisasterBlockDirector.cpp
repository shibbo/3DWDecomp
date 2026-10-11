#include "MapObj/Fury/DisasterBlockDirector.hpp"
#include "MapObj/Fury/BlockHardLaserOnly.hpp"
#include "Library/ActorUtil.hpp"
DisasterBlockDirector::DisasterBlockDirector() { mBlocks.allocBuffer(100, nullptr); }
void DisasterBlockDirector::registerDisasterBlock(BlockHardLaserOnly* block) {
    if (mBlockNum == 100) return;
    mBlocks.pushBack(block);
    ++mBlockNum;
}
void DisasterBlockDirector::notifyBreak(BlockHardLaserOnly* block) {
    BlockHardLaserOnly* nearest = nullptr;
    float nearestDistance = -1.0f;
    for (s32 i = 0; i < mBlockNum; ++i) {
        if (mBlocks.at(i) == block || mBlocks.at(i)->isBreaking(false) || al::isDead(mBlocks.at(i)) || mBlocks.at(i)->getFileID() != block->getFileID() || !mBlocks.at(i)->canChainBreak() || mBlocks.at(i)->isDisabled()) continue;
        float distance = (al::getTrans(mBlocks.at(i)) - al::getTrans(block)).squaredLength();
        float candidateLimit = mBlocks.at(i)->getMaxChainBreakDistance();
        float sourceLimit = block->getMaxChainBreakDistance();
        if (candidateLimit != -1.0f || sourceLimit != -1.0f) {
            float limit;
            if (candidateLimit == -1.0f) limit = sourceLimit;
            else if (sourceLimit == -1.0f) limit = candidateLimit;
            else limit = candidateLimit < sourceLimit ? candidateLimit : sourceLimit;
            if (distance > limit * limit) continue;
        }
        if (nearestDistance == -1.0f || distance < nearestDistance) {
            nearest = mBlocks.at(i);
            nearestDistance = distance;
        }
    }
    if (nearest) nearest->breakBlock(0);
}
bool DisasterBlockDirector::averageDisasterBlockPosition(sead::Vector3f& position, int fileID, bool onlyAlive) {
    position = sead::Vector3f::zero;
    s32 count = 0;
    for (s32 i = 0; i < mBlockNum; ++i) {
        if (fileID != -1 && mBlocks.at(i)->getFileID() != fileID) continue;
        if (onlyAlive && al::isDead(mBlocks.at(i))) continue;
        if (mBlocks.at(i)->isDisabled()) continue;
        ++count;
        position += al::getTrans(mBlocks.at(i));
    }
    if (count == 0) return false;
    position *= 1.0f / count;
    return true;
}
bool DisasterBlockDirector::isClusterDestroyed(float fileID) {
    for (s32 i = 0; i < mBlockNum; ++i) {
        if (mBlocks.at(i)->getFileID() == fileID && !al::isDead(mBlocks.at(i))) return false;
    }
    return true;
}
void DisasterBlockDirector::setGlowSoundPlayer(BlockHardLaserOnly* block) { mGlowSoundPlayer = block; }
BlockHardLaserOnly* DisasterBlockDirector::getGlowSoundPlayer() { return mGlowSoundPlayer; }
