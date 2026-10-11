#pragma once
#include <container/seadPtrArray.h>

namespace al {
class ActorInitInfo;
class LiveActor;
class HitSensor;
class JointSpringController;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
}

namespace KoopaLastFunction {
void initActorKoopaLastCommon(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                              const char* pName,
                              sead::PtrArray<al::JointSpringController>* pControllers);
bool attackSensorCommon(al::HitSensor* pSelf, al::HitSensor* pOther);
void explosionCollision(al::LiveActor* pActor, const char* pName);
bool receiveMsgCommon(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver);
bool receiveMsgScreenPointCommon(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget);
bool isReceivePowBlockMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver);
}
