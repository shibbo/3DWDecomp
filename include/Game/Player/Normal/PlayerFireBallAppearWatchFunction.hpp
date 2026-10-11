#pragma once

#include <basis/seadTypes.h>
#include <prim/seadEnum.h>

namespace al {
class LiveActor;
}  // namespace al

/// How a newly thrown fire ball is shown, decided by how many balls are already out.
SEAD_ENUM(PlayerFireBallAppearType, Normal, LowEffect, NoLight, NoLightForced)

/// Helpers reporting the player's fire balls to the scene's PlayerFireBallAppearWatcher.
namespace PlayerFireBallAppearWatchFunction {
PlayerFireBallAppearType registAppearFireBall(al::LiveActor* pActor);
void registKillFireBall(al::LiveActor* pActor);
bool isEmittableBoundEffect(al::LiveActor* pActor);
}  // namespace PlayerFireBallAppearWatchFunction
