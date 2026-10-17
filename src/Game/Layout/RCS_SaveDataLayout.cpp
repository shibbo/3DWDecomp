#include "Layout/RCS_SaveDataLayout.hpp"

#include <gfx/seadColor.h>
#include <math/seadMathCalcCommon.h>
#include <time/seadCalendarTime.h>
#include <time/seadDateTime.h>
#include "Layout/ButtonGroup.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Layout/ProjectReplaceTagProcessor.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Layout/Switch/ButtonRCSFileParts.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenCaptureExecutor.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/IslandDataList.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"

/// Declares a nerve of this layout whose state function has a different name than the nerve.
#define RCS_SAVE_DATA_LAYOUT_NERVE_DECL(Action, Func)                                             \
    class RCS_SaveDataLayoutNrv##Action : public al::Nerve {                                       \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<RCS_SaveDataLayout>())->exe##Func();                               \
        }                                                                                          \
    };

namespace {
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(End, End)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(Delete, Delete)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(Save, Save)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(FadeButtonInfoIn, FadeButtonInfoIn)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(FadeButtonInfoInAfterDelete, FadeButtonInfoIn)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(AppearSave, Appear)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(AppearLoad, Appear)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(AppearDelete, Appear)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(SelectLoad, Select)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(Load, Load)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(EndBack, End)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(Notify, Notify)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(NotifyPlayingFile, Notify)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(SelectDelete, Select)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(SelectSave, Select)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(ConfirmDelete, Confirm)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(ConfirmSave, Confirm)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(ConfirmLoad, Confirm)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(EndNeedLoad, End)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(WaitConfirmEndLoad, WaitConfirmEnd)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(WaitConfirmEndSave, WaitConfirmEnd)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(WaitConfirmEndDelete, WaitConfirmEnd)
RCS_SAVE_DATA_LAYOUT_NERVE_DECL(FadeButtonInfoOut, FadeButtonInfoOut)

NERVES_MAKE_STRUCT(RCS_SaveDataLayout, End, Delete, Save, FadeButtonInfoIn,
                   FadeButtonInfoInAfterDelete, AppearSave, AppearLoad, AppearDelete, SelectLoad,
                   Load, EndBack, Notify, NotifyPlayingFile, SelectDelete, SelectSave,
                   ConfirmDelete, ConfirmSave, ConfirmLoad, EndNeedLoad, WaitConfirmEndLoad,
                   WaitConfirmEndSave, WaitConfirmEndDelete, FadeButtonInfoOut)

/// Number of save file buttons.
constexpr s32 cFileNum = 4;

/// Highest lost life count shown on a file button.
constexpr s32 cLivesLostMax = 99999;

/// Island number used for the open water between the islands.
constexpr s32 cIslandNoOpenWater = 17;

/// Unlocked phase from which island 1 is shown by name.
constexpr s32 cPhaseShowFirstIslandName = 8;

/// Unlocked phases in which a loaded file resumes from the Plessie chase.
constexpr s32 cPhasePlessieChase1 = 7;
constexpr s32 cPhasePlessieChase2 = 10;

/// Global alpha above which the selected button shows its twinkle effect.
constexpr f32 cTwinkleAlphaThreshold = 0.5f;

typedef sead::WFixedSafeString<16> WString16;
typedef sead::WFixedSafeString<32> WString32;
typedef sead::WFormatFixedSafeString<16> WFormatString16;

/**
 * @brief Reads the slot of the save file currently being played.
 * @param pHolder Game data holder.
 * @return The playing file slot of the active game mode.
 */
inline s32 getPlayingFileId(const GameDataHolder* pHolder) {
    return pHolder->isSingleMode() ? pHolder->getSingleFile()->getFileId() :
                                     pHolder->getPlayingFile()->getFileId();
}

/**
 * @brief Accesses a save file of the active game mode.
 * @param pHolder Game data holder.
 * @param isSingleMode True for the Bowser's Fury files.
 * @param fileId Save file slot.
 * @return The save file.
 */
inline GameDataFileBase* getDataFile(const GameDataHolder* pHolder, bool isSingleMode,
                                     s32 fileId) {
    if (isSingleMode) {
        return pHolder->getSingleModeDataFile(fileId);
    }

    return pHolder->getGameDataFile(fileId);
}

/**
 * @brief Finds the first file that is not new, starting from a given file.
 * @param pHolder Game data holder.
 * @param isSingleMode True for the Bowser's Fury files.
 * @param fileId Save file slot to start from.
 * @param fileNum Number of save file slots.
 * @return The first used file slot, or fileId when every file is new.
 */
inline s32 findUsedFileId(const GameDataHolder* pHolder, bool isSingleMode, s32 fileId,
                          s32 fileNum) {
    if (!getDataFile(pHolder, isSingleMode, fileId)->isNewFile()) {
        return fileId;
    }

    for (s32 i = 1; i < fileNum; i++) {
        s32 nextFileId = (fileId + i) % fileNum;
        if (!getDataFile(pHolder, isSingleMode, nextFileId)->isNewFile()) {
            return nextFileId;
        }
    }

    return fileId;
}

/**
 * @brief Appends one black star mark per clear star level.
 * @param pStr String receiving the stars.
 * @param level Clear star level.
 */
inline void appendClearStars(WString16* pStr, s32 level) {
    for (s32 i = 0; i < level; i++) {
        pStr->append(sead::WSafeString(LayoutFontUtil::getMessageFontMarkBlackStar()));
    }
}
}  // namespace

