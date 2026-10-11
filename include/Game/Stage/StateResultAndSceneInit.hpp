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
private:
    unsigned char mUnreconstructed18[0x48];
};
static_assert(sizeof(StateResultAndSceneInit) == 0x60);
