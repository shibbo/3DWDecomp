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
    Buffer() : m_IsInitialized(false) {
        m_GpuAddress.ToData()->value = 0;
        m_GpuAddress.ToData()->impl = 0;
        m_Offset = 0;
        m_pMappedAddress = nullptr;
    }

    bool Initialize(gfx::Device* pDevice, Heap* pHeap, int gpuAccessFlag, size_t size);
    void Finalize(gfx::Device* pDevice, Heap* pHeap);

    /** Stores the GPU address of the start of the buffer. */
    void UpdateGpuAddress() { m_Buffer.GetGpuAddress(&m_GpuAddress); }

    /** Maps the buffer and resets the cutter. */
    void Begin() {
        m_Offset = 0;
        m_pMappedAddress = m_Buffer.Map();
    }

    /** Flushes the whole buffer and unmaps it. */
    void End() {
        m_Buffer.FlushMappedRange(0, m_BufferSize);
        m_Buffer.Unmap();
    }

    /**
     * Cuts an aligned piece off the mapped buffer.
     * @param size number of bytes requested
     * @return the start of the piece, or nullptr when the buffer is exhausted
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
    u8 _178[0x188 - 0x178];
    size_t m_Alignment;
    gfx::GpuAddress m_GpuAddress;
    size_t m_Offset;
    void* m_pMappedAddress;
};

static_assert(sizeof(Buffer) == 0x1b0);

}  // namespace detail
}  // namespace vfx
}  // namespace nn
