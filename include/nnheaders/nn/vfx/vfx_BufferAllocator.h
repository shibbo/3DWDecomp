/**
 * @file vfx_BufferAllocator.h
 * @brief GPU buffer allocator used by the VFX runtime (minimal declaration).
 */

#pragma once

#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn {
namespace vfx {
namespace detail {

/**
 * Sub-allocates CPU/GPU visible memory out of a single gfx buffer.
 * Only the members touched by the decompiled code are named.
 */
class BufferAllocator {
public:
    /** Parameters for Initialize(). */
    struct InitializeArg {
        InitializeArg()
            : pDevice(nullptr), memoryPoolProperty(0), gpuAccessFlag(0), pWorkMemory(nullptr),
              workMemorySize(0), pPoolMemory(nullptr), poolMemorySize(0) {}

        nn::gfx::Device* pDevice;
        int memoryPoolProperty;
        int gpuAccessFlag;
        void* pWorkMemory;
        size_t workMemorySize;
        void* pPoolMemory;
        size_t poolMemorySize;
    };

    BufferAllocator() {
        m_1a8 = 0;
        m_pBufferTop = nullptr;
        m_1b0 = 0;
        m_GpuAddress.ToData()->impl = 0;
        m_GpuAddress.ToData()->value = 0;
        m_1d0 = 0;
    }

    virtual ~BufferAllocator() {}

    bool Initialize(InitializeArg& rArg);
    void Finalize(nn::gfx::Device* pDevice);
    void FlushFreeList();

    void* Alloc(size_t size, size_t alignment);
    void Free(void* pMemory, bool isImmediate);

    /** @return the alignment every allocation is rounded up to */
    size_t GetAlignment() const { return m_Alignment; }

    /**
     * Computes the GPU address of a CPU pointer inside the managed buffer.
     * @param pOutAddress receives the GPU address
     * @param pCpuAddress CPU address inside the managed buffer
     */
    void GetGpuAddress(nn::gfx::GpuAddress* pOutAddress, const void* pCpuAddress) const {
        *pOutAddress = m_GpuAddress;
        pOutAddress->Offset(static_cast<const u8*>(pCpuAddress) -
                            static_cast<const u8*>(m_pBufferTop));
    }

private:
    nn::gfx::Buffer m_Buffer;
    nn::gfx::MemoryPool m_MemoryPool;
    void* m_pBufferTop;
    u8 _178[0x180 - 0x178];
    nn::util::IntrusiveListNode m_UsedList;
    nn::util::IntrusiveListNode m_FreeList;
    u8 _1a0[0x1A8 - 0x1A0];
    u64 m_1a8;
    u32 m_1b0;
    u8 _1b4[0x1C8 - 0x1B4];
    size_t m_Alignment;
    u64 m_1d0;
    nn::gfx::GpuAddress m_GpuAddress;
};

static_assert(sizeof(BufferAllocator) == 0x1E8);

}  // namespace detail
}  // namespace vfx
}  // namespace nn
