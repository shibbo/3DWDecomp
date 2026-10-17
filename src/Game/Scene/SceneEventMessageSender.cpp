#include "Scene/SceneEventMessageSender.hpp"

#include "Library/HitSensor/SensorMsg.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"

SceneEventMessageSender::SceneEventMessageSender()
    : al::LiveActor("シーンイベントメッセージ送信者") {}

void SceneEventMessageSender::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    initHitSensor(1);
    mSensor = al::addHitSensorMapObj(this, rInfo, "Body", 0.0f, 0, sead::Vector3f(0.0f, 0.0f, 0.0f));
    makeActorDead();
}

void SceneEventMessageSender::sendMessage(const al::SensorMsg& rMsg) {
    if (!mActorGroup) {
        return;
    }
    s32 count = mActorGroup->mNumActors;
    for (s32 i = 0; i < count; ++i) {
        al::LiveActor* actor = mActorGroup->getActor(i);
        if (al::isAlive(actor)) {
            actor->receiveMsg(&rMsg, mSensor, mSensor);
        }
    }
}

void SceneEventMessageSender::setActorGroup(al::LiveActorGroup* pGroup) {
    mActorGroup = pGroup;
}

namespace rc {
SENSOR_MSG(StartGoalDemoHouse);
SENSOR_MSG(StartDemoBossStart);

void sendSceneEventMessage(const al::IUseSceneObjHolder* pHolder, const al::SensorMsg& rMsg) {
    static_cast<SceneEventMessageSender*>(al::getSceneObj(pHolder, 31))->sendMessage(rMsg);
}

void sendSceneEventMessageStartGoalDemoHouse(const al::IUseSceneObjHolder* pHolder) {
    sendSceneEventMessage(pHolder, SensorMsgStartGoalDemoHouse());
}

void sendSceneEventMessageStartDemoBossStart(const al::IUseSceneObjHolder* pHolder) {
    sendSceneEventMessage(pHolder, SensorMsgStartDemoBossStart());
}
}  // namespace rc