/**
 * @brief Creates the file select layout with its four file buttons and confirmation windows.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data holder owning the save files.
 * @param pScreenCaptureExecutor Screen capture used behind the confirmation windows.
 * @param pGuideBar Control guide bar, or nullptr.
 * @param isPauseMenu True when opened from the pause menu instead of the title screen.
 */
RCS_SaveDataLayout::RCS_SaveDataLayout(const al::LayoutInitInfo& rInfo,
                                       GameDataHolder* pGameDataHolder,
                                       al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                                       RCSControlGuideBar* pGuideBar, bool isPauseMenu)
    : al::LayoutActor("RCS_SaveData"), mGameDataHolder(pGameDataHolder),
      mIsPauseMenu(isPauseMenu), mScreenCaptureExecutor(pScreenCaptureExecutor),
      mGuideBar(pGuideBar) {
    al::initLayoutActor(this, rInfo, "RCS_SaveData", isPauseMenu ? "PauseMenu" : nullptr);
    initNerve(&NrvRCS_SaveDataLayout.End, 0);
    mPort = al::getMainControllerPort();
    mEffectMtxs.tryAllocBuffer(cFileNum, nullptr);
    mEffectNames.tryAllocBuffer(cFileNum, nullptr);
    for (s32 i = 0; i < cFileNum; i++) {
        mEffectMtxs[i] = sead::Matrix34f::ident;
    }

    mButtonGroup = new ButtonGroup(rInfo, this, "RCS_SaveData", nullptr, false);
    mButtonGroup->registerButton(this,
                                 new ButtonRCSFileParts(rInfo, "Button1", "ParButton1", this));
    mButtonGroup->registerButton(this,
                                 new ButtonRCSFileParts(rInfo, "Button2", "ParButton2", this));
    mButtonGroup->registerButton(this,
                                 new ButtonRCSFileParts(rInfo, "Button3", "ParButton3", this));
    mButtonGroup->registerButton(this,
                                 new ButtonRCSFileParts(rInfo, "Button4", "ParButton4", this));
    mButtonGroup->resetDestination("Button1", "Button4", "Button2", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Button2", "Button1", "Button3", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Button3", "Button2", "Button4", nullptr, nullptr, true);
    mButtonGroup->resetDestination("Button4", "Button3", "Button1", nullptr, nullptr, true);
    mButtonGroup->setPort(mPort);
    mWindowConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "SaveData", false);
    mWindowReport = new WindowConfirm(WindowConfirmType_Report, rInfo, "PauseMenu", true);
    kill();
}

/** @brief Shows the layout with the current state of every save file. */
void RCS_SaveDataLayout::appear() {
    al::LayoutActor::appear();
    if (mIsPauseMenu) {
        const GameDataFileBase* playingFile =
            mGameDataHolder->isSingleMode() ?
                static_cast<GameDataFileBase*>(mGameDataHolder->getSingleFile()) :
                mGameDataHolder->getPlayingFile();
        al::startAction(mButtonGroup->getButton(playingFile->getFileId()), "Playing", "Playing");
        al::hidePaneNoRecursive(this, "WinFrame");
    } else {
        al::startSe(this, "Appear");
    }

    updateFileSelectButtonState(mGameDataHolder->isSingleMode());
    mButtonGroup->hideCursor();
    mButtonGroup->invalidate();
    mWorldId = -1;
}

/**
 * @brief Updates every file button with the contents of its save file and links the cursor
 *        destinations between the selectable buttons.
 * @param isSingleMode True to show the Bowser's Fury files, false for the 3D World files.
 */
