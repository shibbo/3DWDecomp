#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class GigaBellManager;

/**
 * @brief The three Giga Bells fused together, rung by Plessie in the last battle against
 * Fury Bowser.
 * @note Only what reconstructed code needs is declared so far.
 */
class GigaBell3in1 : public al::LiveActor {
public:
    explicit GigaBell3in1(const char* pName);

    void setGigaBellManager(GigaBellManager* pManager);
    void reset();

private:
    u8 _148[0x210 - 0x148];
};

static_assert(sizeof(GigaBell3in1) == 0x210);
