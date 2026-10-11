#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GuideObj : public al::LiveActor {
public:
    explicit GuideObj(const char* name);
    void startPause();
    void endPause();
private:
    u8 mUnreconstructed[0x14];
};
static_assert(sizeof(GuideObj) == 0x158);
