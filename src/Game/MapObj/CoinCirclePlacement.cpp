#include "MapObj/CoinCirclePlacement.hpp"
#include "MapObj/Coin.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
namespace {
    NERVE_DECL(CoinCirclePlacement, Watch);
    NERVES_MAKE_NOSTRUCT(CoinCirclePlacement, Watch)
}
CoinCirclePlacement::CoinCirclePlacement(const char* name) : al::LiveActor(name) {}
void CoinCirclePlacement::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, info);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvCoinCirclePlacementWatch, 0);
    if (mActorSceneInfo->isSingleMode) {
        al::initActorClipping(this, info);
        al::validateClipping(this);
    }
    float sideScale = al::getScale(this).x;
    float frontScale = al::getScale(this).z;
    mSideRadius = sideScale * 200.0f;
    mFrontRadius = frontScale * 200.0f;
    al::tryGetSide(&mSide, info);
    al::tryGetUp(&mUp, info);
    al::tryGetFront(&mFront, info);
    al::tryGetArg(&mCoinCount, info, "CoinNum");
    int count = mCoinCount;
    if (count <= 0) {
        makeActorDead();
        return;
    }
    mCoins = new Coin*[count];
    float angleStep = 360.0f / count;
    for (int i = 0; i < mCoinCount; ++i) {
        mCoins[i] = new Coin("コイン");
        al::initCreateActorWithPlacementInfo(mCoins[i], info);
        float angle = sead::Mathf::deg2rad(angleStep * i);
        float x = mSideRadius * sead::Mathf::cos(angle);
        float z = mFrontRadius * sead::Mathf::sin(angle);
        sead::Vector3f pos = al::getTrans(this) + x * mSide + 70.0f * mUp + z * mFront;
        al::setTrans(mCoins[i], pos);
        al::setScale(mCoins[i], 1.0f, 1.0f, 1.0f);
        al::updatePoseRotate(mCoins[i], sead::Vector3f(0.0f, 0.0f, 0.0f));
        al::resetPosition(mCoins[i], false);
        mCoins[i]->resetBaseQuat();
    }
    al::tryGetArg(&mSpeed, info, "Speed");
    makeActorAppeared();
}
void CoinCirclePlacement::exeWatch() {
    mAngle = al::wrapAngle(mAngle + mSpeed);
    float angleStep = 360.0f / mCoinCount;
    bool allDead = true;
    for (int i = 0; i < mCoinCount; ++i) {
        if (al::isDead(mCoins[i])) continue;
        if (!al::isNearZero(mSpeed)) {
            float angle = sead::Mathf::deg2rad(al::wrapAngle(mAngle + angleStep * i));
            float x = -mSideRadius * sead::Mathf::cos(angle);
            float z = mFrontRadius * sead::Mathf::sin(angle);
            sead::Vector3f pos = al::getTrans(this) + mSide * x + 70.0f * mUp + z * mFront;
            al::setTrans(mCoins[i], pos);
        }
        allDead = false;
    }
    if (allDead) kill();
}
