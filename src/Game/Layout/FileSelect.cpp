#include "Layout/FileSelect.hpp"

#include <time/seadCalendarTime.h>
#include <time/seadDateTime.h>
#include "Layout/ButtonBackParts.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/ButtonShortCutAndTouchParts.hpp"
#include "Layout/CursorTarget.hpp"
#include "Layout/GuideWindowParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Layout/ProjectReplaceTagProcessor.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/LayoutUtil.hpp"

namespace {
NERVE_DECL(FileSelect, Appear)
NERVE_DECL(FileSelect, SelectBack)
NERVE_DECL(FileSelect, CopyDst)
NERVE_DECL(FileSelect, Delete)
NERVE_DECL(FileSelect, Wait)
NERVE_DECL(FileSelect, EndWait)
NERVE_DECL(FileSelect, Select)
NERVE_DECL(FileSelect, CopyIn)
NERVE_DECL(FileSelect, DeleteIn)
NERVE_DECL(FileSelect, Copy)
NERVE_DECL(FileSelect, CopyDstIn)
NERVE_DECL(FileSelect, End)
NERVE_DECL(FileSelect, PartsHide)
NERVE_DECL(FileSelect, PartsShow)

// Some nerves are constant objects, the others are grouped in one mutable block.
const FileSelectNrvAppear NrvFileSelectAppear{};
const FileSelectNrvCopyDst NrvFileSelectCopyDst{};
const FileSelectNrvDelete NrvFileSelectDelete{};
const FileSelectNrvEndWait NrvFileSelectEndWait{};
const FileSelectNrvSelect NrvFileSelectSelect{};
const FileSelectNrvCopy NrvFileSelectCopy{};
const FileSelectNrvPartsHide NrvFileSelectPartsHide{};

NERVES_MAKE_STRUCT(FileSelect, SelectBack, Wait, CopyIn, DeleteIn, CopyDstIn, End, PartsShow)

/// Number of save files.
constexpr s32 cFileNum = 4;

/// Highest retry count shown in the challenge display of a file.
constexpr s32 cChallengeNumMax = 99999;

/// Frame of the copy animation from which the twinkle effect of a file is shown again.
constexpr f32 cCopyAnimTwinkleFrame = 10.0f;

typedef sead::WFixedSafeString<32> WString32;
typedef sead::WFixedSafeString<128> WString128;
typedef sead::WFixedSafeString<256> WString256;
typedef sead::WFormatFixedSafeString<32> WFormatString32;

/// Animation group of each file button used by the copy / delete animations.
const char* const cCopyAnimGroupNames[] = {"CopyFile1", "CopyFile2", "CopyFile3"};

/// Animation group of the date and challenge display of each file.
const char* const cDateAnimGroupNames[] = {"File1DateChallenge", "File2DateChallenge",
                                           "File3DateChallenge"};

/// Pane showing the last save date of each file.
const char* const cDatePaneNames[] = {"TxtFile1Date", "TxtFile2Date", "TxtFile3Date"};

/// Message label of the challenge count of each file.
const char* const cChallengeLabels[] = {"FileSelect_File1Challenge", "FileSelect_File2Challenge",
                                        "FileSelect_File3Challenge"};

/// Pane showing the challenge count of each file.
const char* const cChallengePaneNames[] = {"TxtFile1Challenge", "TxtFile2Challenge",
                                           "TxtFile3Challenge"};

/// Message label of the world name of each file.
const char* const cWorldLabels[] = {"FileSelect_File1World", "FileSelect_File2World",
                                    "FileSelect_File3World"};

/// Name of the button of each file.
const char* const cFileButtonNames[] = {"ファイル１", "ファイル２", "ファイル３"};
}  // namespace

/**
 * @brief Creates the file select layout with its file buttons, guide window and option buttons.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data holder owning the save files.
 */
