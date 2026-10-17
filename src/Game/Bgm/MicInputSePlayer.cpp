#include "Game/Bgm/MicInputSePlayer.hpp"

#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(MicInputSePlayer, Wait);
NERVE_DECL(MicInputSePlayer, Blow);
NERVES_MAKE_NOSTRUCT(MicInputSePlayer, Wait, Blow);
}  // namespace

/**
 * @brief Creates the microphone audio keeper and enters the waiting state.
 * @param pDirector Audio director used by the keeper.
 */
MicInputSePlayer::MicInputSePlayer(al::AudioDirector* pDirector)
    : al::NerveExecutor("マイク入力SEプレーヤー") {
    mAudioKeeper = al::createAudioKeeper("MicInput", pDirector);
    initNerve(&NrvMicInputSePlayerWait, 0);
}

/** @brief Updates the microphone state and its audio keeper. */
void MicInputSePlayer::update() {
    updateNerve();
    if (mAudioKeeper != nullptr) {
        mAudioKeeper->update();
    }
}

/** @brief Waits until microphone breath input becomes active. */
void MicInputSePlayer::exeWait() {
    al::isFirstStep(this);
    if (al::isMicBreathInputOn(this)) {
        al::setNerve(this, &NrvMicInputSePlayerBlow);
    }
}

/** @brief Returns to waiting when microphone breath input ends. */
void MicInputSePlayer::exeBlow() {
    al::isFirstStep(this);
    if (!al::isMicBreathInputOn(this)) {
        al::setNerve(this, &NrvMicInputSePlayerWait);
    }
}

/**
 * @brief Gets the microphone audio keeper.
 * @return Audio keeper created for microphone input.
 */
al::AudioKeeper* MicInputSePlayer::getAudioKeeper() const {
    return mAudioKeeper;
}
