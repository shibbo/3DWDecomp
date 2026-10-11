#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class Koura;

/** @brief Koopa Troopa: a NokonokoNaked body that wears a Koura shell. Partial layout. */
class Nokonoko : public al::LiveActor {
public:
    explicit Nokonoko(const char* pName);

    bool tryStartWear();
    void attachKoura();

    Koura* getKoura() const { return mKoura; }

private:
    u8 mUnreconstructed[0x14];
    Koura* mKoura;  // 0x158
};

static_assert(sizeof(Nokonoko) == 0x160);
