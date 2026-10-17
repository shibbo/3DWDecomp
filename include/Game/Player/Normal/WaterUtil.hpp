#pragma once

#include <math/seadVector.h>

namespace al {
class AreaObj;
class IUseAreaObj;
class IUseSceneObjHolder;
}  // namespace al

class IUseWaterFlowAccess;

namespace WaterUtil {
bool checkWaterSurface(const al::IUseAreaObj* pAreaUser, sead::Vector3f* pSurfacePos,
                       const sead::Vector3f& rPos, f32 range, const al::AreaObj* pAreaObj);
void calcWaterFlowField(const al::IUseAreaObj* pAreaUser, sead::Vector3f* pFlow,
                        const IUseWaterFlowAccess* pAccess, const sead::Vector3f& rPos);
bool calcWaterSurfacePos(const al::IUseAreaObj* pAreaUser, const sead::Vector3f& rPos, f32 range,
                         sead::Vector3f* pSurfacePos);
bool isInInkArea(const al::IUseSceneObjHolder* pHolder, const sead::Vector3f& rPos);
f32 getOceanWaterHeight(const al::IUseSceneObjHolder* pHolder);
f32 getOceanWaterHeight(const al::IUseSceneObjHolder* pHolder, const sead::Vector3f& rPos,
                        bool isWave);
}  // namespace WaterUtil
