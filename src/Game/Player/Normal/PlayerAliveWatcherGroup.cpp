#include "Player/Normal/PlayerAliveWatcherGroup.hpp"

#include "Layout/GuideFrameOut.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/TractorBubble.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerAmiiboDirector.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(PlayerAliveWatcherCharacter, Kill);
NERVE_DECL(PlayerAliveWatcherCharacter, Alive);
NERVE_DECL(PlayerAliveWatcherCharacter, ReviveWait);
NERVE_DECL(PlayerAliveWatcherCharacter, BubbleWait);
NERVE_DECL(PlayerAliveWatcherCharacter, BubbleWaitForScreenOut);
NERVE_DECL(PlayerAliveWatcherCharacter, BubbleWaitForInput);
NERVE_DECL(PlayerAliveWatcherCharacter, Dead);
NERVE_DECL(PlayerAliveWatcherCharacter, Abyss);
NERVES_MAKE_NOSTRUCT(PlayerAliveWatcherCharacter, Kill, Alive, ReviveWait, BubbleWait,
                     BubbleWaitForScreenOut, BubbleWaitForInput, Dead, Abyss)
}  // namespace

// The following PlayerAliveWatcherCharacter members are emitted in PlayerAliveWatcherCharacter.o
// in the target, but they use this file's nerves and are inlined into the code below, so they
// are only defined (inline) here.

/**
 * @brief Creates the watcher of one player, starting in the Kill state.
 * @param rInfo Actor init info, used for the off-screen guide layout.
 * @param pActor The watched player.
 * @param pGroup The group owning this watcher.
 */
inline PlayerAliveWatcherCharacter::PlayerAliveWatcherCharacter(const al::ActorInitInfo& rInfo,
                                                                PlayerActor* pActor,
                                                                PlayerAliveWatcherGroup* pGroup)
    : mGroup(pGroup), mActor(pActor) {
    mNerveKeeper = new al::NerveKeeper(this, &NrvPlayerAliveWatcherCharacterKill, 0);
    mFrameOut = new GuideFrameOut(al::getLayoutInitInfo(rInfo), mActor);
}

/**
 * @brief Checks whether the player is dead, fell into the abyss or was deactivated.
 * @return True if the player is in one of the dead states.
 */
inline bool PlayerAliveWatcherCharacter::isDead() const {
    return al::isNerve(this, &NrvPlayerAliveWatcherCharacterDead) ||
           al::isNerve(this, &NrvPlayerAliveWatcherCharacterAbyss) ||
           al::isNerve(this, &NrvPlayerAliveWatcherCharacterKill);
}

/**
 * @brief Deactivates the player unless it is already out of the game.
 */
inline void PlayerAliveWatcherCharacter::deactivate() {
    if (!isGameOver()) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterKill);
    }
}

/**
 * @brief Checks whether the player waits for a bubble or was just put back into one.
 * @return True if the player waits for a revive bubble.
 */
inline bool PlayerAliveWatcherCharacter::isWaitBubbleForRevive() const {
    if (al::isNerve(this, &NrvPlayerAliveWatcherCharacterBubbleWait)) {
        return true;
    }

    return al::isNerve(this, &NrvPlayerAliveWatcherCharacterReviveWait) &&
           al::isLessEqualStep(this, 1) && al::isDead(mActor);
}

/**
 * @brief Checks whether the player is alive.
 * @return True if the player is in the Alive state.
 */
inline bool PlayerAliveWatcherCharacter::isAlive() const {
    return al::isNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
}

/**
 * @brief Checks whether the player was deactivated.
 * @return True if the player is in the Kill state.
 */
inline bool PlayerAliveWatcherCharacter::isKill() const {
    return al::isNerve(this, &NrvPlayerAliveWatcherCharacterKill);
}

/**
 * @brief Checks whether the player or its guide icon stayed out of the screen for too long.
 * @param screenOutFrame Frame limit for the player being off screen.
 * @param iconOutFrame Frame limit for the guide icon being off screen.
 * @return True if either limit is exceeded and the player isn't being revived.
 */
