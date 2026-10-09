#include "Layout/PlayerEntryMini.hpp"

#include <prim/seadSafeString.h>

#include "CourseSelect/CourseSelectDirector.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/PlayerEntryItem.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"

namespace nn::hid {
void StartLrAssignmentMode();
void StopLrAssignmentMode();
}  // namespace nn::hid

/// Declares a nerve named Action that runs PlayerEntryMini::exe##Exe (several nerves share an exe).
#define PLAYER_ENTRY_MINI_NERVE(Action, Exe)                                                      \
    class PlayerEntryMiniNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<PlayerEntryMini>()->exe##Exe();                                     \
        }                                                                                          \
    };

namespace {
PLAYER_ENTRY_MINI_NERVE(PlayerEntryRandom, PlayerEntry)
NERVE_DECL(PlayerEntryMini, Wait)
NERVE_DECL(PlayerEntryMini, PlayerEntry)
NERVE_DECL(PlayerEntryMini, Shuffle)
NERVE_DECL(PlayerEntryMini, PlayerEntryEnd)
NERVE_DECL(PlayerEntryMini, PlayerEntryAllDecide)

NERVES_MAKE_NOSTRUCT(PlayerEntryMini, PlayerEntryRandom, Wait, PlayerEntry, Shuffle,
                     PlayerEntryEnd, PlayerEntryAllDecide)

/// Controller port checked for the handheld connection.
constexpr s32 cHandheldPort = 4;
}  // namespace

/**
 * @brief Creates the entry window and one entry item per controller user.
 * @param rInfo Layout init info.
 * @param pHolder Game data holder.
 * @param pPlayerHolder Player holder of the scene.
 * @param sceneType Scene the window is shown in (SceneType).
 */
PlayerEntryMini::PlayerEntryMini(const al::LayoutInitInfo& rInfo, GameDataHolder* pHolder,
                                 al::PlayerHolder* pPlayerHolder, s32 sceneType)
    : al::LayoutActor("プレーヤーのキャラクター選択"), mSceneType(static_cast<SceneType>(sceneType)),
      mAllDecideFrame(0), mGameDataHolder(pHolder), mPlayerHolder(pPlayerHolder),
      mItems(nullptr), mIsItemDecided(false) {
    al::initLayoutActor(this, rInfo, "PlayerEntryMini",
                        sceneType == SceneType_PreStageWipe ? "Sequence" : nullptr);

    if (isKinopioPreStageWipe() || isPreStageWipe()) {
        initNerve(&NrvPlayerEntryMiniPlayerEntryRandom, 0);
    } else {
        initNerve(&NrvPlayerEntryMiniPlayerEntry, 0);
    }

    mItems = new PlayerEntryItem*[cItemNum];
    mItems[0] = new PlayerEntryItem(rInfo, "ParPlayerDRC", "ParPlayerDRC", this, 0, pHolder);
    mItems[1] = new PlayerEntryItem(rInfo, "ParPlayer1", "ParPlayer1", this, 1, pHolder);
    mItems[2] = new PlayerEntryItem(rInfo, "ParPlayer2", "ParPlayer2", this, 2, pHolder);
    mItems[3] = new PlayerEntryItem(rInfo, "ParPlayer3", "ParPlayer3", this, 3, pHolder);
    al::hidePane(this, "Random");
    appear();
}

/**
 * @brief Checks whether the window is shown in the wipe before a stage.
 * @return Whether the scene type is SceneType_PreStageWipe.
 */
bool PlayerEntryMini::isPreStageWipe() const {
    return mSceneType == SceneType_PreStageWipe;
}

/**
 * @brief Shows the window and starts the nerve matching the scene type.
 */
void PlayerEntryMini::appear() {
    al::LayoutActor::appear();
    mIsItemDecided = false;

    if (isPreStageWipe()) {
        al::setNerve(this, &NrvPlayerEntryMiniPlayerEntryRandom);
        return;
    }

    if (isKinopioPreStageWipe()) {
        for (s32 i = 0; i < cItemNum; i++) {
            al::startAction(mItems[i], "SetPlayerEntryCaptainKinopio");
        }

        al::setNerve(this, &NrvPlayerEntryMiniWait);
        return;
    }

    al::setNerve(this, &NrvPlayerEntryMiniPlayerEntry);
}

/**
 * @brief Updates the window.
 */
void PlayerEntryMini::movement() {
    al::LayoutActor::movement();
}

/**
 * @brief Gets the game data holder.
 * @return The game data holder.
 */
GameDataHolder* PlayerEntryMini::getGameDataHolder() {
    return mGameDataHolder;
}

/**
 * @brief Starts the character select and shows every entry item.
 */
void PlayerEntryMini::startCharacterSelect() {
    PlayerEntryFunction::startCharacterSelect(GameDataHolderWriter(mGameDataHolder));

    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->appear();
    }
}

/**
 * @brief Ends the character select on every entry item.
 */
void PlayerEntryMini::endCharacterSelect() {
    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->end();
    }
}

