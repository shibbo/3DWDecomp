#pragma once

#include "Library/Scene/ISceneObj.hpp"

class MysteryBox;

class MysteryBoxConnector : public al::ISceneObj {
public:
    MysteryBoxConnector();
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    const char* getSceneObjName() const override;
    void registerSrcMysteryBox(MysteryBox* pBox);
    void registerDestMysteryBox(MysteryBox* pBox);

private:
    MysteryBox* mSrc = nullptr;
    MysteryBox* mDest = nullptr;
};

namespace MysteryBoxConnectorFunction {
void registerSrcMysteryBox(MysteryBox* pBox);
void registerDestMysteryBox(MysteryBox* pBox);
}
