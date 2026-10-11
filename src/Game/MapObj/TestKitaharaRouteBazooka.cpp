#include "MapObj/TestKitaharaRouteBazooka.hpp"
#include "MapObj/TestKitaharaRouteBazookaEntrance.hpp"
#include "MapObj/TestKitaharaRouteBazookaEntranceGroup.hpp"
#include "MapObj/TestKitaharaRouteBazookaPartsGroup.hpp"
#include "MapObj/TestKitaharaRouteBazookaRider.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include <cfloat>

TestKitaharaRouteBazooka::TestKitaharaRouteBazooka(const char* pName) : al::LiveActor(pName) {}
TestKitaharaRouteBazooka::~TestKitaharaRouteBazooka() {}
void TestKitaharaRouteBazooka::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "RouteDokan", nullptr);
    mParts = new TestKitaharaRouteBazookaPartsGroup();
    mParts->init(rInfo);
    mEntrances = new TestKitaharaRouteBazookaEntranceGroup(nullptr);
    mEntrances->init(mParts, rInfo);
    mEntrances->setHost(this);
    float moveSpeed = 20.0f;
    al::tryGetArg(&moveSpeed, rInfo, "MoveSpeed");
    if (moveSpeed < 1.0f)
        moveSpeed = 1.0f;
    float outSpeed = 30.0f;
    al::tryGetArg(&outSpeed, rInfo, "OutSpeed");
    bool triggerShoot = false;
    al::tryGetArg(&triggerShoot, rInfo, "TriggerShoot");
    mRiderCount = 8;
    mRiders = new TestKitaharaRouteBazookaRider*[mRiderCount];
    for (int i = 0; i < mRiderCount; ++i) {
        mRiders[i] = new TestKitaharaRouteBazookaRider("ルート土管ライダー", 1);
        mRiders[i]->setMoveSpeed(moveSpeed);
        mRiders[i]->setOutSpeed(outSpeed);
        mRiders[i]->setTriggerShoot(triggerShoot);
        al::initCreateActorNoPlacementInfo(mRiders[i], rInfo);
    }
    int entranceCount = mEntrances->getEntranceCount();
    float closestDistance = FLT_MAX;
    for (int i = 0; i < entranceCount; ++i) {
        auto* entrance = mEntrances->getEntrance(i);
        float distance = (al::getTrans(this) - al::getTrans(entrance)).length();
        if (distance < closestDistance) {
            closestDistance = distance;
            mStartEntrance = entrance;
        }
    }
    for (int i = 0; i < entranceCount; ++i) {
        auto* entrance = mEntrances->getEntrance(i);
        if (mStartEntrance == entrance)
            continue;
        auto* exit = new al::LiveActor("発射口");
        al::initActorWithArchiveName(exit, rInfo, "TestKitaharaRouteBazookaTerminate", nullptr);
        al::setTrans(exit, al::getTrans(entrance));
        al::setQuat(exit, al::getQuat(entrance));
        exit->makeActorAppeared();
    }
    if (al::listenStageSwitchOnAppear(this, al::Functor(this, &TestKitaharaRouteBazooka::active)))
        deactive();
    makeActorDead();
}
void TestKitaharaRouteBazooka::active() {
    mParts->active();
    mEntrances->active();
}
void TestKitaharaRouteBazooka::deactive() {
    mParts->deactive();
    mEntrances->deactive();
}
bool TestKitaharaRouteBazooka::isBindStart(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::getSensorHost(pReceiver) != mStartEntrance)
        return false;
    int playerIndex = alPlayerFunction::findPlayerHolderIndex(pSender);
    for (int i = 0; i < mRiderCount; ++i) {
        if (mRiders[i]->isActive(playerIndex))
            return false;
    }
    return true;
}
bool TestKitaharaRouteBazooka::startBind(TestKitaharaRouteBazookaEntrance* pEntrance,
                                       al::HitSensor* pSender, al::HitSensor* pReceiver) {
    for (int i = 0; i < mRiderCount; ++i) {
        if (al::isDead(mRiders[i])) {
            mRiders[i]->startBind(pEntrance, pSender, pReceiver);
            al::sendMsgHoldCancel(pSender, pReceiver);
            return true;
        }
    }
    return false;
}
bool TestKitaharaRouteBazooka::cancelBind(al::HitSensor* pSender) {
    for (int i = 0; i < mRiderCount; ++i)
        mRiders[i]->tryCancelBind(pSender);
    return true;
}
bool TestKitaharaRouteBazooka::damagePuppet(al::HitSensor* pSender) {
    bool damaged = false;
    for (int i = 0; i < mRiderCount; ++i) {
        if (!al::isDead(mRiders[i]))
            damaged |= mRiders[i]->damage(pSender);
    }
    return damaged;
}
