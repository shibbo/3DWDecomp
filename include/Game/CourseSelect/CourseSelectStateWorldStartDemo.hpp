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
 * @brief Course select scene state: Plays the demo shown when a world is entered for the first time.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectStateWorldStartDemo : public al::NerveStateBase {
public:
    CourseSelectStateWorldStartDemo(CourseSelectScene* pScene, const al::ActorInitInfo& rInfo,
                                    al::SceneCameraInfo* pCameraInfo);

private:
    u8 _pad[0x58 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(CourseSelectStateWorldStartDemo) == 0x58);
