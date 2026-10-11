#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectMiniature;

/**
 * @brief Clear flag shown next to a cleared course miniature.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectFlag : public al::LiveActor {
public:
    CourseSelectFlag(const char* pName, CourseSelectMiniature* pMiniature);

    void updateVisibility(bool isVisible);
    void startAppear();
    void startShow();
    void startClearDemo();
    void startWaitClearDemo();

private:
    u8 _144[0x158 - 0x144];
};

static_assert(sizeof(CourseSelectFlag) == 0x158);
