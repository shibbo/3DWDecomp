#include "MapObj/TransparentWall.hpp"
#include "Library/ActorUtil.hpp"

TransparentWall::TransparentWall(const char* pName) : al::LiveActor(pName) {
}

void TransparentWall::init(const al::ActorInitInfo& rInfo) {
    if (al::isObjectName(rInfo, "TransparentWallPlayerOnly")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWall", "PlayerOnly");
    } else if (al::isObjectName(rInfo, "TransparentWallPlayerOnlyClipped")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWall", "PlayerOnlyClipped");
    } else if (al::isObjectName(rInfo, "TransparentWallClipped")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWall", "Clipped");
    } else if (al::isObjectName(rInfo, "TransparentWallRaidonOnly")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWall", "RaidonOnly");
    } else if (al::isObjectName(rInfo, "TransparentWallRaidonOnlySkate")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWallRaidonOnlySkate", nullptr);
    } else if (al::isObjectName(rInfo, "TransparentSphereRaidonOnlySkate")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentSphereRaidonOnlySkate", nullptr);
    } else if (al::isObjectName(rInfo, "TransparentCubeRaidonOnlySkate")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentCubeRaidonOnlySkate", nullptr);
    } else if (al::isObjectName(rInfo, "TransparentWallBowserLaser")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWall", "BowserLaser");
    } else if (al::isObjectName(rInfo, "TransparentWallOceanFloor")) {
        al::initActorWithArchiveName(this, rInfo, "TransparentWallOceanFloor", nullptr);
    } else {
        al::initActor(this, rInfo);
    }
    if (al::isSingleMode(rInfo)) {
        al::validateClipping(this);
    }
    al::trySyncStageSwitchAppearAndKill(this);
}

bool TransparentWall::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return al::isMsgScreenPointInvalidCollisionParts(pMsg);
}

TransparentWall::~TransparentWall() {
}
