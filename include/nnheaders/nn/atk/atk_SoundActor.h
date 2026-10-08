#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_SoundStartable.h>

namespace nn::atk {
class SoundArchivePlayer;

class SoundActor : public SoundStartable {
public:
    static const int ActorPlayerCount = 4;

    SoundActor();
    ~SoundActor() override;

    void Initialize(SoundArchivePlayer* pSoundArchivePlayer);
    void StopAllSound(int fadeFrames);
    void PauseAllSound(bool flag, int fadeFrames);
    int GetPlayingSoundCount(int actorPlayerId) const;

    /**
     * @brief Gets the player that limits the sounds of one actor player id.
     * @param actorPlayerId Actor player id in [0, ActorPlayerCount).
     * @return Actor player, or nullptr when the id is out of range.
     */
    detail::ExternalSoundPlayer* detail_GetActorPlayer(int actorPlayerId) {
        if (actorPlayerId < 0 || actorPlayerId >= ActorPlayerCount) {
            return nullptr;
        }
        return m_pActorPlayers[actorPlayerId];
    }

    /**
     * @brief Gets the parameters applied to every sound of this actor.
     * @return Actor parameters.
     */
    const detail::SoundActorParam& detail_GetActorParam() const { return m_ActorParam; }

    virtual StartResult SetupSound(SoundHandle* pHandle, u32 soundId, const StartInfo* pStartInfo,
                                   void* pSetupArg);
    virtual StartResult SetupSound(SoundHandle* pHandle, u32 soundId, const char* pSoundArchiveName,
                                   const StartInfo* pStartInfo, void* pSetupArg);

private:
    virtual StartResult detail_SetupSoundWithAmbientInfo(SoundHandle* pHandle, u32 soundId,
                                                         const char* pSoundArchiveName,
                                                         const StartInfo* pStartInfo,
                                                         detail::BasicSound::AmbientInfo* pAmbientInfo,
                                                         void* pSetupArg);
    StartResult detail_SetupSound(SoundHandle* pHandle, u32 soundId, bool holdFlag,
                                  const char* pSoundArchiveName, const StartInfo* pStartInfo) override;
    u32 detail_GetItemId(const char* pString) override;
    u32 detail_GetItemId(const char* pString, const char* pSoundArchiveName) override;

    u8 _8[0x90 - 0x8];
    detail::ExternalSoundPlayer* m_pActorPlayers[ActorPlayerCount];
    detail::SoundActorParam m_ActorParam;
    bool m_IsInitialized;
    bool m_IsFinalized;
};
static_assert(sizeof(SoundActor) == 0xd0);
}  // namespace nn::atk
