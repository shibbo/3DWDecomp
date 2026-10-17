#include "MapObj/BlockChoiceWatcher.hpp"
#include "MapObj/BlockChoice.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
    NERVE_DECL(BlockChoiceWatcher, Wait);
    NERVE_DECL(BlockChoiceWatcher, ReleasedItem);
    NERVES_MAKE_NOSTRUCT(BlockChoiceWatcher, Wait, ReleasedItem)
}
BlockChoiceWatcher::BlockChoiceWatcher(const char* pName) : al::LiveActor(pName) {}
BlockChoiceWatcher::~BlockChoiceWatcher() {}
void BlockChoiceWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvBlockChoiceWatcherWait, 0);
    al::initRandomSeedByTick();
    al::initRandomSeedByTickNonSync();
    mBlockCount = al::calcLinkChildNum(rInfo, "WatchBlock");
    mBlocks = new BlockChoice*[mBlockCount];
    for (int i = 0; i < mBlockCount; ++i) {
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, al::getPlacementInfo(rInfo), "WatchBlock", i);
        const char* modelName = "BlockChoice";
        alPlacementFunction::getModelName(&modelName, placement);
        if (al::isEqualString(modelName, "BlockChoiceBig"))
            mBlocks[i] = new BlockChoice("チョイスブロック大");
        else
            mBlocks[i] = new BlockChoice("チョイスブロック小");
        al::initLinksActor(mBlocks[i], rInfo, "WatchBlock", i);
    }
    makeActorAppeared();
}
void BlockChoiceWatcher::exeWait() {
    int index = -1;
    for (int i = 0; i < mBlockCount; ++i) {
        if (mBlocks[i]->isActivated()) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;
    bool isSuccess = al::getRandom(100.0f) < 70.0f;
    mBlocks[index]->appearItemAll(isSuccess);
    disappearOtherBlock(index);
    mIsReleasedItem = true;
    al::setNerve(this, &NrvBlockChoiceWatcherReleasedItem);
}
void BlockChoiceWatcher::disappearOtherBlock(int index) {
    for (int i = 0; i < mBlockCount; ++i)
        if (index != i)
            mBlocks[i]->setDisappear();
}
void BlockChoiceWatcher::exeReleasedItem() {}
