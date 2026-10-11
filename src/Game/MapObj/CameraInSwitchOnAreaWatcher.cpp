#include "MapObj/CameraInSwitchOnAreaWatcher.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"

CameraInSwitchOnAreaWatcher::CameraInSwitchOnAreaWatcher(const char* pName)
    : al::LiveActor(pName) {}

CameraInSwitchOnAreaWatcher::~CameraInSwitchOnAreaWatcher() {}

void CameraInSwitchOnAreaWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    mAreas = al::createLinkAreaGroup(this, rInfo, "CameraInSwitchOnArea",
                                   "カメラINでスイッチONエリア", "子供エリア");
    if (!mAreas) {
        makeActorDead();
        return;
    }
    if (al::isValidSwitchDeadOn(this))
        makeActorAppeared();
    else
        makeActorDead();
}

void CameraInSwitchOnAreaWatcher::control() {
    if (al::tryIsInAreaObj(mAreas, al::getCameraLookAt(this)))
        kill();
}

void CameraInSwitchOnAreaWatcher::kill() {
    al::LiveActor::kill();
    al::onSwitchDeadOn(this);
}
