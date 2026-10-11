#pragma once
#include <nn/atk/atk_StreamSoundPrefetchFile.h>

namespace nn::atk::detail {
class StreamSoundPrefetchFileReader {
public:
    struct PrefetchDataInfo { u32 startFrame, sampleBytes; const void* samples; };
    StreamSoundPrefetchFileReader();
    virtual ~StreamSoundPrefetchFileReader();
    void Initialize(const void* file);
    void Finalize();
    bool IsValidFileHeader(const void* file) const;
    u32 GetRegionDataOffset() const;
    u16 GetRegionInfoBytes() const;
    bool IsIncludeRegionInfo() const;
    bool IsCrc32CheckAvailable() const;
    bool IsRegionIndexCheckAvailable() const;
    bool ReadStreamSoundInfo(StreamSoundFile::StreamSoundInfo* info) const;
    bool ReadDspAdpcmChannelInfo(DspAdpcmParam* param, DspAdpcmLoopParam* loop, int channel) const;
    bool ReadPrefetchDataInfo(PrefetchDataInfo* info, int index) const;
    virtual bool ReadRegionInfo(StreamSoundFile::RegionInfo* info, u32 index) const;

    /** @brief Gets the number of channels in the file. @return Channel count. */
    u32 GetChannelCount() const { return mInfo->GetChannelInfoTable()->count; }
private:
    const StreamSoundPrefetchFile::FileHeader* mHeader;
    const StreamSoundFile::InfoBlockBody* mInfo;
    const StreamSoundPrefetchFile::PrefetchDataBlockBody* mPrefetch;
    u32 mRegionOffset;
    u16 mRegionInfoBytes;
};
static_assert(sizeof(StreamSoundPrefetchFileReader) == 0x28, "StreamSoundPrefetchFileReader size");
}
