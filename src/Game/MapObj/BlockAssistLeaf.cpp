#include "MapObj/BlockAssistLeaf.hpp"
#include "MapObj/BlockAssistFunction.hpp"
#include "MapObj/BlockStateItem.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"

namespace {
    NERVE_DECL(BlockAssistLeaf, Init);
    NERVE_DECL(BlockAssistLeaf, Wait);
    const struct {
        BlockAssistLeafNrvInit Init;
        BlockAssistLeafNrvWait Wait;
    } NrvBlockAssistLeaf{};
}

BlockAssistLeaf::BlockAssistLeaf(const char* pName) : al::LiveActor(pName) {}
BlockAssistLeaf::~BlockAssistLeaf() {}

void BlockAssistLeaf::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = "BlockAssistLeaf";
    alPlacementFunction::tryGetModelName(&modelName, rInfo);
    al::initActorWithArchiveName(this, rInfo, modelName, rc::getBlockSuffixName(rInfo, false));
    al::initNerve(this, &NrvBlockAssistLeaf.Init, 1);
    mStateItem = new BlockStateItem(this, rInfo, false, false, false, true, false);
    al::initNerveState(this, mStateItem, &NrvBlockAssistLeaf.Wait, "BlockItem");
    al::offCollide(this);
    al::hideModel(this);
    BlockAssistFunction::setBlockAssist(this);
    makeActorAppeared();
}

void BlockAssistLeaf::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    mStateItem->attackSensor(pSelf, pOther);
}

bool BlockAssistLeaf::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return mStateItem->receiveMsg(pMsg, pOther, pSelf);
}

bool BlockAssistLeaf::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
    al::ScreenPointTarget* pTarget) {
    return mStateItem->receiveMsgScreenPoint(pMsg, pPointer);
}

void BlockAssistLeaf::exeInit() {
    if ((!GameDataFunction::isCheckpointPass(GameDataHolderAccessor(this)) &&
         BlockAssistFunction::isBlockAssistSetCheckPoint(this)) ||
        (GameDataFunction::isCheckpointPass(GameDataHolderAccessor(this)) &&
         BlockAssistFunction::isBlockAssistSetStartPoint(this))) {
        makeActorDead();
        return;
    }
    if (BlockAssistFunction::isEnableAppearBlockAssist(this)) {
        makeActorAppeared();
        al::showModelIfHide(this);
        al::setNerve(this, &NrvBlockAssistLeaf.Wait);
    } else {
        makeActorDead();
    }
}

void BlockAssistLeaf::exeWait() {
    if (al::updateNerveState(this)) {
        al::startHitReactionDisappear(this);
        kill();
    }
}
