/**
 * @file vfx_VertexBuffer.h
 * @brief Multi-buffered vertex attribute storage used by the VFX runtime.
 */

#pragma once

#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/types.h>
#include <nn/vfx/vfx_BufferAllocator.h>

namespace nn {
namespace vfx {

/** Selects one of the (up to three) per-frame copies of a buffer. */
enum BufferSide {
    BufferSide_FrontBuffer = 0,
    BufferSide_BackBuffer,
    BufferSide_ThirdBuffer,
    BufferSide_Max
};

namespace detail {

/**
 * Linear cutter over one block allocated from a BufferAllocator.
 */
struct BufferCutter {
    /**
     * Cuts an aligned piece off the remaining memory.
     * @param size number of bytes requested
     * @return the start of the piece, or nullptr when the block is exhausted
     */
    void* Cut(size_t size) {
        size_t alignment = m_pAllocator->GetAlignment();
        size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

        if (m_Offset + alignedSize > m_Size) {
            return nullptr;
        }

        void* pPiece = static_cast<u8*>(m_pBuffer) + m_Offset;
        m_Offset += alignedSize;
        return pPiece;
    }

    void* m_pBuffer;
    size_t m_Offset;
    BufferAllocator* m_pAllocator;
    size_t m_Size;
};

/**
 * Vertex attribute with one copy per buffer side.
 */
class Attribute {
public:
    Attribute();

    bool Initialize(BufferAllocator* pAllocator, int bufferCount, size_t size);
    void Finalize();
    void* Map(BufferSide side);
    void Unmap();
    const nn::gfx::GpuAddress* GetGpuAddress(BufferSide side) const;

    /** @return the size of one copy of the attribute */
    size_t GetSize() const { return m_Size; }

private:
    BufferCutter m_Cutter;
    nn::gfx::GpuAddress m_GpuAddress[BufferSide_Max];
    void* m_pCpuAddress[BufferSide_Max];
    u8 _68[8];
    size_t m_Size;
    int m_BufferCount;
};

}  // namespace detail
}  // namespace vfx
}  // namespace nn
