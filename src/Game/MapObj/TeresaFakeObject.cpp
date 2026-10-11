#include "MapObj/TeresaFakeObject.hpp"
#include "Library/ActorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

TeresaFakeObject::TeresaFakeObject(const char* pName) : al::LiveActor(pName) {}
TeresaFakeObject::~TeresaFakeObject() {}

void TeresaFakeObject::init(const al::ActorInitInfo& rInfo) {
    const char* objectName = nullptr;
    al::getObjectName(&objectName, rInfo);
    const char* archiveName = nullptr;
    if (al::isEqualString(objectName, "KinokoOneUpFake"))
        archiveName = "KinokoOneUp";
    else if (al::isEqualString(objectName, "MysteryBoxFake"))
        archiveName = "MysteryBox";
    else if (al::isEqualString(objectName, "GoalPoleFake"))
        archiveName = "GoalPole";
    al::initActorWithArchiveName(this, rInfo, archiveName, "Fake");
    if (al::isExistAction(this, "Wait"))
        al::startAction(this, "Wait");
    if (al::tryGetSubActor(this, "ゴールポール旗"))
        al::startAction(al::getSubActor(this, "ゴールポール旗"), "Wait");
    makeActorAppeared();
}

void TeresaFakeObject::kill() {
    al::startHitReaction(this, "テレサ消滅");
    al::startSe(this, "Fake", nullptr);
    al::LiveActor::kill();
}

bool TeresaFakeObject::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                  al::HitSensor* pSelf) {
    if (!al::isSensorPlayer(pOther))
        return false;
    if (al::isMsgItemGetAll(pMsg)) {
        kill();
        return true;
    }
    return false;
}
