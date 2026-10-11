#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al { class ActorInitInfo; }
class KoopaChase;

class KoopaChaseStateWarp : public al::NerveStateBase {
public:
    KoopaChaseStateWarp(KoopaChase* pHost, const al::ActorInitInfo& rInfo);
    void start(const sead::Vector3f& rWarpPosition);
    void exeWarpStart();
    void exeWarp();
    void exeWaitForRestart();

private:
    KoopaChase* mHost;
    sead::Vector3f mWarpPosition;
};
