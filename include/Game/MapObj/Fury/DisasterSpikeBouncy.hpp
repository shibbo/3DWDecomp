#pragma once

#include "MapObj/DisasterSpike.hpp"

class DisasterSpikeBouncy : public DisasterSpike {
public:
    DisasterSpikeBouncy(const char* pName);

private:
    u8 _318[0x28];
};

static_assert(sizeof(DisasterSpikeBouncy) == 0x340);
