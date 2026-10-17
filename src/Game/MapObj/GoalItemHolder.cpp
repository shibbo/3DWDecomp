#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Layout/HeadIslandClear.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
GoalItemHolder::GoalItemHolder() { mGoalItems.allocBuffer(120, nullptr); }
const char* GoalItemHolder::getSceneObjName() const { return "GoalItemHolder"; }
void GoalItemHolder::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    mClearLayout = new HeadIslandClear("HeadIslandClear", al::getLayoutInitInfo(rInfo), nullptr, false);
    mSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), true);
    mSkipLayout->appear();
}
void GoalItemHolder::initSceneObj() {}
void GoalItemHolder::registerGoalItem(GoalItem* pItem, int index, int islandId) {
    mGoalItems.pushBack(pItem);
    pItem->setRegistration(index, islandId);
}
const sead::PtrArray<GoalItem>& GoalItemHolder::getGoalItems() const { return mGoalItems; }
GoalItem* GoalItemHolder::getGoalItemByIndex(int index) const { return mGoalItems[index]; }
GoalItem* GoalItemHolder::getGoalItem(int islandId, int shineId) const {
    if (shineId < 1) return nullptr;
    for (int i = 0; i < mGoalItems.size(); ++i) {
        GoalItem* item = mGoalItems[i];
        if (item->getIslandId() == islandId && item->getShineId() == shineId) return item;
    }
    return nullptr;
}
int GoalItemHolder::getGoalItemNum() const { return mGoalItems.size(); }
void GoalItemHolder::appearClearLayout() { mClearLayout->appear(); }
void GoalItemHolder::endClearLayout() { mClearLayout->end(); }
void GoalItemHolder::appearWindowProcessing() {
    if (mWindowProcessing) {
        mWindowProcessing->appearWithSystemMessage("SaveSequence", "WindowProcessing_Save", 90, false);
        mWindowProcessing->requestClose();
    }
}
bool GoalItemHolder::isClearLayoutKilled() { return !mClearLayout->isAlive(); }
DemoSkipLayout* GoalItemHolder::getSkipLayout() { return mSkipLayout; }
void GoalItemHolder::setClearLayoutText(int islandId, int shineId) { mClearLayout->updateTextBoxes(islandId, shineId); }
void GoalItemHolder::killEffect() { mClearLayout->killEffect(); }
const char* GoalItemHolder::getNextGoalItemGuideMessage(const al::LiveActor* pActor) const {
    int phase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(pActor));
    int count = SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(pActor));
    switch (phase) {
    case 1:
        return count == 5 ? "ShineCountReached_Phase1_Count1" : nullptr;
        break;
    case 3:
        if (SingleModeDataFunction::isFirstPhase2BossDefeated(GameDataHolderAccessor(pActor))) {
            return count == 20 ? "ShineCountReached_Phase2_Count2" : nullptr;
        } else return count == 15 ? "ShineCountReached_Phase2_Count1" : nullptr;
        break;
    case 5:
        if (SingleModeDataFunction::isFirstPhase3BossDefeated(GameDataHolderAccessor(pActor))) {
            if (count >= 48 && !SingleModeDataFunction::isGuideMessageAlreadySeen(GameDataHolderAccessor(pActor), 15, true))
                return "ShineCountReached_SuperHardDisaster";
        } else return count == 40 ? "ShineCountReached_Phase3_Count1" : nullptr;
        break;
    case 8:
        if (count == SingleModeDataFunction::getMaxCollectableGoalItems()) {
            DisasterModeController* controller = DisasterModeController::tryGetController(mSkipLayout);
            if (controller) controller->setSuperBowserV2(true);
            return "ShineCountReached_Phase4_Count1";
        }
        break;
    case 2:
    case 4:
    case 6:
    case 7:
        return nullptr;
    default:
        return nullptr;
    }
    return nullptr;
}

bool GoalItemHolder::isLastShineNeko() const {
    for (int i = 0; i < mGoalItems.size(); ++i)
        if (!mGoalItems[i]->isCollected() && !mGoalItems.unsafeAt(i)->isNekoShine()) return false;
    return true;
}
bool GoalItemHolder::isLastShineDisaster() const {
    for (int i = 0; i < mGoalItems.size(); ++i)
        if (!mGoalItems[i]->isCollected() && !mGoalItems.unsafeAt(i)->isDisasterShine()) return false;
    return true;
}
