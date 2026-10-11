#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief The seagull flock perched around an island in Bowser's Fury.
 * @note Only what reconstructed code needs is declared so far.
 */
class CatGull : public al::LiveActor {
public:
    explicit CatGull(const char* pName);

    void startFlyReturn();

    /**
     * @brief Get the island the gulls belong to.
     * @return The island (zone) number.
     */
    s32 getIslandId() const { return mIslandId; }

private:
    u8 mUnknown144[0x180 - 0x144];
    s32 mIslandId;  // 0x180
};
