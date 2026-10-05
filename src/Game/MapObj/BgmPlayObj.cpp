#include "MapObj/BgmPlayObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Bgm/BgmUtil.hpp"

namespace {
    NERVE_DECL(BgmPlayObj, Wait);
    NERVE_DECL(BgmPlayObj, Playing);
    NERVES_MAKE_NOSTRUCT(BgmPlayObj, Wait, Playing)
};  // namespace

BgmPlayObj::BgmPlayObj(const char* pName) : al::LiveActor(pName) {
}

// The start switch arms playback; the delay is counted by the Playing nerve.
void BgmPlayObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRMSV(this);
    al::initActorSRT(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    al::initActorClipping(this, rInfo);

    if (!al::tryGetStringArg(&mBgmPlayName, rInfo, "BgmPlayName")) {
        mBgmPlayName = "Stage";
    }
    if (!al::tryGetArg(&mStartDelayFrameNum, rInfo, "StartDelayFrameNum")) {
        mStartDelayFrameNum = 0;
    }
    if (!al::tryGetArg(&mFadeInFrameNum, rInfo, "FadeInFrameNum")) {
        mFadeInFrameNum = 0;
    }
    if (!al::tryGetArg(&mCurBgmFadeOutFrameNum, rInfo, "CurBgmFadeOutFrameNum")) {
        mCurBgmFadeOutFrameNum = 0;
    }

    al::invalidateClipping(this);
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &BgmPlayObj::start))) {
        al::initNerve(this, &NrvBgmPlayObjWait, 0);
    }
    makeActorAppeared();
}

void BgmPlayObj::start() {
    al::setNerve(this, &NrvBgmPlayObjPlaying);
}

void BgmPlayObj::exeWait() {
}

void BgmPlayObj::exePlaying() {
    if (al::isGreaterEqualStep(this, mStartDelayFrameNum)) {
        al::startBgm(this, al::BgmPlayingRequest(mBgmPlayName, mFadeInFrameNum, 0,
                                               mCurBgmFadeOutFrameNum));
        kill();
    }
}

BgmPlayObj::~BgmPlayObj() {
}
