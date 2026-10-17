#include <nn/atk/atk_Sound3DEngine.h>
#include <nn/atk/atk_Sound3DManager.h>
#include <nn/atk/atk_Sound3DParam.h>

namespace nn::atk {
/** @brief Constructs the default spatial engine and speaker arrangement. */
Sound3DEngine::Sound3DEngine() = default;

namespace {
/** @brief Selects the greater spatial parameter, retaining rhs on equality. @tparam T Scalar parameter type.
 * @param lhs Current value. @param rhs Candidate value. @return Greater value, or rhs when unordered. */
template <typename T> inline T MaxParam(T lhs, T rhs) { return rhs < lhs ? lhs : rhs; }
} // namespace

/**
 * @brief Combines spatial settings from the manager's registered listeners.
 * @param pOutValue Ambient parameters to update according to updateFlag.
 * @param pManager Spatial manager supplying listeners and attenuation settings; non-null.
 * @param pParam Sound position, velocity, and attenuation configuration; non-null.
 * @param soundId Unused by the default engine; identifies the sound for custom engines.
 * @param updateFlag Bit mask selecting volume, priority, pan, filter, and Doppler updates.
 */
void Sound3DEngine::UpdateAmbientParam(SoundAmbientParam* pOutValue, const Sound3DManager* pManager,
                                       const Sound3DParam* pParam, u32 soundId, u32 updateFlag) {
    if ((updateFlag & UpdatePriority) != 0) {
        pOutValue->priority = -pManager->GetMaxPriorityReduction();
    }
    if ((updateFlag & UpdateFilter) != 0) {
        pOutValue->biquadFilterValue = 0;
    }
    if ((updateFlag & UpdateVolume) != 0) {
        pOutValue->tvParam.volume = 0;
    }
    const auto& rListeners = pManager->GetListenerList();
    if (rListeners.size() != 1) {
        updateFlag &= ~UpdatePitch;
    }
    int tvListenerCount = 0;
    for (const auto& rListener : rListeners) {
        tvListenerCount += (rListener.GetOutputTypeFlag() & Sound3DListener::ListenerOutputType_Tv) != 0;
    }
    for (const auto& rListener : rListeners) {
        u32 outputFlags = rListener.GetOutputTypeFlag();
        util::Vector3fType offset;
        float distance = 0;
        if ((updateFlag & (UpdateVolume | UpdatePriority | UpdateFilter | UpdatePitch)) != 0) {
            offset._v = vsubq_f32(pParam->position._v, rListener.GetPosition()._v);
            float32x4_t square = vmulq_f32(offset._v, offset._v);
            float32x2_t sum = vadd_f32(vget_high_f32(square), vget_low_f32(square));
            sum = vpadd_f32(sum, sum);
            distance = vgetq_lane_f32(vsqrtq_f32(vcombine_f32(sum, sum)), 0);
        }
        if ((updateFlag & (UpdateVolume | UpdatePriority)) != 0) {
            float volume;
            int priority;
            Sound3DCalculator::CalculateVolumeAndPriority(&volume, &priority, *pManager, rListener, *pParam,
                                                          distance);
            if ((updateFlag & UpdateVolume) != 0) {
                float currentVolume = pOutValue->tvParam.volume;
                for (int output = 0; output < OutputDevice_Count; ++output) {
                    if ((outputFlags & (1u << output)) != 0) {
                        auto& param = pOutValue->tvParam;
                        param.volume = MaxParam(currentVolume, volume);
                    }
                }
            }
            if ((updateFlag & UpdatePriority) != 0) {
                pOutValue->priority = MaxParam(pOutValue->priority, priority);
            }
        }
        if ((updateFlag & (UpdatePan | UpdateSurroundPan)) != 0) {
            if ((outputFlags & Sound3DListener::ListenerOutputType_Tv) != 0) {
                if (tvListenerCount == 1) {

                    float pan, surroundPan;
                    Sound3DCalculator::CalculatePan(&pan, &surroundPan, *pManager, rListener, *pParam,
                                                    m_CalcPanParam);
                    if ((updateFlag & UpdatePan) != 0) {
                        pOutValue->tvParam.pan = pan;
                    }
                    if ((updateFlag & UpdateSurroundPan) != 0) {
                        pOutValue->tvParam.span = surroundPan;
                    }
                }
            }
        }
        if ((updateFlag & UpdatePitch) != 0) {
            float pitch;
            Sound3DCalculator::CalculatePitch(&pitch, *pManager, rListener, *pParam, offset, distance);
            pOutValue->pitch = pitch;
        }
        if ((updateFlag & UpdateFilter) != 0) {
            float value;
            Sound3DCalculator::CalculateBiquadFilterValue(&value, *pManager, rListener, *pParam, distance);
            pOutValue->biquadFilterType = pManager->GetBiquadFilterType();
            pOutValue->biquadFilterValue = MaxParam(pOutValue->biquadFilterValue, value);
        }
    }
}

/**
 * @brief Selects spatial updates from the sound's enabled effects and Doppler factor.
 * @param pManager Manager supplying spatial listeners and settings; non-null.
 * @param pParam Spatial argument describing the sound; non-null.
 * @param soundId Identifier forwarded to the overridable engine implementation.
 * @param pAmbientParam Ambient parameters receiving selected updates; non-null.
 */
void Sound3DEngine::detail_UpdateAmbientParam(const Sound3DManager* pManager, const Sound3DParam* pParam,
                                              u32 soundId, SoundAmbientParam* pAmbientParam) {
    u32 update = pParam->flags & 31;
    if (pParam->dopplerFactor != 0) {
        update |= UpdatePitch;
    }
    UpdateAmbientParam(pAmbientParam, pManager, pParam, soundId, update);
}

/**
 * @brief Queries spatial priority using the engine's start-priority update path.
 * @param pManager Manager supplying spatial listeners and settings; non-null.
 * @param pParam Spatial argument describing the sound; non-null.
 * @param soundId Identifier forwarded to the overridable engine implementation.
 * @return Calculated ambient priority adjustment.
 */
s32 Sound3DEngine::GetAmbientPriority(const Sound3DManager* pManager, const Sound3DParam* pParam,
                                      u32 soundId) {
    u32 update = (pParam->flags & UpdatePriority) | UpdateStartPriority;
    SoundAmbientParam param;
    UpdateAmbientParam(&param, pManager, pParam, soundId, update);
    return param.priority;
}
} // namespace nn::atk
