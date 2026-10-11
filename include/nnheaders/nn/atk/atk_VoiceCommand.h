#pragma once

#include <nn/atk/atk_CommandManager.h>

namespace nn::atk::detail {
/** @brief Command queue carrying low-level voice updates to the sound thread. */
class LowLevelVoiceCommand : public CommandManager {
public:
    static LowLevelVoiceCommand& GetInstance();
};
}  // namespace nn::atk::detail
