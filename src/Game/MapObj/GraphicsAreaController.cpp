#include "MapObj/GraphicsAreaController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
namespace {
    NERVE_DECL(GraphicsAreaController, Wait);
    NERVES_MAKE_NOSTRUCT(GraphicsAreaController, Wait)
    void listenSwitches(GraphicsAreaController* actor) {
    al::listenStageSwitchOn(actor, "SwitchAppear", al::Functor(actor, &GraphicsAreaController::triggerFadeTo));
    al::listenStageSwitchOn(actor, "SwitchKill", al::Functor(actor, &GraphicsAreaController::triggerFadeFrom));
    }
    al::AreaObj* switchArea(al::AreaObj* current, al::AreaObj* next) {
        if (!next || current == next) return current;
        if (current->mPriority == 97) {
            current->mPriority = -1;
            next->mPriority = 97;
        } else next->mPriority = -1;
        return next;
    }
}
GraphicsAreaController::GraphicsAreaController(const char* name) : al::LiveActor(name) {}
void GraphicsAreaController::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "GraphicsAreaController", nullptr);
    al::initNerve(this, &NrvGraphicsAreaControllerWait, 1);
    makeActorAppeared();
    al::hideModelIfShow(this);
    al::invalidateClipping(this);
    int frames;
    {
        const char* graphics = nullptr;
        const char* depthOfField = nullptr;
        al::tryGetStringArg(&graphics, info, "GraphicsAreaObjIdTo");
        al::tryGetStringArg(&depthOfField, info, "DepthOfFieldAreaObjIdTo");
        al::tryGetArg(&frames, info, "AnimationFrames");
        mGraphicsAreaName = graphics;
        mDepthOfFieldAreaName = graphics;
    }
    listenSwitches(this);
}
void GraphicsAreaController::finishInit(const al::ActorInitInfo& info) {
    al::tryGetStringArg(&mNormalDisasterName, info, "DisasterGraphicsArea");
    mGraphicsAreaName = mNormalDisasterName;
    al::tryGetStringArg(&mDepthOfFieldAreaName, info, "DisasterDepthOfFieldArea");
    const char* name;
    int phase = SingleModeDataFunction::getUnlockedPhase(this);
    al::tryGetStringArg(&mHardDisasterName, info, "HardDisasterGraphicsArea");
    al::tryGetStringArg(&mSuperHardDisasterName, info, "SuperHardDisasterGraphicsArea");
    if (rc::isPlessieChase(phase)) {
        if (al::tryGetStringArg(&name, info, "PlessieChaseGraphicsArea")) {
            mGraphicsAreaName = name;
            mUnusedName = name;
            mSuperHardDisasterName = name;
            mHardDisasterName = name;
            mNormalDisasterName = name;
        }
    }
}
void GraphicsAreaController::initAfterPlacement() {
    if (mGraphicsAreaName) mCurrentArea = mNormalDisasterArea = al::tryFindAreaObjByName(this, "GraphicsArea", mGraphicsAreaName);
    if (mHardDisasterName) mHardDisasterArea = al::tryFindAreaObjByName(this, "GraphicsArea", mHardDisasterName);
    if (mSuperHardDisasterName) mSuperHardDisasterArea = al::tryFindAreaObjByName(this, "GraphicsArea", mSuperHardDisasterName);
    if (mDepthOfFieldAreaName) mDepthOfFieldArea = al::tryFindAreaObjByName(this, "DepthOfFieldArea", mDepthOfFieldAreaName);
}
void GraphicsAreaController::switchToNormalDisaster() { mCurrentArea = switchArea(mCurrentArea, mNormalDisasterArea); }
void GraphicsAreaController::switchToHardDisaster() { mCurrentArea = switchArea(mCurrentArea, mHardDisasterArea); }
void GraphicsAreaController::switchToSuperHardDisaster() { mCurrentArea = switchArea(mCurrentArea, mSuperHardDisasterArea); }
void GraphicsAreaController::appear() { al::LiveActor::appear(); al::setNerve(this, &NrvGraphicsAreaControllerWait); }
void GraphicsAreaController::control() {}
void GraphicsAreaController::triggerFadeTo() {
    if (mCurrentArea) mCurrentArea->mPriority = 97;
    if (mDepthOfFieldArea) mDepthOfFieldArea->mPriority = 97;
}
void GraphicsAreaController::triggerFadeFrom() {
    if (mCurrentArea) mCurrentArea->mPriority = -1;
    if (mDepthOfFieldArea) mDepthOfFieldArea->mPriority = -1;
}
void GraphicsAreaController::triggerFadeOverFramesTo(int frames) { setLerpStep(frames); triggerFadeTo(); }
void GraphicsAreaController::setLerpStep(int frames) { getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpStep(frames); }
void GraphicsAreaController::triggerFadeOverFramesFrom(int frames) { setLerpStep(frames); triggerFadeFrom(); }
void GraphicsAreaController::pauseFade() { getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpPaused(true); }
void GraphicsAreaController::resumeFade() { getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpPaused(false); }
void GraphicsAreaController::setLerp(float rate) { getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpRate(rate); }
bool GraphicsAreaController::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }
bool GraphicsAreaController::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) { return al::isMsgTouchAssist(msg); }
void GraphicsAreaController::exeWait() {}
void GraphicsAreaController::exeFadeOut() {}
void GraphicsAreaController::exeFadeIn() {}
