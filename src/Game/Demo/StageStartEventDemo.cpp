#include "Demo/StageStartEventDemo.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

class PlayerRetargettingSelector;
namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
    PlayerRetargettingSelector* pSelector, const sead::Matrix34f* pMtx, bool flag, int count);
bool tryStartDemo(DemoSceneActorHolder* pDemo);
bool tryEndDemo(DemoSceneActorHolder* pDemo);
}

namespace {
NERVE_DECL(StageStartEventDemo, End);
NERVE_DECL(StageStartEventDemo, Play);
NERVE_DECL(StageStartEventDemo, Fade);
NERVES_MAKE_NOSTRUCT(StageStartEventDemo, End, Play, Fade)
}

/** @brief Creates the cutscene event. @param pName Actor name. */
StageStartEventDemo::StageStartEventDemo(const char* pName) : StageStartEventBase(pName) {}

/** @brief Creates the cutscene, skip prompt, and wipe. @param rInfo Actor initialization data. */
void StageStartEventDemo::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvStageStartEventDemoEnd, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    mWipe = new al::WipeSimple("黒フェード", "WipeFadeBlack", al::getLayoutInitInfo(rInfo), "Demo");
    mSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), al::isSingleMode(rInfo));
    al::listenStageSwitchOnAppear(this, al::Functor(this, &StageStartEventDemo::startDemo));
    if (alPlacementFunction::tryGetModelName(&mModelName, rInfo)) {
        mDemo = rc::createDemoSceneHolder(mModelName, rInfo,
            rc::createPlayerRetargettingSelector(this), nullptr, false, 4);
    }
    makeActorDead();
}

/** @brief Signals the demo switch and activates playback and the skip prompt. */
void StageStartEventDemo::startDemo() {
    al::tryOnStageSwitch(this, "SwitchDemoPlayOn");
    appear();
    mSkipLayout->appear();
    al::setNerve(this, &NrvStageStartEventDemoPlay);
}

/** @brief Ends the cutscene and enters the fade sequence. */
void StageStartEventDemo::endDemo() {
    rc::tryEndDemo(mDemo);
    al::setNerve(this, &NrvStageStartEventDemoFade);
}

/** @brief Checks completion. @return Whether the event is in its end state. */
bool StageStartEventDemo::isEndDemo() const {
    return al::isNerve(this, &NrvStageStartEventDemoEnd);
}

/** @brief Starts the cutscene and waits for a skip request or camera completion. */
void StageStartEventDemo::exePlay() {
    if (al::isFirstStep(this)) {
        rc::tryStartDemo(mDemo);
        mDemo->startAction(0, false);
        if (al::isEqualString(mModelName, "DemoCourseStartKoopaCastleStage"))
            al::startBgm(this, "StartDemo", -1, 0, -1, -1);
        else
            al::startBgm(this, "Stage", -1, 0, -1, -1);
    }
    auto ports = rc::getActiveInputPortList(GameDataHolderAccessor(this));
    if (mSkipLayout->isSkip(sead::BitFlag<u16>(ports)) || mDemo->isActionEndCamera(30))
        al::setNerve(this, &NrvStageStartEventDemoFade);
}

/** @brief Closes the wipe, ends the cutscene, and reopens onto gameplay. */
void StageStartEventDemo::exeFade() {
    if (al::isFirstStep(this)) {
        mSkipLayout->kill();
        mWipe->startClose(30);
    } else if (mWipe->isCloseEnd()) {
        al::tryOffStageSwitch(this, "SwitchDemoPlayOn");
        mWipe->startOpen(45);
        rc::tryEndDemo(mDemo);
        al::setNerve(this, &NrvStageStartEventDemoEnd);
    }
}

/** @brief Removes the completed event actor. */
void StageStartEventDemo::exeEnd() {
    kill();
}
