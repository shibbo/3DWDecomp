#include "MapObj/SpotLightPatroller.hpp"
#include "MapObj/SpotLightPatrollerHolder.hpp"
#include "MapObj/SpotLightPatrollFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <math.h>
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
namespace {
bool isOrthoLight(int type) { return type == 1 || type == 3; }
NERVE_DECL(SpotLightPatroller, Patrol)
NERVE_DECL(SpotLightPatroller, Alert)
NERVES_MAKE_NOSTRUCT(SpotLightPatroller, Patrol, Alert)
}
SpotLightPatroller::SpotLightPatroller(const char* name) : al::LiveActor(name) {}
SpotLightPatroller::~SpotLightPatroller() = default;
void SpotLightPatroller::init(const al::ActorInitInfo& info) {
    al::tryGetArg(&mHitCheckType, info, "HitCheckType");
    al::tryGetArg(&mLppType, info, "LppType");
    switch (mLppType) {
    case 0: al::initActorWithArchiveName(this, info, "SpotLightPatroller", nullptr); break;
    default: al::initActorWithArchiveName(this, info, "SpotLightPatroller", nullptr); break;
    case 1: al::initActorWithArchiveName(this, info, "SpotLightPatroller", "Ortho"); mHitCheckType = 0; break;
    case 2: {
        al::initActorWithArchiveName(this, info, "SpotLightPatroller", "Spot");
        float far = 100.0f; al::tryGetArg(&far, info, "ProjLightFar");
        al::setPrePassSpotLightLength(this, "監視スポットライト", far);
        break;
    }
    case 3: al::initActorWithArchiveName(this, info, "SpotLightPatroller", "OrthoLarge"); mHitCheckType = 0; break;
    }
    al::initActorAudioKeeper(this, info, "SpotLightPatroller", nullptr);
    al::tryGetArg(&mColor.r, info, "ColorRed");
    al::tryGetArg(&mColor.g, info, "ColorGreen");
    al::tryGetArg(&mColor.b, info, "ColorBlue");
    al::tryGetArg(&mEffectType, info, "EffectType");
    switch (mEffectType) {
    case 0: al::setEffectFollowMtxPtr(this, "Fovy15", &mEffectMtx); al::setEffectFollowMtxPtr(this, "Fovy15Alert", &mEffectMtx); break;
    case 1: al::setEffectFollowMtxPtr(this, "Fovy30", &mEffectMtx); al::setEffectFollowMtxPtr(this, "Fovy30Alert", &mEffectMtx); break;
    case 2: al::setEffectFollowMtxPtr(this, "Fovy40", &mEffectMtx); al::setEffectFollowMtxPtr(this, "Fovy40Alert", &mEffectMtx); break;
    case 3: al::setEffectFollowMtxPtr(this, "Fovy60", &mEffectMtx); al::setEffectFollowMtxPtr(this, "Fovy60Alert", &mEffectMtx); break;
    }
    al::tryGetArg(&mSpeed, info, "Speed");
    al::tryGetArg(&mIsCheckCollision, info, "IsCheckCollision");
    if (al::isExistRail(info)) al::setSyncRailToNearestPos(this);
    int useShadow = 0; al::tryGetArg(&useShadow, info, "UseShadow");
    float pcfOffset = 1.5f; al::tryGetArg(&pcfOffset, info, "LppPcfOffset");
    if (useShadow) {
        switch (mLppType) {
        case 2: al::setPrePassSpotLightShadow(this, "通常", true, pcfOffset, 0.0f); break;
        case 3: case 1: al::setPrePassProjOrthoLightShadow(this, "通常", true, pcfOffset, 0.0f); break;
        default: al::setPrePassProjLightShadow(this, "通常", true, pcfOffset, 0.0f); break;
        }
    }
    al::killPrePassLight(this, "発見", 0);
    mLightState = 0;
    al::appearPrePassLight(this, "通常", 0);
    al::initNerve(this, &NrvSpotLightPatrollerPatrol, 0);
    al::listenStageSwitchOnKill(this, al::Functor(this, &SpotLightPatroller::killBySwitch));
    SpotLightPatrollFunction::tryCreateAndGetHolder(this)->registerPatroller(this);
    makeActorAppeared();
}
void SpotLightPatroller::killBySwitch() { kill(); }
void SpotLightPatroller::initAfterPlacement() {
    float length, angle;
    sead::Vector3f dir;
    sead::Vector3f hitPos = sead::Vector3f::zero;
    sead::Vector3f size;
    if (isOrthoLight(mLppType)) {
        al::getPrePassProjOrthoLightInfo(this, "通常", &length, &dir, nullptr, &size);
        mRadius = (size.x + size.z) * 0.5f;
    } else {
        if (mLppType == 2) al::getPrePassSpotLightInfo(this, "通常", &angle, &length, &dir, nullptr);
        else al::getPrePassProjLightInfo(this, "通常", &angle, &length, &dir, nullptr);
        alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, nullptr, al::getTrans(this), length * dir, nullptr, nullptr);
        float distance = (al::getTrans(this) - hitPos).length();
        mRadius = distance * tanf(sead::Mathf::deg2rad(angle * 0.5f));
    }
}
void SpotLightPatroller::setAlert() {
    al::killPrePassLight(this, "通常", 0); mLightState = 1; al::appearPrePassLight(this, "発見", 0);
    al::requestPrePassLightColor(this, "発見", mColor * al::getPrePassUserColor(this, "発見色"));
    al::setNerve(this, &NrvSpotLightPatrollerAlert);
}

