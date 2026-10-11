#include "Layout/SingleModeSceneLayout.hpp"

#include <prim/seadSafeString.h>

#include "Course/GuideFrameOutSingleMode.hpp"
#include "Course/ScenarioShineCounterParts.hpp"
#include "Layout/AreaNameParts.hpp"
#include "Layout/CounterCoinParts.hpp"
#include "Layout/CounterGoalItemParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Layout/Switch/ChallengeTimerParts.hpp"
#include "Layout/Switch/CounterTimerGate.hpp"
#include "Layout/Switch/ItemStockTray.hpp"
#include "Layout/Switch/ShardCounterParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(SingleModeSceneLayout, Wait);
NERVES_MAKE_NOSTRUCT(SingleModeSceneLayout, Wait)
}  // namespace

/**
 * @brief Find the Bowser's Fury HUD of the current scene.
 * @param pHolder Any object with access to the scene objects.
 * @return The HUD, or nullptr if the scene has none.
 */
SingleModeSceneLayout*
SingleModeSceneLayout::tryGetSingleModeSceneLayout(const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<SingleModeSceneLayout>(pHolder, SceneObjID_SingleModeSceneLayout);
}

/**
 * @brief Forbid (or allow again) opening the pause menu, if the scene has a HUD.
 * @param pHolder Any object with access to the scene objects.
 * @param isDisable True to forbid pausing.
 */
void SingleModeSceneLayout::tryDisablePause(const al::IUseSceneObjHolder* pHolder,
                                            bool isDisable) {
    SingleModeSceneLayout* layout = tryGetSingleModeSceneLayout(pHolder);
    if (layout != nullptr) {
        layout->mIsDisablePause = isDisable;
    }
}

/**
 * @brief Create the HUD and all of its counters.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder The game data, used by the item tray.
 * @param pName Unused name.
 * @param pGreenStarKeeper Unused.
 * @param pPlayerHolder The players, used to place the hit checks and the area name.
 * @param pAliveWatcher Unused.
 * @param pItemDirector The item director that sends stocked items to this HUD.
 */
SingleModeSceneLayout::SingleModeSceneLayout(const al::LayoutInitInfo& rInfo,
                                             GameDataHolder* pGameDataHolder, const char* pName,
                                             const GreenStarKeeper* pGreenStarKeeper,
                                             const al::PlayerHolder* pPlayerHolder,
                                             const PlayerAliveWatcher* pAliveWatcher,
                                             ProjectItemDirector* pItemDirector)
    : al::LayoutActor("SingleModeSceneLayout"), mGameDataHolder(pGameDataHolder),
      mPlayerHolder(pPlayerHolder), mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "SingleModeSceneLayout", nullptr);
    initNerve(&NrvSingleModeSceneLayoutWait, 0);

    mWipeBlack = new al::WipeSimple("DemoWipeFadeBlack", "WipeFadeBlack", rInfo, nullptr);
    mWipe = new al::WipeSimple("DemoWipeFadeWhite", "WipeFadeWhite", rInfo, nullptr);
    mCounterCoin = new CounterCoinParts(rInfo, "カウンターコイン", "ParCounterCoin", this);
    mCounterGoalItem =
        new CounterGoalItemParts(rInfo, "GoalItemCounter", "ParCounterGoalItem", this);
    pItemDirector->setSceneLayout(this);
    mAreaName = new AreaNameParts(rInfo, "AreaName", "ParAreaName", this, pPlayerHolder);
    mItemStockTray = new ItemStockTray(rInfo, "ItemStockTray", "ParItemStock", this,
                                       mGameDataHolder, pItemDirector);
    mShardCounter = new ShardCounterParts(rInfo, "ShardCounterParts", "ParShardCounter", this);
    mScenarioShineCounter = new ScenarioShineCounterParts(rInfo, "ScenarioShineCounterParts",
                                                          "ParScenarioShine", this);
    mChallengeTimer =
        new ChallengeTimerParts(rInfo, "ChallengeTimerParts", "ParChallengeTimer", this);
    mCounterTimerGate =
        new CounterTimerGate(rInfo, "ChallengeTimerParts", "ParChallengeTimer", this);
    appear();
    updateCounters(-1);

    if (SingleModeDataFunction::isPhase0(GameDataHolderWriter(this))) {
        al::hidePane(this, "Menu");
        mIsInDemo = false;
    }

    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (koopaJr != nullptr) {
        mKoopaJrGuide = new GuideFrameOutSingleMode(rInfo, koopaJr, "BowserJr");
        mKoopaJrGuide->appear();
        mKoopaJrGuide->setDisable();
        mKoopaJrGuide->setUnk148(2.0f);
    }
}