void RCS_SaveDataLayout::updateFileSelectButtonState(bool isSingleMode) {
    s32 usedFileNo[cFileNum] = {-1, -1, -1, -1};
    s32 usedFileNum = 0;
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButtonUnsafe(i);
        button->setCustomSE(nullptr);
        if (mGameDataHolder->isNewFile(i)) {
            if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn)) {
                button->setCustomSE("Decided_NewFile");
            }

            al::startAction(button, "SetFileState_New_NoData", "State");
            if (mEffectFlags.isOnBit(i)) {
                al::tryDeleteEffect(this, mEffectNames[i].cstr());
                mEffectNames[i].clear();
                mEffectFlags.resetBit(i);
            }

            if (mIsPauseMenu && (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoOut) ||
                                 al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn) ||
                                 al::isNerve(this, &NrvRCS_SaveDataLayout.AppearSave))) {
                button->enable();
                button->validate();
            } else {
                button->disable();
                button->invalidate();
            }

            continue;
        }

        usedFileNo[usedFileNum] = i + 1;
        al::startAction(button, "SetFileState_Used", "State");
        al::startAction(button, isSingleMode ? "SetMode_Single" : "SetMode_3DWorld", "Mode");

        const char* dateTimePane = isSingleMode ? "TxtSaveDateTime_Single" : "TxtSaveDateTime";
        ProjectReplaceTagProcessor processor;
        WString32 dateTimeText;
        sead::CalendarTime time;
        if (SaveDataAccessFunction::isEnableSave(mGameDataHolder)) {
            GameDataFileBase* file = getDataFile(mGameDataHolder, isSingleMode, i);
            sead::DateTime lastPlayingTime = file->getLastPlayingTime();
            lastPlayingTime.getCalendarTime(&time);
            processor.replaceArgs(
                dateTimeText.getBuffer(), dateTimeText.getBufferSize(), this,
                al::getSystemMessageString(this, "RCS_SaveData", "RCS_SaveData_FileDateTime"),
                time.getYear(), time.getMonth().getValueOneOrigin(), time.getDay(),
                time.getHour(), time.getMinute());
            al::setPaneString(button, dateTimePane, dateTimeText.cstr());
        } else {
            al::setPaneString(button, dateTimePane, u"");
        }

        usedFileNum++;
        if (isSingleMode) {
            SingleModeData* data = mGameDataHolder->getSingleModeDataFile(i);
            s32 islandNo = data->getCurValidIslandVisited() + 1;
            if (islandNo <= 0 ||
                (islandNo == 1 && data->getUnlockedPhase() < cPhaseShowFirstIslandName) ||
                islandNo == cIslandNoOpenWater) {
                al::setPaneSystemMessage(button, "TxtIsland", "IslandName", "OpenWater");
            } else {
                auto* islandName = reinterpret_cast<const char16_t*>(
                    IslandDataFunction::getIslandName(this, this, islandNo));
                al::setPaneString(
                    button, "TxtIsland",
                    islandName != nullptr ?
                        islandName :
                        sead::WFormatFixedSafeString<32>(u"Placeholder Island Name").cstr());
            }

            WString16 clearStarText;
            appendClearStars(&clearStarText, data->calcClearStarLevel());
            al::setPaneString(button, "TxtFile1ClearStar", clearStarText.cstr());
            sead::WFormatFixedSafeString<5> goalItemText(u"%d", data->getGoalItemNum());
            al::setPaneString(button, "TxtCounter1", goalItemText.cstr());
        } else {
            GameDataFile* file = mGameDataHolder->getGameDataFile(i);
            WString16 clearStarText;
            appendClearStars(&clearStarText, file->calcClearStarLevel());
            al::setPaneString(button, "TxtFile1ClearStar", clearStarText.cstr());

            s32 worldId;
            s32 stageId;
            GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor(mGameDataHolder),
                                                  &worldId, &stageId,
                                                  file->getLastPlayCourseIdSaved());
            rc::setPaneWorldString(this, button, "TxtWorld", "RCS_SaveData",
                                   "RCS_SaveData_World", worldId, nullptr, true);

            WString16 lifeText;
            rc::convertPlayerLifeToText(&lifeText, file->getPlayerLifeSaved());
            al::setPaneString(button, "TxtCounter",
                              LayoutFontUtil::getPictureFontPlayer(
                                  file->getMainPlayerCharacterTypeSaved()));
            al::setPaneString(button, "TxtCounterPlayer", lifeText.cstr());

            WFormatString16 starText(u"%d", file->getTotalAcquireGreenStarNumSaved());
            al::setPaneString(button, "TxtCounterStar", starText.cstr());
            WFormatString16 stampText(u"%d", file->calcTotalIllustItemNum());
            al::setPaneString(button, "TxtCounterStamp", stampText.cstr());
            if (file->isClearNormalEnding()) {
                WFormatString16 livesLostText(
                    u"%d", sead::Mathi::min(file->getRetryCountSaved(), cLivesLostMax));
                al::setPaneString(button, "TxtLivesLost", livesLostText.cstr());
                al::showPane(button, "TxtLivesLost");
            } else {
                al::hidePane(button, "TxtLivesLost");
            }
        }
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.AppearSave)) {
        for (s32 i = 0; i < buttonNum; i++) {
            al::StringTmp<32> buttonName;
            al::StringTmp<32> upName;
            al::StringTmp<32> downName;
            buttonName.format("Button%d", i + 1);
            upName.format("Button%d", i == 0 ? buttonNum : i);
            downName.format("Button%d", i == buttonNum - 1 ? 1 : i + 2);
            mButtonGroup->resetDestination(buttonName.cstr(), upName.cstr(), downName.cstr(),
                                           nullptr, nullptr, true);
        }
    } else {
        for (s32 i = 0; i < usedFileNum; i++) {
            al::StringTmp<32> buttonName;
            al::StringTmp<32> upName;
            al::StringTmp<32> downName;
            buttonName.format("Button%d", usedFileNo[i]);
            upName.format("Button%d", usedFileNo[(i == 0 ? usedFileNum : i) - 1]);
            downName.format("Button%d", i == usedFileNum - 1 ? usedFileNo[0] : usedFileNo[i + 1]);
            mButtonGroup->resetDestination(buttonName.cstr(), upName.cstr(), downName.cstr(),
                                           nullptr, nullptr, true);
        }
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoInAfterDelete) && mIsPauseMenu) {
        mButtonGroup->getButtonUnsafe(getPlayingFileId(mGameDataHolder))
            ->setCustomSE("Decide_Notification");
    }
}

