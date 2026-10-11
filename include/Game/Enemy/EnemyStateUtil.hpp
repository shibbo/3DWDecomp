#pragma once

namespace al {
class SensorMsg;
class HitSensor;
class LiveActor;
class Nerve;
}
class EnemyStateBlowDown;

namespace EnemyStateUtil {
bool isMsgPressDownForCrossoverSensor(const al::SensorMsg* pMsg, const al::HitSensor* pOther, const al::HitSensor* pSelf);
bool tryRequestPressDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, bool isSetItemFactor);
bool tryRequestPressDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, al::LiveActor* pHost, const al::Nerve* pNextNerve, bool isSetItemFactor);
bool isMsgBlowDown(const al::SensorMsg* pMsg);
bool isMsgBlowDownForSpike(const al::SensorMsg* pMsg);
bool isMsgBlowDownForGhost(const al::SensorMsg* pMsg);
bool tryRequestBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor);
void requestBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor);
bool tryRequestAttackFireBlowDown(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, bool isSetItemFactor);
bool tryRequestBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, const al::Nerve* pNextNerve, bool isSetItemFactor);
bool tryRequestAttackFireBlowDownAndNextNerve(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf, EnemyStateBlowDown* pState, const al::Nerve* pNextNerve, bool isSetItemFactor);
bool isMsgRouteDokanAttack(const al::SensorMsg* pMsg);
bool isKillByAreaOrMaterialCode(const al::LiveActor* pActor);
bool tryKillByAreaOrMaterialCode(al::LiveActor* pActor);
bool tryKillByAreaOrMaterialCodeWithHitReaction(al::LiveActor* pActor);
}