/**
 * @brief Refresh the shard and shine counters.
 * @param islandId The island to show, or -1 for the current one.
 */
void SingleModeSceneLayout::updateCounters(s32 islandId) {
    mShardCounter->updateCount(islandId);
    mScenarioShineCounter->updateCount(islandId);
}

/**
 * @brief Set whether the HUD is hidden for a demo, without playing any action.
 * @param isInDemo True while a demo plays.
 */
void SingleModeSceneLayout::setInDemo(bool isInDemo) {
    mIsInDemo = isInDemo;
}

/** @brief Show the HUD with its appear action and the map guide hidden. */
void SingleModeSceneLayout::appear() {
    al::LayoutActor::appear();
    updateCounters(-1);
    updateMenuGuideState(MenuGuideState_Normal);
    al::startAction(this, "Appear", nullptr);
    al::startFreezeActionEnd(this, "MapEnd", "Map");
}

/**
 * @brief Update the button icons of the menu / map guides.
 * @param state Which Joy-Con layout the guides should show.
 */
void SingleModeSceneLayout::updateMenuGuideState(MenuGuideState state) {
    if (mMenuGuideState == state) {
        return;
    }

    mMenuGuideState = state;
    sead::WFormatFixedSafeString<32> menuText(al::getSystemMessageString(
        this, "SingleModeSceneLayout", "SingleModeSceneLayout_GuideMenu"));
    sead::WFormatFixedSafeString<32> mapText(al::getSystemMessageString(
        this, "SingleModeSceneLayout", "SingleModeSceneLayout_GuideMap"));

    switch (state) {
    case MenuGuideState_Normal:
        al::startAction(this, "Set2PAssistSameJoyConOff", "2PAssistMode");
        menuText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonPlus()));
        mapText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonMinus()));
        break;
    case MenuGuideState_SameJoyLeft:
        menuText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonMinus()));
        mapText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonMinus()));
        al::startAction(this, "Set2PAssistSameJoyConOn", "2PAssistMode");
        break;
    case MenuGuideState_SameJoyRight:
        al::startAction(this, "Set2PAssistSameJoyConOn", "2PAssistMode");
        menuText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonPlus()));
        mapText.prepend(sead::WSafeString(LayoutFontUtil::getMessageFontButtonPlus()));
        break;
    default:
        break;
    }

    al::setPaneString(this, "TxtMenu", menuText.cstr(), 0);
    al::setPaneString(this, "TxtMap", mapText.cstr(), 0);
}

/** @brief Pick the guide layout matching the controllers of 2P assist mode. */
void SingleModeSceneLayout::control() {
    if (SingleModeDataFunction::getIs2PAssistMode(this)) {
        s32 mainPort = al::getMainControllerPort();
        s32 subPort = rc::getPadPortByUserId(1);
        if (al::isPadTypeJoyLeft(mainPort) && al::isPadTypeJoyLeft(subPort)) {
            updateMenuGuideState(MenuGuideState_SameJoyLeft);
            return;
        }

        if (al::isPadTypeJoyRight(mainPort) && al::isPadTypeJoyRight(subPort)) {
            updateMenuGuideState(MenuGuideState_SameJoyRight);
            return;
        }
    }

    updateMenuGuideState(MenuGuideState_Normal);
}

/**
 * @brief Hide the HUD for a demo.
 * @param isHideAll True to hide everything immediately.
 * @param isEndAction True to play the end action (when not hiding everything).
 */