/** @brief Keeps the twinkle effects of the file buttons attached to their panes. */
void RCS_SaveDataLayout::control() {
    if (al::isNerve(this, &NrvRCS_SaveDataLayout.Delete) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.Save)) {
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoInAfterDelete)) {
        al::tryDeleteEmitterAndParticleAll(this);
    }

    for (s32 i = 0; i < cFileNum; i++) {
        CursorTarget* button = mButtonGroup->getButton(i);
        al::calcPaneMtx(&mEffectMtxs[i], button, "TxtFile1ClearStar");
        if (mEffectFlags.isOffBit(i)) {
            continue;
        }

        al::StringTmp<64> parentName("ParButton%d", i + 1);
        f32 alpha = al::getGlobalAlpha(this, parentName.cstr());
        if (getEffectKeeper() == nullptr || !al::isEffectExist(this, mEffectNames[i].cstr())) {
            continue;
        }

        bool isEmitting = al::isEffectEmitting(this, mEffectNames[i].cstr());
        if (mButtonGroup->getSelectedButton() != button) {
            al::tryDeleteEffect(this, mEffectNames[i].cstr());
            continue;
        }

        if (alpha > cTwinkleAlphaThreshold && !isEmitting) {
            al::tryEmitEffect(this, mEffectNames[i].cstr(), nullptr);
        } else if (alpha <= cTwinkleAlphaThreshold && isEmitting) {
            al::tryDeleteEffect(this, mEffectNames[i].cstr());
        }
    }
}

/**
 * @brief Opens the layout to choose the file to save to.
 * @param port Controller port operating the layout.
 * @param worldId World the player is in, or -1.
 */
void RCS_SaveDataLayout::appearSave(s32 port, s32 worldId) {
    mPort = port;
    mButtonGroup->reset();
    al::setPaneSystemMessage(this, "TxtHeader", "RCS_SaveData", "RCS_SaveData_HeaderSave");
    al::setNerve(this, &NrvRCS_SaveDataLayout.AppearSave);
    appear();
    mWorldId = worldId;
}

/**
 * @brief Opens the layout to choose the file to load.
 * @param port Controller port operating the layout.
 */
void RCS_SaveDataLayout::appearLoad(s32 port) {
    mPort = port;
    mButtonGroup->reset();
    al::setPaneSystemMessage(this, "TxtHeader", "RCS_SaveData", "RCS_SaveData_HeaderLoad");
    mButtonGroup->invalidate();
    al::setNerve(this, &NrvRCS_SaveDataLayout.AppearLoad);
    appear();
}

/**
 * @brief Enables or disables the file buttons.
 * @param port Controller port (unused).
 * @param isValid True to enable the buttons.
 */
void RCS_SaveDataLayout::updateButtonValidation(s32 port, bool isValid) {
    if (isValid) {
        mButtonGroup->validate();
    } else {
        mButtonGroup->invalidate();
    }
}

/**
 * @brief Opens the layout to choose the file to delete.
 * @param port Controller port operating the layout.
 * @param isAppearTitle True to also show the title guide bar.
 */
void RCS_SaveDataLayout::appearDelete(s32 port, bool isAppearTitle) {
    mPort = port;
    mButtonGroup->reset();
    al::setPaneSystemMessage(this, "TxtHeader", "RCS_SaveData", "RCS_SaveData_HeaderDelete");
    mButtonGroup->invalidate();
    if (mGuideBar != nullptr && !mIsPauseMenu && isAppearTitle) {
        mGuideBar->appearTitle();
    }

    al::setNerve(this, &NrvRCS_SaveDataLayout.AppearDelete);
    appear();
}

/**
 * @brief Resets the buttons and puts the cursor on a file.
 * @param index File to select, or a negative value for the last decided file.
 */
void RCS_SaveDataLayout::resetButtons(s32 index) {
    mButtonGroup->reset();
    if (index >= 0) {
        mButtonGroup->select(index);
    } else {
        mButtonGroup->select(mDecidedFileId);
    }

    mButtonGroup->showCursor();
}

/** @brief Switches to choosing a file to load. */
void RCS_SaveDataLayout::setSelectLoad() {
    al::setNerve(this, &NrvRCS_SaveDataLayout.SelectLoad);
    mDecidedFileId = -1;
    updateEffectColors();
}

/** @brief Updates the twinkle effect of every file from its clear star level and selection. */
void RCS_SaveDataLayout::updateEffectColors() {
    for (s32 i = 0; i < cFileNum; i++) {
        GameDataFileBase* file = getDataFile(mGameDataHolder, mGameDataHolder->isSingleMode(), i);
        s32 level = file->calcClearStarLevel();
        if (level >= 1 && file->isTwinkleData()) {
            mEffectNames[i].format("File%02dTwinkle%02d", i + 1, level);
            if (getEffectKeeper() != nullptr) {
                al::setEffectFollowMtxPtr(this, mEffectNames[i].cstr(), &mEffectMtxs[i]);
            }

            mEffectFlags.setBit(i);
        } else if (mEffectFlags.isOnBit(i)) {
            al::tryDeleteEffect(this, mEffectNames[i].cstr());
            mEffectNames[i].clear();
            mEffectFlags.resetBit(i);
        }

        if (getEffectKeeper() == nullptr || !al::isEffectExist(this, mEffectNames[i].cstr())) {
            continue;
        }

        if (mButtonGroup->isSelect(mButtonGroup->getButton(i))) {
            al::calcPaneMtx(&mEffectMtxs[i], mButtonGroup->getButton(i), "TxtFile1ClearStar");
            al::setEffectEmitterColors(this, mEffectNames[i].cstr(), sead::Color4f::cWhite,
                                       sead::Color4f::cWhite);
            al::tryEmitEffect(this, mEffectNames[i].cstr(), nullptr);
            al::setEffectEmitterColors(this, mEffectNames[i].cstr(), sead::Color4f::cWhite,
                                       sead::Color4f::cWhite);
        } else {
            al::setEffectEmitterColors(this, mEffectNames[i].cstr(),
                                       sead::Color4f(0.5f, 0.5f, 0.5f, sead::Color4f::cElementMax),
                                       sead::Color4f(0.75f, 0.75f, 0.75f,
                                                     sead::Color4f::cElementMax));
            al::tryDeleteEffect(this, mEffectNames[i].cstr());
        }
    }
}