inline bool PlayerAliveWatcherCharacter::isScreenIconOut(s32 screenOutFrame,
                                                         s32 iconOutFrame) const {
    if (al::isNerve(this, &NrvPlayerAliveWatcherCharacterReviveWait)) {
        return false;
    }

    return mFrameOut->getScreenOutFrame() > screenOutFrame ||
           mFrameOut->getIconOutFrame() > iconOutFrame;
}

/**
 * @brief Puts the player into the revive bubble.
 * @return The watched player.
 */
inline PlayerActor* PlayerAliveWatcherCharacter::startBubbleRevive() {
    al::setNerve(this, &NrvPlayerAliveWatcherCharacterBubbleWait);
    return mActor;
}

/**
 * @brief Puts the player into a bubble because it left the screen.
 */
inline void PlayerAliveWatcherCharacter::startBubbleScreenOut() {
    al::setNerve(this, &NrvPlayerAliveWatcherCharacterBubbleWaitForScreenOut);
}

/**
 * @brief Puts the player into a bubble because the bubble button was pressed.
 */
inline void PlayerAliveWatcherCharacter::startBubbleInput() {
    al::setNerve(this, &NrvPlayerAliveWatcherCharacterBubbleWaitForInput);
}

/**
 * @brief Hands the amiibo director to another alive player of the group if one remains.
 */
inline void PlayerAliveWatcherCharacter::tryReassignAmiiboDirector() {
    if (al::getAlivePlayerNum(mActor) > 0) {
        PlayerAmiiboDirector* director = mActor->getAmiiboDirector();
        if (director != nullptr) {
            mGroup->handleAmiiboReassignment(director);
        }
    }
}

/**
 * @brief Gives the amiibo director to the first alive player not already owning it.
 * @param pDirector The amiibo director to hand over.
 */
void PlayerAliveWatcherGroup::handleAmiiboReassignment(PlayerAmiiboDirector* pDirector) {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (mCharacters[i]->isAlive() &&
            !pDirector->isCurrentPlayerActor(mCharacters[i]->getActor())) {
            mCharacters[i]->setAmiiboDirector(pDirector);
            return;
        }
    }
}

/**
 * @brief Sets the amiibo director of the watched player.
 * @param pDirector The amiibo director.
 */
void PlayerAliveWatcherCharacter::setAmiiboDirector(PlayerAmiiboDirector* pDirector) {
    mActor->setAmiiboDirector(pDirector);
}

/**
 * @brief Updates the nerve and whether the off-screen guide may offer the bubble.
 */
void PlayerAliveWatcherCharacter::update() {
    if (mNerveKeeper != nullptr) {
        mNerveKeeper->update();
    }

    bool isEnableBubble = mGroup->mTractorBubble->isEnableBubbleOutFrame(mActor);
    mFrameOut->setEnableBubble(isEnableBubble);
}

/**
 * @brief Initial state: picks Kill or Alive from the player's state.
 */
void PlayerAliveWatcherCharacter::exeInit() {
    if (al::isDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterKill);
    } else {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
    }
}

/**
 * @brief Deactivated: the player is ignored by the camera until it comes back to life.
 */
void PlayerAliveWatcherCharacter::exeKill() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, false);
        al::offAreaTarget(mActor);
    }

    if (!rc::isPlayerDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
    }
}

/**
 * @brief Alive: waits for the player to die or fall into the abyss.
 */
void PlayerAliveWatcherCharacter::exeAlive() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, true);
        al::onAreaTarget(mActor);
    }

    if (!mGroup->mWatcher->mIsSingleMode) {
        mFrameOut->setEnable();
    }

    if (rc::isPlayerAbyss(mActor)) {
        mGroup->mWatcher->mIsAbyss = true;
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAbyss);
        return;
    }

    if (rc::isPlayerDead(mActor)) {
        mGroup->mWatcher->mIsAbyss = false;
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterDead);
    }
}

/**
 * @brief Dead: waits for the death animation to end before handling the death.
 */
