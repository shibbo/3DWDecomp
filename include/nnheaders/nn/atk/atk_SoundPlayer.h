#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_PlayerHeap.h>
#include <nn/types.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
namespace detail {
class OutputAdditionalParam;

/**
 * @brief Node traits for a BasicSound node stored at a fixed offset inside the sound.
 * @tparam Offset Byte offset of the IntrusiveListNode within BasicSound.
 */
template <size_t Offset>
class BasicSoundOffsetNodeTraits {
public:
    /**
     * @brief Gets a sound's list node.
     * @param rSound Sound whose node is requested.
     * @return Node at Offset inside the sound.
     */
    static util::IntrusiveListNode& GetNode(BasicSound& rSound) {
        return *reinterpret_cast<util::IntrusiveListNode*>(reinterpret_cast<char*>(&rSound) +
                                                           Offset);
    }

    /**
     * @brief Gets a sound's list node without modifying it.
     * @param rSound Sound whose node is requested.
     * @return Node at Offset inside the sound.
     */
    static const util::IntrusiveListNode& GetNode(const BasicSound& rSound) {
        return *reinterpret_cast<const util::IntrusiveListNode*>(
            reinterpret_cast<const char*>(&rSound) + Offset);
    }

    /**
     * @brief Recovers the sound that owns a node.
     * @param rNode Node belonging to a sound; must not be a list sentinel.
     * @return Sound containing the node.
     */
    static BasicSound& GetItem(util::IntrusiveListNode& rNode) {
        return *reinterpret_cast<BasicSound*>(reinterpret_cast<char*>(&rNode) - Offset);
    }

    /**
     * @brief Recovers the sound that owns a node without modifying it.
     * @param rNode Node belonging to a sound; must not be a list sentinel.
     * @return Sound containing the node.
     */
    static const BasicSound& GetItem(const util::IntrusiveListNode& rNode) {
        return *reinterpret_cast<const BasicSound*>(reinterpret_cast<const char*>(&rNode) -
                                                    Offset);
    }
};
}  // namespace detail

/** @brief Groups sounds, limits how many play at once and owns per-player heaps. */
class SoundPlayer {
public:
    typedef util::IntrusiveList<detail::BasicSound, detail::BasicSoundOffsetNodeTraits<0x1e0>>
        SoundList;
    typedef util::IntrusiveList<detail::BasicSound, detail::BasicSoundOffsetNodeTraits<0x1f0>>
        PriorityList;
    typedef util::IntrusiveList<detail::PlayerHeap, detail::PlayerHeap::LinkNodeTraits>
        PlayerHeapList;

    static const int PriorityMin = 0;
    static const int PriorityMax = 127;

    SoundPlayer();
    explicit SoundPlayer(detail::OutputAdditionalParam* pParam);
    ~SoundPlayer();

    void Update();
    void DoFreePlayerHeap();
    void StopAllSound(int fadeFrames);
    void PauseAllSound(bool flag, int fadeFrames);
    void PauseAllSound(bool flag, int fadeFrames, PauseMode pauseMode);

    void SetVolume(f32 volume);
    void SetLowPassFilterFrequency(f32 lowPassFrequency);
    void SetBiquadFilter(int type, f32 value);
    void SetDefaultOutputLine(u32 outputLine);
    void SetMainSend(f32 send);
    f32 GetMainSend() const;
    void SetEffectSend(AuxBus bus, f32 send);
    f32 GetEffectSend(AuxBus bus) const;
    void SetSend(int bus, f32 send);
    f32 GetSend(int bus);
    void SetOutputVolume(OutputDevice device, f32 volume);
    void SetPlayableSoundCount(int count);

    /**
     * @brief Counts the sounds currently attached to this player.
     * @return Number of sounds in the sound list.
     */
    int GetPlayingSoundCount() const { return m_SoundList.size(); }

    /**
     * @brief Gets how many sounds may play at once.
     * @return Playable sound count.
     */
    int GetPlayableSoundCount() const { return m_PlayableCount; }

    /**
     * @brief Gets the sound with the lowest priority.
     * @return Front of the priority list; the list must not be empty.
     */
    detail::BasicSound* GetLowestPrioritySound() { return &m_PriorityList.front(); }

    /**
     * @brief Gets the sounds currently attached to this player.
     * @return Sound list in attachment order.
     */
    SoundList& detail_GetSoundList() { return m_SoundList; }

    /** @brief Gets the player volume. @return Linear gain. */
    f32 GetVolume() const { return m_Volume; }
    /** @brief Gets the low-pass filter offset. @return Frequency offset. */
    f32 GetLowPassFilterFrequency() const { return m_LpfFreq; }
    /** @brief Gets the biquad filter type. @return Filter type, or -1 when unset. */
    int GetBiquadFilterType() const { return m_BiquadType; }
    /** @brief Gets the biquad filter strength. @return Filter value. */
    f32 GetBiquadFilterValue() const { return m_BiquadValue; }
    /** @brief Gets the output lines new sounds use. @return Output line flags. */
    u32 GetDefaultOutputLine() const { return m_OutputLineFlag; }
    /** @brief Gets the volume of the main output. @return Linear gain. */
    f32 GetOutputVolume() const { return m_OutputVolume; }
    /** @brief Gets the main send of the main output. @return Send level. */
    f32 GetOutputMainSend() const { return m_MainSend; }
    /**
     * @brief Gets an aux bus send of the main output.
     * @param bus Aux bus.
     * @return Send level.
     */
    f32 GetOutputEffectSend(AuxBus bus) const { return m_FxSend[bus]; }

    void detail_SortPriorityList(bool reverse);
    void detail_SortPriorityList(detail::BasicSound* pSound);
    bool detail_AppendSound(detail::BasicSound* pSound);
    void detail_RemoveSound(detail::BasicSound* pSound);
    void detail_SetPlayableSoundLimit(int limit);
    bool detail_CanPlaySound(int startPriority);
    void detail_AppendPlayerHeap(detail::PlayerHeap* pHeap);
    detail::PlayerHeap* detail_AllocPlayerHeap();
    void detail_FreePlayerHeap(detail::PlayerHeap* pHeap);

private:
    void RemoveSoundList(detail::BasicSound* pSound);
    void InsertPriorityList(detail::BasicSound* pSound);
    void RemovePriorityList(detail::BasicSound* pSound);

    SoundList m_SoundList;
    PriorityList m_PriorityList;
    PlayerHeapList m_PlayerHeapFreeList;
    PlayerHeapList m_PlayerHeapFreeReqList;
    int m_PlayableCount;
    int m_PlayableLimit;
    int m_PlayerHeapCount;
    f32 m_Volume;
    f32 m_LpfFreq;
    int m_BiquadType;
    f32 m_BiquadValue;
    u32 m_OutputLineFlag;
    f32 m_OutputVolume;
    f32 m_MainSend;
    f32 m_FxSend[AuxBus_Count];
    detail::OutputAdditionalParam* m_pOutputAdditionalParam;
    bool m_IsFirstComeBased;
};
static_assert(sizeof(SoundPlayer) == 0x88, "SoundPlayer size");
}  // namespace nn::atk
