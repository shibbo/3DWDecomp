#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class IUseSceneCreator; struct SequenceInitInfo; class LayoutInitInfo; }
class GameDataHolder;
class WipeCurtainResult;
class StageWipeKeeper;
class ControllerConnectChecker;
/** @brief Result display and scene initialization state, partially reconstructed. */
class StateResultAndSceneInit : public al::NerveStateBase {
public:
    StateResultAndSceneInit(al::IUseSceneCreator* pCreator, const al::SequenceInitInfo& rInfo,
                            const al::LayoutInitInfo& rLayoutInfo, GameDataHolder* pHolder,
                            WipeCurtainResult* pWipe, StageWipeKeeper* pWipeKeeper,
                            ControllerConnectChecker* pChecker);
    bool tryOpenWipeForNextScene() const;
    bool tryOpenWipeForTitleScene() const;
    bool isOpenWipeForNextScene() const;
    void setForTitleScene() { mIsForTitleScene = true; }
private:
    alignas(8) unsigned char mUnreconstructed18[0x38];
    bool mIsForTitleScene;
    unsigned char mUnreconstructed51[0xf];
};
static_assert(sizeof(StateResultAndSceneInit) == 0x60);
