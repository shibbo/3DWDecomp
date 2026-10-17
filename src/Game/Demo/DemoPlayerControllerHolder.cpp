#include "Demo/DemoPlayerControllerHolder.hpp"
#include "Demo/DemoPlayerController.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * @brief Allocates capacity for eight demo participants.
 * @param pPlayerHolder Scene player holder supplying the participants.
 */
DemoPlayerControllerHolder::DemoPlayerControllerHolder(al::PlayerHolder* pPlayerHolder)
    : mPlayerHolder(pPlayerHolder), mControllers(nullptr), mPlayers(nullptr),
      mPlayerCount(0), mIsRequested(false) {
    mControllers = new DemoPlayerController[8];
    mPlayers = new PlayerActor*[8];
    for (int i = 0; i < 8; ++i) {
        mPlayers[i] = nullptr;
    }
}

/**
 * @brief Collects living players once and retries starting every participant.
 * @return Whether every collected player has entered the demo.
 */
bool DemoPlayerControllerHolder::requestStartDemo() {
    if (!mIsRequested) {
        mIsRequested = true;
        int playerNum = al::getPlayerNumMax(mPlayerHolder);
        mPlayerCount = 0;
        for (int i = 0; i < playerNum; ++i) {
            auto* pPlayer = static_cast<PlayerActor*>(al::getPlayerActor(mPlayerHolder, i));
            if (!rc::isPlayerDead(pPlayer)) {
                mPlayers[mPlayerCount] = pPlayer;
                mControllers[mPlayerCount].setPlayerActor(pPlayer);
                ++mPlayerCount;
            }
        }
    }
    mIsStarted = true;
    for (int i = 0; i < mPlayerCount; ++i) {
        mIsStarted &= mControllers[i].tryStartDemo();
    }
    return mIsStarted;
}

/** @brief Ends the participants' demos and clears the request and started flags. */
void DemoPlayerControllerHolder::requestEndDemo() {
    endDemo();
    mIsStarted = false;
    mIsRequested = false;
}

/** @brief Checks completion of the start request. @return Whether all participants started. */
bool DemoPlayerControllerHolder::isStartDemo() const {
    return mIsStarted;
}

/** @brief Ends every participant's demo without resetting the holder flags. */
void DemoPlayerControllerHolder::endDemo() {
    for (int i = 0; i < mPlayerCount; ++i) {
        mControllers[i].endDemo();
    }
}

/** @brief Gets the number of collected participants. @return Demo player count. */
int DemoPlayerControllerHolder::getDemoPlayerNum() const {
    return mPlayerCount;
}

/**
 * @brief Gets a participant's controller.
 * @param index Valid zero-based participant index.
 * @return Controller at the requested index.
 */
DemoPlayerController* DemoPlayerControllerHolder::getDemoPlayer(int index) const {
    return &mControllers[index];
}

/**
 * @brief Finds the first participant of the requested character type.
 * @param character Character type to locate.
 * @return Matching controller, or nullptr if no participant matches.
 */
DemoPlayerController* DemoPlayerControllerHolder::getDemoPlayerByCharacter(int character) const {
    for (int i = 0; i < mPlayerCount; ++i) {
        if (rc::getPlayerCharaType(mControllers[i].getPlayerActor()) == character) {
            return &mControllers[i];
        }
    }
    return nullptr;
}

/**
 * @brief Finds a participant by actor identity.
 * @param pActor Actor to locate.
 * @return Matching controller, or nullptr if the actor is not a participant.
 */
DemoPlayerController* DemoPlayerControllerHolder::getDemoPlayerByActor(const al::LiveActor* pActor) const {
    for (int i = 0; i < mPlayerCount; ++i) {
        if (mControllers[i].getPlayerActor() == pActor) {
            return &mControllers[i];
        }
    }
    return nullptr;
}
