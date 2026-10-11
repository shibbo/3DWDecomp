#include "MapObj/TestKitaharaRouteBazookaEntrance.hpp"
#include "MapObj/TestKitaharaRouteBazooka.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(TestKitaharaRouteBazookaEntrance, Wait);
    NERVE_DECL(TestKitaharaRouteBazookaEntrance, Out);
    NERVES_MAKE_NOSTRUCT(TestKitaharaRouteBazookaEntrance, Wait, Out)
}
TestKitaharaRouteBazookaEntrance::TestKitaharaRouteBazookaEntrance(const char* pName,
                                                                 const char* pModelName)
    : al::BlockRailParts(pName), mModelName(pModelName) {}
TestKitaharaRouteBazookaEntrance::~TestKitaharaRouteBazookaEntrance() {}
void TestKitaharaRouteBazookaEntrance::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, mModelName ? mModelName : "RouteDokanTerminate", nullptr);
    al::updatePoseQuat(this, mInitialRotation);
    al::resetPosition(this, mInitialPosition, false);
    initRailLink();
    al::initNerve(this, &NrvTestKitaharaRouteBazookaEntranceWait, 0);
    makeActorAppeared();
}
void TestKitaharaRouteBazookaEntrance::setInitQT(const sead::Quatf& rRotation,
                                               const sead::Vector3f& rPosition) {
    mInitialRotation = rRotation;
    mInitialPosition = rPosition;
}
void TestKitaharaRouteBazookaEntrance::setHost(TestKitaharaRouteBazooka* pHost) { mHost = pHost; }
void TestKitaharaRouteBazookaEntrance::startRouteDokanRider(al::BlockRailRider* pRider) {
    pRider->setRailPart(getLink(0), 0.0f);
    pRider->reverse();
}
bool TestKitaharaRouteBazookaEntrance::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                                 al::HitSensor* pReceiver) {
    if (rc::isMsgRouteDokanPlayerTouch(pMsg)) {
        if (al::isNerve(this, &NrvTestKitaharaRouteBazookaEntranceWait) ||
            (al::isNerve(this, &NrvTestKitaharaRouteBazookaEntranceOut) && al::isGreaterEqualStep(this, 5))) {
            al::setNerve(this, &NrvTestKitaharaRouteBazookaEntranceOut);
            al::invalidateClipping(this);
        }
        return true;
    }
    if (!mHost)
        return false;
    if (al::BlockRailRider* rider = rc::tryGetMsgParamBlockRailRider(pMsg)) {
        startRouteDokanRider(rider);
        return true;
    }
    if (al::isMsgBindStart(pMsg))
        return mHost->isBindStart(pSender, pReceiver);
    if (al::isMsgBindInit(pMsg))
        return mHost->startBind(this, pSender, pReceiver);
    if (al::isMsgBindCancel(pMsg))
        return mHost->cancelBind(pSender);
    if (al::isMsgBindDamage(pMsg))
        return mHost->damagePuppet(pSender);
    return false;
}
void TestKitaharaRouteBazookaEntrance::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryStartAction(this, "Wait");
    }
}
void TestKitaharaRouteBazookaEntrance::exeOut() {
    if ((al::isFirstStep(this) && !al::tryStartAction(this, "Out")) || al::isActionEnd(this))
        al::setNerve(this, &NrvTestKitaharaRouteBazookaEntranceWait);
}
