#include <nn/vfx/vfx_UniformBlock.h>
#include <nn/vfx/vfx_Buffer.h>
#include <nn/vfx/Heap.h>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>

namespace nn::vfx::detail {

/**
 * @brief Finds the constant buffer binding in the vertex and pixel shader stages.
 * @param pShader Initialized shader whose reflection information is queried; must not be null.
 * @param pName Null-terminated constant buffer name.
 */
void ConstantBufferSlot::Initialize(gfx::Shader* pShader, const char* pName) {
    m_VertexSlot =
        pShader->GetInterfaceSlot(gfx::ShaderStage_Vertex, gfx::ShaderInterfaceType_ConstantBuffer, pName);
    m_PixelSlot =
        pShader->GetInterfaceSlot(gfx::ShaderStage_Pixel, gfx::ShaderInterfaceType_ConstantBuffer, pName);
}

/**
 * @brief Allocates a GPU buffer and the memory pool that backs it.
 * @param pDevice Initialized graphics device; must not be null.
 * @param pHeap Heap used to allocate the pool backing memory; must not be null.
 * @param gpuAccessFlag Graphics access flags required by users of the buffer.
 * @param size Requested buffer size in bytes, rounded up for the backing pool.
 * @return True when initialized, including an already initialized buffer; false on allocation failure.
 */
bool Buffer::Initialize(gfx::Device* pDevice, Heap* pHeap, int gpuAccessFlag, size_t size) {
    if (m_IsInitialized) {
        return true;
    }

    m_BufferSize = size;
    m_pPoolMemory = nullptr;
    m_PoolMemorySize = 0;

    gfx::BufferInfo bufferInfo;
    bufferInfo.SetDefault();
    bufferInfo.SetGpuAccessFlags(gpuAccessFlag);

    gfx::MemoryPoolInfo poolInfo;
    poolInfo.SetDefault();
    poolInfo.SetMemoryPoolProperty(gfx::MemoryPoolProperty_CpuUncached | gfx::MemoryPoolProperty_GpuCached);

    m_Alignment = gfx::Buffer::GetBufferAlignment(pDevice, bufferInfo);
    const size_t poolAlignment = gfx::MemoryPool::GetPoolMemoryAlignment(pDevice, poolInfo);
    const size_t bufferSize = m_BufferSize;
    m_PoolMemorySize = bufferSize;
    const size_t granularity = gfx::MemoryPool::GetPoolMemorySizeGranularity(pDevice, poolInfo);
    m_PoolMemorySize = (bufferSize + granularity - 1) & -granularity;
    m_pPoolMemory = pHeap->Alloc(m_PoolMemorySize, poolAlignment);

    if (m_pPoolMemory == nullptr) {
        return false;
    }

    poolInfo.SetPoolMemory(m_pPoolMemory, m_PoolMemorySize);
    m_MemoryPool.Initialize(pDevice, poolInfo);
    bufferInfo.SetSize(m_BufferSize);
    m_Buffer.Initialize(pDevice, bufferInfo, &m_MemoryPool, 0, m_PoolMemorySize);
    m_IsInitialized = true;
    return true;
}

/**
 * @brief Finalizes the graphics resources and releases their backing allocation.
 * @param pDevice Device used to initialize this buffer; must not be null when initialized.
 * @param pHeap Heap that supplied the backing memory; must not be null when memory is owned.
 */
void Buffer::Finalize(gfx::Device* pDevice, Heap* pHeap) {
    if (!m_IsInitialized) {
        return;
    }

    m_Buffer.Finalize(pDevice);
    m_MemoryPool.Finalize(pDevice);
    if (m_pPoolMemory != nullptr) {
        pHeap->Free(m_pPoolMemory);
        m_pPoolMemory = nullptr;
    }
    m_IsInitialized = false;
}

} // namespace nn::vfx::detail
