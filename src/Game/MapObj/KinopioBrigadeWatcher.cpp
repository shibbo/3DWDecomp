#include "MapObj/KinopioBrigadeWatcher.hpp"
#include "MapObj/KinopioBrigadeNpc.hpp"
#include "MapObj/GoalItem.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
namespace {
    NERVE_DECL(KinopioBrigadeWatcher, Watch);
    NERVES_MAKE_NOSTRUCT(KinopioBrigadeWatcher, Watch)
}
KinopioBrigadeWatcher::KinopioBrigadeWatcher(const char* name) : al::LiveActor(name) {}
KinopioBrigadeWatcher::~KinopioBrigadeWatcher() {}
void KinopioBrigadeWatcher::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvKinopioBrigadeWatcherWatch, 0);
    mMemberCount = al::calcLinkChildNum(info, "KinopioUndiscovered");
    ProjectActorFactory factory;
    for (int i = 0; i < mMemberCount; ++i)
        mMembers[i] = static_cast<KinopioBrigadeNpc*>(al::createLinksActorFromFactory(factory, info, "KinopioUndiscovered", i));
    makeActorAppeared();
}
void KinopioBrigadeWatcher::exeWatch() {
    int count = 0;
    for (int i = 0; i < mMemberCount; ++i) {
        GoalItem* item = mMembers[i]->getGoalItem();
        if (item) {
            int island = item->getIslandId();
            GameDataHolderAccessor accessor(this);
            if (SingleModeDataFunction::isScenarioComplete(accessor, island - 1, item->getShineId() - 1)) {
                ++count;
                sead::LookAtCamera camera = getSceneCameraInfo()->getViewAt(0)->getLookAtCam();
                al::getPlayerPos(this, 0);
                al::getTrans(mMembers[i]);
            }
        }
    }
    if (count == 4 && mShineId >= 0 && !SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mIslandId - 1, mShineId - 1))
        al::tryOnStageSwitch(this, "BrigadeAssembledOn");
}
ScenarioInfo KinopioBrigadeWatcher::getScenarioInfo() { return {mIslandId - 1, mShineId - 1}; }
int KinopioBrigadeWatcher::getDiscoveredMemberCount() const {
    int count = 0;
    for (int i = 0; i < mMemberCount; ++i) count += mMembers[i]->isDiscovered();
    return count;
}
sead::BitFlag8 KinopioBrigadeWatcher::getBrigadeCollected() const {
    sead::BitFlag8 flags;
    for (int i = 0; i < mMemberCount; ++i) {
        KinopioBrigadeNpc* member = mMembers[i];
        if (member->isDiscovered()) flags.setBit(member->getMemberIndex());
    }
    return flags;
}
