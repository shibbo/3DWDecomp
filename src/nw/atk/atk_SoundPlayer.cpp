#include <nn/atk/atk_SoundPlayer.h>

#include <nn/atk/atk_OutputAdditionalParam.h>

namespace nn::atk {

/** @brief Creates a player that allows one sound at a time and has no additional send parameters. */
SoundPlayer::SoundPlayer()
    : m_PlayableCount(1), m_PlayableLimit(INT32_MAX), m_PlayerHeapCount(0), m_Volume(1.0f),
      m_LpfFreq(0.0f), m_BiquadType(-1), m_BiquadValue(0.0f), m_OutputLineFlag(1),
      m_pOutputAdditionalParam(nullptr), m_IsFirstComeBased(false) {
    m_OutputVolume = 1.0f;
    m_MainSend = 0.0f;

    for (int i = 0; i < AuxBus_Count; i++) {
        m_FxSend[i] = 0.0f;
    }
}

/**
 * @brief Creates a player that uses caller-owned additional output parameters.
 * @param pParam Additional output parameters, reset if non-null; may be null.
 */
SoundPlayer::SoundPlayer(detail::OutputAdditionalParam* pParam)
    : m_PlayableCount(1), m_PlayableLimit(INT32_MAX), m_PlayerHeapCount(0), m_Volume(1.0f),
      m_LpfFreq(0.0f), m_BiquadType(-1), m_BiquadValue(0.0f), m_OutputLineFlag(1),
      m_pOutputAdditionalParam(pParam), m_IsFirstComeBased(false) {
    m_OutputVolume = 1.0f;
    m_MainSend = 0.0f;

    for (int i = 0; i < AuxBus_Count; i++) {
        m_FxSend[i] = 0.0f;
    }

    if (pParam != nullptr) {
        pParam->Reset();
    }
}

/** @brief Stops every sound still attached to the player. */
SoundPlayer::~SoundPlayer() {
    StopAllSound(0);
}

/**
 * @brief Stops every sound attached to the player.
 * @param fadeFrames Fade-out length in frames.
 */
void SoundPlayer::StopAllSound(int fadeFrames) {
    for (auto it = m_SoundList.begin(); it != m_SoundList.end();) {
        detail::BasicSound& rSound = *it++;
        rSound.Stop(fadeFrames);
    }
}

/** @brief Releases finished heaps, updates every sound and re-sorts the priority list. */
void SoundPlayer::Update() {
    DoFreePlayerHeap();

    for (auto it = m_SoundList.begin(); it != m_SoundList.end();) {
        detail::BasicSound& rSound = *it++;
        rSound.Update();
    }

    detail_SortPriorityList(false);
}

/** @brief Returns heaps whose pending loads have finished to the free heap list. */
void SoundPlayer::DoFreePlayerHeap() {
    for (auto it = m_PlayerHeapFreeReqList.begin(); it != m_PlayerHeapFreeReqList.end();) {
        auto current = it++;
        if (current->IsLoadFinished()) {
            m_PlayerHeapFreeReqList.erase(current);
            m_PlayerHeapFreeList.push_back(*current);
        }
    }
}

/**
 * @brief Rebuilds the priority list with a bucket sort on each sound's player priority.
 * @param reverse Whether sounds of equal priority are re-inserted in reverse order.
 */
void SoundPlayer::detail_SortPriorityList(bool reverse) {
    if (m_PriorityList.size() < 2) {
        return;
    }

    static PriorityList s_PriorityLists[PriorityMax + 1];

    while (!m_PriorityList.empty()) {
        detail::BasicSound& rSound = m_PriorityList.front();
        m_PriorityList.pop_front();
        s_PriorityLists[rSound.GetPlayerPriority()].push_back(rSound);
    }

    if (reverse) {
        for (int i = 0; i < PriorityMax + 1; i++) {
            while (!s_PriorityLists[i].empty()) {
                detail::BasicSound& rSound = s_PriorityLists[i].back();
                s_PriorityLists[i].pop_back();
                m_PriorityList.push_back(rSound);
            }
        }
    } else {
        for (int i = 0; i < PriorityMax + 1; i++) {
            while (!s_PriorityLists[i].empty()) {
                detail::BasicSound& rSound = s_PriorityLists[i].front();
                s_PriorityLists[i].pop_front();
                m_PriorityList.push_back(rSound);
            }
        }
    }
}

/**
 * @brief Pauses or resumes every sound attached to the player.
 * @param flag True to pause, false to resume.
 * @param fadeFrames Fade length in frames.
 */
void SoundPlayer::PauseAllSound(bool flag, int fadeFrames) {
    for (auto it = m_SoundList.begin(); it != m_SoundList.end();) {
        detail::BasicSound& rSound = *it++;
        rSound.Pause(flag, fadeFrames);
    }
}

/**
 * @brief Pauses or resumes every sound attached to the player.
 * @param flag True to pause, false to resume.
 * @param fadeFrames Fade length in frames.
 * @param pauseMode How the pause is applied.
 */
void SoundPlayer::PauseAllSound(bool flag, int fadeFrames, PauseMode pauseMode) {
    for (auto it = m_SoundList.begin(); it != m_SoundList.end();) {
        detail::BasicSound& rSound = *it++;
        rSound.Pause(flag, fadeFrames, pauseMode);
    }
}

/**
 * @brief Sets the player volume.
 * @param volume Volume multiplier; negative values are clamped to zero.
 */
void SoundPlayer::SetVolume(f32 volume) {
    m_Volume = volume < 0.0f ? 0.0f : volume;
}

/**
 * @brief Sets the low-pass filter frequency offset.
 * @param lowPassFrequency Relative cutoff frequency.
 */
void SoundPlayer::SetLowPassFilterFrequency(f32 lowPassFrequency) {
    m_LpfFreq = lowPassFrequency;
}

/**
 * @brief Sets the biquad filter applied to the player's sounds.
 * @param type Biquad filter type.
 * @param value Filter strength.
 */
void SoundPlayer::SetBiquadFilter(int type, f32 value) {
    m_BiquadType = type;
    m_BiquadValue = value;
}

/**
 * @brief Sets the default output line flags for the player's sounds.
 * @param outputLine Output line bit flags.
 */
void SoundPlayer::SetDefaultOutputLine(u32 outputLine) {
    m_OutputLineFlag = outputLine;
}

/**
 * @brief Sets the main send volume.
 * @param send Main send volume.
 */
void SoundPlayer::SetMainSend(f32 send) {
    m_MainSend = send;
}

/**
 * @brief Gets the main send volume.
 * @return Main send volume.
 */
f32 SoundPlayer::GetMainSend() const {
    return m_MainSend;
}

/**
 * @brief Sets the send volume for an effect bus.
 * @param bus Auxiliary bus.
 * @param send Send volume.
 */
void SoundPlayer::SetEffectSend(AuxBus bus, f32 send) {
    m_FxSend[bus] = send;
}

/**
 * @brief Gets the send volume for an effect bus.
 * @param bus Auxiliary bus.
 * @return Send volume.
 */
f32 SoundPlayer::GetEffectSend(AuxBus bus) const {
    return m_FxSend[bus];
}

/**
 * @brief Sets a send volume by bus index (0 = main, 1-3 = effect buses, others = additional).
 * @param bus Bus index.
 * @param send Send volume.
 */
void SoundPlayer::SetSend(int bus, f32 send) {
    if (bus == 0) {
        SetMainSend(send);
    } else if (bus <= AuxBus_Count) {
        SetEffectSend(static_cast<AuxBus>(bus - 1), send);
    } else {
        if (m_pOutputAdditionalParam != nullptr) {
            m_pOutputAdditionalParam->IsAdditionalSendEnabled();
        }

        m_pOutputAdditionalParam->TrySetAdditionalSend(bus, send);
    }
}

/**
 * @brief Gets a send volume by bus index (0 = main, 1-3 = effect buses, others = additional).
 * @param bus Bus index.
 * @return Send volume.
 */
f32 SoundPlayer::GetSend(int bus) {
    if (bus == 0) {
        return GetMainSend();
    }

    if (bus <= AuxBus_Count) {
        return GetEffectSend(static_cast<AuxBus>(bus - 1));
    }

    return m_pOutputAdditionalParam->TryGetAdditionalSend(bus);
}

/**
 * @brief Sets the volume for an output device.
 * @param device Output device; only the main device is supported.
 * @param volume Volume multiplier; negative values are clamped to zero.
 */
void SoundPlayer::SetOutputVolume(OutputDevice device, f32 volume) {
    if (device == OutputDevice_Main) {
        m_OutputVolume = volume < 0.0f ? 0.0f : volume;
    }
}

/**
 * @brief Detaches a sound from the sound list.
 * @param pSound Sound to remove.
 */
void SoundPlayer::RemoveSoundList(detail::BasicSound* pSound) {
    m_SoundList.erase(m_SoundList.iterator_to(*pSound));
    pSound->DetachSoundPlayer(this);
}

/**
 * @brief Inserts a sound into the priority list, keeping it sorted from lowest priority.
 * @param pSound Sound to insert.
 */
void SoundPlayer::InsertPriorityList(detail::BasicSound* pSound) {
    auto it = m_PriorityList.begin();
    for (; it != m_PriorityList.end(); ++it) {
        if (m_IsFirstComeBased) {
            if (pSound->GetPlayerPriority() <= it->GetPlayerPriority()) {
                break;
            }
        } else {
            if (pSound->GetPlayerPriority() < it->GetPlayerPriority()) {
                break;
            }
        }
    }

    m_PriorityList.insert(it, *pSound);
}

/**
 * @brief Removes a sound from the priority list.
 * @param pSound Sound to remove.
 */
void SoundPlayer::RemovePriorityList(detail::BasicSound* pSound) {
    m_PriorityList.erase(m_PriorityList.iterator_to(*pSound));
}

/**
 * @brief Re-sorts one sound after its priority changed.
 * @param pSound Sound to move.
 */
void SoundPlayer::detail_SortPriorityList(detail::BasicSound* pSound) {
    RemovePriorityList(pSound);
    InsertPriorityList(pSound);
}

/**
 * @brief Attaches a sound, finalizing lower-priority sounds to make room if needed.
 * @param pSound Sound to attach.
 * @return True if the sound was attached.
 */
bool SoundPlayer::detail_AppendSound(detail::BasicSound* pSound) {
    int allocPriority = pSound->GetPlayerPriority();

    if (GetPlayableSoundCount() == 0) {
        return false;
    }

    while (GetPlayingSoundCount() >= GetPlayableSoundCount()) {
        detail::BasicSound* pDropSound = GetLowestPrioritySound();
        if (m_IsFirstComeBased) {
            if (allocPriority <= pDropSound->GetPlayerPriority()) {
                return false;
            }
        } else {
            if (allocPriority < pDropSound->GetPlayerPriority()) {
                return false;
            }
        }

        pDropSound->Finalize();
    }

    m_SoundList.push_back(*pSound);
    InsertPriorityList(pSound);
    pSound->AttachSoundPlayer(this);
    return true;
}

/**
 * @brief Detaches a sound from the player.
 * @param pSound Sound to remove.
 */
void SoundPlayer::detail_RemoveSound(detail::BasicSound* pSound) {
    RemovePriorityList(pSound);
    RemoveSoundList(pSound);
}

/**
 * @brief Sets how many sounds may play at once, finalizing the lowest-priority excess sounds.
 * @param count Playable sound count, clamped to [0, playable limit].
 */
void SoundPlayer::SetPlayableSoundCount(int count) {
    m_PlayableCount = count > m_PlayableLimit ? m_PlayableLimit : (count < 0 ? 0 : count);

    while (GetPlayingSoundCount() > GetPlayableSoundCount()) {
        GetLowestPrioritySound()->Finalize();
    }
}

/**
 * @brief Sets the upper limit for the playable sound count.
 * @param limit Maximum playable sound count.
 */
void SoundPlayer::detail_SetPlayableSoundLimit(int limit) {
    m_PlayableLimit = limit;
}

/**
 * @brief Checks whether a sound of the given priority could start now.
 * @param startPriority Player priority of the sound to start.
 * @return True if the sound fits or would replace a lower-priority sound.
 */
bool SoundPlayer::detail_CanPlaySound(int startPriority) {
    if (GetPlayableSoundCount() == 0) {
        return false;
    }

    if (GetPlayingSoundCount() >= GetPlayableSoundCount()) {
        detail::BasicSound* pLowestSound = GetLowestPrioritySound();
        if (m_IsFirstComeBased) {
            if (startPriority <= pLowestSound->GetPlayerPriority()) {
                return false;
            }
        } else {
            if (startPriority < pLowestSound->GetPlayerPriority()) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Gives the player a heap that sounds can allocate.
 * @param pHeap Heap to add.
 */
void SoundPlayer::detail_AppendPlayerHeap(detail::PlayerHeap* pHeap) {
    pHeap->AttachSoundPlayer(this);
    m_PlayerHeapFreeList.push_back(*pHeap);
    m_PlayerHeapCount++;
}

/**
 * @brief Takes a free heap from the player.
 * @return Heap, or null if none is free.
 */
detail::PlayerHeap* SoundPlayer::detail_AllocPlayerHeap() {
    if (m_PlayerHeapFreeList.empty()) {
        return nullptr;
    }

    detail::PlayerHeap& rHeap = m_PlayerHeapFreeList.front();
    m_PlayerHeapFreeList.pop_front();
    return &rHeap;
}

/**
 * @brief Returns a heap to the player, deferring it while its data load is still pending.
 * @param pHeap Heap to return.
 */
void SoundPlayer::detail_FreePlayerHeap(detail::PlayerHeap* pHeap) {
    if (pHeap->IsLoadFinished()) {
        m_PlayerHeapFreeList.push_back(*pHeap);
    } else {
        m_PlayerHeapFreeReqList.push_back(*pHeap);
    }
}

}  // namespace nn::atk
