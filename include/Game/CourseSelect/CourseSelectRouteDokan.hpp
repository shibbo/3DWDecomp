#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief Route dokan of the course-select map, carrying the players along a rail to another
 * place of the map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectRouteDokan : public al::LiveActor {
public:
    explicit CourseSelectRouteDokan(const char* pName);

    void createStopper(const al::ActorInitInfo& rInfo);
    void startAppearEffectW7();
    void active();
    void deactive();
    void setEnableInput();

    /** @brief Gets whether the dokan is deactivated. @return true if deactivated. */
    bool isDeactive() const { return mIsDeactive; }

private:
    u8 _144[0x158 - 0x144];
    bool mIsDeactive;  // 0x158
    u8 _159[0x160 - 0x159];
};

static_assert(sizeof(CourseSelectRouteDokan) == 0x160);
