#pragma once

#include <nn/atk/atk_OutputParam.h>
#include <nn/os.h>

namespace nn::atk {
class OutputReceiver;
}  // namespace nn::atk
namespace nn::atk::detail {
class OutputAdditionalParam;
}  // namespace nn::atk::detail

namespace nn::atk::detail::driver {
/**
 * @brief Driver-side base of every sound player: activity flags and the base output parameters.
 */
class BasicSoundPlayer {
public:
    BasicSoundPlayer();
    /** @brief Releases the player's event. */
    virtual ~BasicSoundPlayer() { os::FinalizeEvent(&mEvent); }
    virtual void Initialize(OutputReceiver* pReceiver);
    virtual void Finalize();
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Pause(bool isPause) = 0;
    /**
     * @brief Marks the player as holding live data.
     * @param isActive Whether the player is active.
     */
    virtual void SetActiveFlag(bool isActive) { mActiveFlag = isActive; }

    /** @brief Tests whether the player holds live data. @return Whether the player is active. */
    bool IsActive() const { return mActiveFlag; }
    /** @brief Tests whether playback has started. @return Whether the player is started. */
    bool IsStarted() const { return mStartedFlag; }
    /** @brief Tests whether the player is paused. @return Whether the player is paused. */
    bool IsPause() const { return mPauseFlag; }
    /** @brief Clears the player's event before a new sound uses the player. */
    void ClearEvent() { os::ClearEvent(&mEvent); }
    /** @brief Tests whether playback reached its end. @return Whether the player is finished. */
    bool IsPlayFinished() const { return mFinishFlag; }
    /**
     * @brief Tests whether the player was finalized because a resource could not be allocated.
     * @return Whether the player gave up for lack of resources.
     */
    bool IsFinalizedForCannotAllocateResource() const { return mIsFinalizedForCannotAllocateResource; }
    /**
     * @brief Sets the additional parameters of the main output.
     * @param pParam Additional parameters, or nullptr.
     */
    void SetTvAdditionalParam(OutputAdditionalParam* pParam) { mTvAdditionalParam = pParam; }

protected:
    os::EventType mEvent;
    OutputReceiver* mOutputReceiver;
    bool mActiveFlag;
    bool mStartedFlag;
    bool mPauseFlag;
    bool mFinishFlag;
    bool mIsFinalizedForCannotAllocateResource;
    u8 _3d[3];
    float mBaseVolume;
    float mBasePitch;
    float mBaseLpfFreq;
    float mBaseBiquadValue;
    u8 mBaseBiquadType;
    u8 _51[3];
    int mPanMode;
    int mPanCurve;
    int mBaseOutputLine;
    OutputParam mTvParam;
    OutputAdditionalParam* mTvAdditionalParam;
    u8 _b8[8];
};
static_assert(sizeof(BasicSoundPlayer) == 0xc0, "BasicSoundPlayer size");
}  // namespace nn::atk::detail::driver