/** @brief Closes the layout immediately unless a save, load or delete is running. */
void RCS_SaveDataLayout::forceExit() {
    if (al::isNerve(this, &NrvRCS_SaveDataLayout.Save) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.Load) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.Delete)) {
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.SelectDelete) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.SelectSave) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.SelectLoad)) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.EndBack);
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmDelete) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmSave) ||
        al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad)) {
        if (!mWindowConfirm->isAlive()) {
            al::setNerve(this, &NrvRCS_SaveDataLayout.EndBack);
            return;
        }

        if (mWindowConfirm->isEnding()) {
            return;
        }

        mWindowConfirm->forceExit();
        al::startAction(this, "Show", "Visibility");
        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }
    } else if (al::isNerve(this, &NrvRCS_SaveDataLayout.Notify) ||
               al::isNerve(this, &NrvRCS_SaveDataLayout.NotifyPlayingFile)) {
        if (!mWindowReport->isAlive()) {
            al::setNerve(this, &NrvRCS_SaveDataLayout.EndBack);
            return;
        }

        if (mWindowReport->isEnding()) {
            return;
        }

        mWindowReport->forceExit();
        al::startAction(this, "Show", "Visibility");
        if (mGuideBar != nullptr) {
            mGuideBar->show();
        }

        if (mPauseMenuLayout != nullptr) {
            al::showPaneRootNoRecursive(mPauseMenuLayout);
        }
    } else {
        al::setNerve(this, &NrvRCS_SaveDataLayout.EndBack);
        return;
    }

    mScreenCaptureExecutor->offDraw(2);
    mScreenCaptureExecutor->onDraw(1, true);
}

/**
 * @brief Checks whether a file is being chosen.
 * @return True while waiting for a file to be selected.
 */
bool RCS_SaveDataLayout::isWaitSelect() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.SelectDelete) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.SelectSave) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.SelectLoad);
}

/**
 * @brief Checks whether a confirmation window is open.
 * @return True while waiting for a confirmation.
 */
bool RCS_SaveDataLayout::isWaitConfirm() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmDelete) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmSave) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad);
}

/** @brief Enables the file buttons. */
void RCS_SaveDataLayout::validateButtons() {
    mButtonGroup->validate();
}

/**
 * @brief Checks whether the chosen file is being loaded.
 * @return True while loading.
 */
bool RCS_SaveDataLayout::isLoading() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.Load);
}

/**
 * @brief Checks whether the layout closed after loading a file.
 * @return True when the loaded file must be started.
 */
bool RCS_SaveDataLayout::isNeedLoad() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.EndNeedLoad);
}

/**
 * @brief Checks whether a save just started.
 * @return True on the first frame of saving.
 */
bool RCS_SaveDataLayout::isSaving() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.Save) && al::isFirstStep(this);
}

/**
 * @brief Checks whether the layout is closing after a cancel.
 * @return True while closing after a cancel.
 */
bool RCS_SaveDataLayout::isEndBack() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.EndBack);
}

/**
 * @brief Checks whether the layout is closing or closed.
 * @return True while in one of the end states.
 */
bool RCS_SaveDataLayout::isEnding() const {
    return al::isNerve(this, &NrvRCS_SaveDataLayout.EndBack) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.EndNeedLoad) ||
           al::isNerve(this, &NrvRCS_SaveDataLayout.End);
}

/** @brief Plays the appear animation and picks the initially selected file. */
void RCS_SaveDataLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearSave)) {
            s32 buttonNum = mButtonGroup->getButtonNum();
            for (s32 i = 0; i < buttonNum; i++) {
                CursorTarget* button = mButtonGroup->getButtonUnsafe(i);
                button->setCustomSE(nullptr);
                if (mGameDataHolder->isNewFile(i)) {
                    button->setCustomSE("Decided_NewFile");
                }
            }
        }

        bool isSingleMode = mGameDataHolder->isSingleMode();
        if (mIsPauseMenu) {
            al::startAction(this, "Appear");
            if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearDelete)) {
                s32 playingFileId = isSingleMode ? mGameDataHolder->getSingleFile()->getFileId() :
                                                   mGameDataHolder->getPlayingFile()->getFileId();
                mButtonGroup->getButtonUnsafe(playingFileId)->setCustomSE("Decide_Notification");
            }
        } else {
            if (isSingleMode) {
                al::startAction(this, "Koopa", "Color");
            } else {
                al::startAction(this, "Mario", "Color");
            }

            al::startAction(this, "Appear_Title");
        }

        s32 fileId = isSingleMode ? mGameDataHolder->getLastSingleModePlayingFileID() :
                                    mGameDataHolder->getLastPlayingFileId();
        if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearLoad) && !mIsPauseMenu) {
            fileId = findUsedFileId(mGameDataHolder, isSingleMode, fileId,
                                    mButtonGroup->getButtonNum());
        }

        al::StringTmp<32> buttonName("Button%d", fileId + 1);
        static_cast<ButtonRCSFileParts*>(mButtonGroup->getButton(buttonName.cstr()))
            ->setNoSoundSelect();
        mButtonGroup->select(buttonName.cstr());
    }

    if (al::isStep(this, 2)) {
        al::startAction(mButtonGroup->getSelectedButton(), "SetSelected", "Main");
        updateEffectColors();
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    mButtonGroup->showCursor();
    mDecidedFileId = -1;
    if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearDelete)) {
        mButtonGroup->validate();
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectDelete);
    } else if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearLoad)) {
        mButtonGroup->validate();
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectLoad);
    } else if (al::isNerve(this, &NrvRCS_SaveDataLayout.AppearSave)) {
        mButtonGroup->validate();
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectSave);
    }
}

