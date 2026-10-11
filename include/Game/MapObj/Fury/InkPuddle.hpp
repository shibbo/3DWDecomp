#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Ink puddle left on the ground by Fury Bowser's ink attacks. */
class InkPuddle : public al::LiveActor {
public:
    explicit InkPuddle(const char* pName);

    void quickEnd();

private:
    u8 _148[0x150 - 0x148];
};
static_assert(sizeof(InkPuddle) == 0x150);
