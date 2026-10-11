#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DisasterSpikeDirector;

/// Spike Fury Bowser launches from his shell onto the island the player is on.
class DisasterSpikeLaunch : public al::LiveActor {
public:
    DisasterSpikeLaunch(const char* pName, DisasterSpikeDirector* pDirector);

    void forceKill();

private:
    u8 _144[0x170 - 0x144];
};

static_assert(sizeof(DisasterSpikeLaunch) == 0x170);
