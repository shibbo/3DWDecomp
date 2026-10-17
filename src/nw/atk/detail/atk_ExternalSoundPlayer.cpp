#include <nn/atk/atk_ExternalSoundPlayer.h>
namespace nn::atk::detail {
/**
 * @brief Creates an empty player allowing one simultaneous sound.
 */
ExternalSoundPlayer::ExternalSoundPlayer() : mPlayableSoundCount(1) {}
/**
 * @brief Detaches the player from every remaining sound.
 */
ExternalSoundPlayer::~ExternalSoundPlayer() {
    for (auto it = mSounds.begin(); it != mSounds.end();) {
        auto& rSound = *it++;
        rSound.DetachExternalSoundPlayer(this);
    }
}
/**
 * @brief Stops every attached sound.
 * @param fadeFrames Fade duration in audio frames; zero stops immediately.
 */
void ExternalSoundPlayer::StopAllSound(int fadeFrames) {
    for (auto it = mSounds.begin(); it != mSounds.end();) {
        auto& rSound = *it++;
        rSound.Stop(fadeFrames);
    }
}
/**
 * @brief Pauses or resumes every sound with the default pause mode.
 * @param pause True to pause, false to resume.
 * @param fadeFrames Transition duration in audio frames.
 */
void ExternalSoundPlayer::PauseAllSound(bool pause, int fadeFrames) {
    for (auto it = mSounds.begin(); it != mSounds.end();) {
        auto& rSound = *it++;
        rSound.Pause(pause, fadeFrames);
    }
}
/**
 * @brief Pauses or resumes every sound with an explicit mode.
 * @param pause True to pause, false to resume.
 * @param fadeFrames Transition duration in audio frames.
 * @param mode Pause policy forwarded to each sound.
 */
void ExternalSoundPlayer::PauseAllSound(bool pause, int fadeFrames, PauseMode mode) {
    for (auto it = mSounds.begin(); it != mSounds.end();) {
        auto& rSound = *it++;
        rSound.Pause(pause, fadeFrames, mode);
    }
}
/**
 * @brief Detaches sounds belonging to one actor.
 * @param pActor Actor whose sounds are removed; compared by identity.
 */
void ExternalSoundPlayer::Finalize(SoundActor* pActor) {
    for (auto it = mSounds.begin(); it != mSounds.end();) {
        auto& rSound = *it++;
        if (rSound.GetSoundActor() == pActor) {
            rSound.DetachSoundActor(pActor);
            RemoveSound(&rSound);
        }
    }
}
/**
 * @brief Unlinks a sound and clears its external-player association.
 * @param pSound Attached sound to remove; non-null.
 */
void ExternalSoundPlayer::RemoveSound(BasicSound* pSound) {
    mSounds.erase(mSounds.iterator_to(*pSound));
    pSound->DetachExternalSoundPlayer(this);
}
/**
 * @brief Attaches a sound, evicting lower-priority sounds when necessary.
 * @param pSound Initialized sound to attach; non-null.
 * @return True if the sound was attached.
 */
bool ExternalSoundPlayer::AppendSound(BasicSound* pSound) {
    int priority = pSound->GetPlayerPriority();
    if (mPlayableSoundCount == 0) {
        return false;
    }
    while (mSounds.size() >= mPlayableSoundCount) {
        BasicSound* pLowest = GetLowestPrioritySound();
        if (pLowest == nullptr) {
            return false;
        }
        if (priority < pLowest->GetPlayerPriority()) {
            return false;
        }
        pLowest->Finalize();
    }
    mSounds.push_back(*pSound);
    pSound->AttachExternalSoundPlayer(this);
    return true;
}
/**
 * @brief Finds the first sound with the lowest effective priority.
 * @return Lowest-priority sound, or null when empty.
 */
BasicSound* ExternalSoundPlayer::GetLowestPrioritySound() {
    int lowestPriority = 128;
    BasicSound* pLowest = nullptr;
    for (auto& rSound : mSounds) {
        int priority = rSound.GetPlayerPriority();
        pLowest = lowestPriority > priority ? &rSound : pLowest;
        lowestPriority = lowestPriority > priority ? priority : lowestPriority;
    }
    return pLowest;
}
/**
 * @brief Changes capacity and evicts excess sounds by priority.
 * @param count Maximum simultaneous sounds; must be nonnegative.
 */
void ExternalSoundPlayer::SetPlayableSoundCount(int count) {
    mPlayableSoundCount = count;
    while (mSounds.size() > mPlayableSoundCount) {
        GetLowestPrioritySound()->Finalize();
    }
}
/**
 * @brief Checks whether a sound can use a free slot or replace an existing sound.
 * @param priority Requested effective priority, normally in [0, 127].
 * @return True if a sound with this priority can be admitted.
 */
bool ExternalSoundPlayer::CanPlaySound(int priority) {
    if (mPlayableSoundCount == 0) {
        return false;
    }
    if (mSounds.size() >= mPlayableSoundCount) {
        BasicSound* pLowest = GetLowestPrioritySound();
        if (pLowest == nullptr) {
            return false;
        }
        if (priority < pLowest->GetPlayerPriority()) {
            return false;
        }
    }
    return true;
}
} // namespace nn::atk::detail
