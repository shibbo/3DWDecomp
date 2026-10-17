#include "MapObj/TeresaFakeDokan.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Util/PlayerUtil.hpp"

TeresaFakeDokan::TeresaFakeDokan(const char* pName) : al::LiveActor(pName) {}

TeresaFakeDokan::~TeresaFakeDokan() {}

void TeresaFakeDokan::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Dokan", "Fake");
    makeActorAppeared();
}

void TeresaFakeDokan::kill() {
    al::startHitReaction(this, "テレサ消滅");
    al::LiveActor::kill();
}

bool TeresaFakeDokan::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor*) {
    if (!al::isSensorPlayer(pSender))
        return false;
    if (!al::isMsgFloorTouch(pMsg))
        return false;
    if (!rc::getPlayerKeyConfig(pSender)->isPadHoldPlayerSquat())
        return false;
    kill();
    return true;
}
