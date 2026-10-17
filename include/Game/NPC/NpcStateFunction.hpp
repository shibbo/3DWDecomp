#pragma once

#include <math/seadVector.h>

namespace al { class LiveActor; }
class NpcStateParam;

namespace NpcStateFunction {
bool isInNPCAvoidArea(const al::LiveActor* pActor, const sead::Vector3f& rPosition);
bool isInNPCAvoidArea(const al::LiveActor* pActor);
void calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam);
void calcPassiveMovement(al::LiveActor* pActor, const NpcStateParam* pParam, bool isGrounded);
bool isFallNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPosition,
                    const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity,
                    float distance, float height, float depth, bool checkWall);
bool isFallNextMove(const al::LiveActor* pActor, float distance, float height, float depth,
                    bool checkWall);
bool isWallNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPosition,
                    const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity,
                    float distance, float height);
bool isWallNextMove(const al::LiveActor* pActor, float distance, float height);
bool isNPCAvoidAreaNextMove(const al::LiveActor* pActor, const sead::Vector3f& rPosition,
                           const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity,
                           float distance, float height, float depth, bool checkWall);
bool isNPCAvoidAreaNextMove(const al::LiveActor* pActor, float distance, float height, float depth,
                           bool checkWall);
}