/** @brief Moves the cursor between the files and handles the decision or cancel. */
void RCS_SaveDataLayout::exeSelect() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEmitterAndParticleAll(this);
    }

    if (rc::isPadTriggerUiCancelByPort(mPort)) {
        mButtonGroup->hideCursor();
        al::startSe(this, "Back");
        if (!mIsPauseMenu) {
            if (al::isNerve(this, &NrvRCS_SaveDataLayout.SelectLoad)) {
                if (mGuideBar != nullptr) {
                    mGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_Title, mPort, true);
                }
            } else if (mGuideBar != nullptr) {
                mGuideBar->changeText(RCSControlGuideBar::GuideBarMsgType_Title,
                                      al::getMainControllerPort(), false);
            }
        }

        al::setNerve(this, &NrvRCS_SaveDataLayout.EndBack);
        return;
    }

    CursorTarget* prevSelected = mButtonGroup->getSelectedButton();
    mButtonGroup->updateAndCursorDefault(mPort);
    if (prevSelected != mButtonGroup->getSelectedButton()) {
        updateEffectColors();
    }

    if (mButtonGroup->isDecideAny()) {
        if (mButtonGroup->getCursor()->isAlive()) {
            mButtonGroup->hideCursor();
        }

        const char* decideButton = mButtonGroup->getDecideButton();
        if (!al::isEqualString(decideButton, mButtonGroup->getSelectedButton()->getName())) {
            mButtonGroup->getSelectedButton()->wait();
            mButtonGroup->select(mButtonGroup->getDecideButton());
        }
    }

    if (!mButtonGroup->isDecideEndAny()) {
        return;
    }

    mDecidedFileId = mButtonGroup->getDecideEndButtonIndex();
    mButtonGroup->invalidate();
    if (al::isNerve(this, &NrvRCS_SaveDataLayout.SelectLoad)) {
        if (mIsPauseMenu) {
            al::setNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad);
        } else {
            al::startSe(this, "GameStart");
            al::setNerve(this, &NrvRCS_SaveDataLayout.Load);
        }

        return;
    }

    bool isNewFile =
        getDataFile(mGameDataHolder, mGameDataHolder->isSingleMode(), mDecidedFileId)->isNewFile();
    bool isSave = al::isNerve(this, &NrvRCS_SaveDataLayout.SelectSave);
    if (isNewFile) {
        if (isSave) {
            mButtonGroup->reset();
            al::setNerve(this, &NrvRCS_SaveDataLayout.Save);
        }

        return;
    }

    if (isSave) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.ConfirmSave);
        return;
    }

    s32 playingFileId = getPlayingFileId(mGameDataHolder);
    if (mIsPauseMenu && playingFileId == mDecidedFileId) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.NotifyPlayingFile);
    } else {
        al::setNerve(this, &NrvRCS_SaveDataLayout.ConfirmDelete);
    }
}

