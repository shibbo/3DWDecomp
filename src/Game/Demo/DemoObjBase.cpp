#include "Demo/DemoObjBase.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Effect/PtclSystem.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Thread/Functor.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

class PlayerRetargettingSelector;
namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
    PlayerRetargettingSelector* pSelector, const sead::Matrix34f* pMtx, bool flag, int count);
bool tryStartDemo(DemoSceneActorHolder* pDemo);
bool tryEndDemo(DemoSceneActorHolder* pDemo);
void killAllPlayersEffect(al::LiveActor* pActor);
}

namespace {
NERVE_DECL(DemoObjBase, Wait);
NERVE_DECL(DemoObjBase, Play);
NERVE_DECL(DemoObjBase, CancelDemo);
NERVES_MAKE_NOSTRUCT(DemoObjBase, Wait, Play, CancelDemo)
}

/** @brief Creates a demo actor with skipping enabled. @param pName Actor name. */
DemoObjBase::DemoObjBase(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes scene services, appearance switch, and wipe. @param rInfo Placement data. */
void DemoObjBase::initDemoPlacement(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    mEffectSystem = rInfo.getEffectSystemInfo()->getEffectSystem();
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvDemoObjBaseWait, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initStageSwitch(this, rInfo);
    makeActorAppeared();
    if (!mDemoName) {
        al::tryGetStringArg(&mDemoName, rInfo, "StageName");
        al::listenStageSwitchOnAppear(this, al::Functor(this, &DemoObjBase::startDemo));
    }
    mWipe = new al::WipeSimple("DemoWipeFadeBlack", "WipeFadeBlack", al::getLayoutInitInfo(rInfo), "Demo");
}

/** @brief Hides Bowser Jr. when required and starts the first action. */
void DemoObjBase::startAction() {
    if (mDemo->isContainKoopaJr() || mHideKoopaJr) {
        if (auto* player = PlayerKoopaJr::tryGetPlayerKoopaJr(this))
            player->hideForDemo();
    }
    mDemo->startAction(0, false);
}

/** @brief Provides an empty per-frame extension point. */
void DemoObjBase::updateDemo() {}
/** @brief Enters the idle state. */
void DemoObjBase::setWait() { al::setNerve(this, &NrvDemoObjBaseWait); }
/** @brief Checks camera completion. @return Whether the configured completion window was reached. */
bool DemoObjBase::checkDemoEnd() const { return mDemo->isActionEndCamera(mEndFrameWindow); }
/** @brief Tests the cancellation wipe. @return Whether the wipe is alive. */
bool DemoObjBase::isCancelWipeActive() const { return mWipe && mWipe->isAlive(); }
/** @brief Assigns the cutscene resource. @param pName Resource name. */
void DemoObjBase::setDemoName(const char* pName) { mDemoName = pName; }
/** @brief Tests startup completion. @return Whether the demo holder has fully started. */
bool DemoObjBase::isFullyStarted() const { return mDemo->isFullyStarted(); }
/** @brief Base implementation ignores music requests. @param pName Music name. @param frame Request frame. @param pKeeper Audio keeper. */
void DemoObjBase::setBgmRequest(const char* pName, unsigned int frame, al::AudioKeeper* pKeeper) {}

/** @brief Creates the cutscene holder and skip layout. @param rInfo Actor initialization data. */
void DemoObjBase::init(const al::ActorInitInfo& rInfo) {
    initDemoPlacement(rInfo);
    mDemo = rc::createDemoSceneHolder(mDemoName, rInfo, rc::createPlayerRetargettingSelector(this), mPlacementBaseMtx, false, 4);
    mSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), al::isSingleMode(rInfo));
    mSkipLayout->appear();
}

/** @brief Activates the hidden skip prompt and playback state. */
void DemoObjBase::startDemo() {
    mSkipLayout->appear();
    mSkipLayout->startHidden();
    al::setNerve(this, &NrvDemoObjBasePlay);
}

/** @brief Ends playback and invokes the completion callback. @param setWaitState Whether to return to idle. */
void DemoObjBase::endDemo(bool setWaitState) {
    if (!mDisableCapture) al::requestCaptureScreenCover(this, 3);
    rc::tryEndDemo(mDemo);
    if (mDemo->isContainKoopaJr()) {
        if (auto* player = PlayerKoopaJr::tryGetPlayerKoopaJr(this)) player->showFromDemo();
    }
    if (setWaitState) al::setNerve(this, &NrvDemoObjBaseWait);
    mSkipLayout->kill();
    callEndDemoHook();
}

