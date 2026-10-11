#pragma once

#include <basis/seadTypes.h>

namespace al {
class AudioDirector;
}  // namespace al

/**
 * @brief Plays sound effects from the microphone input.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class MicInputSePlayer {
public:
    explicit MicInputSePlayer(al::AudioDirector* pAudioDirector);

    void update();

private:
    u8 _0[0x20];
};

static_assert(sizeof(MicInputSePlayer) == 0x20);
