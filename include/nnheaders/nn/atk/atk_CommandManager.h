#pragma once
#include <nn/os.h>
#include <atomic>

namespace nn::atk::detail {
struct Command {
    Command* next;
    u32 type;
    u32 sequence;
    size_t bufferEnd;
};
class CommandBuffer {
public:
    CommandBuffer();
    ~CommandBuffer();
    void Initialize(void* buffer, size_t size);
    void Finalize();
    Command* AllocMemory(size_t size);
    void FreeMemory(Command* command);
    size_t GetCommandBufferSize() const;
    size_t GetAllocatableCommandSize() const;
    size_t GetAllocatedCommandBufferSize() const;
private:
    u32* mBuffer;
    size_t mCapacity;
    size_t mWritePosition;
    size_t mReadPosition;
    bool mFull;
};
class CommandManager {
public:
    using ProcessCallback = void (*)(Command*);
    CommandManager();
    ~CommandManager();
    void Initialize(void* buffer, size_t bufferSize, size_t commandBufferSize,
                    int commandCount, ProcessCallback callback);
    void Finalize();
    static size_t GetRequiredMemSize(size_t commandBufferSize, int commandCount);
    Command* AllocMemory(size_t size, bool option);
    Command* TryAllocMemory(size_t size);
    void RecvCommandReply();
    u32 FlushCommand(bool block, bool option);
    u32 FlushCommand(bool block);
    void WaitCommandReply(u32 sequence);
    Command* RecvCommandReplySync();
    u32 PushCommand(Command* command);
    void FinalizeCommandList(Command* command);
    bool IsFinishCommand(u32 sequence) const;
    /** @brief Checks whether driver command storage is initialized. @return True when commands may be submitted. */
    bool IsInitialized() const { return mInitialized; }
    /** @brief Reads the number of pushed commands not yet processed. @return Pending command count. */
    int GetPendingCommandCount() const { return mPendingCount.load(); }
    size_t GetCommandBufferSize() const;
    size_t GetAllocatableCommandSize() const;
    size_t GetAllocatedCommandBufferSize() const;
    int GetAllocatedCommandCount() const;
    bool ProcessCommand();
protected:
    void SetRequestCallback(void (*callback)()) { mRequestCallback = callback; }
private:
    struct Queue {
        Queue() : initialized(false) {}
        os::MessageQueueType queue;
        u64* buffer;
        bool initialized;
        void Initialize(int count) {
            if (!initialized) {
                os::InitializeMessageQueue(&queue, buffer, count);
                initialized = true;
            }
        }
        void Finalize() {
            if (initialized) {
                os::FinalizeMessageQueue(&queue);
                initialized = false;
            }
        }
    };
    bool mInitialized;
    ProcessCallback mProcessCallback;
    void (*mRequestCallback)();
    Queue mCommands;
    Queue mReplies;
    Command* mHead;
    Command* mTail;
    std::atomic<int> mPendingCount;
    u32 mSequence;
    u32 mFinishedSequence;
    CommandBuffer mBuffer;
    int mAllocatedCount;
};
static_assert(sizeof(Command) == 0x18, "Command header size");
static_assert(sizeof(CommandBuffer) == 0x28, "Command buffer size");
static_assert(sizeof(CommandManager) == 0x118, "Command manager size");
}
