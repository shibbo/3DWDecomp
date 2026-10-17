#include "Game/Bgm/PlayerInvincibleBgmController.hpp"

#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"

/**
 * @brief Creates the audio keeper for the invincible-player BGM.
 * @param pDirector Audio director used by the keeper.
 */
PlayerInvincibleBgmController::PlayerInvincibleBgmController(al::AudioDirector* pDirector) {
    mAudioKeeper = al::createAudioKeeper("Coin", pDirector);
}

/** @brief Processes a pending restart unless an end notification cancels it. */
void PlayerInvincibleBgmController::update() {
    if (mIsStartRequested) {
        bool isCancelRestart = mIsCancelRestart;
        al::stopBgm(this, "Invincible", 0, -1);
        if (!isCancelRestart) {
            al::startBgm(this, "Invincible", -1, 0, -1, -1);
            mIsPlaying = true;
        }
    }
    mIsCancelRestart = false;
    mIsStartRequested = false;
}

/** @brief Saves the playback position and stops the track when pausing is enabled. */
void PlayerInvincibleBgmController::pause() {
    if (!mIsPlaying || mIsPauseDisabled) {
        return;
    }
    mSavedSamplePosition = al::getBgmSamplePos(this, "Invincible");
    al::stopBgm(this, "Invincible", 0, -1);
}

/** @brief Resumes the track from the saved sample position, when one is available. */
void PlayerInvincibleBgmController::resume() {
    if (!mIsPlaying || mIsPauseDisabled) {
        return;
    }
    al::BgmPlayingRequest request("Invincible", 0);
    request.fadeOutFrames = 0;
    if (mSavedSamplePosition >= 0) {
        request._18 = mSavedSamplePosition;
        mSavedSamplePosition = -1;
    }
    al::startBgm(this, request);
}

/**
 * @brief Registers an invincible player unless a shared registration already exists.
 * @param isShared Whether to reuse an existing registration.
 */
void PlayerInvincibleBgmController::notifyStart(bool isShared) {
    if (mPlayerCount > 0 && isShared) {
        return;
    }
    mPlayerCount++;
}

/**
 * @brief Releases a player registration and stops the BGM when none remain.
 * @param isCancelRestart Whether to cancel a restart in the next update.
 */
void PlayerInvincibleBgmController::notifyEnd(bool isCancelRestart) {
    mIsCancelRestart = isCancelRestart;
    mPlayerCount--;
    if (mPlayerCount <= 0) {
        al::stopBgm(this, "Invincible", -1, -1);
        mIsPlaying = false;
    }
}

/** @brief Requests the invincibility BGM to restart during the next update. */
void PlayerInvincibleBgmController::notifyGet() {
    mIsStartRequested = true;
}

/**
 * @brief Gets the controller's audio keeper.
 * @return Audio keeper created for the invincible-player BGM.
 */
al::AudioKeeper* PlayerInvincibleBgmController::getAudioKeeper() const {
    return mAudioKeeper;
}
