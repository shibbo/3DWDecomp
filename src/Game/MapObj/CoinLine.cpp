#include "MapObj/CoinLine.hpp"
#include "MapObj/Coin.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Rail/Rail.hpp"
#include "Library/Rail/RailKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
namespace {
int maxCoinCount(al::DeriveActorGroup<Coin>** groups, int count) {
    int maxCount = -1;
    for (int i = 0; i < count; ++i)
        if (maxCount < groups[i]->mNumActors) maxCount = groups[i]->mNumActors;
    return maxCount;
}
void createCoins(al::DeriveActorGroup<Coin>* group, const al::ActorInitInfo& info, bool singleMode) {
    if (singleMode) {
        for (int i = 0; i < group->mMaxActors; ++i) {
            Coin* coin = new Coin("ライン配置コイン用コイン");
            al::initCreateActorWithPlacementInfo(coin, info);
            group->registerActor(coin);
        }
    } else {
        for (int i = 0; i < group->mMaxActors; ++i) {
            Coin* coin = new Coin("ライン配置コイン用コイン");
            al::initCreateActorNoPlacementInfo(coin, info);
            group->registerActor(coin);
        }
    }
}
int countAtPoints(al::Rail* rail, float spacing) {
    int count = 1;
    for (int j = 0; j < rail->getRailPartCount(); ++j)
        count += sead::Mathf::floor(rail->getPartLength(j) / spacing) + 1;
    return count;
}
void placeAtPoints(al::DeriveActorGroup<Coin>* group, al::Rail* rail, float spacing) {
    rail->calcPos(al::getTransPtr(group->getDeriveActor(0)), 0.0f);
    int coinIndex = 1;
    for (int i = 0; i < rail->getRailPartCount(); ++i) {
        int count = sead::Mathf::floor(rail->getPartLength(i) / spacing);
        sead::Vector3f start(0.0f, 0.0f, 0.0f);
        sead::Vector3f end(0.0f, 0.0f, 0.0f);
        rail->calcRailPointPos(&start, i);
        rail->calcRailPointPos(&end, i + 1);
        for (int j = 0; j <= count; ++j) {
            float rate = al::normalize(float(j + 1), 0.0f, float(count + 1));
            al::lerpVec(al::getTransPtr(group->getDeriveActor(coinIndex++)), start, end, rate);
        }
    }
}
}
CoinLine::CoinLine(const char* name) : al::LiveActor(name) {}
CoinLine::~CoinLine() {}
void CoinLine::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    al::initActorAudioKeeperWithout3D(this, info, "CoinLine", nullptr);
    al::tryGetArg(&mInvalidPopUp, info, "IsInvalidPopUp");
    mRails = al::tryCreateRailKeeperGroup(al::getPlacementInfo(info), "Rail");
    int coinCount = 0;
    float spacing = -1.0f;
    bool placementPoint = false;
    al::tryGetArg(&spacing, info, "PlacementOffset");
    al::tryGetArg(&coinCount, info, "CoinNum");
    al::tryGetArg(&placementPoint, info, "IsPlacementPoint");
    if (al::tryGetArg(&mDelayPerCoin, info, "DelayPerCoin") && mDelayPerCoin < 0) mDelayPerCoin = 7;
    mGroups = new al::DeriveActorGroup<Coin>*[getCoinGroupNum()];
    for (int i = 0; i < getCoinGroupNum(); ++i) {
        al::Rail* rail = mRails->getRailKeeper(i)->getRail();
        int count;
        if (placementPoint) {
            count = rail->getRailPartCount() > 0 ? countAtPoints(rail, spacing) : 1;
        } else if (spacing > 0.0f) count = int(rail->getTotalLength() / spacing) + 1;
        else count = coinCount;
        mGroups[i] = new al::DeriveActorGroup<Coin>("ライン配置コイン用コイングループ", count);
        bool singleMode = al::isSingleMode(info);
        createCoins(mGroups[i], info, singleMode);
        if (placementPoint) placeAtPoints(mGroups[i], rail, spacing);
        else {
            for (int j = 0; j < count; ++j) {
                Coin* coin = mGroups[i]->getDeriveActor(j);
                float rate = al::normalize(float(j), 0.0f, float(mGroups[i]->mMaxActors - 1));
                rail->calcPos(al::getTransPtr(coin), al::lerpValue(rate, 0.0f, rail->getTotalLength()));
            }
        }
    }
    for (int i = 0; i < getCoinGroupNum(); ++i) {
        for (int j = 0; j < mGroups[i]->mNumActors; ++j) {
            al::getTransPtr(mGroups[i]->getDeriveActor(j))->y += 70.0f;
            mGroups[i]->getDeriveActor(j)->makeActorDead();
        }
    }
    al::tryGetArg(&mDelay, info, "DelayStep");
    al::trySyncStageSwitchAppear(this);
    makeActorDead();
}
int CoinLine::getCoinGroupNum() const { return mRails->getRailKeeperNum(); }
void CoinLine::appear() {
    al::LiveActor::appear();
    if (al::isSingleMode(this)) {
        for (int i = 0; i < getCoinGroupNum(); ++i) {
            for (int j = 0; j < mGroups[i]->mNumActors; ++j) {
                Coin* coin = mGroups[i]->getDeriveActor(j);
                if (al::isAlive(coin)) coin->makeActorDead();
            }
        }
    }
    mFrame = 0;
}
void CoinLine::control() {
    if (mDelay - 1 >= 0) { --mDelay; return; }
    if (mDelayPerCoin * (maxCoinCount(mGroups, getCoinGroupNum()) - 1) >= mFrame) {
        if (mFrame % mDelayPerCoin == 0) {
            int index = mFrame == 0 ? 0 : mFrame / mDelayPerCoin;
            for (int i = 0; i < getCoinGroupNum(); ++i) {
                if (mGroups[i]->mNumActors <= index) continue;
                Coin* coin = mGroups[i]->getDeriveActor(index);
                if (mInvalidPopUp) coin->appear();
                else coin->appearJump();
                al::startSe(coin, "PgTransparentAppear", nullptr);
            }
        }
        ++mFrame;
        return;
    }
    {
        for (int i = 0; i < getCoinGroupNum(); ++i) {
            for (int j = 0; j < mGroups[i]->mNumActors; ++j)
                if (al::isAlive(mGroups[i]->getDeriveActor(j))) return;
        }
        al::startSe(this, "Complete", nullptr);
        kill();
        if (auto* player = PlayerKoopaJr::tryGetPlayerKoopaJr(this)) player->tryPraiseReaction(60);
        return;
    }
}
