#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

/** @brief Clone placement tables of Punpun (Motley Bossblob) at level 2. */
namespace PunpunParam {
s32 getDividePosPatternNum(s32 damageCount);
const sead::Vector3f& getDividePos(s32 damageCount, s32 pattern, s32 index);
}  // namespace PunpunParam
