#pragma once

#include "Player/Normal/PlayerActor.hpp"

namespace al {
class ActorInitInfo;
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Player actor walking on the course select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectPlayerActor : public PlayerActor {
public:
    explicit CourseSelectPlayerActor(const sead::Matrix34f* pViewMtx);

    void createBubble(const al::ActorInitInfo& rInfo);
    void initCourseSelectPlayer(const al::LayoutInitInfo& rInfo, bool isKiosk);
    void hidePlayer();
    void showPlayer();
    bool isInCourseSelectBubble() const;

private:
    u8 _648[0x680 - 0x648];
};

static_assert(sizeof(CourseSelectPlayerActor) == 0x680);