void SingleModeSceneLayout::startDemo(bool isHideAll, bool isEndAction) {
    if (mIsInDemo) {
        return;
    }

    if (isHideAll) {
        al::startFreezeActionEnd(this, "End", nullptr);
        if (mIsShowMap) {
            al::startFreezeActionEnd(this, "MapEnd", "Map");
        }

        getLayoutKeeper()->calcAnim(true);
        al::hidePaneRootNoRecursive(this);
    } else if (isEndAction) {
        if (mShardCounter->isShardDemo()) {
            al::startAction(this, "End2", nullptr);
        } else {
            al::startAction(this, "End", nullptr);
        }

        if (mIsShowMap) {
            al::startAction(this, "MapEnd", "Map");
        }
    }

    mCounterCoin->startDemo();
    mItemStockTray->startDemo();
    mChallengeTimer->startDemo();
    mCounterGoalItem->startDemo();
    if (mKoopaJrGuide != nullptr) {
        mKoopaJrGuide->startDemo();
    }

    mIsInDemo = true;
    mIsShowMap = false;
}

/**
 * @brief Show the HUD again after a demo.
 * @param isAppear True to play the appear action.
 * @param isUpdateItems True to let the counters and area name update after the demo.
 */
void SingleModeSceneLayout::endDemo(bool isAppear, bool isUpdateItems) {
    if (!mIsInDemo) {
        return;
    }

    if (mShardCounter->isShardDemo()) {
        mShardCounter->endShardDemo();
    }

    if (al::isHidePaneRoot(this)) {
        al::showPaneRootNoRecursive(this);
        al::startFreezeActionEnd(this, "End", nullptr);
        getLayoutKeeper()->calcAnim(true);
    }

    if (isAppear) {
        al::startAction(this, "Appear", nullptr);
    }

    mItemStockTray->endDemo();
    mCounterGoalItem->endDemo(isUpdateItems);
    mShardCounter->endDemo();
    mScenarioShineCounter->endDemo();
    mChallengeTimer->endDemo();
    mAreaName->endDemo(isUpdateItems && SingleModeDataFunction::getUnlockedPhase(this) != 5);
    mCounterCoin->endDemo();
    if (mKoopaJrGuide != nullptr) {
        mKoopaJrGuide->endDemo();
    }

    mIsInDemo = false;
    mIsEndDemoReady = true;
}

/** @brief Snap the HUD to its hidden state before a demo ends. */
void SingleModeSceneLayout::prepEndDemo() {
    al::startFreezeActionEnd(this, "End", nullptr);
    mIsEndDemoReady = true;
}

/**
 * @brief Play the pause actions.
 * @param isShowItemStock True to also open the item stock.
 */
void SingleModeSceneLayout::startPause(bool isShowItemStock) {
    al::startAction(this, "PauseStart", "Pause");
    if (isShowItemStock) {
        al::startAction(this, "PauseStart", "ItemStock");
    }

    if (mKoopaJrGuide != nullptr) {
        mKoopaJrGuide->startPause();
    }

    if (mBowserGuide != nullptr) {
        mBowserGuide->startPause();
    }
}

/** @brief Play the unpause actions. */
void SingleModeSceneLayout::endPause() {
    al::startAction(this, "PauseEnd", "Pause");
    al::startAction(this, "PauseEnd", "ItemStock");
    if (mKoopaJrGuide != nullptr) {
        mKoopaJrGuide->endPause();
    }

    if (mBowserGuide != nullptr) {
        mBowserGuide->endPause();
    }
}

/** @brief Play the course clear action. */
void SingleModeSceneLayout::courseClear() {
    al::startAction(this, "CourseClear", nullptr);
}

/**
 * @brief Give the area name the white wipe it waits for.
 * @param pWipe The wipe.
 */
void SingleModeSceneLayout::setAreaNameWipeFadeWhite(al::WipeSimple* pWipe) {
    mAreaName->setWipeFadeWhite(pWipe);
}

/**
 * @brief Show the name of the island the player entered, unless a timer runs.
 * @param islandId The island.
 */
