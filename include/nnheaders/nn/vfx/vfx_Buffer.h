/**
 * @file vfx_Buffer.h
 * @brief GPU buffer with its own memory pool, used by the VFX runtime.
 */

#pragma once

#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>

namespace nn {
namespace vfx {
class Heap;

namespace detail {

/**
 * A gfx buffer backed by a memory pool allocated from a VFX heap, with a linear cutter.
 */
class Buffer {
public:
    /** @brief Creates an uninitialized buffer with an empty allocation cursor. */
    Buffer() : m_IsInitialized(false) {
        m_GpuAddress.ToData()->value = 0;
        m_GpuAddress.ToData()->impl = 0;
        m_Offset = 0;
        m_pMappedAddress = nullptr;
    }

    bool Initialize(gfx::Device* pDevice, Heap* pHeap, int gpuAccessFlag, size_t size);
    void Finalize(gfx::Device* pDevice, Heap* pHeap);

    /** @brief Stores the GPU address of the start of the initialized buffer. */
    void UpdateGpuAddress() { m_Buffer.GetGpuAddress(&m_GpuAddress); }

    /** @brief Maps the initialized buffer and resets its allocation cursor. */
    void Begin() {
        m_Offset = 0;
        m_pMappedAddress = m_Buffer.Map();
    }

    /** @brief Flushes the whole mapped buffer and unmaps it. */
    void End() {
        m_Buffer.FlushMappedRange(0, m_BufferSize);
        m_Buffer.Unmap();
    }

    /**
     * @brief Allocates an aligned region from the mapped buffer.
     * @param size Number of bytes requested; the buffer must have been mapped with Begin().
     * @return The start of the region, or nullptr when the buffer is exhausted.
     */
    void* Cut(size_t size) {
        size_t offset = m_Offset;
        size_t end = offset + ((size + m_Alignment - 1) & -m_Alignment);

        if (end > m_BufferSize) {
            return nullptr;
        }

        m_Offset = end;
        return static_cast<u8*>(m_pMappedAddress) + offset;
    }

    gfx::Buffer m_Buffer;
    size_t m_BufferSize;
    bool m_IsInitialized;
    gfx::MemoryPool m_MemoryPool;
    void* m_pPoolMemory;
    size_t m_PoolMemorySize;
    size_t m_Alignment;
    gfx::GpuAddress m_GpuAddress;
    size_t m_Offset;
    void* m_pMappedAddress;
};

static_assert(sizeof(Buffer) == 0x1b0);

}  // namespace detail
}  // namespace vfx
}  // namespace nn
