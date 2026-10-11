#include "MapObj/RouteDokanEntrance.hpp"
#include "MapObj/IUseRouteDokan.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(RouteDokanEntrance, Wait);
    NERVE_DECL(RouteDokanEntrance, Out);
    NERVES_MAKE_NOSTRUCT(RouteDokanEntrance, Wait, Out)
}
RouteDokanEntrance::RouteDokanEntrance(const char* pName,
                                                                 const char* pModelName, const char* pModelSuffix)
    : al::BlockRailParts(pName), mModelName(pModelName), mEntranceModelSuffix(pModelSuffix) {}
RouteDokanEntrance::~RouteDokanEntrance() {}
void RouteDokanEntrance::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, mModelName, mEntranceModelSuffix);
    al::updatePoseQuat(this, mInitialRotation);
    al::resetPosition(this, mInitialPosition, false);
    initRailLink();
    al::initNerve(this, &NrvRouteDokanEntranceWait, 0);
    makeActorAppeared();
}
void RouteDokanEntrance::setInitQT(const sead::Quatf& rRotation,
                                               const sead::Vector3f& rPosition) {
    mInitialRotation = rRotation;
    mInitialPosition = rPosition;
}
void RouteDokanEntrance::setHost(IUseRouteDokan* pHost) { mHost = pHost; }
void RouteDokanEntrance::startRouteDokanRider(al::BlockRailRider* pRider) {
    pRider->setRailPart(getLink(0), 0.0f);
    pRider->reverse();
}
bool RouteDokanEntrance::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                                 al::HitSensor* pReceiver) {
    if (rc::isMsgRouteDokanPlayerTouch(pMsg)) {
        if (al::isNerve(this, &NrvRouteDokanEntranceWait) ||
            (al::isNerve(this, &NrvRouteDokanEntranceOut) && al::isGreaterEqualStep(this, 5))) {
            al::setNerve(this, &NrvRouteDokanEntranceOut);
            al::invalidateClipping(this);
        }
        return true;
    }
    if (!mHost)
        return false;
    if (al::isMsgBindGiant(pMsg))
        return true;
    if (mHost->isEnableActorRouteDokanMove()) {
        if (al::BlockRailRider* rider = rc::tryGetMsgParamBlockRailRider(pMsg)) {
            startRouteDokanRider(rider);
            return true;
        }
    }
    if (al::isMsgBindStart(pMsg)) {
        if (al::isSensorPlayer(pSender) && static_cast<PlayerActor*>(al::getSensorHost(pSender))->isRaidonExist())
            return false;
        return mHost->isBindStart(pSender);
    }
    if (al::isMsgBindInit(pMsg))
        return mHost->startBind(this, pMsg, pSender, pReceiver);
    if (al::isMsgBindCancel(pMsg))
        return mHost->cancelBind(pSender);
    if (al::isMsgBindDamage(pMsg))
        return mHost->damagePuppet(pSender);
    return false;
}
void RouteDokanEntrance::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryStartAction(this, "Wait");
    }
}
void RouteDokanEntrance::exeOut() {
    if ((al::isFirstStep(this) && !mDisableOutAction && !al::tryStartAction(this, "Out")) || al::isActionEnd(this))
        al::setNerve(this, &NrvRouteDokanEntranceWait);
}
