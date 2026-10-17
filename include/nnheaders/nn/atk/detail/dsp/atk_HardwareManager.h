#pragma once

#include <nn/atk/atk_CommandManager.h>
#include <nn/atk/atk_HardwareManager.h>

namespace nn::atk::detail {
/** @brief Driver command that resynchronizes every voice with changed global parameters. */
struct DriverCommandAllVoicesSync : Command {
    static const u32 Id = 77;

    u32 syncFlag;
};
static_assert(sizeof(DriverCommandAllVoicesSync) == 0x20, "All-voices sync command size");

/** @brief Driver command that fades the volume of an aux bus of a sub mix. */
struct DriverCommandAuxBusVolume : Command {
    static const u32 Id = 76;

    AuxBus bus;
    int subMixIndex;
    f32 volume;
    int fadeFrames;
};
static_assert(sizeof(DriverCommandAuxBusVolume) == 0x28, "Aux bus volume command size");

/** @brief Global parameter groups that DriverCommandAllVoicesSync can resynchronize. */
enum AllVoicesSyncFlag : u32 {
    AllVoicesSyncFlag_SrcType = 1 << 2,
    AllVoicesSyncFlag_OutputMode = 1 << 3,
    AllVoicesSyncFlag_Volume = 1 << 6,
};
} // namespace nn::atk::detail
