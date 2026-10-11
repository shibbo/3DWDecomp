#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectMiniature;

/**
 * @brief Green star lock closing a course miniature until enough green stars are collected.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectLock : public al::LiveActor {
public:
    CourseSelectLock(const char* pName, CourseSelectMiniature* pMiniature, s32 greenStarNum);

    void startAppear();
    void startShow();
    void startDisable();
    bool isUnlock() const;

private:
    u8 _144[0x168 - 0x144];
};

static_assert(sizeof(CourseSelectLock) == 0x168);
