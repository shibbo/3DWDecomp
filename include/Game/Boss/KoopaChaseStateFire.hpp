#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
namespace al { class ActorInitInfo; }
class KoopaChase;
class KoopaFireBall;

class KoopaChaseStateFire : public al::HostStateBase<KoopaChase> {
public:
    KoopaChaseStateFire(KoopaChase* pHost, const al::ActorInitInfo& rInfo, bool isProvocation);
    ~KoopaChaseStateFire() override;
    void appear() override;
    void calcMouthPos(sead::Vector3f* pOut) const;
    bool calcFireDir(sead::Vector3f* pOut) const;
    KoopaFireBall* getFireBall() const;
    void exeWait();
    void exeFireStart();
    void exeFireEnd();
    void setFireNum(int fireNum);

private:
    sead::PtrArray<KoopaFireBall> mFireBalls;
    KoopaFireBall* mFireBallStorage[6];
    sead::Vector3f mDirection;
    int mFireNum;
    int mFireCount;
    bool mIsProvocation;
    bool mKillOnCollide;
};
static_assert(sizeof(KoopaChaseStateFire) == 0x78);
