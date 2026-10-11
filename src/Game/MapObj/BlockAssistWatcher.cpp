#include "MapObj/BlockAssistWatcher.hpp"
#include "MapObj/BlockAssistFunction.hpp"
#include "MapObj/BlockAssistLeaf.hpp"
#include "MapObj/CheckpointFlag.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/PlayingStageDataFunction.hpp"
#include "System/CourseInfoHolder.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
namespace { const int cMissCount[4] = {5, 10, 15, 20}; }
BlockAssistWatcher::BlockAssistWatcher(int courseId, GameDataHolder* holder)
    : mGameDataHolder(holder), mCourseId(courseId) {
    mBlocks = new BlockInfo*[2];
    for (int i = 0; i < 2; ++i) mBlocks[i] = new BlockInfo;
}
void BlockAssistWatcher::initAfterPlacementSceneObj(const al::ActorInitInfo& info) {
    mSceneObjHolder = info.getActorSceneInfo().sceneObjHolder;
    if (mBlockCount == 2) {
        for (int i = 0; i < mBlockCount; ++i) {
            float distance = (al::getTrans(mBlocks[i]->block) - mCheckpointTrans).length();
            mBlocks[i]->distance = distance;
        }
        BlockInfo* closer = mBlocks[0]->distance < mBlocks[1]->distance ? mBlocks[0] : mBlocks[1];
        closer->isStartPoint = false;
    }
}
void BlockAssistWatcher::setCheckpointFlagTrans(const CheckpointFlag* flag) {
    if (flag) {
        mHasCheckpoint = true;
        mCheckpointTrans.set(al::getTrans(flag));
    }
}
void BlockAssistWatcher::setBlockAssist(BlockAssistLeaf* block) {
    if (static_cast<unsigned>(mBlockCount) < 2) {
        mBlocks[mBlockCount]->block = block;
        ++mBlockCount;
    }
}
bool BlockAssistWatcher::isBlockAssistSetStartPoint(const BlockAssistLeaf* block) const {
    if (mBlockCount == 1) return true;
    for (int i = 0; i < mBlockCount; ++i)
        if (mBlocks[i]->block == block) return mBlocks[i]->isStartPoint;
    return false;
}
bool BlockAssistWatcher::isBlockAssistSetCheckPoint(const BlockAssistLeaf* block) const {
    if (mBlockCount < 2) return false;
    for (int i = 0; i < mBlockCount; ++i)
        if (mBlocks[i]->block == block) return !mBlocks[i]->isStartPoint;
    return false;
}
bool BlockAssistWatcher::isEnableAppearBlockAssist(const BlockAssistLeaf* block) const {
    int players = rc::getActiveControlUserNum(GameDataHolderAccessor(block));
    PlayingStageDataFunction::setStagePlayerNumForAssistBlock(GameDataHolderAccessor(block), players);
    int playerCount = PlayingStageDataFunction::getStagePlayerNumForAssistBlock(GameDataHolderAccessor(block));
    if (GameDataFunction::isClearWithAssistBlock(GameDataHolderAccessor(block))) return true;
    int misses = cMissCount[playerCount - 1];
    if (GameDataFunction::getMissCount(GameDataHolderAccessor(block)) >= misses) {
        if (!CourseInfoFunction::isClear(GameDataHolderAccessor(block), mCourseId)) return true;
    }
    return false;
}
namespace BlockAssistFunction {
void setCheckpointFlag(const al::IUseSceneObjHolder* holder, const CheckpointFlag* flag) {
    al::getSceneObj<BlockAssistWatcher>(holder, 30)->setCheckpointFlagTrans(flag);
}
void setBlockAssist(BlockAssistLeaf* block) { al::getSceneObj<BlockAssistWatcher>(block, 30)->setBlockAssist(block); }
bool isBlockAssistSetStartPoint(const BlockAssistLeaf* block) { return al::getSceneObj<BlockAssistWatcher>(block, 30)->isBlockAssistSetStartPoint(block); }
bool isBlockAssistSetCheckPoint(const BlockAssistLeaf* block) { return al::getSceneObj<BlockAssistWatcher>(block, 30)->isBlockAssistSetCheckPoint(block); }
bool isEnableAppearBlockAssist(const BlockAssistLeaf* block) { return al::getSceneObj<BlockAssistWatcher>(block, 30)->isEnableAppearBlockAssist(block); }
}
const char* BlockAssistWatcher::getSceneObjName() const { return "アシストブロック監視者"; }
al::SceneObjHolder* BlockAssistWatcher::getSceneObjHolder() const { return mSceneObjHolder; }