/**
 * @brief Shows every entry item for the player entry.
 * @param isDeactivate Whether the items are made inactive first.
 */
void PlayerEntryMini::startPlayerEntry(bool isDeactivate) {
    for (s32 i = 0; i < cItemNum; i++) {
        if (isDeactivate) {
            mItems[i]->setPlayerActive(false);
        }

        mItems[i]->appear();
    }
}

/**
 * @brief Shuffles the players' characters, unless a shuffle is already running.
 */
void PlayerEntryMini::startShuffle() {
    if (al::isNerve(this, &NrvPlayerEntryMiniShuffle)) {
        return;
    }

    PlayerEntryFunction::shufflePlayerModel(GameDataHolderWriter(mGameDataHolder));

    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->startShuffle();
    }

    al::setNerve(this, &NrvPlayerEntryMiniShuffle);
    al::startSe(this, "PgShuffleStart");
}

/**
 * @brief Starts the demo on every entry item.
 */
void PlayerEntryMini::startDemo() {
    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->startDemo();
    }
}

/**
 * @brief Hides every entry item.
 */
void PlayerEntryMini::hide() {
    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->hide();
    }
}

/**
 * @brief Ends the demo on every entry item.
 */
void PlayerEntryMini::endDemo() {
    for (s32 i = 0; i < cItemNum; i++) {
        mItems[i]->endDemo();
    }
}

/**
 * @brief Makes a user's entry item active.
 * @param userId Controller user.
 */
void PlayerEntryMini::setItemActive(s32 userId) {
    mItems[userId]->setPlayerActive(true);
}

/**
 * @brief Checks whether the window is shown in a stage.
 * @return Whether the scene type is SceneType_Stage.
 */
bool PlayerEntryMini::isStageScene() const {
    return mSceneType == SceneType_Stage;
}

/**
 * @brief Checks whether the window is shown in the course select.
 * @return Whether the scene type is SceneType_CourseSelect.
 */
bool PlayerEntryMini::isCourseSelectScene() const {
    return mSceneType == SceneType_CourseSelect;
}

/**
 * @brief Checks whether the window is shown in the wipe before a Captain Toad stage.
 * @return Whether the scene type is SceneType_KinopioPreStageWipe.
 */
bool PlayerEntryMini::isKinopioPreStageWipe() const {
    return mSceneType == SceneType_KinopioPreStageWipe;
}

/**
 * @brief Checks whether the window is shown in a Captain Toad stage.
 * @return Whether the scene type is SceneType_KinopioStage.
 */
bool PlayerEntryMini::isKinopioStage() const {
    return mSceneType == SceneType_KinopioStage;
}

/**
 * @brief Checks whether the window belongs to a Captain Toad stage or its wipe.
 * @return Whether the scene is either Captain Toad scene type.
 */
bool PlayerEntryMini::isKinopioAny() const {
    return isKinopioStage() || isKinopioPreStageWipe();
}

/**
 * @brief Checks whether every active player has decided their character.
 * @return Whether at least one player is active and all active players decided.
 */
bool PlayerEntryMini::isAllPlayerDecided() const {
    s32 decidedNum = 0;

    for (s32 i = 0; i < cItemNum; i++) {
        if (mItems[i]->isNotActive()) {
            continue;
        }

        if (!mItems[i]->isDecided()) {
            return false;
        }

        decidedNum++;
    }

    return decidedNum != 0;
}

/**
 * @brief Checks whether the player entry has ended.
 * @return Whether the window is in the PlayerEntryEnd nerve.
 */
bool PlayerEntryMini::isPlayerEntryEnd() const {
    return al::isNerve(this, &NrvPlayerEntryMiniPlayerEntryEnd);
}

/**
 * @brief Counts the active entry items.
 * @return Number of active items.
 */
s32 PlayerEntryMini::calcActiveItemNum() const {
    s32 num = 0;

    for (s32 i = 0; i < cItemNum; i++) {
        num += !mItems[i]->isNotActive();
    }

    return num;
}

/**
 * @brief Checks whether a user's battery parts may be shown.
 * @param userId Controller user.
 * @return Whether the user's entry item is not showing its layout.
 */
bool PlayerEntryMini::isEnableShowBatteryParts(s32 userId) const {
    return !mItems[userId]->isShowLayout();
}

/**
 * @brief Enters a player and activates them in the scene.
 * @param userId Controller user.
 * @param characterType Character the user picked.
 * @return Whether the player was entered.
 */
bool PlayerEntryMini::playerEntry(s32 userId, s32 characterType) {
    if (isCourseSelectScene() &&
        al::getSceneObj<CourseSelectDirector>(this, SceneObjID_CourseSelectDirector)
            ->isPlayPuppeterDemoAll()) {
        return false;
    }

    if (!PlayerEntryFunction::entryPlayer(GameDataHolderWriter(mGameDataHolder), userId,
                                          characterType)) {
        return false;
    }

    switch (mSceneType) {
    case SceneType_Stage: {
        s32 userCharacterType =
            rc::getControlUserCharacterType(GameDataHolderAccessor(this), userId);
        al::getSceneObj<PlayerAliveWatcher>(this, SceneObjID_PlayerAliveWatcher)
            ->activatePlayer(userCharacterType);
        break;
    }
    case SceneType_KinopioStage:
        al::getSceneObj<PlayerAliveWatcher>(this, SceneObjID_PlayerAliveWatcher)
            ->activatePlayer(userId + 5);
        break;
    case SceneType_CourseSelect:
        al::getSceneObj<CourseSelectDirector>(this, SceneObjID_CourseSelectDirector)
            ->activateUser(userId, false);
        break;
    default:
        break;
    }

    return true;
}

