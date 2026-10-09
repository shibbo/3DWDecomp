#pragma once

#include "nn/gfx/gfx_Buffer.h"
#include "nn/gfx/gfx_DescriptorSlot.h"
#include "nn/gfx/gfx_GpuAddress.h"
#include "nn/gfx/gfx_MemoryPool.h"
#include "nn/gfx/gfx_State.h"
#include "nn/gfx/gfx_Types.h"
#include "nn/ui2d/ui2d_Resources.h"
#include "nn/ui2d/ui2d_ShaderInfo.h"
#include "nn/ui2d/ui2d_Types.h"
#include "nn/util.h"

namespace nn {

namespace gfx {
class BlendTargetStateInfo;
}

namespace font {
class RectDrawer;
};

namespace ui2d {

enum PresetSamplerId {
    PresetSamplerId_ClampToEdgeU_ClampToEdgeV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_ClampToEdgeV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_ClampToEdgeV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_ClampToEdgeU_ClampToEdgeV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_ClampToEdgeU_RepeatV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_RepeatV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_RepeatV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_ClampToEdgeU_RepeatV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_ClampToEdgeU_MirrorV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_MirrorV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_ClampToEdgeU_MirrorV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_ClampToEdgeU_MirrorV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_ClampToEdgeV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_ClampToEdgeV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_ClampToEdgeV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_ClampToEdgeV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_RepeatV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_RepeatV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_RepeatV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_RepeatV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_MirrorV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_MirrorV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_RepeatU_MirrorV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_RepeatU_MirrorV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_ClampToEdgeV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_ClampToEdgeV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_ClampToEdgeV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_ClampToEdgeV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_RepeatV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_RepeatV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_RepeatV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_RepeatV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_MirrorV_MinPoint_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_MirrorV_MinLinear_MagPoint_MipPoint,
    PresetSamplerId_MirrorU_MirrorV_MinPoint_MagLinaer_MipPoint,
    PresetSamplerId_MirrorU_MirrorV_MinLinear_MagLinaer_MipPoint,
    PresetSamplerId_ClampToTransparentBorderColor,
    PresetSamplerId_Max
};

enum PresetBlendStateId {
    PresetBlendStateId_Default,
    PresetBlendStateId_OpaqueOrAlphaTest,
    PresetBlendStateId_Addition,
    PresetBlendStateId_Subtraction,
    PresetBlendStateId_Multiplication,
    PresetBlendStateId_SemitransparencyMaxAlpha,
    PresetBlendStateId_MaxPresetBlendStateId,
    PresetBlendStateId_None,
};

class GraphicsResource {
    NN_NO_COPY(GraphicsResource);

public:
    GraphicsResource();
    ~GraphicsResource();

    void Setup(nn::gfx::Device* pDevice, int charMax, nn::gfx::MemoryPool* pMemoryPool,
               ptrdiff_t memoryPoolOffset, size_t memoryPoolSize, nn::font::RectDrawer* pRectDrawer,
               float zNear);
    void Finalize(nn::gfx::Device*);
    void RegisterCommonSamplerSlot(RegisterSamplerSlot, void*);
    void AcquireCommonSamplerSlot(AcquireSamplerSlot, void*);
    void UnregisterCommonSamplerSlot(UnregisterSamplerSlot, void*);
    void ReleaseCommonSamplerSlot(ReleaseSamplerSlot, void*);
    nn::gfx::DescriptorSlot& GetSamplerDescriptorSlot(TexWrap, TexWrap, TexFilter, TexFilter) const;
    nn::gfx::DescriptorSlot& GetSamplerDescriptorSlot(PresetSamplerId) const;
    void ActivateVertexBuffer(nn::gfx::CommandBuffer*) const;

    static PresetBlendStateId GetPresetBlendStateId(const ResBlendMode*, const ResBlendMode*);
    static size_t SetupBlendStateInfo(nn::gfx::BlendStateInfo* pBlendStateInfo,
                                      nn::gfx::BlendTargetStateInfo* pBlendTargetStateInfo,
                                      const ResBlendMode* pBlendMode,
                                      const ResBlendMode* pBlendModeAlpha);
    nn::gfx::BlendState* GetPresetBlendState(PresetBlendStateId id);

    static const PresetBlendStateId DefalutPresetBlendStateId;

    ShaderInfo m_CommonShaderInfo;
    void* m_pUi2dBuildinShader;
    void** m_pConstantBufferMemories;
    void* m_pBufferMemory;
    void* m_pRectShaderBinary;
    int32_t m_RectShaderBinarySize;
    size_t m_ConstantBufferAlignment;
    size_t m_VertexBufferAlignment;
    size_t m_IndexBufferAlignment;
    nn::font::RectDrawer* m_pFontDrawer;
    nn::gfx::MemoryPool m_MemoryPoolForBuffers;
    nn::gfx::Buffer m_VertexBuffer;
    nn::gfx::GpuAddress m_VertexBufferGpuAddress;
    nn::gfx::Buffer m_IndexBuffer;
    nn::gfx::GpuAddress m_IndexBufferGpuAddress;
    nn::gfx::Sampler* m_pSamplerTable;
    nn::gfx::DescriptorSlot* m_pSamplerDescriptorSlotTable;
    float m_SamplerLodBias;
    nn::gfx::BlendState m_PresetBlendState[6];
    nn::gfx::DepthStencilState m_PresetVectorGraphicsDepthStencilState[3];
    bool m_Initialized;
    bool m_IsDefaultRectDrawerUsed;
};
};  // namespace ui2d
};  // namespace nn