FileSelect::FileSelect(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder)
    : al::LayoutActor("ファイル選択"), mGameDataHolder(pGameDataHolder) {
    al::initLayoutActor(this, rInfo, "FileSelect", nullptr);
    initNerve(&NrvFileSelectAppear, 0);
    mButtonGroup = new ButtonGroup(rInfo, this, "FileSelect", nullptr, false);
    mPort = rc::getPadPortByUserId(0);
    mButtonGroup->setPort(mPort);
    mGuideWindow = new GuideWindowParts(rInfo, "ガイドパーツ", "ParGuide", this);
    mBackButton = new ButtonBackParts(rInfo, "戻るボタンパーツ", "ParBack", this, mPort);
    mCopyButton = new ButtonShortCutAndTouchParts(rInfo, "プラスボタン", "ParGuideStart", this,
                                                  mPort,
                                                  ButtonShortCutAndTouchParts::ShortCutType_Plus);
    mDeleteButton = new ButtonShortCutAndTouchParts(
        rInfo, "マイナスボタン", "ParGuideSelect", this, mPort,
        ButtonShortCutAndTouchParts::ShortCutType_Minus);
    al::initLayoutPartsAudioKeeper(mDeleteButton, rInfo, "FileSelectOptionButton");
    al::initLayoutPartsAudioKeeper(mCopyButton, rInfo, "FileSelectOptionButton");
    mEffectMtxs.tryAllocBuffer(cFileNum, nullptr);
    mEffectNames.tryAllocBuffer(cFileNum, nullptr);
    for (s32 i = 0; i < cFileNum; i++) {
        mEffectMtxs[i] = sead::Matrix34f::ident;
    }

    mWindowConfirm = new WindowConfirm(WindowConfirmType_Notice, rInfo, "FileSelect", false);
    kill();
}

/** @brief Shows the layout and starts its appear animation. */
void FileSelect::appear() {
    al::startAction(this, "Appear", nullptr);
    al::LayoutActor::appear();
    mResult = Result_None;
    mBackButton->appear();
    mBackButton->setInputEnable(false);
    mButtonGroup->reset();
    mButtonGroup->invalidate();
    mCopyButton->setInputEnable(false);
    mDeleteButton->setInputEnable(false);
    mGuideWindow->setText(
        al::getSystemMessageString(this, "FileSelectState", "ParGuide_Guide_Normal"));
    al::setNerve(this, &NrvFileSelectAppear);
}

/** @brief Keeps the twinkle effects of the file buttons attached to their panes. */
void FileSelect::control() {
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButton(i);
        al::calcPaneMtx(&mEffectMtxs[i], button, "TxtFile1ClearStar");
        if (!mEffectFlags.isOnBit(i) ||
            al::isActionPlaying(this, "DeleteFile", cCopyAnimGroupNames[i])) {
            continue;
        }

        if (al::isActionPlaying(this, "CopyFile", cCopyAnimGroupNames[i]) &&
            al::getActionFrame(this, cCopyAnimGroupNames[i]) < cCopyAnimTwinkleFrame) {
            continue;
        }

        al::StringTmp<64> parentName("ParButton%d", i + 1);
        f32 alpha = al::getGlobalAlpha(this, parentName.cstr());
        bool isEmitting = al::isEffectEmitting(this, mEffectNames[i].cstr());
        if (alpha > 0.0f && !isEmitting) {
            al::tryEmitEffect(this, mEffectNames[i].cstr(), nullptr);
        } else if (alpha <= 0.0f && isEmitting) {
            al::tryDeleteEffect(this, mEffectNames[i].cstr());
        }
    }
}

/**
 * @brief Updates every file button with the contents of its save file and hides the copy /
 *        delete buttons when every file is new.
 */
