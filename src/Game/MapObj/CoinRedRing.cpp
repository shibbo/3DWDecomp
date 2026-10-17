#include "MapObj/CoinRedRing.hpp"
#include "MapObj/CoinRed.hpp"
#include "Layout/CollectNumber.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Util/ItemUtil.hpp"
namespace {
NERVE_DECL(CoinRedRing, Wait);
NERVE_DECL(CoinRedRing, CountDown);
NERVES_MAKE_NOSTRUCT(CoinRedRing, Wait, CountDown)
}
CoinRedRing::CoinRedRing(const char* name) : al::LiveActor(name) {}
CoinRedRing::~CoinRedRing() {}
void CoinRedRing::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvCoinRedRingWait, 0);
    int coinCount = al::calcLinkChildNum(info, "CoinRed");
    mCoins = new al::DeriveActorGroup<CoinRed>("赤コインリスト", 8);
    for (int i = 0; i < coinCount; ++i) {
        CoinRed* coin = new CoinRed("赤コイン", this);
        al::initLinksActor(coin, info, "CoinRed", i);
        mCoins->registerActor(coin);
    }
    const char* itemName = nullptr;
    if (al::tryGetStringArg(&itemName, info, "ItemType")) mItemType = rc::getItemType(info);
    if (mItemType == 4) {
        initItemKeeper(1);
        al::addItem(this, info, "1UPキノコ[強制取得]", false);
    } else {
        rc::initItemForRingItem(this, info);
    }
    mNumbers.allocBuffer(8, nullptr);
    for (int i = 0; i < mNumbers.capacity(); ++i)
        mNumbers.pushBack(new CollectNumber(al::getLayoutInitInfo(info), "PopRedCoinNumber", "赤コイン枚数表示"));
    float shadowLength = -1.0f;
    if (al::tryGetArg(&shadowLength, info, "ShadowLength") && shadowLength > 0.0f)
        al::setShadowDropLength(this, shadowLength, "シャドウマスク");
    bool expandClipping = false;
    if (al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength") && expandClipping)
        al::tryExpandClippingByShadowLength(this, &mClippingOffset);
    makeActorAppeared();
}
bool CoinRedRing::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgItemGetAll(msg)) {
        if (al::isSensorHitRingShape(sender, receiver, 40.0f)) {
            mCollector = sender;
            al::setNerve(this, &NrvCoinRedRingCountDown);
            return true;
        }
    }
    return false;
}
void CoinRedRing::appearItemCoinRed() {
    CoinRed* lastCoin = mCoins->getDeriveActor(0);
    al::HitSensor* collector = mCollector;
    for (int i = 1; i < mCoins->mNumActors; ++i) {
        CoinRed* coin = mCoins->getDeriveActor(i);
        if (lastCoin->getCollectedFrame() < coin->getCollectedFrame()) {
            if (coin->getCollector()) collector = coin->getCollector();
            lastCoin = coin;
        }
    }
    if (mItemType == 4) {
        al::appearItem(this, al::getTrans(lastCoin), sead::Vector3f(0.0f, 0.0f, 1.0f), collector);
    } else {
        rc::appearItemForRingItem(this, al::getTrans(lastCoin), lastCoin->getItemDirection());
    }
}
void CoinRedRing::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
void CoinRedRing::exeCountDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
        al::startSe(this, "Start", nullptr);
        al::hideModelIfShow(this);
        al::invalidateClipping(this);
        al::invalidateHitSensors(this);
        mCoins->appearAll();
    }
    for (int i = 0; i < mCoins->mNumActors; ++i) {
        CoinRed* coin = mCoins->getDeriveActor(i);
        CollectNumber* number = mNumbers.at(i);
        if (al::isDead(coin) && !number->hasAppeared()) {
            ++mCollectedCount;
            if (mCollectedCount == 8) {
                number->appearComplete(al::getTrans(coin), mCollectedCount);
                al::startSe(this, "Complete", nullptr);
                PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
                if (koopaJr) koopaJr->tryPraiseReaction(60);
            } else {
                number->appearNormal(al::getTrans(coin), mCollectedCount);
            }
        }
    }
    if (mCoins->calcAliveActorNum() == 0) {
        appearItemCoinRed();
        kill();
    } else if (al::isGreaterEqualStep(this, CoinRed::getTimerFrame())) {
        mCoins->killAll();
        al::startSe(this, "TimeUp", nullptr);
        kill();
    } else if (al::isGreaterEqualStep(this, CoinRed::getTimerFrame() - 180)) {
        al::holdSe(this, "TimerFast", nullptr);
    } else {
        al::holdSe(this, "TimerNormal", nullptr);
    }
}
int CoinRedRing::getSeParamNum() { return mCollectedCount; }
int CoinRedRing::getMaxCoinNum() { return 8; }
