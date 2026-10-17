#pragma once
#include <nn/atk/atk_StreamSoundFile.h>

namespace nn::atk::detail {
class StreamSoundFileReader {
public:
    StreamSoundFileReader();
    struct TrackInfo { u8 volume, pan, surroundPan, _03, channelCount, channels[2]; };
    void Initialize(const void* file);
    static bool IsValidFileHeader(const void* file);
    void Finalize();
    bool IsTrackInfoAvailable() const;
    bool IsOriginalLoopAvailable() const;
    static bool IsOriginalLoopAvailableImpl(const StreamSoundFile::FileHeader* header);
    bool IsCrc32CheckAvailable() const;
    bool IsRegionIndexCheckAvailable() const;
    bool ReadStreamSoundInfo(StreamSoundFile::StreamSoundInfo* info) const;
    bool ReadStreamTrackInfo(TrackInfo* info, int track) const;
    bool ReadDspAdpcmChannelInfo(DspAdpcmParam* param, DspAdpcmLoopParam* loop, int channel) const;

    /** @brief Gets the number of channels the stream stores. @return Channel count. */
    u32 GetChannelCount() const { return mInfo->GetChannelInfoTable()->count; }

    /** @brief Gets the number of tracks the stream stores. @return Track count. */
    u32 GetTrackCount() const { return mInfo->GetTrackInfoTable()->count; }

    /**
     * @brief Gets the file offset of the first sample.
     * @return Offset of the sample data, or 0 when no file is attached.
     */
    u32 GetSampleDataOffset() const {
        if (mHeader == nullptr) {
            return 0;
        }

        return mHeader->GetDataBlockOffset() + mInfo->GetStreamSoundInfo()->sampleData.offset +
               sizeof(BinaryBlockHeader);
    }

    /**
     * @brief Gets the file offset of the seek block.
     * @return Offset of the seek block, or 0 when the stream has none.
     */
    u32 GetSeekBlockOffset() const {
        if (mHeader == nullptr || !mHeader->HasSeekBlock()) {
            return 0;
        }

        return mHeader->GetSeekBlockOffset();
    }

    /**
     * @brief Gets the file offset of the first region entry.
     * @return Offset of the region data, or 0 when the stream has no regions.
     */
    u32 GetRegionDataOffset() const {
        if (mHeader == nullptr || !mHeader->HasRegionBlock()) {
            return 0;
        }

        return mHeader->GetRegionBlockOffset() + mInfo->GetStreamSoundInfo()->regionData.offset +
               sizeof(BinaryBlockHeader);
    }

    /** @brief Gets the size of one region entry. @return Region entry size in bytes. */
    u16 GetRegionInfoBytes() const { return mInfo->GetStreamSoundInfo()->regionInfoSize; }

private:
    const StreamSoundFile::FileHeader* mHeader;
    const StreamSoundFile::InfoBlockBody* mInfo;
};
static_assert(sizeof(StreamSoundFileReader) == 0x10, "StreamSoundFileReader size");
}
