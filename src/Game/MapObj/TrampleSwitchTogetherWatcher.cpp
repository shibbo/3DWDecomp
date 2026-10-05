#include "MapObj/TrampleSwitchTogetherWatcher.hpp"
#include "MapObj/TrampleSwitchTogether.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
    NERVE_DECL(TrampleSwitchTogetherWatcher, Watch);
    NERVE_DECL(TrampleSwitchTogetherWatcher, End);
    NERVES_MAKE_NOSTRUCT(TrampleSwitchTogetherWatcher, Watch, End)
}

TrampleSwitchTogetherWatcher::TrampleSwitchTogetherWatcher(const char* pName) : al::LiveActor(pName) {
}

void TrampleSwitchTogetherWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvTrampleSwitchTogetherWatcherWatch, 0);
    al::initActorAudioKeeperWithout3D(this, rInfo, "TrampleSwitchTogetherWatcher", nullptr);
    al::initStageSwitch(this, rInfo);
    int count = al::calcLinkChildNum(rInfo, "TrampleSwitchTogetherList");
    if (count <= 0) {
        al::setNerve(this, &NrvTrampleSwitchTogetherWatcherEnd);
        makeActorDead();
    } else {
        mSwitches = new al::DeriveActorGroup<TrampleSwitchTogether>("協力踏みスイッチリスト", count);
        for (int i = 0; i < count; ++i) {
            auto* sw = new TrampleSwitchTogether("協力踏みスイッチ");
            al::initLinksActor(sw, rInfo, "TrampleSwitchTogetherList", i);
            mSwitches->registerActor(sw);
        }
        makeActorAppeared();
    }
}

void TrampleSwitchTogetherWatcher::exeWatch() {
    int count = mSwitches->mNumActors;
    bool allOn = true;
    for (int i = 0; i < count; ++i) {
        bool isOn = mSwitches->getDeriveActor(i)->isOnWait();
        allOn &= isOn;
        if (!isOn) {
            break;
        }
    }
    if (allOn) {
        for (int i = 0; i < count; ++i) {
            mSwitches->getDeriveActor(i)->success();
        }
        al::onStageSwitch(this, "SwitchTrampleAllOn");
        al::startSeByName(this, "プログラム", nullptr);
        al::setNerve(this, &NrvTrampleSwitchTogetherWatcherEnd);
        makeActorDead();
    }
}

void TrampleSwitchTogetherWatcher::exeEnd() {
}

TrampleSwitchTogetherWatcher::~TrampleSwitchTogetherWatcher() {
}