void FileSelect::updateFileSelectButtonState() {
    bool isAllNewFile = true;
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButton(i);
        if (mGameDataHolder->isNewFile(i)) {
            al::startAction(button, "StateNew", "State");
            al::startAction(this, "New", cDateAnimGroupNames[i]);
            if (mEffectFlags.isOnBit(i)) {
                al::tryDeleteEffect(this, mEffectNames[i].cstr());
                mEffectNames[i].clear();
                mEffectFlags.resetBit(i);
            }

            continue;
        }

        al::startAction(button, "StateData", "State");
        GameDataFile* file = mGameDataHolder->getGameDataFile(i);
        al::startAction(this, "Date", cDateAnimGroupNames[i]);
        {
            ProjectReplaceTagProcessor processor;
            WString32 dateText;
            sead::CalendarTime time;
            if (SaveDataAccessFunction::isEnableSave(mGameDataHolder)) {
                file->getLastPlayingTime().getCalendarTime(&time);
                processor.replaceArgs(
                    dateText.getBuffer(), dateText.getBufferSize(), this,
                    al::getLayoutMessageString(this, "FileSelect", "FileSelect_File1Date"),
                    time.getYear(), time.getMonth().getValueOneOrigin(), time.getDay());
                al::setPaneString(this, cDatePaneNames[i], dateText.cstr());
            } else {
                al::setPaneString(this, cDatePaneNames[i], u"");
            }
        }

        if (file->isClearNormalEnding()) {
            ProjectReplaceTagProcessor processor;
            WString256 challengeText;
            s32 challengeNum = sead::Mathi::min(file->getRetryCount(), cChallengeNumMax);
            processor.replaceArgs(challengeText.getBuffer(), challengeText.getBufferSize(), this,
                                  al::getLayoutMessageString(this, "FileSelect",
                                                             cChallengeLabels[i]),
                                  challengeNum);
            al::startAction(this, "Challenge", cDateAnimGroupNames[i]);
            al::setPaneString(this, cChallengePaneNames[i], challengeText.cstr());
        }

        WString128 clearStarText;
        s32 level = file->calcClearStarLevel();
        for (s32 j = 0; j < level; j++) {
            clearStarText.append(sead::WSafeString(LayoutFontUtil::getMessageFontMarkBlackStar()));
        }

        al::setPaneString(button, "TxtFile1ClearStar", clearStarText.cstr());
        if (level > 0 && file->isTwinkleData()) {
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

        s32 worldId;
        s32 stageId;
        s32 courseId = file->getLastPlayCourseId();
        GameDataFunction::calcWorldAndStageId(
            GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)), &worldId,
            &stageId, courseId);
        rc::setPaneWorldString(this, button, "TxtButton", "FileSelect", cWorldLabels[i], worldId,
                               nullptr, false);

        WString32 lifeText;
        rc::convertPlayerLifeToText(&lifeText, file->getPlayerLife());
        al::setPaneString(button, "TxtCounter",
                          LayoutFontUtil::getPictureFontPlayer(file->getMainPlayerCharacterType()));
        al::setPaneString(button, "TxtCounterPlayer", lifeText.cstr());

        WFormatString32 starText(u"%d", file->calcTotalAcquireGreenStarNum());
        al::setPaneString(button, "TxtCounterStar", starText.cstr());
        WFormatString32 stampText(u"%d", file->calcTotalIllustItemNum());
        al::setPaneString(button, "TxtCounterStamp", stampText.cstr());
        isAllNewFile = false;
    }

    if (isAllNewFile) {
        tryHidePlusMinus();
    }
}

/** @brief Hides the copy and delete buttons if they are shown. */
void FileSelect::tryHidePlusMinus() {
    if (!mCopyButton->isHide()) {
        mCopyButton->hide();
    }

    if (!mDeleteButton->isHide()) {
        mDeleteButton->hide();
    }
}

/** @brief Returns from the copy or delete mode to the file selection. */
void FileSelect::backToSelect() {
    updateFileSelectButtonState();
    mButtonGroup->reset();
    showAndNextNerve(&NrvFileSelect.SelectBack);
}

/**
 * @brief Closes the guide window and goes to a nerve once the parts are shown again.
 * @param pNerve Nerve to run after the parts are shown.
 */
void FileSelect::showAndNextNerve(const al::Nerve* pNerve) {
    mButtonGroup->reset();
    mGuideWindow->end();
    mBackButton->setInputEnable(false);
    mNextNerve = pNerve;
    al::setNerve(this, &NrvFileSelect.PartsShow);
}

/** @brief Returns to choosing the copy destination. */
void FileSelect::backToCopy() {
    mButtonGroup->reset();
    mButtonGroup->validate();
    mBackButton->setInputEnable(true);
    changeFileSelectButtonStateCopyDst();
    mButtonGroup->showCursorAppear();
    al::setNerve(this, &NrvFileSelectCopyDst);
}

