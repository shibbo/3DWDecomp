#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class KoopaChaseKoopa;
class KoopaChaseBattle;
class KoopaChaseMover;

class KoopaChase : public al::LiveActor {
public:
    static f32 getRunStartDistance();
    static f32 getRunStopDistance();
    void stopEffectAll();

    KoopaChaseKoopa* mKoopa;
    al::LiveActor* mTargetPlayer;
    al::LiveActor* mLookAtTargetPlayer;
    KoopaChaseBattle* mBattle;
    int mValue168;
    KoopaChaseMover* mMover;
    // Remaining actor members are not yet reconstructed.
    unsigned char mActorData178[0x30];
};
