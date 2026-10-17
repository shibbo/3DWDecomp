#include "Scene/Phase0Scene.hpp"

#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"

Phase0Scene::Phase0Scene() : SingleModeScene("Phase0Scene") {}

Phase0Scene::~Phase0Scene() = default;

void Phase0Scene::preInitPlacement(const al::ActorInitInfo& rInfo) {
    SingleModeScene::preInitPlacement(rInfo);
}

void Phase0Scene::initAreaObj(const al::ActorInitInfo& rInfo) {
    SingleModeScene::initAreaObj(rInfo);
}

bool Phase0Scene::isChangePhase() const {
    return mIsChangePhase;
}

void Phase0Scene::kill() {
    SingleModeScene::kill();
    if (mIsChangePhase)
        SingleModeDataFunction::setUnlockedPhase(GameDataHolderWriter(mGameDataHolder), 1);
}

void Phase0Scene::handleGameOver() {
    mIsChangePhase = true;
    SingleModeScene::handleGameOver();
}

void Phase0Scene::appear() {
    SingleModeScene::appear();
}

bool SingleModeScene::isIslandScene() const {
    return false;
}

bool Phase0Scene::doZoneIDCheck() const {
    return false;
}
