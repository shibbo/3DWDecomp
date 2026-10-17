#include "Library/LiveActor/LiveActorFlag.hpp"
#include "MapObj/Fury/InkPatch.hpp"
#include "MapObj/Fury/InkPatchSpecial.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/DemoUtil.hpp"
namespace {
    NERVE_DECL(InkPatch, Wait);
    NERVE_DECL(InkPatch, Done);
    NERVE_DECL(InkPatch, Disappear);
    NERVES_MAKE_NOSTRUCT(InkPatch, Wait, Done, Disappear)
}
InkPatch::InkPatch(const char* name) : al::FixMapParts(name) {}
InkPatch::~InkPatch() {}
void InkPatch::init(const al::ActorInitInfo& info) {
    al::FixMapParts::init(info);
    const char* audio = nullptr;
    if (!al::tryGetStringArg(&audio, info, "AudioUserName")) audio = "InkPatch";
    al::initActorAudioKeeper(this, info, audio, nullptr);
    mCamera = al::initObjectCamera_RS(this, info, nullptr);
    al::initNerve(this, &NrvInkPatchWait, 2);
    al::tryGetArg(&mIsSpecialUnlockInk, info, "isSpecialUnlockInk");
    al::tryGetArg(&mIsSpecialInkUnlocker, info, "isSpecialInkUnlocker");
    if (mIsSpecialUnlockInk || mIsSpecialInkUnlocker) {
        InkPatchSpecial* special = InkPatchSpecial::tryGetInkPatchSpecial(this);
        if (!special) {
            special = new InkPatchSpecial();
            al::setSceneObj(this, special, 61);
        }
        if (mIsSpecialInkUnlocker) special->addUnlockerInkPatch(this);
        else if (mIsSpecialUnlockInk) special->setSpecialPatch(this);
    }
    al::invalidateAllCollisionParts(this);
}
void InkPatch::initAfterPlacement() {}
void InkPatch::appear() {
    al::FixMapParts::appear();
    al::invalidateAllCollisionParts(this);
}
void InkPatch::fullKill(bool force) {
    if (!force) {
        auto* special = InkPatchSpecial::tryGetInkPatchSpecial(this);
        if (special && special->getProgress() > 1) return;
    }
    auto* plessie = static_cast<RaidonSurf*>(al::tryGetSceneObj(this, 51));
    if (plessie) plessie->updateSpawns(false);
    if (!force) al::tryOnStageSwitchInstant(this, "SwitchInkPatchKillStartOn");
    al::tryOnStageSwitchInstant(this, "SwitchInkPatchKillOn");
    al::stopAllSeId(this, "Wait", 0, nullptr);
    al::LiveActor::kill();
}
void InkPatch::kill() { fullKill(false); }
void InkPatch::triggerSpecialInkPatch() {
    auto* patch = InkPatchSpecial::tryGetSpecialInkPatch(this);
    if (!patch) return;
    patch->_142 = true;
    patch->disappear();
    patch->startCamera();
    rc::addDemoActor(patch);
}
void InkPatch::disappear() {
    al::invalidateClipping(this);
    al::setNerve(this, &NrvInkPatchDisappear);
}
void InkPatch::startCamera() {
    if (mCamera) {
        al::invalidateClipping(this);
        al::startCamera_RS(this, mCamera, -1);
        al::setFixActorCameraTarget(mCamera, this);
    }
}
void InkPatch::endSpecialInkPatch(bool active) {
    auto* patch = InkPatchSpecial::tryGetSpecialInkPatch(this);
    if (!patch) return;
    patch->_142 = active;
    if (mCamera) al::endCamera_RS(patch, patch->mCamera, -1, false);
}
al::CameraTicket* InkPatch::getInkPatchCameraTicket() { return mCamera; }
bool InkPatch::isSpecialInkPatchDone() {
    auto* patch = InkPatchSpecial::tryGetSpecialInkPatch(this);
    return patch ? patch->isDone() : false;
}
bool InkPatch::isDone() { return getFlags()->isDead || al::isNerve(this, &NrvInkPatchDone); }
void InkPatch::forceDisappear() {
    al::tryOnStageSwitchInstant(this, "SwitchInkPatchKillStartOn");
    al::setNerve(this, &NrvInkPatchDone);
}
void InkPatch::endCamera() { if (mCamera) al::endCamera_RS(this, mCamera, -1, false); }
void InkPatch::exeWait() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "Wait");
        al::startSe(this, "Wait", nullptr);
    }
    if (al::isStep(this, 2)) al::validateAllCollisionParts(this);
}
void InkPatch::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitchInstant(this, "SwitchInkPatchKillStartOn");
        al::stopAllSeId(this, "Wait", 0, nullptr);
        al::tryStartAction(this, "Disappear");
        al::startSe(this, "Disappear", nullptr);
    }
    if (al::isStep(this, 59)) al::startSe(this, "DisappearJingle", nullptr);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvInkPatchDone);
}
void InkPatch::exeDone() {
    auto* plessie = static_cast<RaidonSurf*>(al::tryGetSceneObj(this, 51));
    if (plessie) plessie->updateSpawns(false);
    al::tryOnStageSwitchInstant(this, "SwitchInkPatchKillOn");
    al::stopAllSeId(this, "Wait", 0, nullptr);
    al::LiveActor::kill();
}
