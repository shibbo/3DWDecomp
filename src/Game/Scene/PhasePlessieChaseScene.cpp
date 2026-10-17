#include "Scene/PhasePlessieChaseScene.hpp"

#include "Layout/SingleModeSceneLayout.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"

PhasePlessieChaseScene::PhasePlessieChaseScene() = default;

PhasePlessieChaseScene::~PhasePlessieChaseScene() {
    SingleModeDataFunction::clearHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 16);
    SingleModeDataFunction::clearHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 29);
    SingleModeDataFunction::recordDisasterMode(GameDataHolderWriter(mGameDataHolder),
        SingleModeDataFunction::DisasterForceSetting_Calm, this);
    SingleModeDataFunction::setAutoForeshadow(GameDataHolderWriter(mGameDataHolder), false);
    SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderAccessor(this).getHolder(), true);
}

void PhasePlessieChaseScene::init(const al::SceneInitInfo& rInfo) {
    mGameDataHolder = GameDataFunction::getGameDataHolder(rInfo.mGameDataHolder);
    if (SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mGameDataHolder)) >= 8) {
        SingleModeDataFunction::setUnlockedPhase(GameDataHolderWriter(mGameDataHolder), 10);
        SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 29);
    } else {
        SingleModeDataFunction::setUnlockedPhase(GameDataHolderWriter(mGameDataHolder), 7);
        SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter(mGameDataHolder), 16);
    }
    SingleModeDataFunction::setPhaseEnd(GameDataHolderWriter(mGameDataHolder), false);
    SaveDataAccessFunction::startSaveDataWriteSync(mGameDataHolder, true);
    SingleModeScene::init(rInfo);
    if (mSceneLayout)
        mSceneLayout->setAreaNamePhaseStart(false);
}

void PhasePlessieChaseScene::appear() {
    PhaseScene::appear();
}

bool PhaseScene::doZoneIDCheck() const {
    return false;
}
