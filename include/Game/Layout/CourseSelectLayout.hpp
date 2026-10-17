#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class ErrorViewer;
class HomeButton;
class LayoutInitInfo;
class NetworkSystem;
class PlayerHolder;
}  // namespace al

class CourseSelectDirector;
class CourseSelectSceneLayout;
class CourseSelectMiniatureCursor;
class PlayerEntryMini;
class GuideCourse;
class GameDataHolder;
class ICourseSelectActorController;

/**
 * @brief Holder of the course select map layouts.
 */
class CourseSelectLayout : public al::NerveExecutor {
public:
    CourseSelectLayout();
    ~CourseSelectLayout() override = default;
    void validateButton();
    bool isDecide() const;
    bool isShowPlayGuide() const;
    void changeUserPort(int srcPort, int dstPort);
    void exeAfterOpening();
    void exeDemo();
    void exePlay();
    bool isAfterOpeningState();

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
    CourseSelectSceneLayout* mSceneLayout = nullptr;
    CourseSelectMiniatureCursor* mCursor = nullptr;
    PlayerEntryMini* mPlayerEntry = nullptr;
    GuideCourse* mGuide = nullptr;
    bool mIsDemo = false;
    bool mHasStartedEntry = false;
    bool mIsAfterOpening = false;
};
static_assert(sizeof(CourseSelectLayout) == 0x38);
