#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Rocket of the course-select map, carrying the players to the next world.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far. A stub
 * definition also exists in Scene/ProjectActorFactoryTypes.hpp; never include both.
 */
class CourseSelectRocket : public al::LiveActor {
public:
    explicit CourseSelectRocket(const char* pName);

    void startOpenDemo();
    bool isDemo() const;

private:
    u8 _148[0x210 - 0x148];
};

static_assert(sizeof(CourseSelectRocket) == 0x210);
