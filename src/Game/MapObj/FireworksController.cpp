#include "MapObj/FireworksController.hpp"
#include "MapObj/FireworksEffectObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"

namespace {
    NERVE_DECL(FireworksController, Loop);
    NERVE_DECL(FireworksController, Wait);
    NERVES_MAKE_NOSTRUCT(FireworksController, Loop, Wait)
}

FireworksController::FireworksController(const char* pName) : al::LiveActor(pName) {}
FireworksController::~FireworksController() {}

void FireworksController::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::tryGetArg(&mLoopFrames, rInfo, "LoopFrame");
    al::tryGetArg(&mStartDelay, rInfo, "CommonDelayFrame");
    int count = al::calcLinkChildNum(rInfo, "Fireworks");
    mCount = count;
    mEffects = new FireworksEffectObj*[count];
    mLaunchFrames = new int[count];
    for (int i = 0; i < mCount; i++) {
        mEffects[i] = new FireworksEffectObj("花火");
        al::initLinksActor(mEffects[i], rInfo, "Fireworks", i);
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, al::getPlacementInfo(rInfo), "Fireworks", i);
        al::tryGetArg(&mLaunchFrames[i], placement, "DelayFrame");
        al::invalidateClipping(mEffects[i]);
    }
    al::trySyncStageSwitchAppear(this);
    if (mStartDelay < 1)
        al::initNerve(this, &NrvFireworksControllerLoop, 0);
    else
        al::initNerve(this, &NrvFireworksControllerWait, 0);
}

void FireworksController::exeWait() {
    mFrame++;
    if (mFrame >= mStartDelay) {
        mFrame = 0;
        al::setNerve(this, &NrvFireworksControllerLoop);
    }
}

void FireworksController::exeLoop() {
    for (int i = 0; i < mCount; i++)
        if (mLaunchFrames[i] == mFrame)
            mEffects[i]->makeActorAppeared();
    mFrame++;
    if (mFrame >= mLoopFrames)
        mFrame -= mLoopFrames;
}

void FireworksController::kill() {
    for (int i = 0; i < mCount; i++)
        mEffects[i]->kill();
    al::LiveActor::kill();
}
