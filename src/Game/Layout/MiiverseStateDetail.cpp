#include "Layout/MiiverseStateDetail.hpp"
#include "Layout/WindowMiiverseDetail.hpp"
#include "Layout/WindowProcessing.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
namespace {
NERVE_DECL(MiiverseStateDetail, Detail);
NERVE_DECL(MiiverseStateDetail, End);
NERVE_DECL(MiiverseStateDetail, YeahWait);
NERVE_DECL(MiiverseStateDetail, Comment);
NERVE_DECL(MiiverseStateDetail, Error);
NERVES_MAKE_NOSTRUCT(MiiverseStateDetail, Detail, End, YeahWait, Comment, Error)
}
/** @brief Creates the detail, processing, and confirmation windows. */
MiiverseStateDetail::MiiverseStateDetail(const al::LayoutInitInfo& rInfo, al::NetworkSystem* pNetwork, al::ErrorViewer* pErrorViewer)
    : al::NerveExecutor("Miiverse詳細表示ステート"), mNetwork(pNetwork), mErrorViewer(pErrorViewer) {
    mDetail = new WindowMiiverseDetail(rInfo);
    mProcessing = new WindowProcessing(rInfo, "PauseMenu");
    mReport = new WindowConfirm(WindowConfirmType_Report, rInfo, "PauseMenu", false);
    mConfirm = new WindowConfirm(WindowConfirmType_Double, rInfo, "PauseMenu", false);
    initNerve(&NrvMiiverseStateDetailDetail, 0);
}
/** @brief Shows a post for the requested controller. */
void MiiverseStateDetail::appear(const al::MiiversePostDataKeeper* pPost, int port) {
    mPort = port;
    mPost = pPost;
    mDetail->startShow();
    al::setNerve(this, &NrvMiiverseStateDetailDetail);
}
/** @brief Reports when the detail window has closed. */
bool MiiverseStateDetail::isEnd() const { return al::isNerve(this, &NrvMiiverseStateDetailEnd) && !mDetail->isAlive(); }
/** @brief Dispatches the selected detail action. */
void MiiverseStateDetail::exeDetail() {
    if (!mDetail->isDecidedAny()) return;
    if (mDetail->isDecidedClose()) {
        mDetail->endShow();
        al::setNerve(this, &NrvMiiverseStateDetailEnd);
    }
    if (mDetail->isDecidedYeah()) {
        mDetail->invalidateButton();
        al::setNerve(this, &NrvMiiverseStateDetailYeahWait);
    }
    if (mDetail->isDecidedComment()) al::setNerve(this, &NrvMiiverseStateDetailComment);
}
/** @brief Shows processing before taking the unavailable-service path. */
void MiiverseStateDetail::exeYeahWait() {
    if (al::isFirstStep(this)) mProcessing->appearWithSystemMessage("MiiverseMessage", "WindowProcessing_Post", 0, true);
    al::setNerve(this, &NrvMiiverseStateDetailError);
}
/** @brief Displays a successful-post acknowledgement and closes the details. */
void MiiverseStateDetail::exeYeahReport() {
    if (al::isFirstStep(this)) mReport->appearWithSystemMessage("MiiverseMessage", "PostSuccess", mPort, nullptr);
    if (!mReport->isAlive()) {
        mDetail->endShow();
        al::setNerve(this, &NrvMiiverseStateDetailEnd);
    }
}
/** @brief Confirms opening comments and handles cancellation. */
void MiiverseStateDetail::exeComment() {
    if (al::isFirstStep(this)) mConfirm->appearWithSystemMessage("MiiverseMessage", "ConfirmComment", mPort, "Rボタン");
    if (mConfirm->isDecideRight()) al::stopAllSequenceBgm(mDetail, 25);
    if (!mConfirm->isAlive()) {
        if (mConfirm->isDecideLeft()) {
            mDetail->reset();
            al::setNerve(this, &NrvMiiverseStateDetailDetail);
        } else {
            mDetail->endShow();
            al::setNerve(this, &NrvMiiverseStateDetailEnd);
        }
    }
}
/** @brief Closes the detail window when the online operation is unavailable. */
void MiiverseStateDetail::exeError() {
    mDetail->endShow();
    al::setNerve(this, &NrvMiiverseStateDetailEnd);
}
/** @brief Releases the selected post reference. */
void MiiverseStateDetail::exeEnd() { mPost = nullptr; }