/** @brief Invokes the optional completion callback. */
void DemoObjBase::callEndDemoHook() { if (mEndHook) (*mEndHook)(); }
/** @brief Checks whether playback is idle. @return Whether the demo has ended. */
bool DemoObjBase::isEndDemo() const { return al::isNerve(this, &NrvDemoObjBaseWait); }
/** @brief Overrides the holder's placement. @param pMtx Replacement matrix. */
void DemoObjBase::overrideBaseMtx(const sead::Matrix34f* pMtx) { mDemo->overrideBaseMtx(pMtx); }
/** @brief Forces the idle state. */
void DemoObjBase::forceWait() { al::setNerve(this, &NrvDemoObjBaseWait); }
/** @brief Configures the ending camera transition. @param frames Interpolation duration. */
void DemoObjBase::setCameraInterpolateFrame(int frames) { mDemo->setEndCameraInterpolateFrame(frames); }
/** @brief Gets camera duration. @return Maximum camera frame. */
int DemoObjBase::getMaxFrame() { return mDemo->getMaxCameraFrame(); }
/** @brief Waits for an external start request. */
void DemoObjBase::exeWait() {}
/** @brief Frame display is disabled in this build. */
void DemoObjBase::displayFrameCount() {}
/** @brief Frame display is disabled in this build. @param frame Current frame. @param maxFrame Last frame. */
void DemoObjBase::sharedDisplayFrameCount(int frame, int maxFrame) {}

/** @brief Runs cutscene updates, frame hooks, and skip/completion handling. */
void DemoObjBase::exePlay() {
    if (al::isFirstStep(this)) {
        if (mKillAllEffects && mEffectSystem)
            mEffectSystem->getPtclSystem()->KillAllEmitterSet();
        else if (mDemo->getPlayerCount() > 0)
            rc::killAllPlayersEffect(this);
        if (mHideActor) {
            al::hideModelIfShow(mHideActor);
            al::hideShadow(mHideActor);
            al::tryKillEmitterAndParticleAll(mHideActor);
        }
        rc::tryStartDemo(mDemo);
        startAction();
    }
    updateDemo();
    mDemo->update();
    callFrameHooks(mDemo->getCurFrame());
    if (mAllowSkip && mSkipLayout->isSkip(sead::BitFlag<u16>(1 << al::getMainControllerPort())) && !isEndDemo())
        al::setNerve(this, &NrvDemoObjBaseCancelDemo);
    else if (checkDemoEnd())
        endDemo(true);
}

/** @brief Invokes callbacks registered for the current frame. @param frame Current cutscene frame. */
void DemoObjBase::callFrameHooks(unsigned int frame) {
    for (unsigned long i = 0; i < mFrameHookCount; ++i)
        if (mFrameHooks[i].frame == frame) (*mFrameHooks[i].callback)();
}

/** @brief Fades out a cancelled demo and reopens the wipe after cleanup. */
void DemoObjBase::exeCancelDemo() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(-1);
        if (mCancelHook) (*mCancelHook)();
    }
    if (al::isStep(this, 2)) mDemo->tryCancelAudio(30, mCancelAudioFlag);
    if (mWipe->isCloseEnd()) {
        if (!isEndDemo()) endDemo(true);
        mWipe->startOpen(-1);
    }
}

/** @brief Disables character-ID matching in the demo holder. */
void DemoObjBase::setForceIgnoreCharId() { mDemo->setForceIgnoreCharId(); }
/** @brief Registers a frame callback. @param rCallback Callback to clone. @param frame Trigger frame. @return Registered callback entry. */
DemoObjBase::FrameHook* DemoObjBase::registerFrameHook(const al::FunctorBase& rCallback, unsigned int frame) {
    FrameHook* hook = &mFrameHooks[mFrameHookCount++];
    hook->callback = rCallback.clone();
    hook->frame = frame;
    return hook;
}
/** @brief Registers a completion callback. @param rCallback Callback to clone. */
void DemoObjBase::registerEndDemoHook(const al::FunctorBase& rCallback) { mEndHook = rCallback.clone(); }
/** @brief Registers a cancellation callback. @param rCallback Callback to clone. */
void DemoObjBase::registerCancelDemoHook(const al::FunctorBase& rCallback) { mCancelHook = rCallback.clone(); }
