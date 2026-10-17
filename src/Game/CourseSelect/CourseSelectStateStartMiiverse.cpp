#include "CourseSelect/CourseSelectStateStartMiiverse.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderWriter.hpp"

namespace {
NERVE_DECL(CourseSelectStateStartMiiverse, InfoMiiverse);
NERVE_DECL(CourseSelectStateStartMiiverse, InfoGhost);
NERVE_DECL(CourseSelectStateStartMiiverse, InfoLocalActMiiverse);
NERVE_DECL(CourseSelectStateStartMiiverse, InfoLocalActGhost);
NERVE_DECL(CourseSelectStateStartMiiverse, Confirm);
NERVE_DECL(CourseSelectStateStartMiiverse, ConfirmLocal);
NERVE_DECL(CourseSelectStateStartMiiverse, Error);
NERVE_DECL(CourseSelectStateStartMiiverse, Decide);
NERVES_MAKE_NOSTRUCT(CourseSelectStateStartMiiverse, InfoMiiverse, InfoGhost, InfoLocalActMiiverse, InfoLocalActGhost, Confirm, ConfirmLocal, Error, Decide)
}
/** @brief Prepare the network setup notices and confirmation windows. */
CourseSelectStateStartMiiverse::CourseSelectStateStartMiiverse(CourseSelectScene* pScene)
    : al::HostStateBase<CourseSelectScene>("Miiverse開始ステート", pScene) {
    mpWindows = pScene->getWindowHolder();
}
void CourseSelectStateStartMiiverse::init() {
    initNerve(&NrvCourseSelectStateStartMiiverseInfoMiiverse, 0);
}
/** @brief Select the notices appropriate to the user's account type. */
void CourseSelectStateStartMiiverse::appear() {
    al::NerveStateBase::appear();
    if (GameDataFunction::isNetworkAccount(GameDataHolderAccessor(getHost()->getGameDataHolderDirect())))
        al::setNerve(this, &NrvCourseSelectStateStartMiiverseInfoMiiverse);
    else al::setNerve(this, &NrvCourseSelectStateStartMiiverseInfoLocalActMiiverse);
}
/** @brief Display the AvailableNetwork notice and advance after dismissal. */
void CourseSelectStateStartMiiverse::exeInfoMiiverse() {
    if (al::isFirstStep(this)) mpWindows->appearMessage("AvailableNetwork");
    if (!mpWindows->isMessageActive()) al::setNerve(this, &NrvCourseSelectStateStartMiiverseInfoGhost);
}
/** @brief Display the AvailableNetwork2nd notice and advance after dismissal. */
void CourseSelectStateStartMiiverse::exeInfoGhost() {
    if (al::isFirstStep(this)) mpWindows->appearMessage("AvailableNetwork2nd");
    if (!mpWindows->isMessageActive()) al::setNerve(this, &NrvCourseSelectStateStartMiiverseConfirm);
}
/** @brief Display the AvailableNetworkLocalAct notice and advance after dismissal. */
void CourseSelectStateStartMiiverse::exeInfoLocalActMiiverse() {
    if (al::isFirstStep(this)) mpWindows->appearMessage("AvailableNetworkLocalAct");
    if (!mpWindows->isMessageActive()) al::setNerve(this, &NrvCourseSelectStateStartMiiverseInfoLocalActGhost);
}
/** @brief Display the AvailableNetworkLocalAct2nd notice and advance after dismissal. */
void CourseSelectStateStartMiiverse::exeInfoLocalActGhost() {
    if (al::isFirstStep(this)) mpWindows->appearMessage("AvailableNetworkLocalAct2nd");
    if (!mpWindows->isMessageActive()) al::setNerve(this, &NrvCourseSelectStateStartMiiverseConfirmLocal);
}
/** @brief Apply the network choice, checking account restrictions before enabling it. */
void CourseSelectStateStartMiiverse::exeConfirm() {
    if (al::isFirstStep(this)) mpWindows->appearConfirmTwoBtn("StartNetwork", true);
    if (mpWindows->isConfirmActive()) return;
    GameDataFlagFunction::setOpenNetworkSetting(GameDataHolderWriter(getHost()->getGameDataHolderDirect()));
    if (mpWindows->isConfirmDecideLeft()) {
        GameDataFunction::setNetworkSettingInit(GameDataHolderWriter(getHost()->getGameDataHolderDirect()), false);
        kill();
    } else if (mpWindows->isConfirmDecideRight()) {
        if (GameDataFunction::isMiiverseForbidden(GameDataHolderAccessor(getHost()->getGameDataHolderDirect()))
            || GameDataFunction::isInGameNetworkForbidden(GameDataHolderAccessor(getHost()->getGameDataHolderDirect())))
            al::setNerve(this, &NrvCourseSelectStateStartMiiverseError);
        else {
            GameDataFunction::setNetworkSettingInit(GameDataHolderWriter(getHost()->getGameDataHolderDirect()), true);
            al::setNerve(this, &NrvCourseSelectStateStartMiiverseDecide);
        }
    }
}
/** @brief Finish local-account setup with network features disabled. */
void CourseSelectStateStartMiiverse::exeConfirmLocal() {
    if (al::isFirstStep(this)) mpWindows->appearConfirmOneBtn("StartNetworkLocal", nullptr);
    if (!mpWindows->isConfirmActive()) {
        GameDataFlagFunction::setOpenNetworkSetting(GameDataHolderWriter(getHost()->getGameDataHolderDirect()));
        GameDataFunction::setNetworkSettingInit(GameDataHolderWriter(getHost()->getGameDataHolderDirect()), false);
        kill();
    }
}
/** @brief Explain the parental restriction and finish when the notice closes. */
void CourseSelectStateStartMiiverse::exeError() {
    if (al::isFirstStep(this)) mpWindows->appearConfirmOneBtn("ParentalLock", "ErrorMessage");
    if (!mpWindows->isConfirmActive()) kill();
}
/** @brief Acknowledge successful network setup. */
void CourseSelectStateStartMiiverse::exeDecide() {
    if (al::isFirstStep(this)) mpWindows->appearConfirmReport("EndSetup");
    if (!mpWindows->isConfirmActive()) kill();
}
