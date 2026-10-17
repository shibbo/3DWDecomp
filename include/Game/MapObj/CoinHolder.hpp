#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
#include <container/seadTList.h>
namespace al { class IUseSceneObjHolder; }
class Coin;
class CoinHolder : public al::ISceneObj {
public:
    static CoinHolder* requestCreateSceneHolder(const al::IUseSceneObjHolder*, const al::ActorInitInfo&);
    static CoinHolder* getCoinHolder(const al::IUseSceneObjHolder*);
    CoinHolder();
    void init(const al::ActorInitInfo&);
    Coin* scoopCoin();
    void sinkCoin(Coin*);
private:
    sead::PtrArray<Coin> mCoins;
    sead::TList<Coin*> mActiveCoins;
    sead::TList<Coin*> mAvailableCoins;
};