void SpotLightPatroller::exePatrol() {
    if (al::isFirstStep(this)) {
        al::killPrePassLight(this, "発見", 0); mLightState = 0; al::appearPrePassLight(this, "通常", 0);
        switch (mEffectType) {
        case 0: al::tryDeleteEffect(this, "Fovy15Alert"); al::emitEffect(this, "Fovy15", nullptr); break;
        case 1: al::tryDeleteEffect(this, "Fovy30Alert"); al::emitEffect(this, "Fovy30", nullptr); break;
        case 2: al::tryDeleteEffect(this, "Fovy40Alert"); al::emitEffect(this, "Fovy40", nullptr); break;
        case 3: al::tryDeleteEffect(this, "Fovy60Alert"); al::emitEffect(this, "Fovy60", nullptr); break;
        }
    }
    if (al::isExistRail(this)) al::moveSyncRailTurn(this, mSpeed);
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    al::makeMtxUpNoSupportPos(&mEffectMtx, up, al::getTrans(this));
    {
        sead::Color4f color = mColor * al::getPrePassUserColor(this, "通常色");
        al::requestPrePassLightColor(this, "通常", color);
    }
    if (!mIsObserved && isPlayerWatch()) { setAlert(); return; }
    alActorPoseFunction::updatePoseTRMSV(this);
}
bool SpotLightPatroller::isPlayerWatch() const {
    sead::Vector3f dir, lightPos;
    float angle, length;
    if (isOrthoLight(mLppType)) al::getPrePassProjOrthoLightInfo(this, "通常", &length, &dir, &lightPos, nullptr);
    else if (mLppType == 2) al::getPrePassSpotLightInfo(this, "通常", &angle, &length, &dir, &lightPos);
    else al::getPrePassProjLightInfo(this, "通常", &angle, &length, &dir, &lightPos);
    length = length < 1.0f ? 1.0f : length;
    for (int i = 0; i < al::getPlayerNumMax(this); ++i) {
        auto* player = al::getPlayerActor(this, i);
        if (al::isDead(player) || rc::isPlayerEquipDisregard(player)) continue;
        sead::Vector3f pos = al::getTrans(player);
        float height;
        if (rc::isPlayerMini(player)) height = 50.0f;
        else height = rc::isPlayerSquat(player) ? 50.0f : 100.0f;
        pos.y += height;
        sead::Vector3f foot;
        if (mHitCheckType == 0) {
            al::calcPerpendicFootToLineInside(&foot, pos, lightPos, length * dir + lightPos);
            if (!((foot - pos).length() < mRadius)) continue;
        } else if (mHitCheckType == 1) {
            if (!al::checkHitSphereSpotLight(pos, 0.0f, lightPos, dir, length, angle * 0.5f)) continue;
        } else continue;
        if (!mIsCheckCollision) return true;
        float distance = (lightPos - foot).length();
        sead::Vector3f start = pos - distance * dir;
        if (!alCollisionUtil::getFirstPolyOnArrow(this, nullptr, start, pos - start, nullptr, nullptr)) return true;
    }
    return false;
}
void SpotLightPatroller::exeAlert() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgOn", nullptr);
        switch (mEffectType) {
        case 0: al::tryDeleteEffect(this, "Fovy15"); al::emitEffect(this, "Fovy15Alert", nullptr); break;
        case 1: al::tryDeleteEffect(this, "Fovy30"); al::emitEffect(this, "Fovy30Alert", nullptr); break;
        case 2: al::tryDeleteEffect(this, "Fovy40"); al::emitEffect(this, "Fovy40Alert", nullptr); break;
        case 3: al::tryDeleteEffect(this, "Fovy60"); al::emitEffect(this, "Fovy60Alert", nullptr); break;
        }
    }
    al::holdSe(this, "PgAlarm", nullptr);
    al::requestPrePassLightColor(this, "発見", mColor * al::getPrePassUserColor(this, "発見色"));
    if (!mIsObserved && !isPlayerWatch() && al::isGreaterStep(this, 30)) al::setNerve(this, &NrvSpotLightPatrollerPatrol);
}
void SpotLightPatroller::movementPaused(bool paused) {
    al::LiveActor::movementPaused(paused);
    if (mLightState == 1) al::requestPrePassLightColor(this, "発見", mColor * al::getPrePassUserColor(this, "発見色"));
    else if (mLightState == 0) al::requestPrePassLightColor(this, "通常", mColor * al::getPrePassUserColor(this, "発見色"));
}
bool SpotLightPatroller::isAlert() const { return al::isNerve(this, &NrvSpotLightPatrollerAlert); }
void SpotLightPatroller::setPatrol() {
    al::killPrePassLight(this, "発見", 0); mLightState = 0; al::appearPrePassLight(this, "通常", 0);
    al::setNerve(this, &NrvSpotLightPatrollerPatrol);
}
