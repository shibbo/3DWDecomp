#pragma once

#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_Device.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/util/util_BitUtil.h>

#include <atomic>

namespace nn {
namespace font {

typedef void* (*AllocateFunction)(size_t size, size_t alignment, void* pUserData);
typedef void (*FreeFunction)(void* ptr, void* pUserData);

class GpuBuffer {
public:
    /** @brief Creates an empty GPU buffer with no active mapping. */
    GpuBuffer()
        : m_Flags(0), m_pBuffers(nullptr), m_pGpuAddresses(nullptr), m_BufferSize(0),
          m_BufferAlignment(1), m_BufferCount(0), m_MappedBufferIndex(-1),
          m_GpuAccessBufferIndex(0), m_pMappedPointer(nullptr) {}

    enum Flag {
        Flag_AtomicAllocation = 1 << 0,
        Flag_Unallocated = 1 << 1,
    };

    struct InitializeArg {
        /** @brief Defaults to a single constant buffer with optional allocation modes disabled. */
        InitializeArg()
            : gpuAccessFlag(nn::gfx::GpuAccess_ConstantBuffer), bufferSize(0), bufferCount(1),
              pMemoryPool(nullptr), memoryPoolOffset(0), pAllocateFunction(nullptr),
              pUserData(nullptr), isAtomicAllocation(false), isUnallocated(false) {}

        int gpuAccessFlag;
        size_t bufferSize;
        uint32_t bufferCount;
        nn::gfx::MemoryPool* pMemoryPool;
        ptrdiff_t memoryPoolOffset;
        AllocateFunction pAllocateFunction;
        void* pUserData;
        bool isAtomicAllocation;
        bool isUnallocated;
    };

    bool Initialize(nn::gfx::Device* pDevice, const InitializeArg& rArg);
    void Finalize(nn::gfx::Device* pDevice, FreeFunction pFreeFunction, void* pUserData);
    void Map(int bufferIndex);
    void Unmap();

    uint64_t Allocate(size_t size) {
        size_t alignedSize = nn::util::align_up(size, m_BufferAlignment);
        if (m_Flags & Flag_AtomicAllocation) {
            return reinterpret_cast<std::atomic<uint64_t>*>(m_pAtomicAllocatedSize)
                ->fetch_add(alignedSize);
        }
        uint64_t offset = m_AllocatedSize;
        m_AllocatedSize += alignedSize;
        return offset;
    }

    /**
     * @brief Reserves @p size bytes without aligning them.
     * @return Offset of the reserved range.
     */
    uint64_t AllocateWithoutAlignment(size_t size) {
        if (m_Flags & Flag_AtomicAllocation) {
            return reinterpret_cast<std::atomic<uint64_t>*>(m_pAtomicAllocatedSize)->fetch_add(size);
        }

        uint64_t offset = m_AllocatedSize;
        m_AllocatedSize += size;
        return offset;
    }

    bool IsUnallocated() const { return (m_Flags & Flag_Unallocated) != 0; }
    size_t GetBufferAlignment() const { return m_BufferAlignment; }
    void* GetMappedPointer() const { return m_pMappedPointer; }
    const nn::gfx::GpuAddress& GetGpuAddress() const {
        return m_pGpuAddresses[m_GpuAccessBufferIndex];
    }

    uint32_t m_Flags;
    nn::gfx::Buffer* m_pBuffers;
    nn::gfx::GpuAddress* m_pGpuAddresses;
    size_t m_BufferSize;
    size_t m_BufferAlignment;
    uint32_t m_BufferCount;
    int m_MappedBufferIndex;
    int m_GpuAccessBufferIndex;
    void* m_pMappedPointer;
    union {
        struct {
            uint64_t m_AllocatedSize;
            uint64_t m_AllocatedSize2;
        };
        struct {
            uint64_t* m_pAtomicAllocatedSize;
            uint64_t* m_pAtomicAllocatedSize2;
        };
    };
};

}  // namespace font
}  // namespace nn
