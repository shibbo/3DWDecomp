#include "MapObj/CandlestandWatcher.hpp"
#include "MapObj/Candlestand.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(CandlestandWatcher, Watch);
    NERVES_MAKE_NOSTRUCT(CandlestandWatcher, Watch)
    bool isAllLit(al::DeriveActorGroup<Candlestand>* pGroup) {
        for (int i = 0; i < pGroup->mNumActors; ++i)
            if (pGroup->getDeriveActor(i)->isLightOff()) return false;
        return true;
    }
}
CandlestandWatcher::CandlestandWatcher(const char* pName) : al::LiveActor(pName) {}
CandlestandWatcher::~CandlestandWatcher() {}
void CandlestandWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initNerve(this, &NrvCandlestandWatcherWatch, 0);
    int count = al::calcLinkChildNum(rInfo, "LinkCandlestand");
    mCandlestands = new al::DeriveActorGroup<Candlestand>("燭台ホルダー", count);
    for (int i = 0; i < count; ++i) {
        Candlestand* actor = new Candlestand("燭台", this);
        al::initLinksActor(actor, rInfo, "LinkCandlestand", i);
        mCandlestands->registerActor(actor);
    }
    makeActorAppeared();
}
void CandlestandWatcher::exeWatch() {
    if (!isAllLit(mCandlestands)) return;
    al::tryOnStageSwitch(this, "SwitchFireAllOn");
    kill();
}
bool CandlestandWatcher::isEnableAddScore() {
    if (al::isDead(this)) return false;
    return isAllLit(mCandlestands);
}
