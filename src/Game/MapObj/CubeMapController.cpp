#include "MapObj/CubeMapController.hpp"
#include <cmath>
#include "Library/ActorUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shader/DeferredRendering/CubeMapDirector.hpp"
#include "Library/Shader/ForwardRendering/CubeMapKeeper.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
    NERVE_DECL(CubeMapController, Wait);
    NERVE_DECL(CubeMapController, FadeOut);
    NERVE_DECL(CubeMapController, FadeIn);
    NERVES_MAKE_NOSTRUCT(CubeMapController, Wait, FadeOut, FadeIn)
}

CubeMapController::CubeMapController(const char* pName) : al::LiveActor(pName) {}
CubeMapController::~CubeMapController() {}

void CubeMapController::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvCubeMapControllerWait, 1);
    makeActorAppeared();
    al::hideModelIfShow(this);
    al::listenStageSwitchOnOffStart(this, al::Functor(this, &CubeMapController::triggerFadeTo),
                                  al::Functor(this, &CubeMapController::triggerFadeFrom));
    al::listenStageSwitchOn(this, "SwitchAppear", al::Functor(this, &CubeMapController::triggerFadeTo));
    al::listenStageSwitchOn(this, "SwitchKill", al::Functor(this, &CubeMapController::triggerFadeFrom));
    al::tryGetStringArg(&mCubeMapName, rInfo, "CubeMapName");
    int frames;
    al::tryGetArg(&frames, rInfo, "AnimationFrames");
    setAnimationFrames(frames);
}

void CubeMapController::triggerFadeTo() {
    mTargetCubeMapName = mCubeMapName;
    al::setNerve(this, &NrvCubeMapControllerFadeOut);
}
void CubeMapController::triggerFadeFrom() {
    mTargetCubeMapName = nullptr;
    al::setNerve(this, &NrvCubeMapControllerFadeOut);
}
void CubeMapController::setAnimationFrames(unsigned int frames) { mFadeSpeed = 2.0f / frames; }
void CubeMapController::initAfterPlacement() {}
void CubeMapController::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvCubeMapControllerWait);
}
void CubeMapController::control() {}
void CubeMapController::update() {}
void CubeMapController::setCubeMap(const char* pName) { mCubeMapName = pName; }
bool CubeMapController::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }
bool CubeMapController::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer*, al::ScreenPointTarget*) {
    return al::isMsgTouchAssist(pMsg);
}
void CubeMapController::exeWait() {}
void CubeMapController::exeFadeOut() {
    auto* keeper = getSceneInfo()->graphicsSystemInfo->getCubeMapDirector()->getShaderCubeMapKeeper();
    if (keeper->getModelLightIntensity() == 0.0f) {
        keeper->setCubeMap(mTargetCubeMapName);
        al::setNerve(this, &NrvCubeMapControllerFadeIn);
    } else {
        keeper->setModelLightIntensity(fmaxf(keeper->getModelLightIntensity() - mFadeSpeed, 0.0f));
    }
}
void CubeMapController::exeFadeIn() {
    auto* keeper = getSceneInfo()->graphicsSystemInfo->getCubeMapDirector()->getShaderCubeMapKeeper();
    if (keeper->getModelLightIntensity() == 1.0f) {
        al::setNerve(this, &NrvCubeMapControllerWait);
    } else {
        keeper->setModelLightIntensity(fminf(keeper->getModelLightIntensity() + mFadeSpeed, 1.0f));
    }
}
