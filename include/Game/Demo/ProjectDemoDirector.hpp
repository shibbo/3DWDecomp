#pragma once

#include "Library/Sequence/DemoDirector.hpp"

namespace al {
class PlayerHolder;
}  // namespace al

class DemoPlayerControllerHolder;
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

    bool requestStartDemoCamera(const al::LiveActor* pActor, const char* pName);
    void requestEndDemoCamera(const al::LiveActor* pActor);
    bool requestStartDemoMovingCamera(const al::LiveActor* pActor, const char* pName);
    void requestEndDemoMovingCamera(const al::LiveActor* pActor);
    bool requestStartDemoIntro(const al::LiveActor* pActor, const char* pName);
    void requestEndDemoIntro(const al::LiveActor* pActor);
    bool requestStartDemoPlayer(const al::LiveActor* pActor);
    void requestEndDemoPlayer(const al::LiveActor* pActor);
    bool requestStartDemoBinding(const al::LiveActor* pActor);
    void requestEndDemoBinding(const al::LiveActor* pActor);
    bool requestStartDemoCutscene(const al::LiveActor* pActor);
    void requestEndDemoCutscene(const al::LiveActor* pActor);
    bool requestStartDemoInGameCutscene(const al::LiveActor* pActor);
    void requestEndDemoInGameCutscene(const al::LiveActor* pActor);
    bool requestStartDemoPlayerCutscene(const al::LiveActor* pActor);
    void requestEndDemoPlayerCutscene(const al::LiveActor* pActor);
    bool isActiveDemoCamera() const;
    bool isActiveDemoMovingCamera() const;
    bool isActiveDemoIntro() const;
    bool isActiveDemoPlayer() const;
    bool isActiveDemoBinding() const;
    bool isActiveDemoPlayerCutscene() const;
    bool isActiveDemoCutscene() const;
    bool isActiveDemoInGameCutscene() const;

    /** @brief Gets the demo player controller holder. @return The player controller holder. */
    DemoPlayerControllerHolder* getPlayerControllerHolder() const {
        return mPlayerControllerHolder;
    }

    /** @brief Gets the demo player model director. @return The player model director. */
    DemoPlayerModelDirector* getPlayerModelDirector() const { return mPlayerModelDirector; }

private:
    DemoPlayerControllerHolder* mPlayerControllerHolder;
    DemoPlayerModelDirector* mPlayerModelDirector;
    u8 _100[0x108 - 0x100];
};

static_assert(sizeof(ProjectDemoDirector) == 0x108);
