#pragma once
#include <nn/atk/atk_CommandManager.h>
#include <nn/atk/atk_DecodeAdpcm.h>

namespace nn::atk::detail {
class StreamSoundPlayer;

struct ReleaseHeapMemoryCommand : Command {
    const void* pMemory;
    size_t size;
};

/** @brief Reply of the task thread after reading a stream sound's header (command 53). */
struct DriverCommandStreamSoundLoadHeader : Command {
    static const int ChannelCountMax = 16;

    StreamSoundPlayer* player;
    audio::AdpcmParameter* adpcmParam[ChannelCountMax];
    bool result;
    u16 assignNumber;
};
static_assert(sizeof(DriverCommandStreamSoundLoadHeader) == 0xa8, "Stream header reply size");

/** @brief Reply of the task thread after loading stream sound data (command 54). */
struct DriverCommandStreamSoundLoadData : Command {
    static const int ChannelCountMax = 16;

    StreamSoundPlayer* player;
    u32 bufferBlockIndex;
    size_t loadSamples;
    s64 startSamplePosition;
    size_t offsetSamples;
    size_t loadSize;
    bool isAdpcmContextAvailable;
    AdpcmContext adpcmContext[ChannelCountMax];
    int loopCount;
    bool isLastBlock;
    bool isStartOffsetOverRegion;
    u8 _b2[6];
    bool result;
    u16 assignNumber;
};
static_assert(sizeof(DriverCommandStreamSoundLoadData) == 0xc0, "Stream data reply size");

/** @brief Request to stop a stream sound whose data can no longer be loaded (command 55). */
struct DriverCommandStreamSoundForceFinish : Command {
    StreamSoundPlayer* player;
};
static_assert(sizeof(DriverCommandStreamSoundForceFinish) == 0x20, "Stream finish command size");
class DriverCommand : public CommandManager {
public:
    DriverCommand();
    static DriverCommand& GetInstance();
    static DriverCommand& GetInstanceForTaskThread();
    size_t GetRequiredMemSize(size_t commandBufferSize, int commandCount);
    void Initialize(void* buffer, size_t bufferSize, size_t commandBufferSize, int commandCount);
    static void ProcessCommandList(Command* command);
    static void RequestProcessCommand();
};
}
