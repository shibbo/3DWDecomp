#include "CourseSelect/CourseSelectStateRevivePlayer.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectWorldWarpDokan.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {
NERVE_DECL(CourseSelectStateRevivePlayer, Start);
NERVE_DECL(CourseSelectStateRevivePlayer, Play);
NERVE_DECL(CourseSelectStateRevivePlayer, End);
NERVES_MAKE_NOSTRUCT(CourseSelectStateRevivePlayer, Start, Play, End)
}
/** @brief Prepare to restore players who died in the preceding course. */
CourseSelectStateRevivePlayer::CourseSelectStateRevivePlayer(CourseSelectScene* pScene, bool isEntryDemo)
    : al::NerveStateBase("プレイヤー復活デモ"), mpScene(pScene), mIsEntryDemo(isEntryDemo) {}
void CourseSelectStateRevivePlayer::init() {
    initNerve(&NrvCourseSelectStateRevivePlayerStart, 0);
}
void CourseSelectStateRevivePlayer::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvCourseSelectStateRevivePlayerStart);
}
/** @brief Count dead players and pause briefly before restoring them. */
void CourseSelectStateRevivePlayer::exeStart() {
    if (al::isFirstStep(this)) {
        mDeadPlayerCount = 0;
        for (int i = rc::getControlUserNumMax() - 1; i >= 0; --i)
            if (rc::isDeadControlUserInStage(GameDataHolderAccessor(mpScene->getGameDataHolderDirect()), i))
                ++mDeadPlayerCount;
        if (mDeadPlayerCount == 0) { kill(); return; }
    }
    if (al::isGreaterEqualStep(this, 60)) al::setNerve(this, &NrvCourseSelectStateRevivePlayerPlay);
}
/** @brief Restore dead users at the last course and wait for their entry demos. */
void CourseSelectStateRevivePlayer::exePlay() {
    if (al::isFirstStep(this)) {
        int courseId = GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(mpScene->getGameDataHolderDirect()));
        CourseSelectMiniature* pMiniature = mpScene->getCourseSelectDirector()->tryFindMiniatureObj(courseId);
        if (!pMiniature) { kill(); return; }
        ICourseSelectActorController* pController;
        if (GameDataFunction::isLastPlayCourseClearWorldWarp(GameDataHolderAccessor(mpScene->getGameDataHolderDirect())))
            pController = pMiniature->getWorldWarpDokan()->getController();
        else pController = pMiniature->getController();
        for (int i = rc::getControlUserNumMax() - 1; i >= 0; --i) {
            bool isDead = rc::isDeadControlUserInStage(GameDataHolderAccessor(mpScene->getGameDataHolderDirect()), i);
            CourseSelectDirector* pDirector = mpScene->getCourseSelectDirector();
            if (isDead) {
                pDirector->reviveUser(i, pController, mIsEntryDemo);
                rc::resetDeadFlagControlUserInStage(GameDataHolderWriter(mpScene->getGameDataHolderDirect()), i);
                GameDataFunction::addPlayerLife(GameDataHolderWriter(mpScene->getGameDataHolderDirect()), 5);
            } else pDirector->getPuppeteerGroup()->startWaitDemo(i);
        }
    } else if (!mpScene->getCourseSelectDirector()->isPlayEntryDemoAny()) {
        mpScene->getCourseSelectDirector()->getPuppeteerGroup()->endDemo();
        al::setNerve(this, &NrvCourseSelectStateRevivePlayerEnd);
    }
}
/** @brief Allow the revival animation to settle before leaving the state. */
void CourseSelectStateRevivePlayer::exeEnd() {
    if (al::isGreaterEqualStep(this, 20)) kill();
}
