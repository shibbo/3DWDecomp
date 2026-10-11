#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

class IUsePlayerCollisionCheckArrow;
class PlayerConstParam;
struct PlayerProperty;

/// Helpers around the player's collision.
namespace PlayerCollisionFunc {
void calcCollisionBodyPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans,
                          const sead::Vector3f& rUp, f32 chestRadius);
void calcCollisionHeadPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans,
                          const sead::Vector3f& rUp, f32 chestRadius, f32 tall);
void calcWallCheckBodyPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans,
                          const sead::Vector3f& rUp, f32 tall);
u32 findNearestArrowCollision(IUsePlayerCollisionCheckArrow* pCheckArrow,
                              const sead::Vector3f& rPos);
bool isBackWall(const sead::Vector3f& rNormal, const sead::Vector3f& rFront);
bool isFrontWall(const sead::Vector3f& rNormal, const sead::Vector3f& rFront);
f32 calcTall(const PlayerProperty* pProperty, const PlayerConstParam* pConstParam);
f32 calcChestRadius(const PlayerProperty* pProperty, const PlayerConstParam* pConstParam);
f32 calcDashCheckRadius(const PlayerProperty* pProperty, const PlayerConstParam* pConstParam);
}  // namespace PlayerCollisionFunc