void SingleModeSceneLayout::updateAreaName(s32 islandId) {
    if (SingleModeDataFunction::isPhase0(this)) {
        return;
    }

    if (mCounterTimerGate != nullptr && mCounterTimerGate->isCountingDown()) {
        return;
    }

    if (mChallengeTimer != nullptr && mChallengeTimer->isCountingDown()) {
        return;
    }

    if (mIsDisableAreaName) {
        return;
    }

    mScenarioInfo.mIslandId = islandId;
    mScenarioInfo.mScenarioIndex =
        SingleModeDataFunction::getCurActiveScenarioIndex(this, islandId);
    mAreaName->changeName(mScenarioInfo);
}

/**
 * @brief Finish the area name appear immediately.
 * @param isPhaseStart True at the start of a phase.
 */
void SingleModeSceneLayout::setAreaNamePhaseStart(bool isPhaseStart) {
    mAreaName->forceEndAppear(isPhaseStart);
}

/**
 * @brief Fade the area name out.
 * @param isForce True to fade out even while appearing.
 */
void SingleModeSceneLayout::fadeOutAreaName(bool isForce) {
    if (!SingleModeDataFunction::isPhase0(this)) {
        mAreaName->fadeOut(isForce);
    }
}

/** @brief Does nothing in Bowser's Fury. */
void SingleModeSceneLayout::disableItemStock() {}

/**
 * @brief Put an item into the item stock and play the player's item get reaction.
 * @param pItem The item.
 * @param pPlayer The player who got it.
 * @param itemType The item type.
 */
void SingleModeSceneLayout::stockItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                                      s32 itemType) {
    mItemStockTray->stockItem(pItem, pPlayer, itemType, true);
    al::startHitReaction(pPlayer, "ItemGet");
}

/**
 * @brief Put an item into the item stock without any effect.
 * @param pItem The item.
 * @param pPlayer The player who got it.
 * @param itemType The item type.
 */
void SingleModeSceneLayout::stockItemSilent(const al::LiveActor* pItem,
                                            const al::LiveActor* pPlayer, s32 itemType) {
    mItemStockTray->stockItem(pItem, pPlayer, itemType, false);
}

/**
 * @brief Open the item stock.
 * @param port The controller port that opened it.
 */
void SingleModeSceneLayout::appearItemStock(s32 port) {
    mItemStockTray->startAppear(port);
}

/**
 * @brief Check whether the item stock started closing.
 * @return True if closing.
 */
bool SingleModeSceneLayout::isItemStockStartClose() const {
    return mItemStockTray->isStartClose();
}

/**
 * @brief Check whether the item stock is closed.
 * @return True if closed.
 */
bool SingleModeSceneLayout::isItemStockEnd() const {
    return mItemStockTray->isIdle();
}

/**
 * @brief Check whether an item can be taken out of the stock.
 * @return True if an item can spawn.
 */
bool SingleModeSceneLayout::canSpawnItem() const {
    return mItemStockTray->canSpawnItem();
}

/**
 * @brief Get the controller port that opened the item stock.
 * @return The port.
 */
s32 SingleModeSceneLayout::getItemStockControllerID() const {
    return mItemStockTray->getPort();
}

/**
 * @brief Give coins for an item that did not fit into the full stock.
 * @param count The number of coins.
 * @param pSensor The sensor the coins are given to.
 */
void SingleModeSceneLayout::spawnItemStockCoins(u8 count, al::HitSensor* pSensor) {
    mItemStockTray->spawnCoins(count, pSensor);
}

/**
 * @brief Find which panes contain any of the given layout points.
 * @param pIsHit In/out: per pane, whether it is hit. Panes already hit are skipped.
 * @param pPaneNames The pane names, checked as "Hit<name>".
 * @param paneNum The number of panes.
 * @param rPoints The layout positions to check.
 */
void SingleModeSceneLayout::calcHitPane(bool* pIsHit, const char** pPaneNames, s32 paneNum,
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

/**
 * @brief Add a shard to the shard counter.
 * @param islandId The island of the shard.
 * @param shardIndex The shard index.
 * @param isComplete True if this completes the shine.
 * @param isDemo True if collected in a demo.
 */
void SingleModeSceneLayout::addShard(s32 islandId, s32 shardIndex, bool isComplete,
                                     bool isDemo) {
    mShardCounter->addShard(islandId, shardIndex, isComplete, isDemo);
}

/** @brief Show the shard counter, except in the phases without shards. */
void SingleModeSceneLayout::showShardCounter() {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase == 7 || phase == 10) {
        return;
    }

    mShardCounter->show();
}

