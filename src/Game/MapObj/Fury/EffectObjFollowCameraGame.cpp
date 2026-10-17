#include "MapObj/Fury/EffectObjFollowCameraGame.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
    using rc::EffectObjFollowCameraGame;
    NERVE_DECL(EffectObjFollowCameraGame, DisasterMode);
    NERVES_MAKE_NOSTRUCT(EffectObjFollowCameraGame, DisasterMode)
}

namespace rc {
EffectObjFollowCameraGame::EffectObjFollowCameraGame(const char* pName)
    : al::EffectObjFollowCamera(pName) {}
EffectObjFollowCameraGame::~EffectObjFollowCameraGame() {}

void EffectObjFollowCameraGame::init(const al::ActorInitInfo& rInfo) {
    al::EffectObjFollowCamera::init(rInfo);
    al::tryGetArg(&mStopInDisasterMode, rInfo, "StopInDisasterMode");
    mIsSingleMode = al::isSingleMode(rInfo);
}

void EffectObjFollowCameraGame::control() {
    al::EffectObjFollowCamera::control();
    if (!mIsSingleMode || !mStopInDisasterMode || isDisappearing())
        return;
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (!controller || controller->isRaining() == mIsRaining)
        return;
    mIsRaining = controller->isRaining();
    if (mIsRaining)
        al::setNerve(this, &NrvEffectObjFollowCameraGameDisasterMode);
    else
        restoreNerve(mShouldAppear);
}

void EffectObjFollowCameraGame::startAppear() {
    al::EffectObjFollowCamera::startAppear();
    mShouldAppear = true;
    if (mIsRaining)
        al::setNerve(this, &NrvEffectObjFollowCameraGameDisasterMode);
}

void EffectObjFollowCameraGame::startDisappear() {
    al::EffectObjFollowCamera::startDisappear();
    mShouldAppear = false;
    if (mIsRaining)
        al::setNerve(this, &NrvEffectObjFollowCameraGameDisasterMode);
}

void EffectObjFollowCameraGame::exeDisasterMode() {
    if (al::isFirstStep(this)) {
        if (al::isEffectEmitting(this, "Wait"))
            al::tryKillEmitterAndParticleAll(this);
        if (getAudioKeeper())
            al::tryStopSe(this, "Wait");
    }
}
}
