#pragma once

#include <cstdio>
#include <nn/gfx/gfx_ResShader.h>
#include <nn/gfx/gfx_State.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn {
namespace ui2d {
class ShaderInfo {
    NN_NO_COPY(ShaderInfo);

public:
    ShaderInfo()
        : m_pResShaderFile(nullptr), m_Flags(0), m_pVertexStates(nullptr),
          m_pVertexShaderSlots(nullptr), m_pGeometryShaderSlots(nullptr),
          m_pPixelShaderSlots(nullptr), m_pTextureSlots(nullptr), m_pVariationTable(nullptr) {
        SetShaderCodeType(nn::gfx::ShaderCodeType_Source);
        SetTextureSlotCount(0);
    }

    void Initialize(nn::gfx::Device*, void*);
    void InitializeWithVariationTable(nn::gfx::Device*, void*, const void*, nn::gfx::MemoryPool*,
                                      ptrdiff_t, size_t, int);
    void SetTextureSlotCount(int);
    void SetShaderCodeType(nn::gfx::ShaderCodeType);
    int GetVariationCount() const;
    const nn::gfx::Shader* GetVertexShader(int) const;
    const nn::gfx::Shader* GetGeometryShader(int) const;
    const nn::gfx::Shader* GetPixelShader(int) const;
    void InitializeVertexStates(nn::gfx::Device*);
    void Finalize(nn::gfx::Device*, bool);
    nn::gfx::ShaderCodeType GetShaderCodeType() const;
    void SetShader(nn::gfx::CommandBuffer&, int) const;
    int GetTextureSlotCount() const;

    /**
     * @param variation Shader variation.
     * @return Constant buffer slot of the vertex shader of variation.
     */
    int GetVertexShaderSlot(int variation) const { return m_pVertexShaderSlots[variation]; }
    /**
     * @param variation Shader variation.
     * @return Constant buffer slot of the geometry shader of variation.
     */
    int GetGeometryShaderSlot(int variation) const { return m_pGeometryShaderSlots[variation]; }
    /**
     * @param variation Shader variation.
     * @return Constant buffer slot of the pixel shader of variation.
     */
    int GetPixelShaderSlot(int variation) const { return m_pPixelShaderSlots[variation]; }

    void InitializeWithVariationTable(nn::gfx::Device* pDevice, void* pShader,
                                      const void* pVariationTable) {
        InitializeWithVariationTable(pDevice, pShader, pVariationTable, nullptr, 0, 0);
    }

    void InitializeWithVariationTable(nn::gfx::Device* pDevice, void* pShader,
                                      const void* pVariationTable, nn::gfx::MemoryPool* pMemoryPool,
                                      ptrdiff_t memoryPoolOffset, size_t memoryPoolSize) {
        InitializeWithVariationTable(pDevice, pShader, pVariationTable, pMemoryPool,
                                     memoryPoolOffset, memoryPoolSize, 3);
    }

    nn::gfx::ResShaderFile* m_pResShaderFile;
    uint32_t m_Flags;
    nn::gfx::VertexState* m_pVertexStates;
    int* m_pVertexShaderSlots;
    int* m_pGeometryShaderSlots;
    int* m_pPixelShaderSlots;
    int* m_pTextureSlots;
    const void* m_pVariationTable;
};
};  // namespace ui2d
};  // namespace nn