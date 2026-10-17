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
 * @brief Course select scene state: Plays the transition into a course.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectStateStageEnter : public al::NerveStateBase {
public:
    CourseSelectStateStageEnter(CourseSelectScene* pScene, const al::LayoutInitInfo& rInfo,
                                al::SceneCameraInfo* pCameraInfo,
                                const al::GameSystemInfo* pSystemInfo);
    bool isCancel() const;

private:
    u8 _pad[0x60 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(CourseSelectStateStageEnter) == 0x60);