void PlayerAliveWatcherCharacter::exeDead() {
    if (al::isFirstStep(this)) {
        tryReassignAmiiboDirector();

        if (al::getAlivePlayerNum(mActor) <= 1) {
            al::setCameraCalcTargetFlag(mActor, false);
            al::offAreaTarget(mActor);
        }
    }

    if (!rc::isPlayerDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
        return;
    }

    if (rc::isPlayerVanishDying(mActor) || !rc::isPlayingDeadAnim(mActor)) {
        deadPlayer();
        return;
    }

    if (al::isDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterKill);
    }
}

/**
 * @brief Abyss: the player fell out of the stage and is handled as dead right away.
 */
void PlayerAliveWatcherCharacter::exeAbyss() {
    if (al::isFirstStep(this)) {
        tryReassignAmiiboDirector();
    }

    deadPlayer();
}

/**
 * @brief BubbleWait: waits until the player may be brought back in a bubble.
 */
void PlayerAliveWatcherCharacter::exeBubbleWait() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, false);
        al::offAreaTarget(mActor);
    }

    if (mIsDemo) {
        return;
    }

    TractorBubble* bubble = mGroup->mTractorBubble;
    if (!mGroup->mWatcher->mIsEnableBubbleRevive || rc::isPlayerChangeDemoAny(bubble) ||
        !al::isGreaterEqualStep(this, 120)) {
        return;
    }

    mGroup->mTractorBubble->activatePlayerWithBubble(mActor);
    al::setNerve(this, &NrvPlayerAliveWatcherCharacterReviveWait);
}

/**
 * @brief BubbleWaitForScreenOut: puts the player that left the screen into a bubble.
 */
void PlayerAliveWatcherCharacter::exeBubbleWaitForScreenOut() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, false);
        al::offAreaTarget(mActor);
    }

    if (rc::isPlayerAbyss(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAbyss);
        return;
    }

    if (rc::isPlayerDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterDead);
        return;
    }

    if (mGroup->mTractorBubble->isEnableBubbleOutFrame(mActor)) {
        mGroup->mTractorBubble->startBubbleWithScreenOut(mActor);
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterReviveWait);
        return;
    }

    al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
}

/**
 * @brief BubbleWaitForInput: puts the player that pressed the bubble button into a bubble.
 */
void PlayerAliveWatcherCharacter::exeBubbleWaitForInput() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, false);
        al::offAreaTarget(mActor);
    }

    if (rc::isPlayerAbyss(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAbyss);
        return;
    }

    if (rc::isPlayerDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterDead);
        return;
    }

    if (mGroup->mTractorBubble->isEnableBubbleInput(mActor)) {
        mGroup->mTractorBubble->startBubbleWithInput(mActor);
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterReviveWait);
        return;
    }

    al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
}

/**
 * @brief ReviveWait: waits for the player to leave the bubble.
 */
void PlayerAliveWatcherCharacter::exeReviveWait() {
    if (al::isFirstStep(this)) {
        al::setCameraCalcTargetFlag(mActor, false);
        al::offAreaTarget(mActor);
    }

    if (rc::isPlayerAbyss(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAbyss);
        return;
    }

    if (rc::isPlayerDead(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterDead);
        return;
    }

    if (!mGroup->mTractorBubble->isPlayerInBubble() && !mGroup->mTractorBubble->isBindWait() &&
        al::isAlive(mActor)) {
        al::setNerve(this, &NrvPlayerAliveWatcherCharacterAlive);
    }
}

/**
 * @brief Creates the watchers of all players of one character type and the shared bubble.
 * @param pWatcher The owning watcher.
 * @param rInfo Actor init info.
 * @param pHolder Holder of all players.
 * @param charaType The character type (EPlayerChara) of this group.
 */
