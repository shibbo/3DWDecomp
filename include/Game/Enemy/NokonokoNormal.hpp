#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Nokonoko;

/** @brief Koopa Troopa wearing its shell. Partial layout. */
class NokonokoNormal : public al::LiveActor {
public:
    NokonokoNormal(const char* pName, Nokonoko* pNokonoko);
    void init(const al::ActorInitInfo& rInfo) override;
    void startAttachKoura();
    void startMove();
    void switchToNaked();
private:
    u8 _144[0x5c];
};
static_assert(sizeof(NokonokoNormal) == 0x1a0);
