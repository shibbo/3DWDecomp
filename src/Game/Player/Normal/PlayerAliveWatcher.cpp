#include "Player/Normal/PlayerAliveWatcher.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Player/Normal/PlayerAliveWatcherGroup.hpp"
#include "Player/Normal/PlayerAmiiboDirectorWatcher.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

/**
 * @brief Creates the watcher with one group per character type.
 * @param rInfo Actor init info of the scene.
 * @param pPlayerHolder The scene's players.
 * @param isNoLifeDecrease Whether losing every player costs no life.
 * @param isSingleMode Whether the scene is played in single mode.
 * @param isNoAmiibo Whether amiibo scanning is disabled.
 */
PlayerAliveWatcher::PlayerAliveWatcher(const al::ActorInitInfo& rInfo,
                                       al::PlayerHolder* pPlayerHolder, bool isNoLifeDecrease,
                                       bool isSingleMode, bool isNoAmiibo)
    : mIsNoLifeDecrease(isNoLifeDecrease), mIsSingleMode(isSingleMode) {
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    mAmiiboDirectorWatcher = new PlayerAmiiboDirectorWatcher(isNoAmiibo);
    mGroupNum = cGroupNum;
    mGroups = new PlayerAliveWatcherGroup*[cGroupNum];

    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i] = new PlayerAliveWatcherGroup(this, rInfo, pPlayerHolder, i);
    }

    mReviveBubbleRequests = new DisableBubbleRequestHolder;
    mFrameOutRequests = new DisableBubbleRequestHolder;
}

/**
 * @brief Starts watching every group.
 */
void PlayerAliveWatcher::appear() {
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->appear();
    }
}

/**
 * @brief Get the scene's player alive watcher.
 * @param pHolder Object with access to the scene object holder.
 * @return The watcher.
 */
PlayerAliveWatcher* PlayerAliveWatcher::getPlayerAliveWatcher(
    const al::IUseSceneObjHolder* pHolder) {
    return al::getSceneObj<PlayerAliveWatcher>(pHolder, SceneObjID_PlayerAliveWatcher);
}

/**
 * @brief Get the scene's player alive watcher if it exists.
 * @param pHolder Object with access to the scene object holder.
 * @return The watcher, or nullptr.
 */
PlayerAliveWatcher* PlayerAliveWatcher::tryGetPlayerAliveWatcher(
    const al::IUseSceneObjHolder* pHolder) {
    return al::tryGetSceneObj<PlayerAliveWatcher>(pHolder, SceneObjID_PlayerAliveWatcher);
}

/**
 * @brief Set the delay before off-screen players are put into a bubble.
 * @param frames The delay in frames; negative values restore the default.
 */
void PlayerAliveWatcher::addBubbleDelayTime(s32 frames) {
    mBubbleDelayTime = frames < 0 ? cDefaultBubbleDelayTime : frames;
}

/**
 * @brief Notifies every group that a demo starts.
 */
void PlayerAliveWatcher::startDemo() {
    mBubbleDelayTime = cDefaultBubbleDelayTime;

    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->startDemo();
    }
}

/**
 * @brief Notifies every group that a demo ended.
 */
void PlayerAliveWatcher::endDemo() {
    mBubbleDelayTime = cDefaultBubbleDelayTime;

    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->endDemo();
    }
}

/**
 * @brief Notifies every group that the game is paused.
 */
void PlayerAliveWatcher::startPause() {
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->startPause();
    }
}

/**
 * @brief Notifies every group that the pause ended.
 */
void PlayerAliveWatcher::endPause() {
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->endPause();
    }
}

/**
 * @brief Notifies every group of the game over.
 */
void PlayerAliveWatcher::onGameOver() {
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->onGameOver();
    }
}

/**
 * @brief Checks whether every group is out of the game.
 * @return True if all groups are game over.
 */
