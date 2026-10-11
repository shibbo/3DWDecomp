#include "MapObj/Fury/CoinStackMoving.hpp"
#include "MapObj/CoinStackBase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(CoinStackMoving, Wait);
    NERVES_MAKE_NOSTRUCT(CoinStackMoving, Wait)
}
CoinStackMoving::CoinStackMoving(const char* pName) : al::LiveActor(pName) {}
CoinStackMoving::~CoinStackMoving() {}
void CoinStackMoving::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 2);
    al::tryGetArg(&mCoinCount, rInfo, "CoinStackNum");
    if (mCoinCount <= 0)
        mCoinCount = 1;
    mConnector = al::createMtxConnector(this);
    al::initSubActorKeeperNoFile(this, rInfo, mCoinCount);
    mCoins = new CoinStackBase*[mCoinCount];
    const sead::Vector3f& basePosition = al::getTrans(this);
    for (int i = 0; i < mCoinCount; ++i) {
        mCoins[i] = new CoinStackBase("積みコイン単体", true);
        mCoins[i]->init(rInfo);
        sead::Vector3f position = basePosition + sead::Vector3f(0.0f, mCoins[i]->getStackHeight() * i, 0.0f);
        mCoins[i]->initPosition(position);
        al::registerSubActorSyncClipping(this, mCoins[i], false);
    }
    float radius = mCoins[0]->getStackHeight() * mCoinCount * 0.5f;
    mClippingCenter = basePosition + sead::Vector3f(0.0f, radius, 0.0f);
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::initNerve(this, &NrvCoinStackMovingWait, 0);
    makeActorAppeared();
}
void CoinStackMoving::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mConnector, this, false);
    for (int i = 0; i < mCoinCount; ++i)
        mCoins[i]->initConnector(mConnector);
}
void CoinStackMoving::control() {
    al::connectPoseQT(this, mConnector);
    const sead::Vector3f& position = al::getTrans(this);
    float radius = mCoins[0]->getStackHeight() * mCoinCount * 0.5f;
    mClippingCenter = position + sead::Vector3f(0.0f, radius, 0.0f);
    al::setClippingInfo(this, radius, &mClippingCenter);
}
void CoinStackMoving::exeWait() {
    if (mCollectedCount == mCoinCount) {
        kill();
        return;
    }
    for (int i = 0; i < mCoinCount; ++i) {
        CoinStackBase* coin = mCoins[i];
        if (coin->isCollected()) {
            coin->clearCollected();
            coin->kill();
            ++mCollectedCount;
            if (i == mCoinCount - 1)
                return;
            int fallIndex = 0;
            for (int j = i + 1; j < mCoinCount; ++j) {
                if (!al::isDead(mCoins[j])) {
                    mCoins[j]->requestFall(fallIndex);
                    ++fallIndex;
                }
            }
        }
    }
}

void CoinStackMoving::updateLinkedTrans(const sead::Vector3f& rPosition) {
    al::setTrans(this, rPosition);
    for (int i = 0; i < mCoinCount; ++i) {
        sead::Vector3f position = rPosition + sead::Vector3f(0.0f, mCoins[i]->getStackHeight() * i, 0.0f);
        mCoins[i]->updateLinkedTrans(position);
    }
}
void CoinStackMoving::appear() {
    for (int i = 0; i < mCoinCount; ++i)
        mCoins[i]->appear();
    al::LiveActor::appear();
}
void CoinStackMoving::kill() {
    for (int i = 0; i < mCoinCount; ++i)
        mCoins[i]->kill();
    al::LiveActor::kill();
}
void CoinStackMoving::makeActorDead() {
    for (int i = 0; i < mCoinCount; ++i)
        mCoins[i]->makeActorDead();
    al::LiveActor::makeActorDead();
}
