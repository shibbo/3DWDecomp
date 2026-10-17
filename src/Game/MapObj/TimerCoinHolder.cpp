#include "MapObj/TimerCoinHolder.hpp"
#include "MapObj/TimerCoin.hpp"
#include "MapObj/TimerManager.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(TimerCoinHolder, CountDown);
    NERVES_MAKE_NOSTRUCT(TimerCoinHolder, CountDown)
}
TimerCoinHolder::TimerCoinHolder(const char* pName) : al::LiveActor(pName) {}
TimerCoinHolder::~TimerCoinHolder() {}
void TimerCoinHolder::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TimerCoinWatcher", nullptr);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initNerve(this, &NrvTimerCoinHolderCountDown, 0);
    mCoinCount = al::calcLinkChildNum(rInfo, "TimerCoin");
    al::tryGetArg(&mTimerFrame, rInfo, "TimerFrame");
    mCoins = new al::DeriveActorGroup<TimerCoin>("タイマーコインリスト", mCoinCount);
    for (int i = 0; i < mCoinCount; ++i) {
        auto* coin = new TimerCoin("タイマーコイン");
        al::initLinksActor(coin, rInfo, "TimerCoin", i);
        coin->setTimerFrame(mTimerFrame);
        mCoins->registerActor(coin);
    }
    al::tryListenStageSwitchAppear(this);
    makeActorDead();
}
void TimerCoinHolder::appear() {
    al::LiveActor::appear();
    if (mIsSingleMode)
        al::invalidateClipping(this);
    mIsComplete = false;
    mCollectedCount = 0;
    al::tryOffStageSwitch(this, "SwitchAllGetOn");
    al::setNerve(this, &NrvTimerCoinHolderCountDown);
}
void TimerCoinHolder::exeCountDown() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgAppear");
        al::invalidateClipping(this);
        mCoins->appearAll();
    }
    al::HitSensor* collectSensor = nullptr;
    TimerCoin* lastCoin = nullptr;
    for (int i = 0; i < mCoins->mNumActors; ++i) {
        auto* coin = mCoins->getDeriveActor(i);
        if (al::isDead(coin) && !coin->isCounted()) {
            ++mCollectedCount;
            coin->setCounted();
            if (mCollectedCount == mCoinCount) {
                collectSensor = coin->getCollectSensor();
                al::startSe(this, mIsSingleMode ? "PgCompleteSingleMode" : "PgComplete");
                if (auto* player = PlayerKoopaJr::tryGetPlayerKoopaJr(this))
                    player->tryPraiseReaction(60);
                lastCoin = coin;
            }
        }
    }
    if (mCollectedCount == mCoinCount) {
        kill();
        if (collectSensor)
            rc::addScore(this, collectSensor, al::getTrans(lastCoin), 0);
        al::tryOnStageSwitch(this, "SwitchAllGetOn");
        mIsComplete = true;
        return;
    }
    bool isTimerActive = TimerManager::isTimerManagerActive(this);
    if (al::isGreaterEqualStep(this, mTimerFrame)) {
        mCoins->killAll();
        if (!isTimerActive) {
            const char* sound = "PgTimeUpSingleMode";
            if (!mIsSingleMode)
                sound = "PgTimeUp";
            al::startSe(this, sound);
        }
        kill();
    } else if (!isTimerActive) {
        bool isSingleMode = mIsSingleMode;
        if (al::isGreaterEqualStep(this, mTimerFrame - 180))
            al::holdSe(this, isSingleMode ? "PgTimerFastSingleMode" : "PgTimerFast");
        else
            al::holdSe(this, isSingleMode ? "PgTimerNormalSingleMode" : "PgTimerNormal");
    }
}
void TimerCoinHolder::forceReset() {
    if (al::isAlive(this)) {
        mCoins->killAll();
        kill();
    }
}
