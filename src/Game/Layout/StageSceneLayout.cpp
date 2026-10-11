#include "Layout/StageSceneLayout.hpp"

#include "Layout/ButtonItemStockParts.hpp"
#include "Layout/CounterCoinParts.hpp"
#include "Layout/CounterGreenStarParts.hpp"
#include "Layout/CounterPlayerParts.hpp"
#include "Layout/CounterScoreParts.hpp"
#include "Layout/CounterStampParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/StageTimer.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(StageSceneLayout, Wait);
NERVES_MAKE_NOSTRUCT(StageSceneLayout, Wait)

/**
 * @brief Check whether a course is one of the small rooms that hide the timer and the score.
 * @param pGameDataHolder The game data.
 * @param courseId The course.
 * @return True for Toad houses, fairy houses, the casino room and the golden express.
 */
inline bool isStageNoTimer(GameDataHolder* pGameDataHolder, s32 courseId) {
    return GameDataFunction::isStageKinopioHouse(GameDataHolderAccessor(pGameDataHolder),
                                                 courseId) ||
           GameDataFunction::isStageKinopioHouseHide(GameDataHolderAccessor(pGameDataHolder),
                                                     courseId) ||
           GameDataFunction::isStageFairyHouse(GameDataHolderAccessor(pGameDataHolder),
                                               courseId) ||
           GameDataFunction::isStageCasinoRoom(GameDataHolderAccessor(pGameDataHolder),
                                               courseId) ||
           GameDataFunction::isStageGoldenExpress(GameDataHolderAccessor(pGameDataHolder),
                                                  courseId);
}
}  // namespace

/**
 * @brief Create the stage HUD and all of its counters.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder The game data.
 * @param pStageName The name of the stage, given to the timer.
 * @param pGreenStarKeeper The green stars of the stage.
 * @param pIllustItemKeeper The stamps of the stage, or nullptr if it has none.
 * @param pPlayerHolder The players.
 * @param pPlayerAliveWatcher Watches the alive players, used by the player counter.
 * @param pItemDirector The item director that sends stocked items to this HUD.
 */
StageSceneLayout::StageSceneLayout(const al::LayoutInitInfo& rInfo,
                                   GameDataHolder* pGameDataHolder, const char* pStageName,
                                   const GreenStarKeeper* pGreenStarKeeper,
                                   const IllustItemKeeper* pIllustItemKeeper,
                                   const al::PlayerHolder* pPlayerHolder,
                                   const PlayerAliveWatcher* pPlayerAliveWatcher,
                                   ProjectItemDirector* pItemDirector)
    : al::LayoutActor("ステージシーンレイアウト"), mGameDataHolder(pGameDataHolder),
      mPlayerHolder(pPlayerHolder), mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "StageSceneLayout", nullptr);
    initNerve(&NrvStageSceneLayoutWait, 0);

    mCounterCoin = new CounterCoinParts(rInfo, "カウンターコイン", "ParCounterCoin", this);
    mStageTimer = new StageTimer(rInfo, "ステージタイマー", "ParCounterTime", this,
                                 pGameDataHolder->getStageDataHolderPtr(), pStageName);
    mCounterPlayer = new CounterPlayerParts(rInfo, "カウンタープレイヤー", "ParCounterPlayer",
                                            this, pPlayerAliveWatcher, false);
    mCounterScore = new CounterScoreParts(rInfo, "カウンタースコア", "ParCounterScore", this,
                                          pGameDataHolder);
    if (pIllustItemKeeper != nullptr) {
        mCounterStamp = new CounterStampParts(rInfo, "カウンタースタンプ", "ParCounterStamp",
                                              this, pIllustItemKeeper);
    }

    mCounterGreenStar = new CounterGreenStarParts(rInfo, "カウンターグリーンスター",
                                                  "ParCounterGreenStar", this, pGreenStarKeeper);
    mButtonItemStock = new ButtonItemStockParts(rInfo, "アイテムストック", "ParItemStock", this,
                                                pItemDirector, false);
    pItemDirector->setSceneLayout(this);
    appear();

    s32 courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(mGameDataHolder));
    al::setSceneObj(this, mStageTimer, SceneObjID_StageTimer);
    if (isStageNoTimer(mGameDataHolder, courseId)) {
        al::hidePaneRoot(mCounterScore);
    }

    if (isStageNoTimer(mGameDataHolder, courseId)) {
        al::hidePaneRoot(mStageTimer);
    }
}

/**
 * @brief Hide the HUD for a demo.
 * @param isHideAll True to hide everything immediately.
 * @param isEndAction True to play the end action (when not hiding everything).
 */
void StageSceneLayout::startDemo(bool isHideAll, bool isEndAction) {
    if (isHideAll) {
        al::startFreezeActionEnd(this, "End", nullptr);
        getLayoutKeeper()->calcAnim(true);
        al::hidePaneRootNoRecursive(this);
    } else if (isEndAction) {
        al::startAction(this, "End", nullptr);
    }

    mButtonItemStock->startDemo();
    mStageTimer->startDemo();
    mCounterPlayer->startDemo();
    setNameplatesVisible(false, isHideAll);
}

/**
 * @brief Show or hide the nameplates of all alive players.
 * @param isVisible True to show the nameplates.
 * @param isForce True to change them immediately.
 */
