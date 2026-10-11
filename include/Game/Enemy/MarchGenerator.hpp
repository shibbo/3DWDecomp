#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class MarchGenerator : public al::LiveActor {
public:
    static s32 calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo);
    explicit MarchGenerator(const char* pName);

private:
    u8 mUnreconstructed[0x6c];
};
static_assert(sizeof(MarchGenerator) == 0x1b0);