/** @brief Asks for confirmation before deleting, overwriting or loading the chosen file. */
void RCS_SaveDataLayout::exeConfirm() {
    if (al::isFirstStep(this)) {
        mButtonGroup->invalidate();
        mWindowConfirm->setDefaultSelectLeft(false);
        const char* message;
        if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmDelete)) {
            message = "WindowConfirmDouble_Delete";
        } else if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad)) {
            mWindowConfirm->setDefaultSelectLeft(true);
            message = "WindowConfirmDouble_Load";
        } else {
            message = "WindowConfirmDouble_Overwrite";
        }

        if (mScreenCaptureExecutor != nullptr) {
            mScreenCaptureExecutor->requestCapture(true, 2, true);
        }

        mWindowConfirm->appearWithSystemMessage("RCS_SaveData", message, mPort, nullptr);
    }

    if (al::isStep(this, 2)) {
        al::startAction(this, "Hide", "Visibility");
        if (mGuideBar != nullptr) {
            mGuideBar->hide();
        }
    }

    if (al::isLessStep(this, 15)) {
        return;
    }

    if (al::isStep(this, 15)) {
        mButtonGroup->reset();
    }

    if (mWindowConfirm->isDecideLeft()) {
        mWindowConfirm->setDefaultSelectLeft(false);
        al::startAction(mButtonGroup->getButton(mDecidedFileId), "SetSelected", "Main");
        mScreenCaptureExecutor->offDraw(2);
        if (!mIsPauseMenu && al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmDelete)) {
            mScreenCaptureExecutor->onDraw(1, true);
        }

        if (mGuideBar != nullptr && mIsPauseMenu) {
            mGuideBar->show();
        }

        al::startAction(this, "Show", "Visibility");
        mButtonGroup->showCursor();
        if (mIsPauseMenu) {
            mScreenCaptureExecutor->onDraw(1, true);
            al::hidePane(this, "WinFrame");
        }

        if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad)) {
            mButtonGroup->validate();
            al::setNerve(this, &NrvRCS_SaveDataLayout.WaitConfirmEndLoad);
            return;
        }

        bool isSave = al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmSave);
        mButtonGroup->validate();
        if (isSave) {
            al::setNerve(this, &NrvRCS_SaveDataLayout.WaitConfirmEndSave);
        } else {
            al::setNerve(this, &NrvRCS_SaveDataLayout.WaitConfirmEndDelete);
        }

        return;
    }

    if (!mWindowConfirm->isDecideRightEnd()) {
        return;
    }

    if (mIsPauseMenu) {
        mScreenCaptureExecutor->onDraw(1, true);
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmLoad)) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.Load);
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.ConfirmSave)) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.Save);
    } else {
        mScreenCaptureExecutor->onDraw(1, true);
        al::setNerve(this, &NrvRCS_SaveDataLayout.Delete);
    }

    if (mGuideBar != nullptr) {
        mGuideBar->show();
    }

    al::startAction(this, "Show", "Visibility");
    mScreenCaptureExecutor->offDraw(2);
}

/** @brief Tells the player that the file being played cannot be deleted. */
void RCS_SaveDataLayout::exeNotify() {
    if (al::isFirstStep(this)) {
        mButtonGroup->invalidate();
        if (al::isNerve(this, &NrvRCS_SaveDataLayout.NotifyPlayingFile)) {
            mWindowReport->appearWithSystemMessage("RCS_SaveData",
                                                   "WindowConfirmReport_PlayingFile", mPort,
                                                   nullptr);
        }

        if (mScreenCaptureExecutor != nullptr) {
            mScreenCaptureExecutor->requestCapture(true, 2, true);
            if (mScreenCaptureExecutor->isDraw(1)) {
                mScreenCaptureExecutor->offDraw(1);
            }
        }
    }

    if (al::isStep(this, 2)) {
        al::startAction(this, "Hide", "Visibility");
        if (mGuideBar != nullptr) {
            mGuideBar->hide();
        }

        if (mPauseMenuLayout != nullptr) {
            al::hidePaneRootNoRecursive(mPauseMenuLayout);
        }
    }

    if (al::isStep(this, 15)) {
        mButtonGroup->reset();
    }

    if (mWindowReport->isAlive()) {
        return;
    }

    al::startAction(mButtonGroup->getButton(mDecidedFileId), "SetSelected", "Main");
    if (mGuideBar != nullptr) {
        mGuideBar->show();
    }

    if (mPauseMenuLayout != nullptr) {
        al::showPaneRootNoRecursive(mPauseMenuLayout);
    }

    al::startAction(this, "Show", "Visibility");
    mButtonGroup->showCursor();
    mButtonGroup->validate();
    mScreenCaptureExecutor->onDraw(1, true);
    mScreenCaptureExecutor->offDraw(2);
    al::setNerve(this, &NrvRCS_SaveDataLayout.SelectDelete);
}

/** @brief Waits for the confirmation window to close and returns to the file selection. */
void RCS_SaveDataLayout::exeWaitConfirmEnd() {
    if (mWindowConfirm->isAlive()) {
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.WaitConfirmEndLoad)) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectLoad);
    } else if (al::isNerve(this, &NrvRCS_SaveDataLayout.WaitConfirmEndSave)) {
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectSave);
    } else {
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectDelete);
    }
}

/** @brief Reads the save data and makes the chosen file the playing file. */
void RCS_SaveDataLayout::exeLoad() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataRead(mGameDataHolder, true);
    }

    if (!SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        return;
    }

    if (mGameDataHolder->isSingleMode()) {
        mGameDataHolder->setSingleModePlayingFileID(mDecidedFileId, false);
        SingleModeData* data = mGameDataHolder->getSingleFile();
        if (data->getUnlockedPhase() == cPhasePlessieChase1 ||
            data->getUnlockedPhase() == cPhasePlessieChase2) {
            data->setPhaseFromPlessieChase();
        }
    } else {
        mGameDataHolder->setPlayingFileId(mDecidedFileId);
    }

    al::setNerve(this, &NrvRCS_SaveDataLayout.EndNeedLoad);
}

/** @brief Fades out the information of the selected file button. */
void RCS_SaveDataLayout::exeFadeButtonInfoOut() {
    if (al::isFirstStep(this)) {
        al::startAction(mButtonGroup->getSelectedButton(), "FileDataChange_Out",
                        "FileDataChange");
        return;
    }

    if (al::isGreaterEqualStep(this, 5) &&
        al::isActionEnd(mButtonGroup->getSelectedButton(), "FileDataChange")) {
        const al::Nerve* nextNerve = &NrvRCS_SaveDataLayout.FadeButtonInfoInAfterDelete;
        if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoOut)) {
            nextNerve = &NrvRCS_SaveDataLayout.FadeButtonInfoIn;
        }

        al::setNerve(this, nextNerve);
    }
}

