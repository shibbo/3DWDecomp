#pragma once

#include <math/seadVector.h>

namespace al { class LiveActor; }
struct WalkerStateParam;

namespace WalkerStateFunction {
void calcPassiveMovement(al::LiveActor* pHost, const WalkerStateParam* pParam);
void calcPassiveMovement(al::LiveActor* pHost, const WalkerStateParam* pParam, bool isOnGround);
bool isFallNextMove(const al::LiveActor* pHost, const sead::Vector3f& rPosition,
                    const sead::Vector3f& rVelocity, const sead::Vector3f& rGravity,
                    float distance, float rise, float drop, bool checkWall);
bool isFallNextMove(const al::LiveActor* pHost, float distance, float rise, float drop, bool checkWall);
bool isFallNextMoveY(const al::LiveActor* pHost, float offsetY, float distance,
                     float rise, float drop, bool checkWall);
}
