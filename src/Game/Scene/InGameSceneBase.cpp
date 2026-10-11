#include "Scene/InGameSceneBase.hpp"

InGameSceneBase::InGameSceneBase(const char* pName) : al::Scene(pName) {}

bool InGameSceneBase::isRestartCheck() const {
    return false;
}

bool InGameSceneBase::isWorldWarp() const {
    return false;
}

bool InGameSceneBase::isTriggerPause(s32* pPort) const {
    return true;
}

void InGameSceneBase::recordClearData() {}
