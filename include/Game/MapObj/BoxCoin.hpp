#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BoxCoin : public al::LiveActor {
public:
    explicit BoxCoin(const char*);
    void applyVisDark();
    void appearHipDrop(bool);
    void disappear(bool);
    bool isCarried() const;
private:
    u8 mUnreconstructed[0x6c];
};
static_assert(sizeof(BoxCoin) == 0x1b0);
