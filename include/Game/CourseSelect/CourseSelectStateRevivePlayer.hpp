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
 * @brief Course select scene state: Revives the players that died in the last course.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectStateRevivePlayer : public al::NerveStateBase {
public:
    CourseSelectStateRevivePlayer(CourseSelectScene* pScene, bool isAfterEnding);

private:
    u8 _pad[0x30 - sizeof(al::NerveStateBase)];
};

static_assert(sizeof(CourseSelectStateRevivePlayer) == 0x30);
