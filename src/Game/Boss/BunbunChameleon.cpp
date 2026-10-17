#include "Boss/BunbunChameleon.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
namespace {
float interpolate(float a, float b, float rate) { return a * (1.0f - rate) + b * rate; }
}
/** @brief Initializes fully white indirect-material vectors. @param pName Actor name. */
BunbunChameleon::BunbunChameleon(const char* pName) : al::LiveActor(pName) {
    mParam = new RenderMaterialIndirectParam{0.06f, 0.0f, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
}
/** @brief Destroys the actor. */
BunbunChameleon::~BunbunChameleon() = default;
/** @brief Updates material visibility from player distance and adjusts each shadow. */
void BunbunChameleon::control() {
    auto* player = rc::tryFindNearestActivePlayerActorInSphere(this, 1500.0f);
    if (player) {
        sead::Vector3f pos = al::getTrans(this);
        sead::Vector3f playerPos = al::getTrans(player);
        float distance = (pos - playerPos).length();
        float rate = 1.0f - sead::Mathf::clamp(distance / 1000.0f, 0.0f, 1.0f);
        float squared = rate * rate;
        float intensity = squared * 0.8f + (1.0f - squared) * 0.0f;
        sead::Vector4f vector0(mParam->mVector0.x, mParam->mVector0.y, mParam->mVector0.z, interpolate(1.0f, 1.0f, squared));
        sead::Vector4f vector2(rate * 0.2f, rate * 0.2f, rate * 0.2f, rate);
        ChameleonStateUtil::updateIndirectParam(mParam, 0.06f, intensity, vector0, sead::Vector4f::ones, vector2);
    }
    ChameleonStateUtil::setRenderMaterialIndirectParam(this, mParam);
    if (al::isExistShadow(this)) {
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "JointRoot");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinArm0");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandL0");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandR0");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinArm1");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandL1");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandR1");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinArm2");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandL2");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "BunbunSpinHandR2");
        al::setShadowIntensityUser(this, static_cast<int>(mParam->mIntensity * 255.0f), "Body");
    }
}
