#pragma once

#include "Layout/TextBoxTextInfo.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class NetworkSystem;
class ErrorViewer;
class HomeButton;
}  // namespace al

class GameDataHolder;
class CourseSelectMiniatureCursor;
class CourseSelectDirector;
class CounterPlayerParts;
class ButtonItemStockParts;

/**
 * @brief Main layout of the course select map (counters, world/stage name, button guides and
 * the demo in/out animations).
 */
class CourseSelectSceneLayout : public al::LayoutActor {
public:
    CourseSelectSceneLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                            const CourseSelectMiniatureCursor* pCursor,
                            CourseSelectDirector* pDirector, al::NetworkSystem* pNetworkSystem,
                            al::ErrorViewer* pErrorViewer, al::HomeButton* pHomeButton);

    void updateGreenStarCount();
    void updateIllustItemCount();
    void updateCoinCount();
    void control() override;
    void updateMainGroupAnim();
    void updateGreenStarAnim();
    void updateButtonIcons();
    void setWorldId(s32 worldId);
    void updateWorldStageString();
    void updateStartDemoAnim();
    void updateEndDemoAnim();
    void startDemo(bool isSkipAnim);
    void endDemo(bool isSkipAnim);
    void startPause();
    void endPause();

    void exeWait();
    void exeAppear();
    void exePreAppear();
    void exeEnd();
    void exeStartDemo();
    void exeEndDemo();

private:
    const CourseSelectMiniatureCursor* mCursor;  // 0x128
    CounterPlayerParts* mCounterPlayer = nullptr;  // 0x130
    ButtonItemStockParts* mItemStock = nullptr;  // 0x138
    CourseSelectDirector* mDirector;  // 0x140
    s32 mMoveFrame = 0;  // 0x148
    s32 mWorldId = 0;  // 0x14c
    s32 mShownWorldId = 0;  // 0x150
    bool mIsShowStageName = false;  // 0x154
    s32 mShownCourseId = 0;  // 0x158
    GameDataHolder* mGameDataHolder;  // 0x160
    s32 mCounterShowFrame = 0;  // 0x168
    s32 mPlayerLife = -1;  // 0x16c
    s32 mCoinNum = -1;  // 0x170
    fix::TextBoxTextInfo mWorldTextInfo;  // 0x178
};

static_assert(sizeof(CourseSelectSceneLayout) == 0x198);
