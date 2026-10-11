#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
#include <math/seadQuat.h>
#include <prim/seadBitFlag.h>
namespace al { class MtxConnector; template <class T> class DeriveActorGroup; }
class GreenCoin;
class GreenStar;
class CollectNumber;
class GreenRing : public al::LiveActor {
public:
    GreenRing(const char*);
    ~GreenRing() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appearItem();
    void exeWait();
    void exeCountDown();
    int getSeParamNum();
private:
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    al::DeriveActorGroup<GreenCoin>* mCoins = nullptr;
    sead::BitFlag32 mCollectedCoins;
    sead::PtrArray<CollectNumber> mNumbers;
    GreenStar* mStar = nullptr;
    int mCollectedCount = 0;
};
static_assert(sizeof(GreenRing) == 0x190);
