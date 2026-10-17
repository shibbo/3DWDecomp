#pragma once
#include <basis/seadTypes.h>
namespace al { class LiveActor; class SensorMsg; class HitSensor; class JointAimInfo; class JointSpringControllerHolder; }
namespace SyumockUtil {
al::JointAimInfo* createSyumockEyeJointInfo(al::LiveActor* pActor);
al::JointSpringControllerHolder* initJointSyumockJointSpringController(al::LiveActor* pActor);
bool isMsgSyumockDeath(const al::SensorMsg* pMsg);
bool isMsgSyumockDeathSingleMode(const al::SensorMsg* pMsg);
bool isMsgSyumockReaction(const al::SensorMsg* pMsg, s32 reactionStep);
bool isMsgSyumockReactionSingleMode(const al::SensorMsg* pMsg, s32 reactionStep);
bool isMsgSyumockPush(const al::SensorMsg* pMsg);
void doKill(al::LiveActor* pActor, const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
}
