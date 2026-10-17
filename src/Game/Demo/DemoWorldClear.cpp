#include "Demo/DemoWorldClear.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectFairy.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}

namespace {
NERVE_DECL(DemoWorldClear, Wait);
NERVE_DECL(DemoWorldClear, Play);
NERVE_DECL(DemoWorldClear, Fade);
NERVE_DECL(DemoWorldClear, FadeOpenWait);
NERVE_DECL(DemoWorldClear, OpenRouteDokan);
NERVE_DECL(DemoWorldClear, Ending);
NERVE_DECL(DemoWorldClear, Cancel);
NERVES_MAKE_NOSTRUCT(DemoWorldClear, Wait, Play, Fade, FadeOpenWait, OpenRouteDokan, Ending, Cancel)
}

/** @brief Creates the world-clear event. @param pMiniature Castle miniature. */
DemoWorldClear::DemoWorldClear(CourseSelectMiniature* pMiniature)
    : al::LiveActor("ワールド終了デモ"), mMiniature(pMiniature) {}

/** @brief Creates the cutscene, wipe, and skip prompt. @param rInfo Placement data. */
void DemoWorldClear::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvDemoWorldClearWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    makeActorDead();
    al::StringTmp<64> name("DemoWorldClear%02dStage", mMiniature->getWorldId());
    mDemo = rc::tryCreateDemoSceneHolder(name.cstr(), rInfo,
        rc::createPlayerRetargettingSelector(this), nullptr, false, 4);
    mDemo->setGraphicsAreaFindByPlayerPos();
    rc::setEndCameraInterpolateFrame(mDemo, 100);
    mWipe = new al::WipeSimple("白フェード", "WipeFadeWhite", al::getLayoutInitInfo(rInfo), "Demo");
    mSkipLayout = new DemoSkipLayout(al::getLayoutInitInfo(rInfo), false);
    mSkipLayout->appear();
}

/** @brief Leaves per-frame work to the nerve. */
void DemoWorldClear::control() {}

/** @brief Starts the fairy celebration and cutscene. */
void DemoWorldClear::startDemo() {
    appear();
    mMiniature->getFairy()->startWorldClearDemo();
    al::setNerve(this, &NrvDemoWorldClearPlay);
}

/** @brief Ends playback and starts the fade. */
void DemoWorldClear::endDemo() {
    rc::tryEndDemo(mDemo);
    al::setNerve(this, &NrvDemoWorldClearFade);
}

/** @brief Checks completion. @return Whether the event is waiting. */
bool DemoWorldClear::isEndDemo() const { return al::isNerve(this, &NrvDemoWorldClearWait); }
/** @brief Waits for the event to be started. */
void DemoWorldClear::exeWait() {}

/** @brief Plays the celebration and waits for its camera to finish. */
void DemoWorldClear::exePlay() {
    if (al::isFirstStep(this)) {
        rc::tryStartDemo(mDemo);
        mDemo->setCameraFovyDegree(25.0f);
        mDemo->startAction(0, false);
    }
    checkCancel();
    if (mDemo->isActionEndCamera(30))
        al::setNerve(this, &NrvDemoWorldClearFade);
}

/** @brief Responds to a skip request from an active player. */
void DemoWorldClear::checkCancel() {
    auto ports = rc::getActiveInputPortList(GameDataHolderAccessor(this));
    if (mSkipLayout->isSkip(sead::BitFlag<u16>(ports)))
        al::setNerve(this, &NrvDemoWorldClearCancel);
}

/** @brief Fades to white before the route opens. */
void DemoWorldClear::exeFade() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(30);
        al::startSe(mMiniature, "PgWipeFadeWhite", nullptr);
    } else if (mWipe->isCloseEnd()) {
        al::setNerve(this, &NrvDemoWorldClearFadeOpenWait);
    }
}

/** @brief Plays route-creation sounds while the screen is closed. */
void DemoWorldClear::exeFadeOpenWait() {
    if (al::isStep(this, 90)) al::startSe(mMiniature, "PgWorldRouteDokanCompleted", nullptr);
    if (al::isStep(this, 0)) al::startSe(mMiniature, "PgWorldRouteDokanCreated", nullptr);
    if (al::isGreaterEqualStep(this, 120))
        al::setNerve(this, &NrvDemoWorldClearOpenRouteDokan);
}

/** @brief Returns the fairy to the castle and reveals the newly opened route. */
void DemoWorldClear::exeOpenRouteDokan() {
    if (al::isFirstStep(this)) {
        al::StringTmp<64> name("FairyPrincess%02d", mMiniature->getWorldId());
        auto* fairy = mDemo->tryFindDemoSceneActor(name.cstr());
        if (fairy && al::isExistJoint(fairy, "AllRoot")) {
            const auto* mtx = al::getJointMtxPtr(fairy, "AllRoot");
            mMiniature->getFairy()->startBackToCastleDemo(mtx);
        }
        for (int i = 0; i < mDemo->getActorCount(); ++i)
            al::hideModelIfShow(mDemo->getDemoSceneActor(i));
        mWipe->startOpen(45);
        mMiniature->startOpenRouteDokan();
        mMiniature->getDirector()->openRoadImmediately(mMiniature->getNode(), mMiniature->tryFindNextNodeEnd());
    }
    checkCancel();
    if (al::isGreaterEqualStep(this, 210)) al::setNerve(this, &NrvDemoWorldClearEnding);
}

/** @brief Restores the camera, then removes the finished event. */
void DemoWorldClear::exeEnding() {
    if (al::isFirstStep(this)) rc::tryEndDemo(mDemo);
    if (al::isGreaterEqualStep(this, 100)) {
        al::setNerve(this, &NrvDemoWorldClearWait);
        kill();
    }
}

/** @brief Completes route opening and fairy placement when the event is skipped. */
void DemoWorldClear::exeCancel() {
    if (al::isFirstStep(this)) {
        mSkipLayout->kill();
        mWipe->startClose(-1);
    }
    if (mWipe->isCloseEnd()) {
        mMiniature->startOpenRouteDokan();
        mMiniature->getDirector()->openRoadImmediately(mMiniature->getNode(), mMiniature->tryFindNextNodeEnd());
        rc::tryEndDemo(mDemo);
        al::setNerve(this, &NrvDemoWorldClearWait);
        mWipe->startOpen(-1);
        mMiniature->getFairy()->forceBackToCastle();
        kill();
    }
}
