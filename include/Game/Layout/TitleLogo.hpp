#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al
class GameDataHolder;
class RCSControlGuideBar;

/**
 * @brief Title logo layout with the title screen menu (new game, resume, Luigi Bros...).
 * @note Only the members used by already-decompiled callers are declared.
 */
class TitleLogo : public al::LayoutActor {
public:
    TitleLogo(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder);

    void updateLuigiButtonState();
    void startAppearAnim(bool isSkipAnim);
    void appearFromMenu();
    bool isStartNewGame() const;
    bool isDecideResume();
    bool isEnd() const;
    bool isConfirmLuigi() const;
    bool isStartLuigi() const;

    /**
     * @brief Gets whether the player chose to return to the top menu.
     * @return True if the top menu was chosen.
     */
    bool isReturnToTopMenu() const { return mIsReturnToTopMenu; }

    /**
     * @brief Gets the file slot a new game is started in.
     * @return The file id, negative when every file is in use.
     */
    s32 getNewFileId() const { return mNewFileId; }

    /**
     * @brief Sets the control guide bar shown below the logo.
     * @param pGuideBar The control guide bar.
     */
    void setGuideBar(RCSControlGuideBar* pGuideBar) { mGuideBar = pGuideBar; }

private:
    u8 _121[0x148 - 0x121];
    bool mIsReturnToTopMenu;  // 0x148
    u8 _149[0x15c - 0x149];
    s32 mNewFileId;  // 0x15c
    RCSControlGuideBar* mGuideBar;  // 0x160
};

static_assert(sizeof(TitleLogo) == 0x168);
