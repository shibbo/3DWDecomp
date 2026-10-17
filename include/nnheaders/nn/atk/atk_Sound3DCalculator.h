#pragma once

#include <nn/types.h>
#include <nn/util/util_Constants.h>
#include <nn/util/util_MathTypes.h>

namespace nn::atk {
class Sound3DListener;
class Sound3DManager;
struct Sound3DParam;

class Sound3DCalculator {
  public:
    struct CalculatePanParam {
        /** @brief Initializes the stereo and surround speaker angles in radians. */
        CalculatePanParam()
            : speakerAngleStereo(util::FloatPi * 0.25f), frontSpeakerAngleSurround(util::FloatPi / 6.0f),
              rearSpeakerAngleSurround(util::FloatPi * 2.0f / 3.0f), initPan(0) {}

        f32 speakerAngleStereo;
        f32 frontSpeakerAngleSurround;
        f32 rearSpeakerAngleSurround;
        f32 initPan;
    };

    static void CalculateVolumeAndPriority(f32* pVolume, s32* pPriority, const Sound3DManager& rManager,
                                           const Sound3DListener& rListener, const Sound3DParam& rParam);
    static void CalculateVolumeAndPriority(f32* pVolume, s32* pPriority, const Sound3DManager& rManager,
                                           const Sound3DListener& rListener, const Sound3DParam& rParam,
                                           float distance);
    static void CalculateBiquadFilterValue(f32* pValue, const Sound3DManager& rManager,
                                           const Sound3DListener& rListener, const Sound3DParam& rParam,
                                           float distance);
    static void CalculatePitch(f32* pPitch, const Sound3DManager& rManager, const Sound3DListener& rListener,
                               const Sound3DParam& rParam, const util::Vector3fType& rOffset, float distance);
    static void CalculatePan(f32* pPan, f32* pSurroundPan, const Sound3DManager& rManager,
                             const Sound3DListener& rListener, const Sound3DParam& rParam,
                             const CalculatePanParam& rPanParam);
    static void CalculatePitch(f32* pPitch, const Sound3DManager& rManager, const Sound3DListener& rListener,
                               const Sound3DParam& rParam);
    static void CalculateBiquadFilterValue(f32* pValue, const Sound3DManager& rManager,
                                           const Sound3DListener& rListener, const Sound3DParam& rParam);
};
} // namespace nn::atk
