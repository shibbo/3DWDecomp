#include "MapObj/KillerTankPartsNeedle.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

KillerTankPartsNeedle::KillerTankPartsNeedle(const char* pName) : al::LiveActor(pName) {}

KillerTankPartsNeedle::~KillerTankPartsNeedle() {}

void KillerTankPartsNeedle::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
}

void KillerTankPartsNeedle::appear() {
    al::LiveActor::appear();
    al::tryStartAction(this, "Appear");
}

bool KillerTankPartsNeedle::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    return al::isMsgAskSafetyPoint(pMsg);
}
