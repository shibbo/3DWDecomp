#pragma once

#include <basis/seadTypes.h>

namespace al {
class AudioDirector;
}  // namespace al

/**
 * @brief Changes the stage BGM while a player is giant.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class PlayerBigBgmController {
public:
    explicit PlayerBigBgmController(al::AudioDirector* pAudioDirector);

    void update();

private:
    u8 _0[0x28];
};

static_assert(sizeof(PlayerBigBgmController) == 0x28);
