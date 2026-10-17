#include "MapObj/ChikaChikaBlockWatcher.hpp"
#include "MapObj/ChikaChikaBlock.hpp"
#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
namespace {
    NERVE_DECL(ChikaChikaBlockWatcher, Watch);
    NERVE_DECL(ChikaChikaBlockWatcher, DisAppear);
    NERVE_DECL(ChikaChikaBlockWatcher, AllAppear);
    NERVES_MAKE_NOSTRUCT(ChikaChikaBlockWatcher, Watch, DisAppear, AllAppear)
}
ChikaChikaBlockWatcher::ChikaChikaBlockWatcher(const char* pName) : al::LiveActor(pName) {}
ChikaChikaBlockWatcher::~ChikaChikaBlockWatcher() {}
void ChikaChikaBlockWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initNerve(this, &NrvChikaChikaBlockWatcherWatch, 0);
    bool speedMode = true;
    bool bgmMute = false;
    al::tryGetArg(&speedMode, rInfo, "SpeedMode");
    al::tryGetArg(&bgmMute, rInfo, "IsBgmMute");
    int count = al::calcLinkChildNum(rInfo, "SyncBlock");
    mBlocks = new al::DeriveActorGroup<ChikaChikaBlock>("チカチカブロックホルダー", count);
    for (int i = 0; i < count; ++i) {
        auto* block = new ChikaChikaBlock("チカチカブロック");
        al::initLinksActor(block, rInfo, "SyncBlock", i);
        block->setBgmMute(bgmMute);
        if (!speedMode)
            block->disableSpeedMode();
        mBlocks->registerActor(block);
        if (i == 0)
            mSwitchInterval = block->getSwitchTime();
    }
    if (al::isValidStageSwitch(this, "SwitchInArea") && !al::isOnStageSwitch(this, "SwitchInArea"))
        al::setNerve(this, &NrvChikaChikaBlockWatcherDisAppear);
    al::tryGetArg(&mIsSyncBgm, rInfo, "IsSyncBgm");
    if (mIsSyncBgm) {
        static_cast<ChikaChikaBlockSynchronizer*>(al::createSceneObj(this, 29))->registerWatcher(this);
        for (int i = 0; i < count; ++i)
            mBlocks->getDeriveActor(i)->onBgmSync();
    }
    makeActorAppeared();
}
void ChikaChikaBlockWatcher::exeWatch() {
    if (!mIsSyncBgm) {
        if (mSwitchInterval == mFrame)
            startSwitchOffSign();
        if ((mSwitchInterval + 1) * 2 == mFrame) {
            startSwitch();
            al::setNerve(this, &NrvChikaChikaBlockWatcherWatch);
            mFrame = 0;
            return;
        }
        ++mFrame;
    }
    if (al::isValidStageSwitch(this, "SwitchEnd") && al::isOnStageSwitch(this, "SwitchEnd"))
        al::setNerve(this, &NrvChikaChikaBlockWatcherAllAppear);
    if (al::isValidStageSwitch(this, "SwitchInArea") && !al::isOnStageSwitch(this, "SwitchInArea"))
        al::setNerve(this, &NrvChikaChikaBlockWatcherDisAppear);
}
void ChikaChikaBlockWatcher::startSwitchOffSign() {
    if (al::isNerve(this, &NrvChikaChikaBlockWatcherWatch)) {
        for (int i = 0; i < mBlocks->mNumActors; ++i)
            mBlocks->getDeriveActor(i)->startSwitchOffSign();
    }
}
void ChikaChikaBlockWatcher::startSwitch() {
    if (al::isNerve(this, &NrvChikaChikaBlockWatcherWatch)) {
        for (int i = 0; i < mBlocks->mNumActors; ++i)
            mBlocks->getDeriveActor(i)->startSwitch();
    }
}
void ChikaChikaBlockWatcher::exeAllAppear() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mBlocks->mNumActors; ++i)
            mBlocks->getDeriveActor(i)->startSwitchOn();
    }
}
void ChikaChikaBlockWatcher::exeDisAppear() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mBlocks->mNumActors; ++i)
            al::offDrawClipping(mBlocks->getDeriveActor(i));
    }
    if (al::isValidStageSwitch(this, "SwitchInArea") && al::isOnStageSwitch(this, "SwitchInArea")) {
        for (int i = 0; i < mBlocks->mNumActors; ++i)
            al::onDrawClipping(mBlocks->getDeriveActor(i));
        al::setNerve(this, &NrvChikaChikaBlockWatcherWatch);
        mFrame = mSwitchInterval;
    }
}
bool ChikaChikaBlockWatcher::isStopBgm() {
    return al::isValidStageSwitch(this, "SwitchBgmOffArea") && al::isOnStageSwitch(this, "SwitchBgmOffArea");
}
