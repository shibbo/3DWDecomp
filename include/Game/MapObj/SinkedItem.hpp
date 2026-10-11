#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Shards;
class SinkedItem : public al::LiveActor {
public:
    explicit SinkedItem(const char*);
    Shards* getShard() const { return mShard; }
private:
    u8 mUnknown144[0x1c];
    Shards* mShard;
    u8 mUnknown168[0x18];
};
static_assert(sizeof(SinkedItem) == 0x180);
