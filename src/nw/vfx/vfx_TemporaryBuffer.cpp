#include <nn/vfx/Callback.h>
#include <nn/vfx/Heap.h>
#include <nn/vfx/vfx_EmitterCalc.h>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_Device.h>

namespace nn::vfx::detail {
Heap* GetStaticHeap();
void OutputWarning(const char* pFormat, ...);
} // namespace nn::vfx::detail

namespace nn::vfx {

/**
 * @brief Allocates the per-frame temporary GPU buffers.
 * @param pDevice Initialized graphics device; must not be null.
 * @param size Requested capacity of each buffer in bytes, rounded to the GPU alignment.
 * @param bufferingMode Selects three buffers for triple buffering; other values use two.
 * @return True when every required buffer was initialized; false on allocation failure.
 */
bool TemporaryBuffer::Initialize(gfx::Device* pDevice, size_t size, BufferingMode bufferingMode) {
    m_CurrentBuffer = 0;
    m_BufferingMode = bufferingMode;
    gfx::BufferInfo info;
    info.SetDefault();
    info.SetGpuAccessFlags(gfx::GpuAccess_ConstantBuffer);
    size_t alignment = gfx::Buffer::GetBufferAlignment(pDevice, info);
    m_BufferSize = (alignment + (size - 1)) & -alignment;
    if (!m_Buffer[0].Initialize(pDevice, detail::GetStaticHeap(), gfx::GpuAccess_ConstantBuffer,
                                m_BufferSize)) {
        return false;
    }
    m_Buffer[0].UpdateGpuAddress();
    if (!m_Buffer[1].Initialize(pDevice, detail::GetStaticHeap(), gfx::GpuAccess_ConstantBuffer,
                                m_BufferSize)) {
        return false;
    }
    m_Buffer[1].UpdateGpuAddress();
    if (m_BufferingMode == BufferingMode_Triple) {
        if (!m_Buffer[2].Initialize(pDevice, detail::GetStaticHeap(), gfx::GpuAccess_ConstantBuffer,
                                    m_BufferSize)) {
            return false;
        }
        m_Buffer[2].UpdateGpuAddress();
    }
    m_IsInitialized = true;
    return true;
}

/** @brief Marks the buffers invalid and clears all allocation statistics. */
void TemporaryBuffer::Invalidate() {
    m_CurrentBuffer = 0;
    m_IsInitialized = false;
    m_PeakUsage = 0;
    m_UsedSize[0] = 0;
    m_UsedSize[1] = 0;
    m_UsedSize[2] = 0;
}

/**
 * @brief Releases the graphics buffers and their backing memory.
 * @param pDevice Device used to initialize these buffers; must not be null.
 */
void TemporaryBuffer::Finalize(gfx::Device* pDevice) {
    Heap* pHeap0 = detail::GetStaticHeap();
    m_Buffer[0].Finalize(pDevice, pHeap0);
    Heap* pHeap1 = detail::GetStaticHeap();
    m_Buffer[1].Finalize(pDevice, pHeap1);
    if (m_BufferingMode == BufferingMode_Triple) {
        Heap* pHeap2 = detail::GetStaticHeap();
        m_Buffer[2].Finalize(pDevice, pHeap2);
    }
}

/** @brief Flushes the current frame and maps the next buffer with an empty allocation cursor. */
void TemporaryBuffer::Swap() {
    if (!m_IsInitialized) {
        return;
    }
    if (m_PeakUsage < m_UsedSize[m_CurrentBuffer]) {
        m_PeakUsage = m_UsedSize[m_CurrentBuffer];
    }
    m_Buffer[m_CurrentBuffer].End();
    const int bufferCount = m_BufferingMode == BufferingMode_Triple ? 3 : 2;
    m_CurrentBuffer = (m_CurrentBuffer + 1) % bufferCount;
    m_Buffer[m_CurrentBuffer].Begin();
    m_UsedSize[m_CurrentBuffer] = 0;
}

/**
 * @brief Allocates an aligned temporary region and returns its GPU address.
 * @param pAddress Receives the region's GPU address on success; must not be null.
 * @param size Requested byte count; zero does not allocate anything.
 * @return CPU address of the allocated region, or nullptr when invalid or exhausted.
 */
void* TemporaryBuffer::Map(gfx::GpuAddress* pAddress, size_t size) {
    if (size == 0 || !m_IsInitialized) {
        return nullptr;
    }
    auto& rBuffer = m_Buffer[m_CurrentBuffer];
    void* pMemory = rBuffer.Cut(size);
    if (pMemory != nullptr) {
        *pAddress = m_Buffer[m_CurrentBuffer].m_GpuAddress;
        pAddress->Offset(static_cast<u8*>(pMemory) - static_cast<u8*>(rBuffer.m_pMappedAddress));
        size_t alignment = m_Buffer[m_CurrentBuffer].m_Alignment;
        m_UsedSize[m_CurrentBuffer] += (alignment + (size - 1)) & -alignment;
        return pMemory;
    }
    detail::Warning(nullptr, detail::RuntimeWarningId_TemporaryBufferExhausted);
    detail::OutputWarning("TemporaryBuffer Allocation Failed. Allocation size : %d Byte.\n", size);
    detail::OutputWarning("Current TemporaryBuffer Allocation : %d/%d.\n", m_UsedSize[m_CurrentBuffer],
                          m_BufferSize);
    return nullptr;
}

/** @brief Ends access to one allocation; flushing is deferred until Swap(). */
void TemporaryBuffer::Unmap() {}

} // namespace nn::vfx