void StageSceneLayout::setNameplatesVisible(bool isVisible, bool isForce) {
    s32 playerNum = al::getPlayerNumMax(mPlayerHolder);
    for (s32 i = 0; i < playerNum; i++) {
        auto* player = static_cast<PlayerActor*>(al::getPlayerActor(mPlayerHolder, i));
        if (al::isAlive(player)) {
            player->setNameplateVisible(isVisible, isForce);
        }
    }
}

/**
 * @brief Show the HUD again after a demo.
 * @param isAppear True to play the appear action.
 * @param isResetAction True to snap the HUD to its hidden state first.
 */
void StageSceneLayout::endDemo(bool isAppear, bool isResetAction) {
    al::showPaneRootNoRecursive(this);
    if (isResetAction) {
        al::startFreezeActionEnd(this, "End", nullptr);
    }

    getLayoutKeeper()->calcAnim(true);
    if (isAppear) {
        al::startAction(this, "Appear", nullptr);
    }

    mButtonItemStock->endDemo();
    mStageTimer->endDemo();
    mCounterPlayer->endDemo();
    setNameplatesVisible(true, false);
}

/** @brief Play the pause action. */
void StageSceneLayout::startPause() {
    al::startAction(this, "PauseStart", "Pause");
}

/** @brief Play the unpause action. */
void StageSceneLayout::endPause() {
    al::startAction(this, "PauseEnd", "Pause");
}

/** @brief Play the course clear action. */
void StageSceneLayout::courseClear() {
    al::startAction(this, "CourseClear", nullptr);
}

/** @brief Disable the item stock button. */
void StageSceneLayout::disableItemStock() {
    mButtonItemStock->setDisable();
}

/**
 * @brief Put an item into the item stock.
 * @param pItem The item.
 * @param pPlayer The player who got it.
 * @param itemType The item type.
 */
void StageSceneLayout::stockItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                                 s32 itemType) {
    mButtonItemStock->stockItem(pItem, pPlayer, itemType);
}

/**
 * @brief Does nothing in a normal stage.
 * @param pItem The item.
 * @param pPlayer The player who got it.
 * @param itemType The item type.
 */
void StageSceneLayout::stockItemSilent(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                                       s32 itemType) {}

/**
 * @brief Find which panes contain any of the given layout points.
 * @param pIsHit In/out: per pane, whether it is hit. Panes already hit are skipped.
 * @param pPaneNames The pane names, checked as "Hit<name>".
 * @param paneNum The number of panes.
 * @param rPoints The layout positions to check.
 */
void StageSceneLayout::calcHitPane(bool* pIsHit, const char** pPaneNames, s32 paneNum,
                                   const HitPointBuffer& rPoints) {
    s32 hitNum = 0;
    for (s32 i = 0; i < rPoints.size(); i++) {
        for (s32 j = 0; j < paneNum; j++) {
            if (pIsHit[j]) {
                continue;
            }

            al::StringTmp<128> paneName("Hit%s", pPaneNames[j]);
            if (al::isContainPointPane(this, paneName.cstr(), rPoints[i])) {
                hitNum++;
                if (hitNum == paneNum) {
                    return;
                }

                pIsHit[j] = true;
            }
        }
    }
}

/** @brief Fade out the counters the players (or the touch) are behind. */
void StageSceneLayout::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }

    HitPointBuffer points;
    s32 playerNum = mPlayerHolder->getPlayerNum();
    for (s32 i = 0; i < playerNum; i++) {
        if (rc::isPlayerDeadOrBubble(mPlayerHolder->getPlayer(i))) {
            continue;
        }

        const sead::Vector3f& trans = al::getTrans(mPlayerHolder->getPlayer(i));
        sead::Vector2f layoutPos = sead::Vector2f::zero;
        sead::Vector3f pos;
        pos.setScaleAdd(150.0f, sead::Vector3f::ey, trans);
        al::calcLayoutPosFromWorldPos(&layoutPos, this, pos, 0);
        points.pushBack(layoutPos);
    }

    s32 touchPort = rc::calcTouchPanelPortByPortNum(al::getMainControllerPort());
    if (al::isPadHoldTouch(touchPort)) {
        sead::Vector2f touchPos;
        al::calcTouchLayoutPos(&touchPos, touchPort);
        points.pushBack(touchPos);
    }

    // The timer pane is checked last but named first.
    const char* timerPaneName = "CounterTime";
    const char* paneNames[] = {"CounterCoin",  "CounterGreenStar", "CounterPlayer",
                               "CounterScore", "CounterStamp",     timerPaneName};
    bool isHit[6] = {};
    calcHitPane(isHit, paneNames, 6, points);

    for (s32 i = 0; i < 6; i++) {
        bool isFadeOut = al::isActionPlaying(this, "FadeOut", paneNames[i]);
        if (isHit[i] != isFadeOut) {
            al::startAction(this, isHit[i] ? "FadeOut" : "FadeIn", paneNames[i]);
        }
    }
}

/** @brief Switch the player counter to its Toad Brigade style. */
void StageSceneLayout::setStageKinopioBrigade() {
    mCounterPlayer->setKinopioBrigade();
}
