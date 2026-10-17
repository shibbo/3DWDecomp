#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class HeadIslandClear : public al::LayoutActor {
public:
    HeadIslandClear(const char*, const al::LayoutInitInfo&, const char*, bool);
    void end();
    void updateTextBoxes(int, int);
    void killEffect();
private:
    u8 mUnreconstructed[0x37];
};
static_assert(sizeof(HeadIslandClear) == 0x158);
