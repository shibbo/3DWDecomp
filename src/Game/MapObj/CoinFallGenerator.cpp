#include "MapObj/CoinFallGenerator.hpp"
#include "MapObj/CoinBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
    inline int getActorCount(const al::LiveActorGroup* group) { return group->mNumActors; }
    NERVE_DECL(CoinFallGenerator, Standby);
    NERVE_DECL(CoinFallGenerator, Fall);
    NERVES_MAKE_NOSTRUCT(CoinFallGenerator, Standby, Fall)
}
CoinFallGenerator::CoinFallGenerator(const char* name) : al::LiveActor(name) {}
CoinFallGenerator::~CoinFallGenerator() {}
void CoinFallGenerator::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, info);
    al::initActorClipping(this, info);
    al::initGroupClipping(this, info, 256);
    al::initStageSwitch(this, info);
    al::initActorAudioKeeperWithout3D(this, info, "CoinBlowGenerator", nullptr);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvCoinFallGeneratorStandby, 0);
    al::tryGetArg(&mCoinCount, info, "CoinNum");
    al::tryGetArg(&mFallDelay, info, "FallDelay");
    bool offCollider = false;
    al::tryGetArg(&offCollider, info, "IsOffCollider");
    float width = al::getScale(this).x * 1000.0f;
    float depth = al::getScale(this).z * 1000.0f;
    int count = int(width / 100.0f) * int(depth / 100.0f);
    if (mCoinCount > count) mCoinCount = count;
    mPositions = new int[count];
    for (int i = 0; i < count; ++i) mPositions[i] = i;
    al::setClippingInfo(this, width > depth ? width : depth, nullptr);
    for (int i = count; i > 1; --i) {
        int index = al::getRandom(i);
        int value = mPositions[index];
        int last = mPositions[i - 1];
        mPositions[i - 1] = value;
        mPositions[index] = last;
    }
    mCoins = new al::DeriveActorGroup<CoinBlow>("落下コインリスト", mCoinCount);
    for (int i = 0; i < mCoinCount; ++i) {
        auto* coin = new CoinBlow("落下コイン");
        al::initCreateActorNoPlacementInfo(coin, info);
        coin->setLifeTime(mLifeTime);
        if (offCollider) coin->disableCollider();
        mCoins->registerActor(coin);
    }
    al::tryListenStageSwitchAppear(this);
    makeActorDead();
}
void CoinFallGenerator::exeStandby() {
    if (al::isGreaterEqualStep(this, mFallDelay)) al::setNerve(this, &NrvCoinFallGeneratorFall);
}
void CoinFallGenerator::exeFall() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgAppear", nullptr);
        al::invalidateClipping(this);
        float extentX = al::getScale(this).x * 1000.0f;
        float extentZ = al::getScale(this).z * 1000.0f;
        int width = int(extentX / 100.0f);
        int depth = int(extentZ / 100.0f);
        sead::Vector3f center = al::getTrans(this);
        center.x = center.x - (width / 2) * 100.0f + 50.0f;
        center.z = center.z - (depth / 2) * 100.0f + 50.0f;
        sead::Vector3f velocity = sead::Vector3f::zero;
        al::calcUpDir(&velocity, this);
        velocity *= -20.0f;
        for (int i = 0; i < mCoinCount; ++i) {
            auto* coin = mCoins->getDeriveActor(i);
            if (!al::isDead(coin)) continue;
            int index = mPositions[i];
            sead::Vector3f pos(center.x + (index % width) * 100.0f,
                               center.y,
                               center.z + (index / width) * 100.0f);
            pos.x += al::getRandom(30.0f) - 15.0f;
            pos.y += al::getRandom(300.0f) - 150.0f;
            pos.z += al::getRandom(30.0f) - 15.0f;
            al::setVelocity(coin, velocity);
            al::setTrans(coin, pos);
            coin->appear();
        }
    }
    if (mFinishedCount != mCoinCount) {
        if (al::isGreaterEqualStep(this, mLifeTime)) {
            mCoins->killAll();
        } else {
            for (int i = 0; i < getActorCount(mCoins); ++i) {
                auto* coin = mCoins->getDeriveActor(i);
                if (!al::isDead(coin)) continue;
                if (coin->isCounted()) continue;
                ++mFinishedCount;
                coin->setCounted();
            }
            return;
        }
    }
    kill();
}
