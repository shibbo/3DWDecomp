#include "MapObj/ChameleonStateUtil.hpp"
#include "MapObj/ChameleonStateGiantPlayer.hpp"
#include "MapObj/ChameleonStateHipDrop.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <nn/g3d/g3d_ModelObj.h>

namespace {
bool sDrawTransparentObject = false;
}


namespace ChameleonStateUtil {
bool tryRequestGiantPlayerAppearChameleon(const al::SensorMsg*, al::HitSensor* sensor,
                                          ChameleonStateGiantPlayer* state) {
    if (!al::isSensorPlayer(sensor)) return false;
    if (rc::isGiantPlayerWalking(sensor) || rc::isGiantPlayerRunning(sensor) ||
        rc::isGiantPlayerLanding(sensor)) {
        state->setPlayerSensor(sensor);
        return true;
    }
    return false;
}

bool tryRequestHipDropAppearChameleon(const al::SensorMsg* msg, al::HitSensor* sensor,
                                      ChameleonStateHipDrop* state) {
    if (al::isSensorPlayer(sensor)) {
        if (!al::isMsgPlayerHipDropKnockDown(msg) && !al::isMsgPlayerGiantHipDrop(msg) &&
            !al::isMsgPlayerStatueDrop(msg) && !al::isMsgPlayerCooperationHipDrop(msg)) return false;
    } else {
        if (!al::isSensorKoopaJr(sensor)) return false;
        if (!al::isMsgPlayerObjHipDropAll(msg)) return false;
    }
    const sead::Vector3f& trans = al::getTrans(state->mHostActor);
    float distance = (trans - al::getSensorPos(sensor)).length();
    state->setAppearDelay(static_cast<u32>(distance * 0.03f));
    return true;
}

bool isMsgHitAppearChameleon(const al::SensorMsg* msg) {
    return al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerSpinAttack(msg) ||
           al::isMsgPlayerBoomerangAttack(msg) || al::isMsgEnemyAttackFire(msg) ||
           al::isMsgExplosion(msg) || al::isMsgLaserAttack(msg) || rc::isMsgKillerShockWave(msg);
}

void updateIndirectParam(RenderMaterialIndirectParam* out, const RenderMaterialIndirectParam* param) {
    updateIndirectParam(out, param->mBlendRate, param->mIntensity,
                        param->mVector0, param->mVector1, param->mVector2);
}

void updateIndirectParam(RenderMaterialIndirectParam* out, float rate, float intensity,
        const sead::Vector4f& v0, const sead::Vector4f& v1, const sead::Vector4f& v2) {
    updateIndirectParam(out, rate, out->mIntensity, out->mVector0, out->mVector1, out->mVector2,
                        intensity, v0, v1, v2);
}

void updateIndirectParam(RenderMaterialIndirectParam* out, float rate, float intensity, float alpha) {
    out->mIntensity = al::lerpValue(rate, out->mIntensity, intensity);
    out->mVector0.w = al::lerpValue(rate, out->mVector0.w, alpha);
}

void updateIndirectParam(RenderMaterialIndirectParam* out, float rate, float fromIntensity,
        const sead::Vector4f& from0, const sead::Vector4f& from1, const sead::Vector4f& from2,
        float toIntensity, const sead::Vector4f& to0, const sead::Vector4f& to1,
        const sead::Vector4f& to2) {
    out->mIntensity = al::lerpValue(rate, fromIntensity, toIntensity);
    out->mVector0.x = al::lerpValue(rate, from0.x, to0.x);
    out->mVector0.y = al::lerpValue(rate, from0.y, to0.y);
    out->mVector0.z = al::lerpValue(rate, from0.z, to0.z);
    out->mVector0.w = al::lerpValue(rate, from0.w, to0.w);
    out->mVector1.x = al::lerpValue(rate, from1.x, to1.x);
    out->mVector1.y = al::lerpValue(rate, from1.y, to1.y);
    out->mVector1.z = al::lerpValue(rate, from1.z, to1.z);
    out->mVector1.w = al::lerpValue(rate, from1.w, to1.w);
    out->mVector2.x = al::lerpValue(rate, from2.x, to2.x);
    out->mVector2.y = al::lerpValue(rate, from2.y, to2.y);
    out->mVector2.z = al::lerpValue(rate, from2.z, to2.z);
    out->mVector2.w = al::lerpValue(rate, from2.w, to2.w);
}

void updateIndirectParam(RenderMaterialIndirectParam* out, float rate,
        const RenderMaterialIndirectParam* from, const RenderMaterialIndirectParam* to) {
    updateIndirectParam(out, rate, from->mIntensity, from->mVector0, from->mVector1, from->mVector2,
                        to->mIntensity, to->mVector0, to->mVector1, to->mVector2);
}

void setRenderMaterialIndirectParam(al::LiveActor* actor, const RenderMaterialIndirectParam* param) {
    nn::g3d::ModelObj* model = actor->getModelKeeper()->getModelCafe()->getModelG3D()->getModelObj();
    int count = model->GetNumMaterials();
    for (int i = 0; i < count; ++i) {
        nn::g3d::MaterialObj* material = model->GetMaterial(i);
        int index = material->FindShaderParamIndex("cEta");
        if (index != -1) {
            float* value = material->EditShaderParam<float>(index);
            *value = param->mIntensity;
        }
        index = material->FindShaderParamIndex("cAbsorbRefraction");
        if (index != -1) *material->EditShaderParam<sead::Vector4f>(index) = param->mVector0;
        index = material->FindShaderParamIndex("cSelectiveReflection");
        if (index != -1) *material->EditShaderParam<sead::Vector4f>(index) = param->mVector1;
        index = material->FindShaderParamIndex("cModelLightColor");
        if (index != -1) *material->EditShaderParam<sead::Vector4f>(index) = param->mVector2;
    }
    al::setMaterialProgrammable(actor);
}

bool isVisible(const RenderMaterialIndirectParam* param, const RenderMaterialIndirectParam* near,
               const RenderMaterialIndirectParam* far) {
    float farIntensity = far->mIntensity;
    float intensityRange = near->mIntensity - farIntensity;
    if ((intensityRange > 0.0f) && ((param->mIntensity - farIntensity) / intensityRange > 0.5f))
        return true;
    float farAlpha = far->mVector0.w;
    float alphaRange = near->mVector0.w - farAlpha;
    if ((alphaRange > 0.0f) && ((param->mVector0.w - farAlpha) / alphaRange > 0.5f))
        return true;
    return false;
}

void changeDrawTransparentObject() {
    sDrawTransparentObject = !sDrawTransparentObject;
}
}