PlayerAliveWatcherGroup::PlayerAliveWatcherGroup(PlayerAliveWatcher* pWatcher,
                                                 const al::ActorInitInfo& rInfo,
                                                 const al::PlayerHolder* pHolder, s32 charaType)
    : mWatcher(pWatcher), mCharaType(charaType) {
    PlayerActor* players[8] = {};

    s32 playerNumMax = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNumMax; i++) {
        auto* player = static_cast<PlayerActor*>(al::getPlayerActor(pHolder, i));
        if (rc::isPlayerChara(player, charaType)) {
            players[mCharacterNum] = player;
            mCharacterNum++;
        }
    }

    mCharacters = new PlayerAliveWatcherCharacter*[mCharacterNum];
    for (s32 i = 0; i < mCharacterNum; i++) {
        mCharacters[i] = new PlayerAliveWatcherCharacter(rInfo, players[i], this);
    }

    if (players[0] != nullptr) {
        players[0]->createPlayerAmiiboDirector(mWatcher->mAmiiboDirectorWatcher, rInfo);
    }

    mTractorBubble = new TractorBubble(pWatcher->mIsSingleMode);
    mTractorBubble->init(rInfo);
    mTractorBubble->kill();
}

/**
 * @brief Starts watching: dead players are deactivated, the others are alive.
 */
void PlayerAliveWatcherGroup::appear() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        PlayerAliveWatcherCharacter* character = mCharacters[i];
        if (rc::isPlayerDead(character->getActor())) {
            al::setNerve(character, &NrvPlayerAliveWatcherCharacterKill);
        } else {
            al::setNerve(character, &NrvPlayerAliveWatcherCharacterAlive);
        }
    }
}

/**
 * @brief Hides the nameplates and off-screen guides during a demo.
 */
void PlayerAliveWatcherGroup::startDemo() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        PlayerAliveWatcherCharacter* character = mCharacters[i];
        character->mActor->setNameplateVisible(false, false);
        character->mFrameOut->startDemo();
        character->mIsDemo = true;
    }
}

/**
 * @brief Shows the nameplates and off-screen guides again after a demo.
 */
void PlayerAliveWatcherGroup::endDemo() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        PlayerAliveWatcherCharacter* character = mCharacters[i];
        character->mActor->setNameplateVisible(true, false);
        character->mFrameOut->endDemo();
        character->mIsDemo = false;
    }
}

/**
 * @brief Removes the equipment of all living players on game over.
 */
void PlayerAliveWatcherGroup::onGameOver() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!al::isDead(mCharacters[i]->getActor())) {
            rc::removeAllEquipFromPlayer(mCharacters[i]->getActor());
        }
    }
}

/**
 * @brief Pauses the off-screen guides.
 */
void PlayerAliveWatcherGroup::startPause() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        mCharacters[i]->mFrameOut->startPause();
    }
}

/**
 * @brief Resumes the off-screen guides.
 */
void PlayerAliveWatcherGroup::endPause() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        mCharacters[i]->mFrameOut->endPause();
    }
}

/**
 * @brief Finds the control user playing this group's character type.
 * @return The control user id, or a negative value if nobody plays it.
 */
s32 PlayerAliveWatcherGroup::tryCalcUserId() const {
    s32 charaType = mCharaType;
    return rc::tryCalcControlUserIdByCharacterType(mCharacters[0]->getActor(), charaType, true);
}

/**
 * @brief Checks whether all players were deactivated.
 * @return True if every player is in the Kill state.
 */
bool PlayerAliveWatcherGroup::isAllDeactive() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isKill()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Deactivates every player that isn't already out of the game.
 */
void PlayerAliveWatcherGroup::deactivateAll() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        mCharacters[i]->deactivate();
    }
}

/**
 * @brief Checks whether all players are dead.
 * @return True if every player is in a dead state.
 */
bool PlayerAliveWatcherGroup::isAllDead() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isDead()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether all players are dead or in a bubble.
 * @return True if no player is active.
 */
bool PlayerAliveWatcherGroup::isAllDeadOrBubble() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!rc::isPlayerDeadOrBubble(mCharacters[i]->getActor())) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether all players are out of the game.
 * @return True if every player is out of the game.
 */
bool PlayerAliveWatcherGroup::isGameOver() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isGameOver()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether an alive player may leave the stage.
 * @return True if a player is in a state allowing to exit the stage.
 */
