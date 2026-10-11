#pragma once

#include <nn/types.h>

namespace nn::atk {
struct AdpcmContext;

/** @brief One block of sample data queued on a voice. */
struct WaveBuffer {
    /** @brief Progress of the buffer through the renderer. */
    enum Status {
        Status_Free,
        Status_Wait,
        Status_Play,
        Status_Done,
    };

    /** @brief Creates an empty, unqueued buffer. */
    WaveBuffer() { Initialize(); }

    /** @brief Detaches the sample data and marks the buffer free. */
    void Initialize() {
        bufferAddress = nullptr;
        bufferSize = 0;
        sampleLength = 0;
        sampleOffset = 0;
        pAdpcmContext = nullptr;
        pUserParam = nullptr;
        loopFlag = false;
        status = Status_Free;
        next = nullptr;
    }

    const void* bufferAddress;
    size_t bufferSize;
    size_t sampleLength;
    size_t sampleOffset;
    AdpcmContext* pAdpcmContext;
    void* pUserParam;
    bool loopFlag;
    Status status;
    WaveBuffer* next;
};
static_assert(sizeof(WaveBuffer) == 0x40, "WaveBuffer size");
}  // namespace nn::atk
