#pragma once

#include <basis/seadTypes.h>

namespace al {
class ErrorViewer;
class HomeButton;
class LayoutInitInfo;
class NetworkSystem;
class PlayerHolder;
}  // namespace al

class CourseSelectDirector;
class CourseSelectSceneLayout;
class GameDataHolder;
class ICourseSelectActorController;

/**
 * @brief Holder of the course select map layouts.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectLayout {
public:
    CourseSelectLayout();

    void init(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
              al::PlayerHolder* pPlayerHolder, CourseSelectDirector* pDirector,
              al::NetworkSystem* pNetworkSystem, al::ErrorViewer* pErrorViewer,
              al::HomeButton* pHomeButton);
    void startDemo(bool isSkipAnim);
    void invalidateButton();
    bool isEnableEnterCourse() const;
    bool isActiveMiniatureCursor() const;
    bool setSelectedActorController(ICourseSelectActorController* pController, bool isDrc,
                                    bool isForce);
    void startDecide();
    void setWorldId(s32 worldId);
    void appear();
    void update();
    void startPause();
    void endPause();
    bool isDemo() const;
    void endDemo(bool isSkipAnim);

    /**
     * Gets the main layout of the course select map.
     * @return The scene layout.
     */
    CourseSelectSceneLayout* getSceneLayout() const { return mSceneLayout; }

    /** @brief Marks the layouts as shown after the opening demo. */
    void setAfterOpening() { mIsAfterOpening = true; }

private:
    u8 _0[0x10];
    CourseSelectSceneLayout* mSceneLayout;  // 0x10
    u8 _18[0x32 - 0x18];
    bool mIsAfterOpening;  // 0x32
    u8 _33[0x38 - 0x33];
};

static_assert(sizeof(CourseSelectLayout) == 0x38);