bool PlayerAliveWatcher::isGameOver() const {
    for (s32 i = 0; i < mGroupNum; i++) {
        if (!mGroups[i]->isGameOver()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether a character waits for a revive bubble.
 * @param characterType The character type.
 * @return True if a player of that character waits for a bubble.
 */
bool PlayerAliveWatcher::isWaitBubbleForRevive(s32 characterType) const {
    return mGroups[characterType]->isWaitBubbleForRevive();
}

/**
 * @brief Checks whether every player of a character is deactivated.
 * @param characterType The character type.
 * @return True if the character is deactivated.
 */
bool PlayerAliveWatcher::isDeactivePlayer(s32 characterType) const {
    return mGroups[characterType]->isAllDeactive();
}

/**
 * @brief Deactivates every player of a character.
 * @param characterType The character type.
 */
void PlayerAliveWatcher::deactivatePlayer(s32 characterType) {
    mGroups[characterType]->deactivateAll();
}

/**
 * @brief Checks whether a character may exit the stage.
 * @param characterType The character type.
 * @return True if exiting the stage is allowed.
 */
bool PlayerAliveWatcher::isEnableExitStage(s32 characterType) const {
    return mGroups[characterType]->isEnableExitStage();
}

/**
 * @brief Checks whether a character may warp on the island.
 * @param characterType The character type.
 * @return True if the island warp is allowed.
 */
bool PlayerAliveWatcher::isEnableIslandWarp(s32 characterType) const {
    return mGroups[characterType]->isEnableIslandWarp();
}

/**
 * @brief Checks whether the gyro camera may be used, i.e. exactly one player is in the game.
 * @return True if only a single player is left.
 */
bool PlayerAliveWatcher::isEnableGyroCamera() const {
    bool isFound = false;

    for (s32 i = 0; i < mGroupNum; i++) {
        s32 num = mGroups[i]->calcNoKillPlayerNum();
        if (num > 1) {
            return false;
        }

        if (num == 1) {
            if (isFound) {
                return false;
            }

            isFound = true;
        }
    }

    return isFound;
}

/**
 * @brief Counts the active players of every group.
 * @return The number of active players.
 */
s32 PlayerAliveWatcher::calcAllActivePlayerNum() const {
    s32 num = 0;

    for (s32 i = 0; i < mGroupNum; i++) {
        num += mGroups[i]->calcActivePlayerNum();
    }

    return num;
}

/**
 * @brief Checks whether another group allows a player to enter a bubble with the button.
 * @param pGroup The group asking.
 * @return True if any other group allows it.
 */
bool PlayerAliveWatcher::isEnableBubbleWithInput(PlayerAliveWatcherGroup* pGroup) const {
    for (s32 i = 0; i < mGroupNum; i++) {
        PlayerAliveWatcherGroup* group = mGroups[i];
        if (group != pGroup && group->isEnableOtherGroupBubbleWithInput()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Activates a player of a character.
 * @param characterType The character type.
 * @return The activated player, or nullptr if no group watches that character.
 */
PlayerActor* PlayerAliveWatcher::activatePlayer(s32 characterType) {
    for (s32 i = 0; i < mGroupNum; i++) {
        if (mGroups[i]->getCharaType() == characterType) {
            return mGroups[i]->activatePlayer();
        }
    }

    return nullptr;
}

/**
 * @brief Counts the extra players created by double cherries.
 * @return The number of active players beyond the first of each character.
 */
s32 PlayerAliveWatcher::calcDoubleMarioNum() const {
    s32 num = 0;

    for (s32 i = 0; i < mGroupNum; i++) {
        s32 activeNum = mGroups[i]->calcActivePlayerNum();
        num += activeNum > 1 ? activeNum - 1 : 0;
    }

    return num;
}

/**
 * @brief Updates every group, the shadow quality and the disable requests.
 */
void PlayerAliveWatcher::update() {
    mUpdatedGroupNum = 0;

    for (s32 i = 0; i < mGroupNum; i++) {
        if (mGroups[i]->update()) {
            mUpdatedGroupNum++;
        }
    }

    s32 updatedGroupNum = mUpdatedGroupNum;
    if (calcDoubleMarioNum() + updatedGroupNum > 4) {
        for (s32 i = 0; i < mGroupNum; i++) {
            mGroups[i]->changeShadowLight();
        }
    } else {
        for (s32 i = 0; i < mGroupNum; i++) {
            mGroups[i]->changeShadowNormal();
        }
    }

    mIsEnableBubbleRevive = mReviveBubbleRequests->update();

    if (mBubbleDelayTime > 0) {
        mBubbleDelayTime--;
    }

    if (mAmiiboDirectorWatcher != nullptr) {
        mAmiiboDirectorWatcher->update();
    }
}

/**
 * @brief Disables the revive bubble for a while.
 * @param pActor The requesting actor.
 */
void PlayerAliveWatcher::setDisableReviveBubble(al::LiveActor* pActor) {
    mReviveBubbleRequests->setDisableRequest(pActor);
    mIsEnableBubbleRevive = false;
}

/**
 * @brief Drops an actor's revive bubble request.
 * @param pActor The requesting actor.
 */
void PlayerAliveWatcher::resetDisableReviveBubble(al::LiveActor* pActor) {
    mIsEnableBubbleRevive = mReviveBubbleRequests->resetDisableRequest(pActor);
}

/**
 * @brief Disables the off-screen bubble for a while.
 * @param pActor The requesting actor.
 */
void PlayerAliveWatcher::setDisableBubbleFrameOut(al::LiveActor* pActor) {
    mFrameOutRequests->setDisableRequest(pActor);
    mIsEnableBubbleScreenOut = false;
}

/**
 * @brief Drops an actor's off-screen bubble request.
 * @param pActor The requesting actor.
 */
void PlayerAliveWatcher::resetDisableBubbleFrameOut(al::LiveActor* pActor) {
    mIsEnableBubbleScreenOut = mFrameOutRequests->resetDisableRequest(pActor);
}

/**
 * @brief Finds the character controlled from a controller port.
 * @param port The controller port.
 * @return The character type of the active user on that port, or -1.
 */
s32 PlayerAliveWatcher::isActivePlayerPort(s32 port) const {
    s32 userIds[4] = {};
    s32 userNum = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(mSceneObjHolder));
    s32 characterType = -1;

    for (s32 i = 0; i < userNum; i++) {
        if (rc::getControlUserPortNumber(GameDataHolderAccessor(mSceneObjHolder), userIds[i]) ==
            port) {
            s32 userId = userIds[i];
            if (mIsSingleMode) {
                characterType = userId + 5;
            } else {
                characterType = rc::getControlUserCharacterType(
                    GameDataHolderAccessor(mSceneObjHolder), userId);
            }

            break;
        }
    }

    return characterType;
}
