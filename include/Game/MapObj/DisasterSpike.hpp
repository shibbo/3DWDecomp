#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class DisasterSpikeDirector;

class DisasterSpike : public al::LiveActor {
public:
    DisasterSpike(const char* pName);
    void setDisasterSpikeDirector(DisasterSpikeDirector* pDirector);
    bool isTriggered();
    bool tryAppear(bool isImmediate);
    void setKillOutOfView(bool isKill);
    void setTriggered(bool isTriggered);

private:
    // Actor state is not yet reconstructed; allocation size is verified at its callers.
    alignas(8) u8 _148[0x1D0];
};

static_assert(sizeof(DisasterSpike) == 0x318);
