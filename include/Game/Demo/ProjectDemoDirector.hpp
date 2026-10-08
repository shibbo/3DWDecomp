#pragma once

#include "Library/Sequence/DemoDirector.hpp"

namespace al {
class PlayerHolder;
}  // namespace al

class DemoPlayerModelDirector;

/**
 * @brief Project-side demo director: knows the names of the game's demo kinds.
 * @note Only the constructor and the demo-name getters are declared so far.
 */
class ProjectDemoDirector : public al::DemoDirector {
public:
    ProjectDemoDirector(al::PlayerHolder* pPlayerHolder, s32 maxActors);

    static const char* getDemoNameCamera();
    static const char* getDemoNameMovingCamera();
    static const char* getDemoNameIntro();
    static const char* getDemoNamePlayer();
    static const char* getDemoNameBinding();
    static const char* getDemoNameCutscene();
    static const char* getDemoNameInGameCutscene();
    static const char* getDemoNamePlayerCutscene();

    /** @brief Gets the demo player model director. @return The player model director. */
    DemoPlayerModelDirector* getPlayerModelDirector() const { return mPlayerModelDirector; }

private:
    u8 _f0[0xf8 - 0xf0];
    DemoPlayerModelDirector* mPlayerModelDirector;
    u8 _100[0x108 - 0x100];
};

static_assert(sizeof(ProjectDemoDirector) == 0x108);
