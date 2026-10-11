#pragma once
#include <nn/atk/atk_StreamSoundFileReader.h>

namespace nn::atk::detail {
namespace fnd {
class FileStream;
}

class IRegionInfoReadable {
public:
    virtual ~IRegionInfoReadable();
    /**
     * @brief Read a stream region's playback information.
     * @param pInfo Destination for the region information; must not be null.
     * @param index Zero-based region index in the stream resource.
     * @return Whether the region information was read successfully.
     */
    virtual bool ReadRegionInfo(StreamSoundFile::RegionInfo* pInfo, u32 index) const = 0;
};

class StreamSoundFileLoader : public IRegionInfoReadable {
public:
    /**
     * @brief Creates a loader reading from an opened stream.
     * @param pFileStream Opened stream of the stream sound file; not owned.
     */
    explicit StreamSoundFileLoader(fnd::FileStream* pFileStream)
        : mFileStream(pFileStream), mSeekBlockOffset(0), mRegionBlockOffset(0),
          mRegionInfoBytes(0) {}
    ~StreamSoundFileLoader() override;
    bool LoadFileHeader(StreamSoundFileReader* pReader, void* pBuffer, size_t bufferSize);
    bool ReadSeekBlockData(u16* pYn1, u16* pYn2, int blockIndex, int channelCount);
    bool ReadRegionInfo(StreamSoundFile::RegionInfo* pInfo, u32 index) const override;

    /**
     * @brief Attaches another stream and forgets the block offsets of the previous file.
     * @param pFileStream Opened stream of the stream sound file, or nullptr to detach; not owned.
     */
    void Reset(fnd::FileStream* pFileStream) {
        mFileStream = pFileStream;
        mSeekBlockOffset = 0;
        mRegionBlockOffset = 0;
        mRegionInfoBytes = 0;
    }

private:
    fnd::FileStream* mFileStream;
    u32 mSeekBlockOffset;
    u32 mRegionBlockOffset;
    u16 mRegionInfoBytes;
};
}
