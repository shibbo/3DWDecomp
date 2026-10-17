#include "Scene/TitleStateFileSelect.hpp"
#include "Layout/RCS_SaveDataLayout.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/GameDataHolder.hpp"
#include "System/SaveDataAccessFunction.hpp"
namespace {
NERVE_DECL(TitleStateFileSelect, Select)
NERVE_DECL(TitleStateFileSelect, Copy)
NERVE_DECL(TitleStateFileSelect, CopyResult)
NERVE_DECL(TitleStateFileSelect, CopyEnd)
NERVE_DECL(TitleStateFileSelect, Delete)
NERVE_DECL(TitleStateFileSelect, DeleteResult)
NERVE_DECL(TitleStateFileSelect, DeleteEnd)
NERVES_MAKE_NOSTRUCT(TitleStateFileSelect, Select, Copy, CopyResult, CopyEnd, Delete, DeleteResult,
                     DeleteEnd)
} // namespace

TitleStateFileSelect::TitleStateFileSelect(RCS_SaveDataLayout* pLayout,
                                           GameDataHolder* pGameDataHolder,
                                           const al::LayoutInitInfo& rInfo)
    : al::NerveStateBase("ファイル選択ステート"), mSaveDataLayout(pLayout),
      mGameDataHolder(pGameDataHolder) {
    mProcessing = new WindowProcessing(rInfo, "FileSelect");
    mReport = new WindowConfirm(WindowConfirmType_Report, rInfo, "FileSelect", false);
    mConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "FileSelect", false);
    initNerve(&NrvTitleStateFileSelectSelect, 0);
    kill();
}
void TitleStateFileSelect::appear() {
    mIsDead = false;
    mResult = 0;
    mSaveDataLayout->appearLoad(al::getMainControllerPort());
    al::setNerve(this, &NrvTitleStateFileSelectSelect);
}
void TitleStateFileSelect::setControllerPort(s32 port) {}
void TitleStateFileSelect::exeSelect() {
    if (mSaveDataLayout->isEndBack()) {
        mResult = 1;
    } else if (mSaveDataLayout->isNeedLoad()) {
        mResult = 2;
    } else {
        return;
    }
    kill();
}
void TitleStateFileSelect::exeCopyConfirm() {
    if (al::isFirstStep(this)) {
        bool isNewFile = mGameDataHolder->isNewFile(0);
        if (isNewFile)
            mConfirm->appearWithSystemMessage("FileSelectState", "WindowConfirmDouble_Copy",
                                              al::getMainControllerPort(), nullptr);
        else
            mConfirm->appearWithSystemMessage("FileSelectState", "WindowConfirmDouble_Replace",
                                              al::getMainControllerPort(), nullptr);
    }
    if (!mConfirm->isAlive()) {
        if (mConfirm->isDecideLeft()) {
            al::setNerve(this, &NrvTitleStateFileSelectSelect);
        } else {
            mGameDataHolder->copySaveFile(0, 0);
            mFileId = 0;
            al::setNerve(this, &NrvTitleStateFileSelectCopy);
        }
    }
}
void TitleStateFileSelect::exeCopy() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, false, false);
        mProcessing->appearWithSystemMessage("FileSelectState", "WindowConfirmProcessing_Copy", 0,
                                             true);
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        mProcessing->requestClose();
        if (mProcessing->isEnd())
            al::setNerve(this, &NrvTitleStateFileSelectCopyResult);
    }
}
void TitleStateFileSelect::exeCopyResult() {
    if (!mProcessing->isAlive()) {
        mReport->appearWithSystemMessage("FileSelectState", "WindowConfirmReport_Copy",
                                         al::getMainControllerPort(), nullptr);
        al::setNerve(this, &NrvTitleStateFileSelectCopyEnd);
    }
}
void TitleStateFileSelect::exeCopyEnd() {
    if (!mReport->isAlive())
        al::setNerve(this, &NrvTitleStateFileSelectSelect);
}
void TitleStateFileSelect::backToSelect() { al::setNerve(this, &NrvTitleStateFileSelectSelect); }
void TitleStateFileSelect::exeDeleteConfirm() {
    if (al::isFirstStep(this))
        mConfirm->appearWithSystemMessage("FileSelectState", "WindowConfirmDouble_Delete",
                                          al::getMainControllerPort(), nullptr);
    if (!mConfirm->isAlive()) {
        if (mConfirm->isDecideLeft()) {
            al::setNerve(this, &NrvTitleStateFileSelectSelect);
        } else {
            mGameDataHolder->deleteSaveFile(mFileId);
            al::setNerve(this, &NrvTitleStateFileSelectDelete);
        }
    }
}
void TitleStateFileSelect::exeDelete() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWriteNoWindow(mGameDataHolder, false, false);
        mProcessing->appearWithSystemMessage("FileSelectState", "WindowConfirmProcessing_Delete", 0,
                                             true);
    }
    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        mProcessing->requestClose();
        if (mProcessing->isEnd())
            al::setNerve(this, &NrvTitleStateFileSelectDeleteResult);
    }
}
void TitleStateFileSelect::exeDeleteResult() {
    if (!mProcessing->isAlive()) {
        mReport->appearWithSystemMessage("FileSelectState", "WindowConfirmReport_Delete",
                                         al::getMainControllerPort(), nullptr);
        al::setNerve(this, &NrvTitleStateFileSelectDeleteEnd);
    }
}
void TitleStateFileSelect::exeDeleteEnd() {
    if (!mReport->isAlive())
        al::setNerve(this, &NrvTitleStateFileSelectSelect);
}
