#pragma once

#include <nn/atk/atk_Sound3DCalculator.h>
#include <nn/types.h>

namespace nn::atk {
class Sound3DManager;
struct Sound3DParam;

struct OutputAmbientParam {
    f32 volume;
    f32 pan;
    f32 span;
    f32 additionalOutputParams[3];
};

struct SoundAmbientParam {
    /** @brief Initializes neutral ambient parameters and unrestricted output selection. */
    SoundAmbientParam()
        : volume(1), pitch(1), lpf(0), biquadFilterValue(0), biquadFilterType(-1), priority(0), userData(0),
          outputLineFlag(0xffffffff), tvParam{1, 0, 0, {0, 0, 0}} {}
    f32 volume;
    f32 pitch;
    f32 lpf;
    f32 biquadFilterValue;
    s32 biquadFilterType;
    s32 priority;
    u32 userData;
    u32 outputLineFlag;
    OutputAmbientParam tvParam;
};

namespace detail {
class ISound3DEngine {
  public:
    /** @brief Destroys the spatial engine interface. */
    virtual ~ISound3DEngine() = default;

    virtual void detail_UpdateAmbientParam(const Sound3DManager* pManager, const Sound3DParam* pParam,
                                           u32 soundId, SoundAmbientParam* pAmbientParam) = 0;
    virtual s32 GetAmbientPriority(const Sound3DManager* pManager, const Sound3DParam* pParam,
                                   u32 soundId) = 0;
};
} // namespace detail

class Sound3DEngine : public detail::ISound3DEngine {
  public:
    Sound3DEngine();
    /** @brief Destroys the default spatial engine. */
    ~Sound3DEngine() override = default;

    /**
     * @brief Replaces the speaker arrangement used for spatial panning.
     * @param rParam Stereo
     * and surround speaker angles in radians, and the initial pan offset.
     */
    void SetCalculatePanParam(const Sound3DCalculator::CalculatePanParam& rParam) { m_CalcPanParam = rParam; }

    /** @brief Gets the configured speaker arrangement. @return Parameters owned by this engine. */
    const Sound3DCalculator::CalculatePanParam& GetCalculatePanParam() const { return m_CalcPanParam; }

  protected:
    static const u32 UpdateVolume = 1 << 0;
    static const u32 UpdatePriority = 1 << 1;
    static const u32 UpdatePan = 1 << 2;
    static const u32 UpdateSurroundPan = 1 << 3;
    static const u32 UpdateFilter = 1 << 4;
    static const u32 UpdatePitch = 1 << 5;
    static const u32 UpdateStartPriority = 1 << 24;

    virtual void UpdateAmbientParam(SoundAmbientParam* pOutValue, const Sound3DManager* pManager,
                                    const Sound3DParam* pParam, u32 soundId, u32 updateFlag);

  private:
    s32 GetAmbientPriority(const Sound3DManager* pManager, const Sound3DParam* pParam, u32 soundId) override;
    void detail_UpdateAmbientParam(const Sound3DManager* pManager, const Sound3DParam* pParam, u32 soundId,
                                   SoundAmbientParam* pAmbientParam) override;

    Sound3DCalculator::CalculatePanParam m_CalcPanParam;
};
static_assert(sizeof(Sound3DEngine) == 0x18);
} // namespace nn::atk
