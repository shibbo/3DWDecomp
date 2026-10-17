#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al
class GameDataHolder;
class RCSControlGuideBar;
class StageWipeKeeper;
class TopMenuScene;

/**
 * @brief How the top menu shows up when the top menu scene starts it.
 */
enum TopMenuAppearState : s32 {
    TopMenuAppearState_3DWorld = 0,          ///< Cursor on Super Mario 3D World.
    TopMenuAppearState_SingleMode = 1,       ///< Cursor on Bowser's Fury.
    TopMenuAppearState_SingleModeTitle = 2,  ///< Directly in the Bowser's Fury title menu.
    TopMenuAppearState_Reset = 3,            ///< Shown as 3D World by the next top menu.
};

/**
 * @brief Top menu layout choosing between Super Mario 3D World and Bowser's Fury.
 * @note Only the members used by already-decompiled callers are declared.
 */
class TopMenu : public al::LayoutActor {
public:
    TopMenu(TopMenuScene* pScene, const al::LayoutInitInfo& rInfo,
            GameDataHolder* pGameDataHolder, StageWipeKeeper* pStageWipeKeeper,
            const char* pSuffix);

    void control() override;
    void appear(TopMenuAppearState state);
    bool isEnd();
    bool isDecideSingleModeNewGame();
    bool isDecideSingleModeResume();
    void returnFromFileSelect(const char* pActionName, bool isShowGuide);

    /**
     * @brief Sets the control guide bar shown under the menu.
     * @param pGuideBar The control guide bar.
     */
    void setGuideBar(RCSControlGuideBar* pGuideBar) { mControlGuideBar = pGuideBar; }

private:
    u8 mUnreconstructed[0x158 - sizeof(al::LayoutActor)];
    RCSControlGuideBar* mControlGuideBar;
};
static_assert(sizeof(TopMenu) == 0x160);
