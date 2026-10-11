#pragma once

#include <basis/seadTypes.h>

namespace al {
class FunctorBase;
}

/// Collects functors run after a player model's world matrices were updated.
class PlayerModelWorldMtxCallbackHolder {
public:
    void registerCallback(const al::FunctorBase& rFunctor);
};
