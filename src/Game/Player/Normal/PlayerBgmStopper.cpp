#include "Game/Player/Normal/PlayerBgmStopper.hpp"

#include "Game/Player/IUsePlayerActionObserver.hpp"

/**
 * @brief Creates a controller that records entry into the player death demo.
 * @param pObserver Observer used to detect the death demo.
 */
PlayerBgmStopper::PlayerBgmStopper(IUsePlayerActionObserver* pObserver) : mObserver(pObserver) {}

/** @brief Destroys the controller without additional cleanup. */
PlayerBgmStopper::~PlayerBgmStopper() = default;

/** @brief Latches the stopped state when the player enters the death demo. */
void PlayerBgmStopper::update() {
    if (!mIsStopped && mObserver->isDeadDemo()) {
        mIsStopped = true;
    }
}
