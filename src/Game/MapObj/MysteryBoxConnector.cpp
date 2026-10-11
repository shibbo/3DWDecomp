#include "MapObj/MysteryBoxConnector.hpp"
#include "MapObj/MysteryBox.hpp"
#include "Library/Scene/SceneObjUtil.hpp"

MysteryBoxConnector::MysteryBoxConnector() {}

void MysteryBoxConnector::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    if (mSrc && mDest)
        mSrc->setDestMysteryBox(mDest);
}

void MysteryBoxConnector::registerSrcMysteryBox(MysteryBox* pBox) {
    mSrc = pBox;
}

void MysteryBoxConnector::registerDestMysteryBox(MysteryBox* pBox) {
    mDest = pBox;
}

namespace MysteryBoxConnectorFunction {
void registerSrcMysteryBox(MysteryBox* pBox) {
    static_cast<MysteryBoxConnector*>(al::createSceneObj(pBox, 12))->registerSrcMysteryBox(pBox);
}

void registerDestMysteryBox(MysteryBox* pBox) {
    static_cast<MysteryBoxConnector*>(al::createSceneObj(pBox, 12))->registerDestMysteryBox(pBox);
}
}

const char* MysteryBoxConnector::getSceneObjName() const {
    return "ミステリーボックス接続者";
}
