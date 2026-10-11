#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class LayoutInitInfo;
class PlayerHolder;
class SceneCameraInfo;
struct GameSystemInfo;
}  // namespace al

class CourseSelectDirector;
class CourseSelectScene;

/**
 * @brief Course select scene state: Plays the event shown when returning to the map after the ending.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectStateAfterEndingEvent : public al::NerveStateBase {
public:
    CourseSelectStateAfterEndingEvent(CourseSelectScene* pScene, CourseSelectDirector* pDirector);

private:
    u8 _pad[0x30 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(CourseSelectStateAfterEndingEvent) == 0x30);
