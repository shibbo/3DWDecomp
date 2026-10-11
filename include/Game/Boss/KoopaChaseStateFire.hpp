#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
namespace al { class ActorInitInfo; }
class KoopaChase;
class KoopaFireBall;

class KoopaChaseStateFire : public al::NerveStateBase {
public:
    KoopaChaseStateFire(KoopaChase* pHost, const al::ActorInitInfo& rInfo, bool isProvocation);
    void setFireNum(int fireNum);

private:
    KoopaChase* mHost;
    sead::FixedPtrArray<KoopaFireBall, 6> mFireBalls;
    sead::Vector3f mDirection;
    int mFireNum;
    int mValue70;
    bool mIsProvocation;
    bool mValue75;
};
static_assert(sizeof(KoopaChaseStateFire) == 0x78);
