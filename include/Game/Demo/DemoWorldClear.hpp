#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class CourseSelectMiniature;

/**
 * @brief World clear demo played at Bowser's castle miniature of the course-select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DemoWorldClear : public al::LiveActor {
public:
    explicit DemoWorldClear(CourseSelectMiniature* pMiniature);

    void startDemo();
    bool isEndDemo() const;

private:
    u8 _144[0x168 - 0x144];
};

static_assert(sizeof(DemoWorldClear) == 0x168);
