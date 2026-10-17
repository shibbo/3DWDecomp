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
 * @brief Course select scene state: Plays the transition back from a course.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectStateStageExit : public al::NerveStateBase {
public:
    CourseSelectStateStageExit(CourseSelectScene* pScene, al::SceneCameraInfo* pCameraInfo);

private:
    u8 _pad[0x40 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(CourseSelectStateStageExit) == 0x40);
