#include "MapObj/SaveDataChecker.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"

namespace {
    NERVE_DECL(SaveDataChecker, Wait);
    NERVES_MAKE_NOSTRUCT(SaveDataChecker, Wait)
}

SaveDataChecker::SaveDataChecker(const char* pName) : al::LiveActor(pName) {}
SaveDataChecker::~SaveDataChecker() {}

void SaveDataChecker::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvSaveDataCheckerWait, 0);
    al::tryGetArg(&mCutsceneId, rInfo, "UnlockFlag");
    mCutsceneId = rc::convertMapUnitFlagToCutscene(mCutsceneId);
    al::listenStageSwitchOn(this, "OnSaveSetSwitch", al::Functor(this, &SaveDataChecker::onSwitchSetSave));
    if (mCutsceneId >= 0)
        makeActorAppeared();
    else
        makeActorDead();
}

void SaveDataChecker::onSwitchSetSave() {
    if (mCutsceneId < 0)
        return;
    SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter(this), mCutsceneId);
    SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderAccessor(this).getHolder(), true);
}

void SaveDataChecker::appear() { al::LiveActor::appear(); }
void SaveDataChecker::initAfterPlacement() {}
void SaveDataChecker::exeDead() {}

void SaveDataChecker::exeWait() {
    if (SingleModeDataFunction::hasSeenCutscene(GameDataHolderAccessor(this), mCutsceneId))
        al::tryOnStageSwitch(this, "SwitchSaveIsSetOn");
    else
        al::tryOnStageSwitch(this, "SwitchSaveIsUnsetOn");
    kill();
}
