#pragma once
#include <nn/atk/atk_Global.h>
#include <nn/types.h>

namespace nn::atk {
class SoundArchive;
enum SampleFormat : int;
namespace detail {
class SoundArchiveLoader;
struct LoadItemInfo;
namespace Util {
/** @brief Outcome of resolving the wave archive a bank plays from. */
enum WaveArchiveLoadStatus {
    WaveArchiveLoadStatus_Error = -1,
    WaveArchiveLoadStatus_Ok,
    WaveArchiveLoadStatus_Noneed,
    WaveArchiveLoadStatus_NotYet,
};
/** @brief Shape of the curve that maps a pan position to channel gains. */
enum PanCurve {
    PanCurve_Sqrt,
    PanCurve_SinCos,
    PanCurve_Linear,
};

/** @brief Pan curve selection consumed by CalcPanRatio() and CalcSurroundPanRatio(). */
struct PanInfo {
    /** @brief Selects the square-root curve with no modifiers. */
    PanInfo()
        : curve(PanCurve_Sqrt), centerZeroFlag(false), zeroClampFlag(false), _6(false),
          isCompatibleMode(false) {}

    PanCurve curve;
    bool centerZeroFlag;
    bool zeroClampFlag;
    bool _6;
    bool isCompatibleMode;
};

float CalcPanRatio(float pan, const PanInfo& rInfo, OutputMode mode);
float CalcSurroundPanRatio(float span, const PanInfo& rInfo);
u16 CalcLpfFreq(float scale);
u32 CalcRandom();
const void* GetWaveFileOfWaveSound(const void* pWaveSoundFile, u32 index, const SoundArchive& rArchive,
                                   const SoundArchiveLoader& rLoader);
WaveArchiveLoadStatus GetWaveArchiveOfBank(LoadItemInfo& rWarcInfo, bool& rIsLoadIndividual,
                                           const void* pBankFile, const SoundArchive& rArchive,
                                           const SoundArchiveLoader& rLoader);
size_t GetByteBySample(size_t samples, SampleFormat format);
} // namespace Util
} // namespace detail
} // namespace nn::atk
