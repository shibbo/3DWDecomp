#include "MapObj/BgmRegionChanger.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Bgm/BgmUtil.hpp"

namespace {
    NERVE_DECL(BgmRegionChanger, Wait);
    NERVES_MAKE_NOSTRUCT(BgmRegionChanger, Wait)
};  // namespace

BgmRegionChanger::BgmRegionChanger(const char* pName) : al::LiveActor(pName) {
}

void BgmRegionChanger::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);

    if (!al::tryGetStringArg(&mBgmSituationName, rInfo, "BgmSituationName")) {
        mBgmSituationName = nullptr;
    }

    al::listenStageSwitchOnKill(this, al::Functor(this, &BgmRegionChanger::changeRegion));
    al::initNerve(this, &NrvBgmRegionChangerWait, 0);
    makeActorAppeared();
}

void BgmRegionChanger::changeRegion() {
    if (mBgmSituationName != nullptr) {
        al::changeBgmSituation(this, mBgmSituationName);
    }
}

void BgmRegionChanger::exeWait() {
    if (al::isOnStageSwitch(this, "SwitchKill")) {
        kill();
    }
}

BgmRegionChanger::~BgmRegionChanger() {
}