/** @brief Enables every file button except the copy source. */
void FileSelect::changeFileSelectButtonStateCopyDst() {
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButtonUnsafe(i);
        if (i == mCopySrcIndex) {
            button->disable();
        } else {
            button->enable();
        }
    }

    mButtonGroup->update();
}

/** @brief Returns to choosing the file to delete. */
void FileSelect::backToDelete() {
    mButtonGroup->reset();
    mButtonGroup->validate();
    mBackButton->setInputEnable(true);
    changeFileSelectButtonStateDelete();
    mButtonGroup->showCursorAppear();
    al::setNerve(this, &NrvFileSelectDelete);
}

/** @brief Enables the buttons of the used files only. */
void FileSelect::changeFileSelectButtonStateDelete() {
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButton(i);
        if (mGameDataHolder->isNewFile(i)) {
            button->disable();
        } else {
            button->enable();
        }
    }

    mButtonGroup->update();
}

/**
 * @brief Starts the animation of a file button leaving (deleted or overwritten).
 * @param fileIndex Index of the file.
 */
void FileSelect::startOutAnim(s32 fileIndex) {
    if (mEffectFlags.isOnBit(fileIndex)) {
        al::tryDeleteEffect(this, mEffectNames[fileIndex].cstr());
        mEffectFlags.resetBit(fileIndex);
    }

    al::startAction(this, "DeleteFile", cCopyAnimGroupNames[fileIndex]);
}

/**
 * @brief Starts the animation of a file button coming in (copied).
 * @param fileIndex Index of the file.
 */
void FileSelect::startInAnim(s32 fileIndex) {
    al::startAction(this, "CopyFile", cCopyAnimGroupNames[fileIndex]);
}

/**
 * @brief Checks whether the in / out animation of a file button has ended.
 * @param fileIndex Index of the file.
 * @return True once the animation has ended.
 */
bool FileSelect::isEndInOutAnim(s32 fileIndex) const {
    return al::isActionEnd(this, cCopyAnimGroupNames[fileIndex]);
}

/**
 * @brief Gets the file whose button was decided.
 * @return Index of the decided file, 0 when none was.
 */
s32 FileSelect::getSelectedFileIndex() const {
    if (mButtonGroup->isDecide(cFileButtonNames[0])) {
        return 0;
    }

    if (mButtonGroup->isDecide(cFileButtonNames[1])) {
        return 1;
    }

    return mButtonGroup->isDecide(cFileButtonNames[2]) ? 2 : 0;
}

/**
 * @brief Gets the files chosen for a copy.
 * @param pSrcIndex Receives the index of the copied file.
 * @param pDstIndex Receives the index of the overwritten file.
 */
void FileSelect::getCopyTargetIndex(s32* pSrcIndex, s32* pDstIndex) const {
    *pSrcIndex = mCopySrcIndex;
    *pDstIndex = mCopyDstIndex;
}

/**
 * @brief Sets the controller port operating the layout and its buttons.
 * @param port Controller port.
 */
void FileSelect::setControllerPort(s32 port) {
    mPort = port;
    mButtonGroup->setPort(port);
    mBackButton->setPort(port);
    mCopyButton->setPort(port);
    mDeleteButton->setPort(port);
}

/**
 * @brief Checks whether the layout was closed with the back button.
 * @return True once closed by going back.
 */
bool FileSelect::isEndBack() const {
    return al::isNerve(this, &NrvFileSelectEndWait) && mResult == Result_Back;
}

/**
 * @brief Checks whether the layout was closed by choosing a file.
 * @return True once closed by choosing a file.
 */
bool FileSelect::isEndNext() const {
    return al::isNerve(this, &NrvFileSelectEndWait) && mResult == Result_Decide;
}

/**
 * @brief Checks whether the layout waits for a copy to be done.
 * @return True while waiting for the copy.
 */
bool FileSelect::isWaitCopy() const {
    return al::isNerve(this, &NrvFileSelect.Wait) && mResult == Result_Copy;
}

/**
 * @brief Checks whether the layout waits for a deletion to be done.
 * @return True while waiting for the deletion.
 */