bool PlayerAliveWatcherGroup::isEnableExitStage() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isAlive()) {
            continue;
        }

        PlayerActor* player = mCharacters[i]->getActor();
        al::HitSensor* bindSensor = player->getBindSensor();
        if (bindSensor != nullptr) {
            if (rc::sendMsgIsEnableExitStage(bindSensor, bindSensor)) {
                return true;
            }
        } else if ((rc::isPlayerOnGround(player) && mOnGroundFrame > 1) ||
                   rc::isPlayerInWater(player)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether an alive player may warp to another island.
 * @return True if a player is in a state allowing the island warp.
 */
bool PlayerAliveWatcherGroup::isEnableIslandWarp() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isAlive()) {
            continue;
        }

        PlayerActor* player = mCharacters[i]->getActor();
        al::HitSensor* bindSensor = player->getBindSensor();
        if (bindSensor != nullptr) {
            if (rc::sendMsgIsEnableIslandWarp(bindSensor, bindSensor)) {
                return true;
            }
        } else if ((rc::isPlayerOnGround(player) && mOnGroundFrame > 23) ||
                   rc::isPlayerInWater(player)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether a player waits for a revive bubble.
 * @return True if any player waits for a revive bubble.
 */
bool PlayerAliveWatcherGroup::isWaitBubbleForRevive() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (mCharacters[i]->isWaitBubbleForRevive()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Counts the players that weren't deactivated.
 * @return The number of players not in the Kill state.
 */
s32 PlayerAliveWatcherGroup::calcNoKillPlayerNum() const {
    s32 num = 0;
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isKill()) {
            num++;
        }
    }

    return num;
}

/**
 * @brief Checks whether a player is the only one of the group still active.
 * @param pActor The player to check.
 * @return True if every other player was deactivated.
 */
bool PlayerAliveWatcherGroup::isLastOne(const al::LiveActor* pActor) const {
    bool isLast = false;
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (mCharacters[i]->isKill()) {
            continue;
        }

        if (mCharacters[i]->getActor() != pActor) {
            return false;
        }

        isLast = true;
    }

    return isLast;
}

/**
 * @brief Counts the alive players.
 * @return The number of players in the Alive state.
 */
s32 PlayerAliveWatcherGroup::calcActivePlayerNum() const {
    s32 num = 0;
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (mCharacters[i]->isAlive()) {
            num++;
        }
    }

    return num;
}

/**
 * @brief Finds the first player that is neither dead nor in a bubble.
 * @return The player, or nullptr if there is none.
 */
PlayerActor* PlayerAliveWatcherGroup::findActivePlayerFirst() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!rc::isPlayerDeadOrBubble(mCharacters[i]->getActor())) {
            return mCharacters[i]->getActor();
        }
    }

    return nullptr;
}

/**
 * @brief Finds the first player that is neither dead nor in a bubble.
 * @return The player, or nullptr if there is none.
 */
PlayerActor* PlayerAliveWatcherGroup::tryFindActivePlayerFirst() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!rc::isPlayerDeadOrBubble(mCharacters[i]->getActor())) {
            return mCharacters[i]->getActor();
        }
    }

    return nullptr;
}

/**
 * @brief Brings the group's first player back in a revive bubble.
 * @return The revived player.
 */
PlayerActor* PlayerAliveWatcherGroup::activatePlayer() {
    return mCharacters[0]->startBubbleRevive();
}

/**
 * @brief Checks whether an alive player isn't riding a cloud bonus cannon.
 * @return True if a player of this group may let another group enter a bubble.
 */
bool PlayerAliveWatcherGroup::isEnableOtherGroupBubbleWithInput() const {
    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isAlive()) {
            continue;
        }

        al::HitSensor* bindSensor = mCharacters[i]->getActor()->getBindSensor();
        if (bindSensor == nullptr) {
            return true;
        }

        if (!al::isSensorHostName(bindSensor, "雲ボーナス大砲") &&
            !al::isSensorHostName(bindSensor, "雲ボーナス大砲（暗闇用）")) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Switches the players to the circle shadow used in dark areas.
 */
