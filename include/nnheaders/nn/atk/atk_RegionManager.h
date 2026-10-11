#pragma once

#include <attributes.h>
#include <nn/atk/atk_DecodeAdpcm.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/types.h>

namespace nn::atk::detail {
class IRegionInfoReadable;
struct StreamDataInfoDetail;

/** @brief Tracks the playback position of a stream sound across its regions. */
class ALIGNED(64) RegionManager {
public:
    static const int ChannelCountMax = 16;

    /** @brief DSP ADPCM decoder state kept on its own cache line. */
    struct ALIGNED(64) AlignedAdpcmContext {
        AdpcmContext context;
    };

    /** @brief Constructs a manager positioned at the start of an empty region. */
    RegionManager() : mCurrentPosition(0), mRegionStartPosition(0), mRegionEndPosition(0) {}

    void Initialize();
    bool InitializeRegion(IRegionInfoReadable* pReader, StreamDataInfoDetail* pInfo);
    bool TryMoveNextRegion(IRegionInfoReadable* pReader, StreamDataInfoDetail* pInfo);
    void SetPosition(long position);
    void AddPosition(long samples);
    bool IsInFirstRegion() const;

    /**
     * @brief Sets the callback that picks the next region.
     * @param callback Region callback, or nullptr to play the regions in order.
     * @param pArg Argument passed to the callback.
     */
    void SetRegionCallback(StreamRegionCallback callback, void* pArg) {
        mRegionCallback = callback;
        mRegionCallbackArg = pArg;
    }

    /** @brief Gets the current sample position. @return Position in samples. */
    s64 GetCurrentPosition() const { return mCurrentPosition; }

    /** @brief Gets the sample position where the current region ends. @return End position. */
    s64 GetCurrentRegionEndPosition() const { return mRegionEndPosition; }

    /**
     * @brief Gets the position the start offset ADPCM contexts were computed for.
     * @return Sample position, or -1 when none were computed.
     */
    s64 GetAdpcmContextForStartOffsetFrame() const { return mAdpcmContextForStartOffsetFrame; }

    /**
     * @brief Records the position the start offset ADPCM contexts were computed for.
     * @param frame Sample position.
     */
    void SetAdpcmContextForStartOffsetFrame(s64 frame) { mAdpcmContextForStartOffsetFrame = frame; }

    /**
     * @brief Gets the ADPCM decoder state at the start offset.
     * @param channel Channel index, in [0, ChannelCountMax).
     * @return Decoder state of the channel.
     */
    AdpcmContext& GetAdpcmContextForStartOffset(int channel) {
        return mAdpcmContextForStartOffset[channel].context;
    }

private:
    // Region selection state preceding the position awaits reconstruction.
    u8 _0[8];
    StreamRegionCallback mRegionCallback;
    void* mRegionCallbackArg;
    u8 _18[0x10];
    s64 mCurrentPosition;
    s64 mRegionStartPosition;
    s64 mRegionEndPosition;
    u8 _40[8];
    s64 mAdpcmContextForStartOffsetFrame;
    AlignedAdpcmContext mAdpcmContextForStartOffset[ChannelCountMax];
};
static_assert(sizeof(RegionManager) == 0x480, "RegionManager size");
}  // namespace nn::atk::detail
