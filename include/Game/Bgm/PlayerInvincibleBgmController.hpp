#pragma once

#include <basis/seadTypes.h>

namespace al {
class AudioDirector;
}  // namespace al

/**
 * @brief Plays the invincibility BGM while a player is invincible.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class PlayerInvincibleBgmController {
public:
    explicit PlayerInvincibleBgmController(al::AudioDirector* pAudioDirector);

    void update();

private:
    u8 _0[0x20];
};

static_assert(sizeof(PlayerInvincibleBgmController) == 0x20);
