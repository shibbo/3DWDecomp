#include "MapObj/CoinHolder.hpp"
#include "MapObj/Coin.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
CoinHolder* CoinHolder::requestCreateSceneHolder(const al::IUseSceneObjHolder* pUser, const al::ActorInitInfo& rInfo) {
    CoinHolder* holder;
    if (auto* existing = static_cast<CoinHolder*>(al::tryGetSceneObj(pUser, 0))) {
        holder = existing;
    } else {
        holder = new CoinHolder();
        al::setSceneObj(pUser, holder, 0);
        holder->init(rInfo);
    }
    return holder;
}

void CoinHolder::init(const al::ActorInitInfo& rInfo) {
    mCoins.allocBuffer(70, nullptr);
    auto* nodes = new sead::TListNode<Coin*>[70];
    for (int i = 0; i < 70; ++i) {
        auto* coin = new Coin("コイン");
        al::initCreateActorWithPlacementInfo(coin, rInfo);
        coin->makeActorDead();
        mCoins.pushBack(coin);
        nodes[i].mData = coin;
        mAvailableCoins.pushBack(&nodes[i]);
    }
}
CoinHolder* CoinHolder::getCoinHolder(const al::IUseSceneObjHolder* pUser) {
    return static_cast<CoinHolder*>(al::tryGetSceneObj(pUser, 0));
}
CoinHolder::CoinHolder() {}
Coin* CoinHolder::scoopCoin() {
    auto* node = mAvailableCoins.popFront();
    if (node) {
        node->mList = nullptr;
        mActiveCoins.pushBack(node);
        return node->mData;
    }
    return nullptr;
}
void CoinHolder::sinkCoin(Coin* pCoin) {
    auto* found = [&]() -> sead::TListNode<Coin*>* {
        for (auto it = mActiveCoins.begin(); it != mActiveCoins.end(); ++it) {
            if (*it == pCoin) return it.getNode();
        }
        return nullptr;
    }();
    if (found) mAvailableCoins.pushBack(found);
}