bool FileSelect::isWaitDelete() const {
    return al::isNerve(this, &NrvFileSelect.Wait) && mResult == Result_Delete;
}

/** @brief Plays the appear animation, then selects the last played file. */
void FileSelect::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        changeFileSelectButtonStateSelect();
    }

    if (al::isStep(this, 30)) {
        updateFileSelectButtonState();
    }

    if (al::isActionEnd(this, nullptr)) {
        mCopyButton->setInputEnable(true);
        mDeleteButton->setInputEnable(true);
        mBackButton->setInputEnable(true);
        s32 lastFileId = mGameDataHolder->getLastPlayingFileId();
        mButtonGroup->select(cFileButtonNames[lastFileId]);
        mButtonGroup->showCursorAppear();
        mButtonGroup->validate();
        al::setNerve(this, &NrvFileSelectSelect);
    }
}

/** @brief Enables every file button and shows the copy / delete buttons if a file is used. */
void FileSelect::changeFileSelectButtonStateSelect() {
    s32 buttonNum = mButtonGroup->getButtonNum();
    bool isAllNewFile = true;
    for (s32 i = 0; i < buttonNum; i++) {
        mButtonGroup->getButtonUnsafe(i)->enable();
        isAllNewFile &= mGameDataHolder->isNewFile(i);
    }

    if (isAllNewFile) {
        tryHidePlusMinus();
    } else {
        mCopyButton->appear();
        mDeleteButton->appear();
    }

    mButtonGroup->update();
}

/** @brief Waits for a file, the back button or the copy / delete buttons to be decided. */
void FileSelect::exeSelect() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }

    mButtonGroup->updateAndCursorDefault(mPort);
    lockButtonEachOther();
    if (mBackButton->isDecideEnd()) {
        goToEnd(Result_Back);
        return;
    }

    if (mButtonGroup->isDecideAny()) {
        if (!mIsDecideSePlayed) {
            al::startSe(this, "DecidedGo");
        }

        mIsDecideSePlayed = true;
    }

    if (mButtonGroup->isDecideEndAny()) {
        mIsDecideSePlayed = false;
        s32 fileIndex = getSelectedFileIndex();
        // The result is unused.
        mGameDataHolder->isNewFile(fileIndex);
        goToEnd(Result_Decide);
        return;
    }

    if (mCopyButton->isDecideEnd()) {
        hideAndNextNerve(&NrvFileSelect.CopyIn);
    } else if (mDeleteButton->isDecideEnd()) {
        hideAndNextNerve(&NrvFileSelect.DeleteIn);
    }
}

/** @brief Locks every other button while one of the buttons is being decided. */
void FileSelect::lockButtonEachOther() {
    if (mBackButton->isDecide()) {
        mButtonGroup->hideCursor();
        mButtonGroup->reset();
        mButtonGroup->invalidate();
        mBackButton->setInputEnable(false);
        if (mCopyButton->isDecide()) {
            mCopyButton->wait();
        }

        mCopyButton->setInputEnable(false);
        if (mDeleteButton->isDecide()) {
            mDeleteButton->wait();
        }

        mDeleteButton->setInputEnable(false);
    } else if (mButtonGroup->isDecideAny()) {
        mButtonGroup->hideCursor();
        mButtonGroup->invalidate();
        mBackButton->setInputEnable(false);
        if (mCopyButton->isDecide()) {
            mCopyButton->wait();
        }

        mCopyButton->setInputEnable(false);
        if (mDeleteButton->isDecide()) {
            mDeleteButton->wait();
        }

        mDeleteButton->setInputEnable(false);
    } else if (mCopyButton->isDecide()) {
        mButtonGroup->hideCursor();
        mButtonGroup->reset();
        mButtonGroup->invalidate();
        mBackButton->setInputEnable(false);
        mCopyButton->setInputEnable(false);
        if (mDeleteButton->isDecide()) {
            mDeleteButton->wait();
        }

        mDeleteButton->setInputEnable(false);
    } else if (mDeleteButton->isDecide()) {
        mButtonGroup->hideCursor();
        mButtonGroup->reset();
        mButtonGroup->invalidate();
        mBackButton->setInputEnable(false);
        mCopyButton->setInputEnable(false);
        mDeleteButton->setInputEnable(false);
    }
}

