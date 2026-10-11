#pragma once
#include <container/seadPtrArray.h>

class KoopaChase;
class KoopaChaseMovePos;
class KoopaChaseMover {
public:
    float calcDistanceToCurrentPoint() const;
    void tryKillWarpCubeIfAlive();
    void tryKillWarpDummyIfAlive();

    KoopaChase* mHost;
    bool mIsOverPoint;
    KoopaChaseMovePos* mCurrentPoint;
    KoopaChaseMovePos* mPreviousPoint;
    sead::PtrArray<KoopaChaseMovePos> mPoints;
};
