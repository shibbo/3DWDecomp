#pragma once
#include "System/GameDataHolderWriter.hpp"
#include "Library/LiveActor/LiveActor.hpp"
class BoxPropeller : public al::LiveActor {
public:
    explicit BoxPropeller(const char*);
    bool isCarry();
    static void resetPropellerJumpGuideMessage(GameDataHolderWriter writer);
    void setAppearFromHipDrop() { mAppearFromHipDrop = true; }
private:
    u8 mUnknown144[0x21];
    bool mAppearFromHipDrop;
    u8 mUnknown166[0x42];
};
static_assert(sizeof(BoxPropeller) == 0x1a8);