/**
 * @brief Closes the layout.
 * @param result How the layout was left.
 */
void FileSelect::goToEnd(Result result) {
    mBackButton->setInputEnable(false);
    mResult = result;
    al::setNerve(this, &NrvFileSelect.End);
}

/**
 * @brief Closes the guide window and the copy / delete buttons and goes to a nerve once they
 *        are hidden.
 * @param pNerve Nerve to run after the parts are hidden.
 */
void FileSelect::hideAndNextNerve(const al::Nerve* pNerve) {
    mButtonGroup->reset();
    mGuideWindow->end();
    tryDisappearPlusMinus();
    mBackButton->setInputEnable(false);
    mNextNerve = pNerve;
    al::setNerve(this, &NrvFileSelectPartsHide);
}

/** @brief Shows the normal guide again and waits for the parts before selecting a file. */
void FileSelect::exeSelectBack() {
    if (al::isFirstStep(this)) {
        mGuideWindow->appear("ColorNormal");
        mGuideWindow->setText(
            al::getSystemMessageString(this, "FileSelectState", "ParGuide_Guide_Normal"));
        changeFileSelectButtonStateSelect();
    }

    if (mGuideWindow->isWait() && (mCopyButton->isWait() || mCopyButton->isHide()) &&
        (mDeleteButton->isWait() || mDeleteButton->isHide())) {
        mBackButton->setInputEnable(true);
        mButtonGroup->validate();
        mCopyButton->setInputEnable(true);
        mDeleteButton->setInputEnable(true);
        mButtonGroup->showCursorAppear();
        al::setNerve(this, &NrvFileSelectSelect);
    }
}

/** @brief Shows the copy guide, then lets the copy source be chosen. */
void FileSelect::exeCopyIn() {
    if (al::isFirstStep(this)) {
        mGuideWindow->appear("ColorCopy");
        mGuideWindow->setText(
            al::getSystemMessageString(this, "FileSelectState", "ParGuide_Guide_Copy"));
        changeFileSelectButtonStateCopySrc();
    }

    if (mGuideWindow->isWait()) {
        mButtonGroup->showCursorAppear();
        mButtonGroup->validate();
        mBackButton->setInputEnable(true);
        al::setNerve(this, &NrvFileSelectCopy);
    }
}

/** @brief Enables the buttons of the used files only. */
void FileSelect::changeFileSelectButtonStateCopySrc() {
    s32 buttonNum = mButtonGroup->getButtonNum();
    for (s32 i = 0; i < buttonNum; i++) {
        CursorTarget* button = mButtonGroup->getButton(i);
        if (mGameDataHolder->isNewFile(i)) {
            button->disable();
        } else {
            button->enable();
        }
    }

    mButtonGroup->update();
}

/** @brief Waits for the copy source to be chosen. */
void FileSelect::exeCopy() {
    mButtonGroup->updateAndCursorDefault(mPort);
    lockButtonEachOther();
    if (mButtonGroup->isDecideAny()) {
        if (!mIsDecideSePlayed) {
            al::startSe(this, "CopyFileSelected");
        }

        mIsDecideSePlayed = true;
    }

    if (mButtonGroup->isDecideEndAny()) {
        mCopySrcIndex = getSelectedFileIndex();
        mButtonGroup->reset();
        mIsDecideSePlayed = false;
        showAndNextNerve(&NrvFileSelect.CopyDstIn);
        return;
    }

    if (mBackButton->isDecideEnd()) {
        mBackButton->reactivate();
        showAndNextNerve(&NrvFileSelect.SelectBack);
    }
}

/** @brief Shows the copy destination guide, then lets the destination be chosen. */
void FileSelect::exeCopyDstIn() {
    if (al::isFirstStep(this)) {
        mGuideWindow->appear("ColorCopy");
        mGuideWindow->setText(
            al::getSystemMessageString(this, "FileSelectState", "ParGuide_Guide_CopyDst"));
        changeFileSelectButtonStateCopyDst();
    }

    if (mGuideWindow->isWait()) {
        mButtonGroup->validate();
        mBackButton->setInputEnable(true);
        al::setNerve(this, &NrvFileSelectCopyDst);
    }
}

