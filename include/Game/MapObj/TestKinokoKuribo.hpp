#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class TestKinokoKuribo : public al::LiveActor {
public:
    explicit TestKinokoKuribo(const char*);
    void setHeadWorn(bool worn) { mHeadWorn = worn; }
private:
    unsigned char _144[0x68];
    bool mHeadWorn;
};
static_assert(sizeof(TestKinokoKuribo) == 0x1b0);
