#pragma once
#include <math/seadQuat.h>
#include <math/seadMatrix.h>
namespace al { class LiveActor; class HitSensor; }

class BrosMoveStepKeeper;

namespace rc {
bool isCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper);
bool isCurrentStepPoseFixed(const BrosMoveStepKeeper* pMoveStepKeeper);
const sead::Quatf& getCurrentKeyQuat(const BrosMoveStepKeeper* pMoveStepKeeper);
bool isTurnToDefaultDirectionCurrent(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper);
bool isFaceToDefaultDirectionCurrent(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper);
void turnToTargetOrDefaultDirectionCurrent(al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper, f32 degree);
void tryRequestSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper);
void tryRequestEndSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper);
void turnToTargetOrDefaultDirectionNext(al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper, f32 degree);
bool isFaceToDefaultDirectionNext(const al::LiveActor* pActor, const BrosMoveStepKeeper* pMoveStepKeeper);
const sead::Quatf& getNextKeyQuat(const BrosMoveStepKeeper* pMoveStepKeeper);
const sead::Vector3f& getCurrentKeyTrans(const BrosMoveStepKeeper* pMoveStepKeeper);
const sead::Vector3f& getNextKeyTrans(const BrosMoveStepKeeper* pMoveStepKeeper);
void updateWeaponPose(al::LiveActor* pActor, const sead::Matrix34f* pParentMtx, const sead::Vector3f& rTrans, const sead::Vector3f& rRotate);
void killStepKuriboAll(const BrosMoveStepKeeper* pMoveStepKeeper, al::HitSensor* pSender);
s32 getCurrentStepKuriboNum(const BrosMoveStepKeeper* pMoveStepKeeper);
}  // namespace rc