/** @brief Hide the shard counter. */
void SingleModeSceneLayout::hideShardCounter() {
    mShardCounter->hide();
}

/** @brief Mark the shard counter as shown by a shard demo. */
void SingleModeSceneLayout::startShardDemo() {
    mShardCounter->startShardDemo();
}

/** @brief Show the shine counter. */
void SingleModeSceneLayout::showShineCounter() {
    mScenarioShineCounter->show();
}

/** @brief Hide the shine counter. */
void SingleModeSceneLayout::hideShineCounter() {
    mScenarioShineCounter->hide();
}

/** @brief Finish the shine counter appear immediately. */
void SingleModeSceneLayout::forceShineCounterEndAppear() {
    mScenarioShineCounter->forceEndAppear(mScenarioInfo.mIslandId);
}

/** @brief Hide the shine counter immediately. */
void SingleModeSceneLayout::forceHideShineCounter() {
    mScenarioShineCounter->forceHide();
}

/** @brief Add a shine to the shine counter. */
void SingleModeSceneLayout::addShine() {
    mScenarioShineCounter->addShine();
    al::startHitReaction(this, "ShineGet", nullptr);
}

/**
 * @brief Show the map guide when idle and fade out the counters the players stand behind.
 */
void SingleModeSceneLayout::exeWait() {
    if (al::isFirstStep(this)) {
        if (!al::isAnyActionPlaying(this, nullptr) ||
            !al::isActionPlaying(this, "Appear", "Main") || al::isActionEnd(this, "Main")) {
            al::startAction(this, "Wait", nullptr);
        }
    }

    if (mIsInDemo) {
        return;
    }

    bool isIdle = true;
    if (al::isPadHoldAnyABXY(al::getMainControllerPort()) ||
        al::isPadHoldLeftStick(al::getMainControllerPort())) {
        isIdle = false;
    }

    if (mIsShowMap) {
        if (isIdle) {
            mMapHideTimer = 90;
        } else if (mMapHideTimer <= 0) {
            al::startAction(this, "MapEnd", "Map");
            mIsShowMap = false;
            mMapHideTimer = 90;
        } else {
            mMapHideTimer--;
        }
    } else if (isIdle) {
        al::startAction(this, "MapAppear", "Map");
        mIsShowMap = true;
        mMapHideTimer = 90;
    }

    HitPointBuffer points;
    if (al::isAnyActionPlaying(this, "Main") && al::isActionPlaying(this, "Appear", "Main") &&
        !al::isActionEnd(this, "Main")) {
        mIsEndDemoReady = false;
        return;
    }

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

    const char* paneNames[] = {"CounterCoin",  "CounterGoalItem", "ItemStock",
                               "ShardCounter", "ChallengeTimer",  "CounterScenarioShine"};
    bool isHit[6] = {};
    calcHitPane(isHit, paneNames, 6, points);

    for (s32 i = 0; i < 6; i++) {
        if (isHit[i]) {
            if (!al::isActionPlaying(this, "FadeOut", paneNames[i])) {
                al::startAction(this, "FadeOut", paneNames[i]);
            }
        } else if (al::isActionPlaying(this, "FadeOut", paneNames[i])) {
            al::startAction(this, "FadeIn", paneNames[i]);
        }
    }
}

/**
 * @brief Set the off-screen guide of Fury Bowser.
 * @param pGuide The guide.
 */
void SingleModeSceneLayout::setBowserGuideFrameOut(GuideFrameOutSingleMode* pGuide) {
    mBowserGuide = pGuide;
}

/** @brief Show the challenge timer. */
void SingleModeSceneLayout::showTimer() {
    mChallengeTimer->show();
}

/**
 * @brief Set the challenge timer.
 * @param frames The remaining time in frames.
 */
void SingleModeSceneLayout::setTimer(s32 frames) {
    mChallengeTimer->setTimer(frames);
}

/** @brief Start the challenge timer countdown. */
void SingleModeSceneLayout::startTimer() {
    mChallengeTimer->startTimer();
}

