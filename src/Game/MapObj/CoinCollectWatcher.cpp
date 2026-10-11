#include "MapObj/CoinCollectWatcher.hpp"
#include "MapObj/CoinRed.hpp"
#include "Layout/CollectNumber.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
    NERVE_DECL(CoinCollectWatcher, Watch);
    NERVES_MAKE_NOSTRUCT(CoinCollectWatcher, Watch)
}
CoinCollectWatcher::CoinCollectWatcher(const char* pName) : al::LiveActor(pName) {}
CoinCollectWatcher::~CoinCollectWatcher() {}
void CoinCollectWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvCoinCollectWatcherWatch, 0);
    int count = al::calcLinkChildNum(rInfo, "CoinRed");
    mCoins = new al::DeriveActorGroup<CoinRed>("WatchedCoins", 8);
    for (int i = 0; i < count; ++i) {
        auto* coin = new CoinRed("RedCoin", this);
        al::initLinksActor(coin, rInfo, "CoinRed", i);
        mCoins->registerActor(coin);
        coin->appear();
        coin->disableCountdown();
    }
    mNumbers.allocBuffer(8, nullptr);
    for (int i = 0; i < mNumbers.capacity(); ++i)
        mNumbers.pushBack(new CollectNumber(al::getLayoutInitInfo(rInfo), "PopRedCoinNumber", "赤コイン枚数表示"));
    makeActorAppeared();
}
int CoinCollectWatcher::getSeParamNum() { return mCollectedCount; }
void CoinCollectWatcher::exeWatch() {
    for (int i = 0; i < mCoins->mNumActors; ++i) {
        auto* coin = mCoins->getDeriveActor(i);
        auto* number = mNumbers[i];
        if (al::isDead(coin) && !number->hasAppeared()) {
            if (++mCollectedCount == 8) {
                number->appearComplete(al::getTrans(coin), mCollectedCount);
                al::startSe(this, "Complete");
                if (auto* player = PlayerKoopaJr::tryGetPlayerKoopaJr(this))
                    player->tryPraiseReaction(60);
            } else {
                number->appearNormal(al::getTrans(coin), mCollectedCount);
            }
        }
        if (mCoins->calcAliveActorNum() == 0) {
            al::tryOnStageSwitch(this, "SwitchCoinsCollectedOn");
            kill();
            return;
        }
    }
}
int CoinCollectWatcher::getMaxCoinNum() { return 8; }
