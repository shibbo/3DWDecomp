#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}  // namespace al

class NpcStateParam;

namespace NpcStateFunction {
bool isInNPCAvoidArea(const al::LiveActor* pActor, const sead::Vector3f& rPos);
bool isInNPCAvoidArea(const al::LiveActor* pActor);
void calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam);
void calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam, bool isOnGround);
bool isFallNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPos,
                    const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity, f32 distance,
                    f32 rise, f32 drop, bool isCheckWall);
bool isFallNextMove(const al::LiveActor* pActor, f32 distance, f32 rise, f32 drop,
                    bool isCheckWall);
bool isWallNextMove(const al::LiveActor* pActor, f32 distance, f32 height);
bool isWallNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPos,
                    const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity, f32 distance,
                    f32 height);
bool isNPCAvoidAreaNextMove(const al::LiveActor* pActor, f32 distance, f32 rise, f32 drop,
                            bool isCheckWall);
bool isNPCAvoidAreaNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPos,
                            const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity,
                            f32 distance, f32 rise, f32 drop, bool isCheckWall);
}  // namespace NpcStateFunction
