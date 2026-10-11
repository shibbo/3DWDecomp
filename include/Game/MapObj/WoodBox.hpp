#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class WoodBox : public al::LiveActor {
public:
    static s32 calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo);
    explicit WoodBox(const char* pName);

private:
    u8 mUnreconstructed[0x2c];
};
static_assert(sizeof(WoodBox) == 0x170);
