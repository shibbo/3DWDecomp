#include "MapObj/FortressGoal.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"

FortressGoal::FortressGoal(const char* pName) : al::LiveActor(pName) {}

FortressGoal::~FortressGoal() {}

void FortressGoal::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    int worldId = GameDataFunction::calcPlayingWorldId(GameDataHolderAccessor(this));
    if (worldId < 1)
        worldId = 1;
    al::startAction(this, "Color");
    al::setMtpAnimFrameAndStop(this, worldId);
    makeActorAppeared();
}

void FortressGoal::appear() {
    al::LiveActor::appear();
    al::tryStartAction(this, "Appear");
}

bool FortressGoal::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
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
