#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <prim/seadEnum.h>
namespace al { class ActorInitInfo; template<class T> class DeriveActorGroup; }
class KoopaChase;
class BombBound;
class KoopaChaseStateThrow : public al::NerveStateBase {
public:
    SEAD_ENUM(ThrowType, LeftOver, RightOver, LeftUnder, RightUnder);
    SEAD_ENUM(BallType, Bomb);
    KoopaChaseStateThrow(KoopaChase* pHost, const al::ActorInitInfo& rInfo, f32 throwSpeed);
    ~KoopaChaseStateThrow() override;
    void kill() override;
    bool tryStartThrow(bool isForce);
    bool canThrow() const;
    void startThrow(bool isLeft);
    void vanishAll();
    void doThrow(ThrowType type, BallType ball);
    void doThrowBomb(ThrowType type, bool isStartFuse, int throwCount);
    void exeThrowSign();
    void exeThrow();
private:
    KoopaChase* mHost;
    ThrowType mThrowType;
    BallType mBallType;
    int mThrowMode;
    al::DeriveActorGroup<BombBound>* mBombs;
    int mThrowCount;
    bool mIsForce;
    float mThrowSpeed;
    float mThrowDuration;
};
static_assert(sizeof(KoopaChaseStateThrow) == 0x48);
