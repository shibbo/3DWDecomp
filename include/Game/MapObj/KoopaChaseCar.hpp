#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadBuffer.h>
#include <prim/seadBitFlag.h>

class KoopaChaseCar : public al::LiveActor {
public:
    explicit KoopaChaseCar(const char* pName);
    ~KoopaChaseCar() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startClipped() override;
    void exeWait();
    void exeReaction();
private:
    sead::BitFlag16 mReactingPlayers;
    sead::Buffer<int> mReactionTimers;
    int mReactionCount = 0;
    bool mIsPlayingMusic = false;
};
static_assert(sizeof(KoopaChaseCar) == 0x160);
