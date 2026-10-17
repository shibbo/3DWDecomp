#include "Game/Bgm/PlayerBigBgmController.hpp"

#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"

/**
 * @brief Creates the audio keeper for the giant-player BGM.
 * @param pDirector Audio director used by the keeper.
 */
PlayerBigBgmController::PlayerBigBgmController(al::AudioDirector* pDirector) {
    mAudioKeeper = al::createAudioKeeper("Coin", pDirector);
}

/**
 * @brief Advances the time-up situations and handles a requested BGM restart.
 */
void PlayerBigBgmController::update() {
    if (mIsActive) {
        mTimer++;
        if (mSituationIndex < 2) {
            if (mTimer >= 930 && mSituationIndex == 0) {
                al::changeBgmSituation(this, "GiantTimeUp");
                mSituationIndex++;
            } else if (mTimer >= 1125 && mSituationIndex == 1) {
                al::changeBgmSituation(this, "GiantTimeUpFast");
                mSituationIndex++;
            }
        }
    }

    if (mIsStartRequested) {
        al::stopBgm(this, "Giant", -1, -1);
        al::startBgm(this, "Giant", -1, 0, -1, -1);
    }
    mIsStartRequested = false;
}

/** @brief Registers another giant player. */
void PlayerBigBgmController::notifyStart() {
    mPlayerCount++;
}

/** @brief Stops the giant BGM when the last registered giant player finishes. */
void PlayerBigBgmController::notifyEnd() {
    if (mPlayerCount == 0) {
        return;
    }
    mPlayerCount--;
    if (mPlayerCount == 0) {
        al::changeBgmSituation(this, "GiantEnd");
        al::stopBgm(this, "Giant", -1, -1);
        mIsActive = false;
    }
}

/** @brief Resets the giant timer and requests the BGM to start on the next update. */
void PlayerBigBgmController::notifyGet() {
    mTimer = 0;
    mIsStartRequested = true;
    mIsActive = true;
    mSituationIndex = 0;
    al::changeBgmSituation(this, "GiantEnd");
}

/**
 * @brief Gets the controller's audio keeper.
 * @return Audio keeper created for the giant-player BGM.
 */
al::AudioKeeper* PlayerBigBgmController::getAudioKeeper() const {
    return mAudioKeeper;
}
