#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DisasterSpikeDirector;
class SuperBowser;

/// Spike Fury Bowser launches from his shell onto the island the player is on.
class DisasterSpikeLaunch : public al::LiveActor {
public:
    DisasterSpikeLaunch(const char* pName, DisasterSpikeDirector* pDirector);

    void init(const al::ActorInitInfo& info) override;
    virtual void appear(SuperBowser* bowser);
    void forceKill();
    void exeShoot();

private:
    DisasterSpikeDirector* mDirector;
    SuperBowser* mBowser = nullptr;
    sead::Vector3f mStart = sead::Vector3f::zero;
    sead::Vector3f mEnd = sead::Vector3f::zero;
};

static_assert(sizeof(DisasterSpikeLaunch) == 0x170);
