#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Dokan of the course-select map warping the players to another world.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectWorldWarpDokan : public al::LiveActor {
public:
    explicit CourseSelectWorldWarpDokan(const char* pName);

private:
    u8 _144[0x158 - 0x144];
};

static_assert(sizeof(CourseSelectWorldWarpDokan) == 0x158);
