#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Coin : public al::LiveActor {
public:
    explicit Coin(const char*);
    void resetBaseQuat();
    void appearJump();
private:
    unsigned char _144[0x44];
};
static_assert(sizeof(Coin) == 0x188);
