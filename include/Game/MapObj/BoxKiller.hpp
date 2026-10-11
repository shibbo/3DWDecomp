#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BoxKiller : public al::LiveActor {
public:
    explicit BoxKiller(const char*, int = 0, int = 0);
    bool isCarrying() const;
    void setAppearFromHipDrop() { mAppearFromHipDrop = true; }
private:
    u8 mUnknown144[0x2c];
    bool mAppearFromHipDrop;
    u8 mUnknown171[0x37];
};
static_assert(sizeof(BoxKiller) == 0x1a8);