void PlayerAliveWatcherGroup::changeShadowLight() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        PlayerActor* player = mCharacters[i]->getActor();
        if (!player->isValidCircleShadow()) {
            player->validateCircleShadow();
        }
    }
}

/**
 * @brief Switches the players back to their normal shadow.
 */
void PlayerAliveWatcherGroup::changeShadowNormal() {
    for (s32 i = 0; i < mCharacterNum; i++) {
        PlayerActor* player = mCharacters[i]->getActor();
        if (player->isValidCircleShadow()) {
            player->invalidateCircleShadow();
        }
    }
}

/**
 * @brief Updates the watchers, bubbles players that left the screen, revives the group with a
 * life when everyone died and handles the bubble button.
 * @return True if a player of the group is alive.
 */
bool PlayerAliveWatcherGroup::update() {
    if (mCharacterNum <= 0 || tryCalcUserId() < 0) {
        return false;
    }

    bool isAnyAlive = false;
    bool isAnyOnGround = false;
    for (s32 i = 0; i < mCharacterNum; i++) {
        mCharacters[i]->update();
        bool isOnGround = rc::isPlayerOnGround(mCharacters[i]->getActor());

        if (mCharacters[i]->isAlive()) {
            isAnyAlive = true;

            s32 giantFrame = rc::isPlayerGiant(mCharacters[i]->getActor()) ? 300 : 0;
            s32 iconOutFrame = rc::isPlayerGiant(mCharacters[i]->getActor()) ? 300 : 90;
            if (mCharacters[i]->isScreenIconOut(mWatcher->mBubbleDelayTime + giantFrame,
                                                iconOutFrame) &&
                mWatcher->mIsEnableBubbleScreenOut) {
                if (isLastOne(mCharacters[i]->getActor())) {
                    if (mWatcher->calcAllActivePlayerNum() >= 2) {
                        mCharacters[i]->startBubbleScreenOut();
                    }
                } else if (mCharacters[i]->isScreenIconOut(180, 180)) {
                    al::stopScene(mCharacters[i]->getActor(), 20, 0, false, false);
                    al::startSe(mCharacters[i]->getActor(), "PgDoublePlayerDead");
                    mCharacters[i]->deadPlayer();
                }
            }
        }

        isAnyOnGround |= isOnGround;
    }

    if (isAnyOnGround) {
        mOnGroundFrame++;
    } else {
        mOnGroundFrame = 0;
    }

    if (isAllDead() && !mIsReviveRequested &&
        !GameDataFunction::isGameOver(mCharacters[0]->getActor()) &&
        !mWatcher->mIsNoLifeDecrease) {
        GameDataHolderAccessor accessor(mCharacters[0]->getActor());
        GameDataFunction::addPlayerLife(accessor, -1);
        if (!GameDataFunction::isGameOver(mCharacters[0]->getActor())) {
            mIsReviveRequested = true;
        }
    }

    if (isAllDeactive() && mIsReviveRequested && mWatcher->calcAllActivePlayerNum() > 0) {
        mIsReviveRequested = false;
        activatePlayer();
        return isAnyAlive;
    }

    PlayerActor* player = findActivePlayerFirst();
    if (player == nullptr || !player->getInput()->isBubbleTrigOn() ||
        mTractorBubble->isBubble()) {
        return isAnyAlive;
    }

    if (calcActivePlayerNum() != 1) {
        al::startSe(mTractorBubble, "Invalid");
        return isAnyAlive;
    }

    if (!mWatcher->isEnableBubbleWithInput(this) || !mWatcher->mIsEnableBubbleRevive) {
        return isAnyAlive;
    }

    for (s32 i = 0; i < mCharacterNum; i++) {
        if (!mCharacters[i]->isAlive()) {
            continue;
        }

        PlayerActor* alivePlayer = mCharacters[i]->getActor();
        if (rc::isPlayerOnGround(alivePlayer) &&
            mTractorBubble->isEnableBubbleInput(alivePlayer)) {
            mCharacters[i]->startBubbleInput();
            al::startSe(alivePlayer, "PgGetInBubble");
        }

        break;
    }

    return isAnyAlive;
}
