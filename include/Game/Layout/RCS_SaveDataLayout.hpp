#pragma once

#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <math/seadMatrix.h>
#include <prim/seadBitFlag.h>
#include <prim/seadSafeString.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
}  // namespace al
class ButtonGroup;
class GameDataHolder;
class RCSControlGuideBar;
class WindowConfirm;

/**
 * @brief Save file select screen (save / load / delete) of the title screen and the pause menu.
 */
class RCS_SaveDataLayout : public al::LayoutActor {
public:
    RCS_SaveDataLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                       al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                       RCSControlGuideBar* pGuideBar, bool isPauseMenu);

    void appear() override;
    void updateFileSelectButtonState(bool isSingleMode);
    void control() override;
    void appearSave(s32 port, s32 worldId);
    void appearLoad(s32 port);
    void updateButtonValidation(s32 port, bool isValid);
    void appearDelete(s32 port, bool isAppearTitle);
    void resetButtons(s32 index);
    void setSelectLoad();
    void updateEffectColors();
    void forceExit();
    bool isWaitSelect() const;
    bool isWaitConfirm() const;
    void validateButtons();
    bool isLoading() const;
    bool isNeedLoad() const;
    bool isSaving() const;
    bool isEndBack() const;
    bool isEnding() const;

    /**
     * @brief Register the pause menu layout hidden while this layout is shown.
     * @param pLayout Pause menu frame layout.
     */
    void setPauseMenuLayout(al::LayoutActor* pLayout) { mPauseMenuLayout = pLayout; }

    /**
     * @brief Sets the control guide bar driven by the file select.
     * @param pGuideBar The control guide bar.
     */
    void setGuideBar(RCSControlGuideBar* pGuideBar) { mGuideBar = pGuideBar; }

    void exeAppear();
    void exeSelect();
    void exeConfirm();
    void exeNotify();
    void exeWaitConfirmEnd();
    void exeLoad();
    void exeFadeButtonInfoOut();
    void exeFadeButtonInfoIn();
    void exeSave();
    void exeDelete();
    void exeEnd();

private:
    GameDataHolder* mGameDataHolder;
    s32 mDecidedFileId = -1;
    ButtonGroup* mButtonGroup;
    s32 mPort = -1;
    void* mUnknown148 = nullptr;
    sead::Buffer<sead::Matrix34f> mEffectMtxs;
    sead::Buffer<sead::FixedSafeString<64>> mEffectNames;
    sead::BitFlag8 mEffectFlags;
    bool mIsPauseMenu;
    WindowConfirm* mWindowConfirm = nullptr;
    WindowConfirm* mWindowReport;
    al::ScreenCaptureExecutor* mScreenCaptureExecutor;
    RCSControlGuideBar* mGuideBar;
    al::LayoutActor* mPauseMenuLayout = nullptr;
    s32 mWorldId = -1;
};
static_assert(sizeof(RCS_SaveDataLayout) == 0x1a8);
