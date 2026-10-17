#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "MapObj/ChikaChikaBlockWatcher.hpp"
#include "Library/Bgm/ChikaChikaBgmSequencer.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
ChikaChikaBlockSynchronizer::ChikaChikaBlockSynchronizer() {
    mWatchers = new al::DeriveActorGroup<ChikaChikaBlockWatcher>("チカチカブロック監視リスト", 32);
    mSequencer = new al::ChikaChikaBgmSequencer();
}
void ChikaChikaBlockSynchronizer::initAudioKeeper(al::ActorInitInfo& rInfo) { mSequencer->init(rInfo); }
void ChikaChikaBlockSynchronizer::initAfterPlacementSceneObj(const al::ActorInitInfo&) {
    mSwitchInterval = mWatchers->getDeriveActor(0)->getSwitchInterval();
    if (mSwitchInterval < 60)
        mSequencer->setBeatCount(true);
}
void ChikaChikaBlockSynchronizer::update() {
    for (int i = 0; i < mWatchers->mNumActors; ++i) {
        if (mWatchers->getDeriveActor(i)->isStopBgm()) {
            if (!mIsBgmStopped) {
                mSequencer->stopAll();
                mIsBgmStopped = true;
            }
            return;
        }
    }
    mSequencer->update(mFrame, mMeasure);
    ++mFrame;
    if ((mSwitchInterval + 1) * 2 == mFrame) {
        mFrame = 0;
        ++mMeasure;
        mPhase = !mPhase;
        mIsFirstMeasure = false;
    }
}
void ChikaChikaBlockSynchronizer::updateSceneStop() { mSequencer->updateSceneStop(); }
void ChikaChikaBlockSynchronizer::registerWatcher(ChikaChikaBlockWatcher* pWatcher) {
    if (mWatchers->mNumActors >= 32)
        return;
    if (mWatchers->mNumActors <= 0)
        mSwitchInterval = pWatcher->getSwitchInterval();
    mWatchers->registerActor(pWatcher);
}
namespace rc {
void updateChikaChikaSynchronizer(const al::IUseSceneObjHolder* pUser) {
    al::getSceneObj<ChikaChikaBlockSynchronizer>(pUser, 29)->update();
}
void updateSceneStopChikaChikaSynchronizer(const al::IUseSceneObjHolder* pUser) {
    al::getSceneObj<ChikaChikaBlockSynchronizer>(pUser, 29)->updateSceneStop();
}
}
const char* ChikaChikaBlockSynchronizer::getSceneObjName() const { return "チカチカブロック同期監視"; }