/** @brief Waits for the copy destination to be chosen. */
void FileSelect::exeCopyDst() {
    mButtonGroup->updateAndCursorDefault(mPort);
    lockButtonEachOther();
    if (mButtonGroup->isDecideAny()) {
        if (!mIsDecideSePlayed) {
            al::startSe(this, "CopyFileDstSelected");
        }

        mIsDecideSePlayed = true;
    }

    if (mButtonGroup->isDecideEndAny()) {
        mIsDecideSePlayed = false;
        mCopyDstIndex = getSelectedFileIndex();
        goToWait(Result_Copy);
        return;
    }

    if (mBackButton->isDecideEnd()) {
        mBackButton->reactivate();
        showAndNextNerve(&NrvFileSelect.SelectBack);
    }
}

/**
 * @brief Waits for the owner to process a copy or deletion.
 * @param result The requested operation.
 */
void FileSelect::goToWait(Result result) {
    mBackButton->setInputEnable(false);
    mButtonGroup->invalidate();
    mResult = result;
    al::setNerve(this, &NrvFileSelect.Wait);
}

/** @brief Shows the delete guide, then lets the file to delete be chosen. */
void FileSelect::exeDeleteIn() {
    if (al::isFirstStep(this)) {
        mGuideWindow->appear("ColorDelete");
        mGuideWindow->setText(
            al::getSystemMessageString(this, "FileSelectState", "ParGuide_Guide_Delete"));
        changeFileSelectButtonStateDelete();
    }

    if (mGuideWindow->isWait()) {
        mButtonGroup->showCursorAppear();
        mButtonGroup->validate();
        mBackButton->setInputEnable(true);
        al::setNerve(this, &NrvFileSelectDelete);
    }
}

/** @brief Waits for the file to delete to be chosen. */
void FileSelect::exeDelete() {
    mButtonGroup->updateAndCursorDefault(mPort);
    lockButtonEachOther();
    if (mButtonGroup->isDecideAny()) {
        if (!mIsDecideSePlayed) {
            al::startSe(this, "DeleteFileSelected");
        }

        mIsDecideSePlayed = true;
    }

    if (mButtonGroup->isDecideEndAny()) {
        goToWait(Result_Delete);
        mIsDecideSePlayed = false;
    }

    if (mBackButton->isDecideEnd()) {
        mBackButton->reactivate();
        showAndNextNerve(&NrvFileSelect.SelectBack);
    }
}

/** @brief Waits for the copy / delete buttons and the guide window to be hidden. */
void FileSelect::exePartsHide() {
    if (!mCopyButton->isActive() && !mDeleteButton->isActive() && mGuideWindow->isWait()) {
        al::setNerve(this, mNextNerve);
    }
}

/** @brief Waits for the guide window to be shown. */
void FileSelect::exePartsShow() {
    if (mGuideWindow->isWait()) {
        al::setNerve(this, mNextNerve);
    }
}

/** @brief Waits for the owner to process the chosen operation. */
void FileSelect::exeWait() {}

/** @brief Shows the auto power down notice and closes the layout once it is closed. */
void FileSelect::exeConfirm() {
    if (al::isFirstStep(this)) {
        mWindowConfirm->appearWithSystemMessage("FileSelectState", "WindowConfirm_AutoPowerDown",
                                                mPort, nullptr);
    }

    if (!mWindowConfirm->isAlive()) {
        goToEnd(Result_Decide);
    }
}

/** @brief Plays the end animation. */
void FileSelect::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvFileSelectEndWait);
    }
}

/** @brief Waits for the owner to close the layout. */
void FileSelect::exeEndWait() {}

/** @brief Ends the copy and delete buttons if they are shown or decided. */
void FileSelect::tryDisappearPlusMinus() {
    if (mCopyButton->isActive() || mCopyButton->isDecideEnd()) {
        mCopyButton->end();
    }

    if (mDeleteButton->isActive() || mDeleteButton->isDecideEnd()) {
        mDeleteButton->end();
    }
}
