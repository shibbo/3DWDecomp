#pragma once
#include "Library/Nerve/NerveStateBase.hpp"

namespace al { class ActorInitInfo; }
class KoopaChase;

class KoopaChaseStateThrow : public al::NerveStateBase {
public:
    KoopaChaseStateThrow(KoopaChase* pHost, const al::ActorInitInfo& rInfo, f32 throwSpeed);
    bool tryStartThrow(bool isForce);

private:
    // Remaining state members are not yet reconstructed.
    unsigned char mStateData11[0x37];
};
static_assert(sizeof(KoopaChaseStateThrow) == 0x48);
