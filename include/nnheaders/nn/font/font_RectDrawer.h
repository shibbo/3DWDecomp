#pragma once

#include <nn/font/font_DispStringBuffer.h>
#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_ResShader.h>
#include <nn/gfx/gfx_Sampler.h>
#include <nn/gfx/gfx_SamplerInfo.h>
#include <nn/gfx/gfx_Shader.h>
#include <nn/gfx/gfx_State.h>
#include <nn/gfx/gfx_Types.h>

namespace nn {
namespace font {

class RectDrawer {
public:
    static const int ShaderVariationCount = 6;

    typedef bool (*RegisterSamplerSlot)(nn::gfx::DescriptorSlot* pDstSlot,
                                        const nn::gfx::Sampler& sampler, void* pUserData);
    typedef void (*UnregisterSamplerSlot)(nn::gfx::DescriptorSlot* pDstSlot,
                                          const nn::gfx::Sampler& sampler, void* pUserData);
    typedef bool (*AcquireSamplerSlot)(nn::gfx::DescriptorSlot* pDstSlot,
                                       const nn::gfx::SamplerInfo& info, void* pUserData);
    typedef void (*ReleaseSamplerSlot)(nn::gfx::DescriptorSlot* pDstSlot,
                                       const nn::gfx::SamplerInfo& info, void* pUserData);

    static size_t GetWorkBufferAlignment();
    static size_t GetWorkBufferSize(nn::gfx::Device* pDevice, uint32_t charCount);
    static size_t CalculateMemoryPoolSize(nn::gfx::Device* pDevice, uint32_t charCount);
    static size_t CalculateMemoryPoolAlignment(nn::gfx::Device* pDevice);

    RectDrawer();
    virtual ~RectDrawer();

    virtual bool Initialize(nn::gfx::Device* pDevice, void* pWorkMemory, uint32_t charCount,
                            nn::gfx::MemoryPool* pMemoryPool, ptrdiff_t memoryPoolOffset,
                            size_t memoryPoolSize);
    virtual void Finalize(nn::gfx::Device* pDevice);

    void RegisterSamplerToDescriptorPool(RegisterSamplerSlot pRegisterSamplerSlot,
                                         void* pUserData);
    void UnregisterSamplerFromDescriptorPool(UnregisterSamplerSlot pUnregisterSamplerSlot,
                                             void* pUserData);
    void AcquireCommonSamplerSlot(AcquireSamplerSlot pAcquireSamplerSlot, void* pUserData);
    void ReleaseCommonSamplerSlot(ReleaseSamplerSlot pReleaseSamplerSlot, void* pUserData);

    virtual void Draw(nn::gfx::CommandBuffer& rCommandBuffer, const DispStringBuffer& rBuffer) const;

    const nn::gfx::Shader* GetVertexShader(int variation) const;
    const nn::gfx::Shader* GetPixelShader(int variation) const;

    static void CreateIndices(uint16_t* pIndices, uint32_t charCount);

private:
    void AddDrawCommand(nn::gfx::CommandBuffer& rCommandBuffer,
                        const DispStringBuffer::VertexBufferData& rVertexBufferData,
                        uint32_t shaderVariationFlags,
                        const nn::gfx::GpuAddress& rConstantBufferAddress,
                        const nn::gfx::GpuAddress& rPerCharacterParamsAddress) const;

    nn::gfx::ResShaderProgram* GetResShaderProgram(int variation) const {
        return m_pResShaderFile->GetShaderContainer()
            ->GetResShaderVariation(variation)
            ->GetResShaderProgram(m_CodeType);
    }

    nn::gfx::ResShaderFile* m_pResShaderFile;
    nn::gfx::ShaderCodeType m_CodeType;
    int m_VertexShaderSlots[ShaderVariationCount];
    int m_VertexShaderPerCharacterParamsSlots[ShaderVariationCount];
    int m_PixelShaderSlots[ShaderVariationCount];
    int m_BlackWhiteInterpolationSlots[ShaderVariationCount];
    int m_TextureSlots[ShaderVariationCount];
    int32_t m_CharCountMax;
    nn::gfx::VertexState m_VertexStates[ShaderVariationCount];
    nn::gfx::MemoryPool m_MemoryPoolForBuffers;
    nn::gfx::Buffer m_VertexBuffer;
    nn::gfx::Buffer m_IndexBuffer;
    nn::gfx::Buffer m_ShaderParamBlackWhiteInterpolationEnabledBuffer;
    nn::gfx::Buffer m_ShaderParamBlackWhiteInterpolationDisabledBuffer;
    nn::gfx::Sampler m_Sampler;
    nn::gfx::DescriptorSlot m_DescriptorSlotForSampler;
    void* m_WorkMemory;
};

}  // namespace font
}  // namespace nn
