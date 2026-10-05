#pragma once

#include "Library/LiveActor/LiveActor.hpp"

// Caller declarations; the actor's fields have not yet been reconstructed.
class MysteryBox : public al::LiveActor {
public:
    void setDestMysteryBox(const MysteryBox* pDest);
};
