#include "MapObj/StrongHipDropReactPoint.hpp"
#include "Library/ActorUtil.hpp"

StrongHipDropReactPoint::StrongHipDropReactPoint(const char* pName) : al::LiveActor(pName) {}

void StrongHipDropReactPoint::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    initHitSensor(1);
    al::addHitSensor(this, rInfo, "body", 12, 100.0f, 8, sead::Vector3f(0.0f, 0.0f, 0.0f));
    al::initStageSwitch(this, rInfo);
    makeActorAppeared();
}

bool StrongHipDropReactPoint::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                       al::HitSensor* pSelf) {
    if (al::isMsgPlayerDisregard(pMsg))
        return true;
    if (al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg)) {
        al::tryOnStageSwitch(this, "SwitchHipDropOn");
        return true;
    }
    return false;
}

StrongHipDropReactPoint::~StrongHipDropReactPoint() {}
