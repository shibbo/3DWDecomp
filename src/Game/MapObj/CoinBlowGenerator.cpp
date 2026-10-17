#include "MapObj/CoinBlowGenerator.hpp"
#include "MapObj/CoinBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/AreaObjUtil.hpp"
namespace {
    NERVE_DECL(CoinBlowGenerator, Blow);
    NERVES_MAKE_NOSTRUCT(CoinBlowGenerator, Blow)
}
CoinBlowGenerator::CoinBlowGenerator(const char* name) : al::LiveActor(name) {}
CoinBlowGenerator::~CoinBlowGenerator() {}
void CoinBlowGenerator::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, info);
    al::initActorClipping(this, info);
    al::initGroupClipping(this, info, 128);
    al::initStageSwitch(this, info);
    al::initActorAudioKeeperWithout3D(this, info, "CoinBlowGenerator", nullptr);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvCoinBlowGeneratorBlow, 0);
    al::tryGetArg(&mCoinCount, info, "CoinNum");
    al::tryGetArg(&mBlowSpeed, info, "CoinBlowSpeed");
    mCoins = new al::DeriveActorGroup<CoinBlow>("吹き出しコインリスト", mCoinCount);
    for (int i = 0; i < mCoinCount; ++i) {
        auto* coin = new CoinBlow("吹き出しコイン");
        al::initCreateActorWithPlacementInfo(coin, info);
        coin->setLifeTime(mLifeTime);
        mCoins->registerActor(coin);
    }
    al::tryListenStageSwitchAppear(this);
    makeActorDead();
}
void CoinBlowGenerator::initWithParam(const InitParam& param, const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, info);
    al::initActorClipping(this, info);
    al::initGroupClipping(this, info, 128);
    al::initActorAudioKeeperWithout3D(this, info, "CoinBlowGenerator", nullptr);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvCoinBlowGeneratorBlow, 0);
    mCoinCount = param.coinCount;
    mBlowSpeed = param.blowSpeed;
    mRandomSpeed = param.randomSpeed;
    mRadialSpeed = param.radialSpeed;
    mCoins = new al::DeriveActorGroup<CoinBlow>("吹き出しコインリスト", mCoinCount);
    for (int i = 0; i < mCoinCount; ++i) {
        auto* coin = new CoinBlow("吹き出しコイン");
        al::initCreateActorNoPlacementInfo(coin, info);
        coin->setLifeTime(mLifeTime);
        mCoins->registerActor(coin);
    }
    makeActorDead();
}
void CoinBlowGenerator::exeBlow() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startSe(this, "PgAppear", nullptr);
        bool water = rc::isInWaterArea(this);
        sead::Vector3f up, front;
        al::calcUpDir(&up, this);
        al::calcFrontDir(&front, this);
        up *= water ? 25.0f : mBlowSpeed;
        for (int i = 0; i < mCoinCount; ++i) {
            CoinBlow* coin = mCoins->getDeriveActor(i);
            if (!al::isDead(coin)) continue;
            sead::Vector3f velocity;
            if (mIsConcentric) {
                al::rotateVectorDegreeY(&front, 360.0f / mCoinCount);
                velocity.set(mRadialSpeed * front.x + up.x, mRadialSpeed * front.y + up.y, mRadialSpeed * front.z + up.z);
                coin->setOffCollide(30);
                coin->setHideModel(10);
            } else {
                al::getRandomVector(&velocity, 1.0f);
                velocity = (water ? 6.0f : mRandomSpeed) * velocity + up;
            }
            al::setVelocity(coin, velocity);
            al::setTrans(coin, al::getTrans(this));
            coin->appear();
        }
    }
    if (al::isGreaterEqualStep(this, mLifeTime)) {
        mCoins->killAll();
        kill();
    }
}
