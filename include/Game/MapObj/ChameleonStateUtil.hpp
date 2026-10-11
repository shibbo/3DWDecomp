#pragma once

#include "MapObj/RenderMaterialIndirectParam.hpp"

namespace al { class LiveActor; class SensorMsg; class HitSensor; }
class ChameleonStateHipDrop;
class ChameleonStateGiantPlayer;
namespace ChameleonStateUtil {
void updateIndirectParam(RenderMaterialIndirectParam*, float, float, float);
void setRenderMaterialIndirectParam(al::LiveActor*, const RenderMaterialIndirectParam*);
bool tryRequestHipDropAppearChameleon(const al::SensorMsg*, al::HitSensor*, ChameleonStateHipDrop*);
bool tryRequestGiantPlayerAppearChameleon(const al::SensorMsg*, al::HitSensor*, ChameleonStateGiantPlayer*);
bool isMsgHitAppearChameleon(const al::SensorMsg*);
void updateIndirectParam(RenderMaterialIndirectParam*, float, const RenderMaterialIndirectParam*,
                         const RenderMaterialIndirectParam*);
void updateIndirectParam(RenderMaterialIndirectParam*, const RenderMaterialIndirectParam*);
bool isVisible(const RenderMaterialIndirectParam*, const RenderMaterialIndirectParam*,
               const RenderMaterialIndirectParam*);
void updateIndirectParam(RenderMaterialIndirectParam* pParam, float blendRate, float intensity,
                         const sead::Vector4f& rVector0, const sead::Vector4f& rVector1,
                         const sead::Vector4f& rVector2);
}
