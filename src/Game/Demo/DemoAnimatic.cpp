#include "Demo/DemoAnimatic.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/DemoUtil.hpp"

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}

/** @brief Creates an animatic without binding players. @param pName Actor name.
 * @param type Audio demo category. */
DemoAnimatic::DemoAnimatic(const char* pName, alSeFunction::DemoType type)
    : DemoCutscene(pName, type) {
    mIsBindPlayer = false;
}

/** @brief Initializes layout playback and the skip prompt. @param rInfo Placement data. */
void DemoAnimatic::init(const al::ActorInitInfo& rInfo) {
    initDemoPlacement(rInfo);
    initPuppets(rInfo);
    al::tryGetArg(&mIsRequireOnGround, rInfo, "RequireOnGround");
    mDemo = rc::createDemoSceneHolderLayoutOnly(mDemoName, rInfo,
        rc::createPlayerRetargettingSelector(this), nullptr, false, 4);
    al::tryGetArg(&mIsEndPhaseWhenDone, rInfo, "EndPhaseWhenDone");
    if (mIsEndPhaseWhenDone)
        setEndSceneFlag();
    al::tryGetArg(&mCutsceneId, rInfo, "UnlockFlag");
    mCutsceneId = rc::convertMapUnitFlagToCutscene(mCutsceneId);
    mSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), al::isSingleMode(rInfo));
    mSkipLayout->appear();
}
