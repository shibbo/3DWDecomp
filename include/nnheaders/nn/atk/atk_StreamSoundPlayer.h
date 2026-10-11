#pragma once

#include <nn/types.h>

namespace nn::atk::detail {
/** @brief Driver-side player of a stream sound. */
class StreamSoundPlayer {
public:
    /**
     * @brief Checks whether the player stopped accepting loaded data.
     * @return True when pending load tasks should give up.
     */
    bool IsTaskCancelled() const { return mIsTaskCancelled; }

private:
    // Playback state preceding the cancel flag awaits reconstruction.
    u8 _0[0xda];
    bool mIsTaskCancelled;
};
}  // namespace nn::atk::detail
