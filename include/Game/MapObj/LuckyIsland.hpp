#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class LuckyIslandController;
class LuckyIsland : public al::LiveActor {
public:
    explicit LuckyIsland(const char* pName);
    void DisasterAppear();
    void DisasterDisappear();
    void setController(LuckyIslandController*);
    void StartDemo();
    void disableCollision();
private:
    u8 mUnreconstructed[0x1ec];
};
static_assert(sizeof(LuckyIsland) == 0x330);
