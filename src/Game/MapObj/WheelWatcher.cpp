#include "MapObj/WheelWatcher.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/MapObj/FixMapParts.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
    NERVE_DECL(WheelWatcher, Wait);
    NERVES_MAKE_NOSTRUCT(WheelWatcher, Wait)
}

WheelWatcher::WheelWatcher(const char* pName) : al::LiveActor(pName) {
}

void WheelWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::onDrawClipping(this);
    mWheelCount = al::calcLinkChildNum(rInfo, "WheelPartsLink");
    mWheels = new al::FixMapParts*[mWheelCount];
    for (int i = 0; i < mWheelCount; ++i) {
        mWheels[i] = new al::FixMapParts("車輪足場");
        al::initLinksActor(mWheels[i], rInfo, "WheelPartsLink", i);
    }
    al::initNerve(this, &NrvWheelWatcherWait, 0);
    al::listenStageSwitchOnStop(this, al::Functor(this, &WheelWatcher::stop));
    makeActorAppeared();
}

void WheelWatcher::stop() {
    for (int i = 0; i < mWheelCount; ++i) {
        al::setMtsAnimFrameRate(mWheels[i], 0.0f);
        al::tryStopSe(mWheels[i], "Drive");
    }
}

void WheelWatcher::exeWait() {
}

WheelWatcher::~WheelWatcher() {
}