/**
 * @brief Hide the challenge timer.
 * @param isForce True to hide it immediately.
 */
void SingleModeSceneLayout::hideTimer(bool isForce) {
    mChallengeTimer->hide(isForce);
}

/** @brief Turn the challenge timer red. */
void SingleModeSceneLayout::setTimerRed() {
    mChallengeTimer->setTimerRed();
}

/**
 * @brief Pause or resume the challenge timer, if it is counting down.
 * @param isPause True to pause, false to resume.
 */
void SingleModeSceneLayout::pauseTimer(bool isPause) {
    if (mChallengeTimer->isCountingDown()) {
        if (isPause) {
            mChallengeTimer->pauseTimer();
        } else {
            mChallengeTimer->unpauseTimer();
        }
    }
}

/** @brief Show the time of the challenge timer. */
void SingleModeSceneLayout::displayTimer() {
    mChallengeTimer->displayTime();
}

/**
 * @brief Check whether a timer is shown.
 * @return True if the Plessie timer or the challenge timer is visible.
 */
bool SingleModeSceneLayout::isTimerActive() {
    return mCounterTimerGate->isVisible() || mChallengeTimer->isVisible();
}

/** @brief Show the Plessie (timer gate) timer. */
void SingleModeSceneLayout::showPlessieTimer() {
    mCounterTimerGate->show();
}

/**
 * @brief Set the Plessie (timer gate) timer.
 * @param frames The remaining time in frames.
 * @param maxFrames The total time in frames.
 */
void SingleModeSceneLayout::setPlessieTimer(s32 frames, s32 maxFrames) {
    mCounterTimerGate->setTimer(frames, maxFrames);
}

/** @brief Start the Plessie (timer gate) timer countdown. */
void SingleModeSceneLayout::startPlessieTimer() {
    mCounterTimerGate->startTimer();
}

/**
 * @brief Hide the Plessie (timer gate) timer.
 * @param isForce True to hide it immediately.
 */
void SingleModeSceneLayout::hidePlessieTimer(bool isForce) {
    mCounterTimerGate->hide(isForce);
}

/**
 * @brief Pause or resume the Plessie (timer gate) timer, if it is counting down.
 * @param isPause True to pause, false to resume.
 */
void SingleModeSceneLayout::pausePlessieTimer(bool isPause) {
    if (mCounterTimerGate->isCountingDown()) {
        if (isPause) {
            mCounterTimerGate->pauseTimer();
        } else {
            mCounterTimerGate->unpauseTimer();
        }
    }
}

/**
 * @brief Set the timer gate the Plessie timer belongs to.
 * @param pTimerGate The timer gate.
 */
void SingleModeSceneLayout::setPlessieTimerGate(TimerGate* pTimerGate) {
    mCounterTimerGate->setTimerGate(pTimerGate);
}

/**
 * @brief Enable or disable the off-screen guide of Bowser Jr., if there is one.
 * @param isEnable True to enable it.
 */
void SingleModeSceneLayout::setEnableKoopaJrGuideFrameOut(bool isEnable) {
    if (mKoopaJrGuide == nullptr) {
        return;
    }

    if (isEnable) {
        mKoopaJrGuide->setEnable();
        mKoopaJrGuide->setEnableWarpGuide(true);
    } else {
        mKoopaJrGuide->setDisable();
        mKoopaJrGuide->setEnableWarpGuide(false);
    }
}

/**
 * @brief Forbid (or allow again) showing the area name; forbidding fades it out.
 * @param isDisable True to forbid the area name.
 */
void SingleModeSceneLayout::setDisableAreaName(bool isDisable) {
    if (isDisable == mIsDisableAreaName) {
        return;
    }

    if (isDisable && !SingleModeDataFunction::isPhase0(this)) {
        mAreaName->fadeOut(true);
    }

    mIsDisableAreaName = isDisable;
}

/** @brief Reset the area name and the shine counter after warping to another island. */
void SingleModeSceneLayout::handleIslandWarp() {
    mAreaName->handleIslandWarp();
    mScenarioShineCounter->kill();
    mScenarioShineCounter->handleIslandWarp();
    al::startFreezeActionEnd(mScenarioShineCounter, "Hide", nullptr);
}
