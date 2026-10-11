#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class LiveActorGroup;
}

class SceneEventMessageSender : public al::LiveActor, public al::ISceneObj {
public:
    SceneEventMessageSender();
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    void sendMessage(const al::SensorMsg& rMsg);
    void setActorGroup(al::LiveActorGroup* pGroup);

private:
    al::LiveActorGroup* mActorGroup = nullptr;
    al::HitSensor* mSensor = nullptr;
};

static_assert(sizeof(SceneEventMessageSender) == 0x160);

namespace rc {
void sendSceneEventMessage(const al::IUseSceneObjHolder* pHolder, const al::SensorMsg& rMsg);
void sendSceneEventMessageStartGoalDemoHouse(const al::IUseSceneObjHolder* pHolder);
void sendSceneEventMessageStartDemoBossStart(const al::IUseSceneObjHolder* pHolder);
}
