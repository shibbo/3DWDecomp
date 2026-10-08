#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_BusMixVolumePacket.h>
#include <nn/atk/atk_CommandManager.h>
#include <nn/atk/atk_OutputParam.h>

namespace nn::atk::detail {
/** @brief Driver command ids sent by BasicSound to its driver player. */
enum DriverCommandId {
    DriverCommandId_PlayerInit = 3,
    DriverCommandId_PlayerPanMode = 4,
    DriverCommandId_PlayerPanCurve = 5,
    DriverCommandId_PlayerFinalize = 6,
    DriverCommandId_PlayerStart = 7,
    DriverCommandId_PlayerStop = 8,
    DriverCommandId_PlayerPause = 9,
    DriverCommandId_PlayerParam = 10,
    DriverCommandId_PlayerAdditionalSend = 11,
    DriverCommandId_PlayerBusMixVolumeUsed = 12,
    DriverCommandId_PlayerBusMixVolume = 13,
    DriverCommandId_PlayerBusMixVolumeEnabled = 14,
    DriverCommandId_PlayerBinaryVolume = 15,
    DriverCommandId_PlayerVolumeThroughModeUsed = 16,
    DriverCommandId_PlayerVolumeThroughMode = 17,
    DriverCommandId_PlayerClearResourceFlag = 18,
};

/** @brief Binds a driver player to its output receiver. */
struct DriverCommandPlayerInit : Command {
    driver::BasicSoundPlayer* player;
    OutputReceiver* pOutputReceiver;
    bool* availableFlag;
};
static_assert(sizeof(DriverCommandPlayerInit) == 0x30, "Player init command size");

/** @brief Command addressed to a driver player with no arguments. */
struct DriverCommandPlayerClearResourceFlag : Command {
    driver::BasicSoundPlayer* player;
};
static_assert(sizeof(DriverCommandPlayerClearResourceFlag) == 0x20, "Clear resource command size");

/** @brief Start, stop, pause or finalize request for a driver player. */
struct DriverCommandPlayer : Command {
    driver::BasicSoundPlayer* player;
    bool flag;
};
static_assert(sizeof(DriverCommandPlayer) == 0x28, "Player command size");

/** @brief Pan mode or pan curve change of a driver player. */
struct DriverCommandPlayerPanParam : Command {
    driver::BasicSoundPlayer* player;
    u8 panMode;
    u8 panCurve;
};
static_assert(sizeof(DriverCommandPlayerPanParam) == 0x28, "Pan command size");

/** @brief Per-frame parameters of a driver player. */
struct DriverCommandPlayerParam : Command {
    driver::BasicSoundPlayer* player;
    f32 volume;
    f32 pitch;
    f32 lpfFreq;
    int biquadFilterType;
    f32 biquadFilterValue;
    u32 outputLineFlag;
    OutputParam tvParam;
};
static_assert(sizeof(DriverCommandPlayerParam) == 0x88, "Player param command size");

/** @brief Send level of an additional bus of a driver player. */
struct DriverCommandPlayerAdditionalSend : Command {
    driver::BasicSoundPlayer* player;
    int bus;
    f32 send;
};
static_assert(sizeof(DriverCommandPlayerAdditionalSend) == 0x28, "Additional send command size");

/** @brief Whether a driver player uses its bus mix volumes. */
struct DriverCommandPlayerBusMixVolumeUsed : Command {
    driver::BasicSoundPlayer* player;
    bool isUsed;
};
static_assert(sizeof(DriverCommandPlayerBusMixVolumeUsed) == 0x28, "Bus mix used command size");

/** @brief Bus mix volumes of a driver player. */
struct DriverCommandPlayerBusMixVolume : Command {
    driver::BasicSoundPlayer* player;
    OutputBusMixVolume volume;
};
static_assert(sizeof(DriverCommandPlayerBusMixVolume) == 0xe0, "Bus mix volume command size");

/** @brief Whether one bus of a driver player uses its bus mix volume. */
struct DriverCommandPlayerBusMixVolumeEnabled : Command {
    driver::BasicSoundPlayer* player;
    int bus;
    bool isEnabled;
};
static_assert(sizeof(DriverCommandPlayerBusMixVolumeEnabled) == 0x28, "Bus mix enable command size");

/** @brief Volume of the buses of a driver player that are in binary through mode. */
struct DriverCommandPlayerBinaryVolume : Command {
    driver::BasicSoundPlayer* player;
    f32 volume;
};
static_assert(sizeof(DriverCommandPlayerBinaryVolume) == 0x28, "Binary volume command size");

/** @brief Whether a driver player uses volume through modes. */
struct DriverCommandPlayerVolumeThroughModeUsed : Command {
    driver::BasicSoundPlayer* player;
    bool isVolumeThroughModeUsed;
};
static_assert(sizeof(DriverCommandPlayerVolumeThroughModeUsed) == 0x28, "Through used command size");

/** @brief Volume through mode of one bus of a driver player. */
struct DriverCommandPlayerVolumeThroughMode : Command {
    driver::BasicSoundPlayer* player;
    int bus;
    u8 mode;
};
static_assert(sizeof(DriverCommandPlayerVolumeThroughMode) == 0x28, "Through mode command size");
}  // namespace nn::atk::detail
