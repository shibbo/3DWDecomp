#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Fairy princess shown next to Bowser's castle on the course select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectFairy : public al::LiveActor {
public:
    CourseSelectFairy(const char* pName, s32 worldId, bool isHide);

    void startWorldClearDemo();
    void startWorldStartDemo();
    void endWorldStartDemo();
    void startPause();
    void endPause();

private:
    u8 _144[0x1d0 - 0x144];
};

static_assert(sizeof(CourseSelectFairy) == 0x1d0);