/**
 * @brief Retires a player.
 * @param userId Controller user.
 */
void PlayerEntryMini::playerCancel(s32 userId) {
    PlayerEntryFunction::retirePlayer(GameDataHolderWriter(mGameDataHolder), userId);
}

/**
 * @brief Marks two users' entry items for a controller port swap.
 * @param srcUserId User giving up the port.
 * @param dstUserId User receiving the port.
 */
void PlayerEntryMini::changeUserPort(s32 srcUserId, s32 dstUserId) {
    if (srcUserId == dstUserId) {
        return;
    }

    mItems[srcUserId]->setChangeSrcUser();
    mItems[dstUserId]->setChangeDstUser();
}

/**
 * @brief Called when an entry item decides its character.
 * @param pItem The entry item.
 */
void PlayerEntryMini::onDecideItem(PlayerEntryItem* pItem) {
    mIsItemDecided = true;
}

/**
 * @brief Waits while checking the handheld connection.
 */
void PlayerEntryMini::exeWait() {
    checkPadConnection();
}

/**
 * @brief Disconnects the handheld controller when it presses any button but Home.
 */
void PlayerEntryMini::checkPadConnection() {
    if (al::isPadConnected(cHandheldPort) && al::isPadTriggerAny(cHandheldPort) &&
        !al::isPadTriggerHome(cHandheldPort)) {
        al::setPadDisconnect(cHandheldPort);
    }
}

/**
 * @brief Waits for every player to decide; Plus or Minus shuffles in the random entry.
 */
void PlayerEntryMini::exePlayerEntry() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPlayerEntryMiniPlayerEntryRandom)) {
            al::showPane(this, "Random");
            al::startSe(this, "PgAppear");
        } else {
            al::hidePane(this, "Random");
        }

        al::startAction(this, "Appear");
        mAllDecideFrame = 0;
    }

    checkPadConnection();
    al::holdSe(this, "PgWaitLv");

    if (al::isNerve(this, &NrvPlayerEntryMiniPlayerEntryRandom)) {
        for (s32 i = 0; i < cItemNum; i++) {
            if (mItems[i]->isNotActive()) {
                continue;
            }

            if (al::isPadTriggerPlus(mItems[i]->getPortNum()) ||
                al::isPadTriggerMinus(mItems[i]->getPortNum())) {
                startShuffle();
                return;
            }
        }
    }

    if (!isAllPlayerDecided()) {
        mAllDecideFrame = 0;
        return;
    }

    s32 waitFrame = mIsItemDecided ? 60 : 150;
    if (waitFrame <= mAllDecideFrame++) {
        al::setNerve(this, &NrvPlayerEntryMiniPlayerEntryAllDecide);
    }
}

/**
 * @brief Plays the all-decided effect, then ends the entry.
 */
void PlayerEntryMini::exePlayerEntryAllDecide() {
    if (al::isFirstStep(this)) {
        GameDataHolder* holder = mGameDataHolder;
        s32 courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(holder));

        if (!GameDataFunction::isStageEvent(GameDataHolderAccessor(holder), courseId)) {
            al::startSe(this, "PgDecideAll");
        }

        for (s32 i = 0; i < cItemNum; i++) {
            mItems[i]->startEntryEnd();
        }

        al::startAction(this, "End");
    }

    if (al::isGreaterStep(this, 40)) {
        al::setNerve(this, &NrvPlayerEntryMiniPlayerEntryEnd);
    }
}

/**
 * @brief Does nothing once the entry has ended.
 */
void PlayerEntryMini::exePlayerEntryEnd() {}

/**
 * @brief Waits for the shuffle to finish with every player decided.
 */
void PlayerEntryMini::exeShuffle() {
    if (isAllPlayerDecided()) {
        al::setNerve(this, &NrvPlayerEntryMiniPlayerEntryAllDecide);
    }
}

/**
 * @brief Starts the system's single Joy-Con assignment mode.
 */
void PlayerEntryMini::requestStartLrAssignMode() {
    nn::hid::StartLrAssignmentMode();
}

/**
 * @brief Stops the single Joy-Con assignment mode once no visible item is still choosing.
 */
void PlayerEntryMini::requestStopLrAssignMode() {
    for (s32 i = 0; i < cItemNum; i++) {
        if (mItems[i]->isLayoutVisible() && !mItems[i]->isDecided()) {
            return;
        }
    }

    nn::hid::StopLrAssignmentMode();
}