/** @brief Fades the updated file information back in and returns to the file selection. */
void RCS_SaveDataLayout::exeFadeButtonInfoIn() {
    if (al::isFirstStep(this)) {
        al::startAction(mButtonGroup->getSelectedButton(), "FileDataChange_In", "FileDataChange");
        updateFileSelectButtonState(mGameDataHolder->isSingleMode());
    }

    if (al::isLessStep(this, 10)) {
        return;
    }

    if (al::isNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn)) {
        mButtonGroup->validate();
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectSave);
    } else {
        s32 playingFileId = getPlayingFileId(mGameDataHolder);
        if (!mIsPauseMenu) {
            al::setNerve(this, &NrvRCS_SaveDataLayout.Load);
            return;
        }

        mButtonGroup->validate();
        mButtonGroup->select(mButtonGroup->getButton(playingFileId));
        al::setNerve(this, &NrvRCS_SaveDataLayout.SelectDelete);
    }

    updateEffectColors();
    al::startAction(mButtonGroup->getSelectedButton(), "SetSelected", "Main");
    mButtonGroup->showCursor();
}

/** @brief Writes the playing progress into the chosen file and saves it. */
void RCS_SaveDataLayout::exeSave() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEmitterAndParticleAll(this);
        bool isSingleMode = GameDataFunction::isSingleMode(this);
        GameDataFileBase* playingFile =
            isSingleMode ? static_cast<GameDataFileBase*>(mGameDataHolder->getSingleFile()) :
                           mGameDataHolder->getPlayingFile();
        al::startAction(mButtonGroup->getButton(mDecidedFileId), "SetSelected", "Main");

        GameDataFileBase* saveFile;
        if (playingFile->getFileId() == mDecidedFileId) {
            saveFile = playingFile;
        } else if (isSingleMode) {
            saveFile = mGameDataHolder->getSingleModeDataFile(mDecidedFileId);
            mGameDataHolder->copySingleModeFile(playingFile->getFileId(), mDecidedFileId);
        } else {
            saveFile = mGameDataHolder->getGameDataFile(mDecidedFileId);
            mGameDataHolder->copySaveFile(playingFile->getFileId(), mDecidedFileId);
        }

        if (!isSingleMode && mWorldId != -1) {
            mGameDataHolder->getGameDataFile(mDecidedFileId)->onGotoTitle(mWorldId);
        }

        saveFile->startSave();
        SaveDataAccessFunction::startSaveDataWriteNoWindow(
            mGameDataHolder, playingFile->getFileId() != mDecidedFileId, true);
        if (mGuideBar != nullptr) {
            mGuideBar->startSave();
            if (mIsPauseMenu && !GameDataFunction::isSingleMode(this)) {
                mGuideBar->endIcon();
            }
        }
    }

    if (!SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        return;
    }

    if (mGuideBar != nullptr) {
        if (!mGuideBar->isOverlayFinished()) {
            return;
        }

        if (mIsPauseMenu && !GameDataFunction::isSingleMode(this)) {
            mGuideBar->appearIcon();
        }
    }

    al::setNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoIn);
}

/** @brief Deletes the chosen file and saves the result. */
void RCS_SaveDataLayout::exeDelete() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEmitterAndParticleAll(this);
        mGameDataHolder->deleteSaveFile(mDecidedFileId);
        SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, mIsPauseMenu, true);
        if (mGuideBar != nullptr) {
            mGuideBar->startDelete();
            if (mIsPauseMenu && !GameDataFunction::isSingleMode(this)) {
                mGuideBar->endIcon();
            }
        }

        al::startAction(mButtonGroup->getButton(mDecidedFileId), "SetSelected", "Main");
    }

    if (!SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        return;
    }

    if (mGuideBar != nullptr) {
        if (!mGuideBar->isOverlayFinished()) {
            return;
        }

        if (mIsPauseMenu && !GameDataFunction::isSingleMode(this)) {
            mGuideBar->appearIcon();
        }
    }

    if (GameDataFunction::isSingleMode(this)) {
        GameDataFunction::initTotalPlayTimeSM(GameDataHolderAccessor(mGameDataHolder),
                                              mDecidedFileId);
    } else {
        GameDataFunction::initTotalPlayTimeOG(GameDataHolderAccessor(mGameDataHolder),
                                              mDecidedFileId);
    }

    al::setNerve(this, &NrvRCS_SaveDataLayout.FadeButtonInfoInAfterDelete);
}

/** @brief Plays the end animation and hides the layout. */
void RCS_SaveDataLayout::exeEnd() {
    if (al::isNerve(this, &NrvRCS_SaveDataLayout.EndNeedLoad)) {
        al::tryDeleteEmitterAndParticleAll(this);
        if (!mIsPauseMenu) {
            return;
        }

        kill();
    }

    if (al::isFirstStep(this)) {
        if (mGuideBar != nullptr && !mIsPauseMenu) {
            mGuideBar->endTitle(true);
        }

        al::startAction(this, "End");
    }

    if (al::isActionEnd(this)) {
        al::tryDeleteEmitterAndParticleAll(this);
        kill();
    }
}
