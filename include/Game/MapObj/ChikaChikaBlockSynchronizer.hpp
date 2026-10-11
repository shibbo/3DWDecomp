#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
namespace al { class ChikaChikaBgmSequencer; class IUseSceneObjHolder; }
class ChikaChikaBlockWatcher;
class ChikaChikaBlockSynchronizer : public al::ISceneObj {
public:
    ChikaChikaBlockSynchronizer();
    void initAudioKeeper(al::ActorInitInfo&);
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    void update();
    int getFrame() const { return mFrame; }
    int getPhase() const { return mPhase; }
    bool isFirstMeasure() const { return mIsFirstMeasure; }
    void updateSceneStop();
    void registerWatcher(ChikaChikaBlockWatcher*);
    const char* getSceneObjName() const override;
private:
    al::DeriveActorGroup<ChikaChikaBlockWatcher>* mWatchers = nullptr;
    int mFrame = 0;
    int mSwitchInterval = 120;
    int mPhase = 0;
    int mMeasure = 0;
    bool mIsFirstMeasure = true;
    al::ChikaChikaBgmSequencer* mSequencer = nullptr;
    bool mIsBgmStopped = false;
};
namespace rc {
void updateChikaChikaSynchronizer(const al::IUseSceneObjHolder*);
void updateSceneStopChikaChikaSynchronizer(const al::IUseSceneObjHolder*);
}
