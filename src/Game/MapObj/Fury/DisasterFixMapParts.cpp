#include "MapObj/Fury/DisasterFixMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
DisasterAnimPart::DisasterAnimPart() : mNormalAction(""), mDisasterAction("") {}
void DisasterAnimPart::initAnimPart(al::LiveActor* actor, const al::ActorInitInfo& info) {
    const char* action = al::getActionName(actor);
    if (action) {
        mNormalAction.format("%s", action);
        mDisasterAction.format("%s_Disaster", action);
    }
    al::tryGetStringArg(&mNormalCubeMap, info, "CubeMapUnitName");
    al::tryGetStringArg(&mDisasterCubeMap, info, "CubeMapUnitNameDisaster");
    al::tryGetArg(&mSyncFarLod, info, "IsNeedFarLodAnimSync");
}
void DisasterAnimPart::initAnimPartAfterPlacement(al::LiveActor* actor) {
    if (auto* controller = DisasterModeController::tryGetController(actor)) controller->registerStateListener(this);
}
void DisasterAnimPart::startDisasterModeAnim(al::LiveActor* actor, DisasterModeController::State state) {
    if (state == DisasterModeController::State::Disaster) {
        al::tryStartAction(actor, mDisasterAction.cstr());
        if (mSyncFarLod && actor->getFarLodActor()) al::tryStartAction(actor->getFarLodActor(), mDisasterAction.cstr());
        if (mDisasterCubeMap) al::forceApplyCubeMap(actor->getModelKeeper(), actor->getSceneInfo()->graphicsSystemInfo, mDisasterCubeMap);
    } else if (state == DisasterModeController::State::Normal) {
        al::tryStartAction(actor, mNormalAction.cstr());
        if (mSyncFarLod && actor->getFarLodActor()) al::tryStartAction(actor->getFarLodActor(), mNormalAction.cstr());
        if (mNormalCubeMap) al::forceApplyCubeMap(actor->getModelKeeper(), actor->getSceneInfo()->graphicsSystemInfo, mNormalCubeMap);
    }
}
DisasterFixMapParts::DisasterFixMapParts(const char* name) : al::FixMapParts(name) {}
void DisasterFixMapParts::init(const al::ActorInitInfo& info) { al::FixMapParts::init(info); initAnimPart(this, info); }
void DisasterFixMapParts::initAfterPlacement() { initAnimPartAfterPlacement(this); }
void DisasterFixMapParts::startFarLod() {
    if (mSyncFarLod && getFarLodActor()) {
        al::tryStartAction(getFarLodActor(), al::getActionName(this));
        if (al::isSklAnimExist(this) && al::isSklAnimExist(getFarLodActor())) al::setSklAnimFrame(getFarLodActor(), al::getSklAnimFrame(this, 0), 0);
        if (al::isMtpAnimExist(this) && al::isMtpAnimExist(getFarLodActor())) al::setMtpAnimFrame(getFarLodActor(), al::getMtpAnimFrame(this));
        if (al::isMclAnimExist(this) && al::isMclAnimExist(getFarLodActor())) al::setMclAnimFrame(getFarLodActor(), al::getMclAnimFrame(this));
        if (al::isMtsAnimExist(this) && al::isMtsAnimExist(getFarLodActor())) al::setMtsAnimFrame(getFarLodActor(), al::getMtsAnimFrame(this));
        if (al::isVisAnimExist(this) && al::isVisAnimExist(getFarLodActor())) al::setVisAnimFrame(getFarLodActor(), al::getVisAnimFrame(this));
    }
    al::LiveActor::startFarLod();
}
void DisasterFixMapParts::endFarLod() {
    al::LiveActor::endFarLod();
    if (mSyncFarLod && getFarLodActor()) {
        al::tryStartAction(this, al::getActionName(getFarLodActor()));
        if (al::isSklAnimExist(this) && al::isSklAnimExist(getFarLodActor())) al::setSklAnimFrame(this, al::getSklAnimFrame(getFarLodActor(), 0), 0);
        if (al::isMtpAnimExist(this) && al::isMtpAnimExist(getFarLodActor())) al::setMtpAnimFrame(this, al::getMtpAnimFrame(getFarLodActor()));
        if (al::isMclAnimExist(this) && al::isMclAnimExist(getFarLodActor())) al::setMclAnimFrame(this, al::getMclAnimFrame(getFarLodActor()));
        if (al::isMtsAnimExist(this) && al::isMtsAnimExist(getFarLodActor())) al::setMtsAnimFrame(this, al::getMtsAnimFrame(getFarLodActor()));
        if (al::isVisAnimExist(this) && al::isVisAnimExist(getFarLodActor())) al::setVisAnimFrame(this, al::getVisAnimFrame(getFarLodActor()));
    }
}
void DisasterFixMapParts::onDisasterModeStateChange(DisasterModeController::State state) { startDisasterModeAnim(this, state); }
