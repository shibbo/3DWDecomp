#include "MapObj/Fury/EffectObjGame.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace rc {
EffectObjGame::EffectObjGame(const char* pName) : al::EffectObj(pName) {}
EffectObjGame::~EffectObjGame() {}

void EffectObjGame::init(const al::ActorInitInfo& rInfo) {
    al::EffectObj::init(rInfo);
    al::tryGetArg(&mStopInDisasterMode, rInfo, "StopInDisasterMode");
    al::tryGetArg(&mClearEffectOnKill, rInfo, "ClearEffectOnKill");
    mIsSingleMode = al::isSingleMode(rInfo);
}

void EffectObjGame::control() {
    al::EffectObj::control();
    if (!mIsSingleMode || !mStopInDisasterMode)
        return;
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (!controller || controller->isDisasterMode() == mIsDisasterMode)
        return;
    mIsDisasterMode = controller->isDisasterMode();
    if (mIsDisasterMode) {
        al::tryKillEmitterAndParticleAll(this);
        al::tryStopSeByName(this, "Wait");
    } else {
        al::emitEffect(this, "Wait", nullptr);
        al::startSe(this, "Wait", nullptr);
    }
}

void EffectObjGame::kill() {
    al::EffectObj::kill();
    if (mClearEffectOnKill)
        al::tryKillEmitterAndParticleAll(this);
}
}
