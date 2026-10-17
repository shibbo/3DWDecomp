#pragma once
#include <container/seadPtrArray.h>

namespace al { class ActorInitInfo; }
class KoopaChase;
class KoopaChaseMovePos;
class KoopaChaseMover {
public:
    KoopaChaseMover(KoopaChase* pHost, const al::ActorInitInfo& rInfo);
    void update();
    float calcDistanceToCurrentPoint() const;
    void tryKillWarpCubeIfAlive();
    void tryKillWarpDummyIfAlive();

    KoopaChase* mHost;
    bool mIsOverPoint;
    KoopaChaseMovePos* mCurrentPoint;
    KoopaChaseMovePos* mPreviousPoint;
    sead::PtrArray<KoopaChaseMovePos> mPoints;
};

static_assert(sizeof(KoopaChaseMover) == 0x30);
