#pragma once

#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <math/seadMatrix.h>
#include <prim/seadBitFlag.h>
#include <prim/seadSafeString.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class Nerve;
}  // namespace al
class ButtonBackParts;
class ButtonGroup;
class ButtonShortCutAndTouchParts;
class GameDataHolder;
class GuideWindowParts;
class WindowConfirm;

/**
 * @brief Save file select screen of the title menu (3D World): choose, copy or delete a file.
 */
class FileSelect : public al::LayoutActor {
public:
    /**
     * @brief How the file select was left.
     * @note Passed as a 64-bit value (`mov x19, x1`) but stored as 32 bits.
     */
    enum Result : s64 {
        Result_None = 0,    ///< Still selecting.
        Result_Back = 1,    ///< Closed with the back button.
        Result_Decide = 2,  ///< A file was chosen to play.
        Result_Copy = 3,    ///< A copy source and destination were chosen.
        Result_Delete = 4,  ///< A file was chosen to delete.
    };

    FileSelect(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder);

    void appear() override;
    void control() override;
    void updateFileSelectButtonState();
    void tryHidePlusMinus();
    void backToSelect();
    void showAndNextNerve(const al::Nerve* pNerve);
    void backToCopy();
    void changeFileSelectButtonStateCopyDst();
    void backToDelete();
    void changeFileSelectButtonStateDelete();
    void startOutAnim(s32 fileIndex);
    void startInAnim(s32 fileIndex);
    bool isEndInOutAnim(s32 fileIndex) const;
    s32 getSelectedFileIndex() const;
    void getCopyTargetIndex(s32* pSrcIndex, s32* pDstIndex) const;
    void setControllerPort(s32 port);
    bool isEndBack() const;
    bool isEndNext() const;
    bool isWaitCopy() const;
    bool isWaitDelete() const;

    void exeAppear();
    void changeFileSelectButtonStateSelect();
    void exeSelect();
    void lockButtonEachOther();
    void goToEnd(Result result);
    void hideAndNextNerve(const al::Nerve* pNerve);
    void exeSelectBack();
    void exeCopyIn();
    void changeFileSelectButtonStateCopySrc();
    void exeCopy();
    void exeCopyDstIn();
    void exeCopyDst();
    void goToWait(Result result);
    void exeDeleteIn();
    void exeDelete();
    void exePartsHide();
    void exePartsShow();
    void exeWait();
    void exeConfirm();
    void exeEnd();
    void exeEndWait();
    void tryDisappearPlusMinus();

private:
    const GameDataHolder* mGameDataHolder;
    s32 mCopySrcIndex = 0;
    s32 mCopyDstIndex = 0;
    s32 mResult = Result_None;
    GuideWindowParts* mGuideWindow = nullptr;
    ButtonBackParts* mBackButton = nullptr;
    ButtonGroup* mButtonGroup = nullptr;
    ButtonShortCutAndTouchParts* mCopyButton = nullptr;
    ButtonShortCutAndTouchParts* mDeleteButton = nullptr;
    s32 mPort = -1;
    const al::Nerve* mNextNerve = nullptr;
    sead::Buffer<sead::Matrix34f> mEffectMtxs;
    sead::Buffer<sead::FixedSafeString<64>> mEffectNames;
    sead::BitFlag8 mEffectFlags;
    bool mIsDecideSePlayed = false;
    WindowConfirm* mWindowConfirm = nullptr;
};
static_assert(sizeof(FileSelect) == 0x1a8);
