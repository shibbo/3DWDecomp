#include "MapObj/HexScrollAppearParts.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

HexScrollAppearParts::HexScrollAppearParts(const char* pName) : al::LiveActor(pName) {}

HexScrollAppearParts::~HexScrollAppearParts() {}

void HexScrollAppearParts::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
}

void HexScrollAppearParts::appear() {
    al::LiveActor::appear();
    al::startHitReactionAppear(this);
}

bool HexScrollAppearParts::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgAskSafetyPoint(pMsg))
        return true;
    if (al::isMsgShowModel(pMsg)) {
        al::showModelIfHide(this);
        return true;
    }
    if (al::isMsgHideModel(pMsg)) {
        al::hideModelIfShow(this);
        return true;
    }
    return false;
}

void HexScrollAppearParts::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}
