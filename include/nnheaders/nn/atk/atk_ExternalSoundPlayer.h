#pragma once
#include <nn/atk/atk_BasicSound.h>
#include <nn/util/util_IntrusiveList.h>
namespace nn::atk::detail {
struct ExternalSoundNodeTraits {
    /**
     * @brief Gets a sound's external-player list node.
     * @param rSound Sound whose node is requested.
     * @return Intrusive node at the recovered 0x200 offset.
     */
    static util::IntrusiveListNode& GetNode(BasicSound& rSound) { return rSound.GetExternalPlayerNode(); }
    /**
     * @brief Recovers a sound from its external-player node.
     * @param rNode Node belonging to a BasicSound.
     * @return Sound containing the node.
     */
    static BasicSound& GetItem(util::IntrusiveListNode& rNode) {
        return *reinterpret_cast<BasicSound*>(reinterpret_cast<char*>(&rNode) - 0x200);
    }
    /**
     * @brief Recovers a read-only sound from its list node.
     * @param rNode Node belonging to a BasicSound.
     * @return Sound containing the node.
     */
    static const BasicSound& GetItem(const util::IntrusiveListNode& rNode) {
        return *reinterpret_cast<const BasicSound*>(reinterpret_cast<const char*>(&rNode) - 0x200);
    }
};
class ExternalSoundPlayer {
public:
    ExternalSoundPlayer();
    virtual ~ExternalSoundPlayer();
    void StopAllSound(int fadeFrames);
    void PauseAllSound(bool pause, int fadeFrames);
    void PauseAllSound(bool pause, int fadeFrames, PauseMode mode);
    void Finalize(SoundActor* pActor);
    void RemoveSound(BasicSound* pSound);
    bool AppendSound(BasicSound* pSound);
    BasicSound* GetLowestPrioritySound();
    void SetPlayableSoundCount(int count);
    virtual bool CanPlaySound(int priority);

private:
    using SoundList = util::IntrusiveList<BasicSound, ExternalSoundNodeTraits>;
    SoundList mSounds;
    int mPlayableSoundCount;
};
static_assert(sizeof(ExternalSoundPlayer) == 0x20, "ExternalSoundPlayer size");
} // namespace nn::atk::detail
