/**
 * @file System.h
 * @brief VFX system implementation.
 */

#pragma once

#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/os/os_MutexTypes.h>
#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/Callback.h>
#include <nn/vfx/Config.h>
#include <nn/vfx/EmitterRes.h>
#include <nn/vfx/EmitterSet.h>
#include <nn/vfx/Heap.h>
#include <nn/vfx/Resource.h>
#include <nn/vfx/vfx_Buffer.h>
#include <nn/vfx/vfx_BufferAllocator.h>

// this class is massive
namespace nn {
namespace vfx {

class Handle;
class Heap;

/** Camera parameters uploaded to the shaders for one processing index. */
struct ViewParam {
    util::Float4 viewMatrix[3];
    u8 _30[0x140 - 0x30];
};

static_assert(sizeof(ViewParam) == 0x140);

namespace detail {
struct SortData;
struct ResTextureSampler;

/** How the particles of an emitter are ordered before drawing. */
enum ParticleSortType {
    ParticleSortType_None,
    ParticleSortType_AscendingOrder,
    ParticleSortType_ZSort,
    ParticleSortType_DescendingOrder,
};
}  // namespace detail

typedef bool (*RegisterTextureViewSlot)(gfx::DescriptorSlot* pSlot,
                                        const gfx::TextureView& rTextureView, void* pUserData);
typedef bool (*RegisterSamplerSlot)(gfx::DescriptorSlot* pSlot, const gfx::Sampler& rSampler,
                                    void* pUserData);
typedef void (*UnregisterTextureViewSlot)(gfx::DescriptorSlot* pSlot,
                                          const gfx::TextureView& rTextureView, void* pUserData);
typedef void (*UnregisterSamplerSlot)(gfx::DescriptorSlot* pSlot, const gfx::Sampler& rSampler,
                                      void* pUserData);

enum TextureSlotId {
    TextureSlotId_FrameBuffer,
    TextureSlotId_DepthBuffer,
};


struct RenderingInfo;

class System {
public:
    struct SortEmitterSetData;

    static const s32 GroupMax = 64;
    static const s32 CallbackIdMax = 17;
    static const s32 DrawPathCallbackMax = 8;
    static const s32 CustomConstantBufferMax = 5;

    System(nn::vfx::Config const&);

    virtual ~System();
    virtual void Initialize(nn::vfx::Heap*, nn::vfx::Heap*, nn::vfx::Config const&);

    bool EntryResource(Heap* pHeap, void* pData, gfx::MemoryPool* pMemoryPool,
                       size_t memoryPoolOffset, size_t memoryPoolSize, s32 resourceId,
                       bool isDelay, Resource* pResource);
    void ClearResource(Heap* pHeap, s32 resourceId);
    bool EntryResource(Heap* pHeap, void* pData, s32 resourceId, bool isDelay,
                       Resource* pResource);
    EmitterSet* AllocEmitterSet();
    Emitter* AllocEmitter();
    void InitializeEmitter(Emitter* pEmitter);
    void FinalizeEmitter(Emitter* pEmitter);
    bool CreateEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId, s32 groupId,
                            s32 maxParticleCount, Heap* pHeap, bool isDelay);
    void AddEmitterSetList(EmitterSet* pEmitterSet, s32 groupId);
    void AddDelayCreateEmitterSetList(EmitterSet* pEmitterSet, s32 groupId);
    bool CreateEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId, s32 groupId,
                            Heap* pHeap, bool isDelay);
    bool CreateManualEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId,
                                  s32 groupId, s32 maxParticleCount,
                                  s32 maxEmitCountPerFrame,
                                  EmitReservationInfo* pEmitReservationInfo, Heap* pHeap,
                                  CallbackSet* pCallbackSet, s32 emitterSetLife);
    bool ReCreateEmitterSet(s32 resourceId, s32 emitterSetId);
    void RecreateEmitterSet2(const char* pEmitterSetName, s32 oldResourceId,
                             s32 newResourceId);
    void KillEmitterSet(EmitterSet* pEmitterSet, bool isImmediate);
    void KillEmitterSet(const char* pEmitterSetName, s32 resourceId);
    void RemoveDelayCreateEmitterSetList(EmitterSet* pEmitterSet);
    void KillEmitterSetGroup(s32 groupId);
    void KillAllEmitterSet();
    void RemoveEmitterSetList(EmitterSet* pEmitterSet);
    void BeginFrame();
    bool Calculate(EmitterSet* pEmitterSet, f32 frameRate, BufferSwapMode swapMode);
    void Calculate(s32 groupId, f32 frameRate, BufferSwapMode swapMode);
    void FlushGpuCache();
    void SetViewParam(s32 processingIndex, ViewParam* pViewParam);
    void SetViewParam(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                      ViewParam* pViewParam);
    void Draw(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer, s32 groupId,
              u32 drawPathFlag, bool isEmitterSetSort, bool isDoComputeShaderProcess,
              void* pUserParam);
    void AddSortBuffer(s32 processingIndex, s32 groupId, u32 drawPathFlag);
    void DrawSortBuffer(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                        bool isDoComputeShaderProcess, void* pUserParam);
    void DrawEmitter(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                     bool isDoComputeShaderProcess, void* pUserParam,
                     DrawParameterArg* pDrawParameterArg);
    void SwapBuffer();
    void* AllocFromTempBuffer(s32 processingIndex, gfx::GpuAddress* pAddress, size_t size);
    void FlushTempBuffer();
    void UpdateFromResource(EmitterResource* pEmitterResource, bool isResetEmitterSet);
    s32 SearchEmitterSetId(const char* pEmitterSetName, s32 resourceId) const;
    const char* SearchEmitterSetName(s32 emitterSetId, s32 resourceId) const;
    void SetDrawPathRenderStateSetCallback(DrawPathFlag flag,
                                           DrawPathRenderStateSetCallback callback);
    DrawPathRenderStateSetCallback GetDrawPathRenderStateSetCallback(DrawPathFlag flag);
    void AddComputeShaderEmitterList(Emitter* pEmitter);
    void BatchCalculationComputeShaderEmitter(gfx::CommandBuffer* pCommandBuffer, void* pUserParam,
                                              u32 groupBitFlag);
    void BatchCalculationComputeShaderEmitter(s32 processingIndex,
                                              gfx::CommandBuffer* pCommandBuffer,
                                              void* pUserParam, u32 groupBitFlag);
    bool GetSortedParticleList(detail::SortData** ppSortData, s32* pSortDataNum,
                               Emitter* pEmitter, detail::ParticleSortType sortType, f32 time,
                               s32 processingIndex) const;
    bool InvokeBeforeRenderCallbacks(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                                     ShaderType shaderType, void* pUserParam,
                                     DrawParameterArg* pDrawParameterArg);
    bool BindCustomShaderTexture(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                                 CustomShaderTextureType textureType, gfx::DescriptorSlot slot);
    void RegisterTextureViewToDescriptorPool(RegisterTextureViewSlot pFunc, void* pUserData);
    void UnregisterTextureViewFromDescriptorPool(UnregisterTextureViewSlot pFunc,
                                                 void* pUserData);
    void RegisterSamplerToDescriptorPool(RegisterSamplerSlot pFunc, void* pUserData);
    void UnregisterSamplerFromDescriptorPool(UnregisterSamplerSlot pFunc, void* pUserData);

    DrawParameterArg* GetDrawParameterArg(s32 processingIndex);
    void SetCurrentShader(s32 processingIndex, detail::Shader* pShader);

    Resource* GetResource(s32 resourceId) const {
        Resource* resource = m_Resources[resourceId];

        if (resource == nullptr) {
            return nullptr;
        }

        return resource->IsInitialized() ? resource : nullptr;
    }

    EmitterSet* GetEmitterSetHead(u8 groupId) const { return m_EmitterSetHead[groupId]; }
    s32 GetRenderingInfoNum() const { return m_RenderingInfoNum; }
    const RenderingInfo& GetRenderingInfo(s32 index) const;
    s32 GetResourceNum() const { return m_ResourceNum; }

    void SetCallback(s32 id, const CallbackSet& rCallbackSet) {
        m_IsEnableCallback[id] = true;
        m_CallbackSet[id] = rCallbackSet;
        m_CallbackSet[id].endianFlip = EndianFlipCallbackImpl;
    }

    void SetCustomFieldCallback(CustomFieldCallback callback) { m_CustomFieldCallback = callback; }

    void SetTextureSlot(s32 processingIndex, TextureSlotId id, gfx::DescriptorSlot slot);
    detail::Shader* GetCurrentShader(s32 processingIndex) const;

    void SetDrawPathRenderStateSetCallback(DrawPathCallbackId id, DrawPathFlag flag,
                                           DrawPathRenderStateSetCallback callback);

    s32 m_RenderingInfoNum;
    RenderingInfo* m_RenderingInfos;
    size_t m_RenderingInfoWorkSize;
    DrawParameterArg* m_DrawParameterArgs;
    size_t m_DrawParameterArgWorkSize;
    u8 _30[0x32 - 0x30];
    bool m_IsEnableCalculation;
    bool m_IsEnableDraw;
    bool m_IsEnableComputeShader;
    bool m_IsBatchProcessComputeShaderEmitter;
    bool m_IsTripleBuffer;
    u8 _37[0x38 - 0x37];
    gfx::Device* m_pDevice;
    detail::CalculateAllocatedSizeHeap m_StaticHeap;
    BufferingMode m_BufferingMode;
    s32 m_TotalResourceNum;
    s32 m_ResourceNum;
    s32 m_EmitterSetNum;
    s32 m_EmitterNum;
    s32 m_EmitterSetIndex;
    s32 m_EmitterIndex;
    s32 m_EmitterSetCreateId;
    s32 m_FreeEmitterNum;
    s32 m_FrameCount;
    u32 m_CalculatedGroupFlag;
    s32 m_StripeNum;
    s32 m_SuperStripeNum;
    u8 _94[0x98 - 0x94];
    Resource** m_Resources;
    EmitterSet* m_EmitterSets;
    Emitter* m_Emitters;
    EmitterSet* m_EmitterSetHead[GroupMax];
    EmitterSet* m_EmitterSetTail[GroupMax];
    EmitterSet* m_DelayCreateEmitterSetHead[GroupMax];
    EmitterSet* m_DelayCreateEmitterSetTail[GroupMax];
    EmitterSet** m_DelayKillEmitterSets;
    s32 m_DelayKillEmitterSetNum;
    detail::EmitterCalculator* m_pEmitterCalculator;
    bool m_IsEnableCallback[CallbackIdMax];
    u8 _8d9[0x8e0 - 0x8d9];
    CallbackSet m_CallbackSet[25];
    void* m_1178;
    void* m_1180;
    u32 m_DrawPathFlag[DrawPathCallbackMax];
    DrawPathRenderStateSetCallback m_DrawPathCallback[DrawPathCallbackMax];
    DrawEmitterProfileCallback m_DrawEmitterProfileCallback;
    EmitterCalculateLodCallback m_EmitterCalculateLodCallback;
    EmitterDrawCullingCallback m_EmitterDrawCullingCallback;
    CustomFieldCallback m_CustomFieldCallback;
    void* m_pResidentMemory;
    size_t m_RandomWorkSize;
    size_t m_DelayFreeWorkSize;
    size_t m_ResourceWorkSize;
    size_t m_EmitterSetWorkSize;
    size_t m_DelayKillEmitterSetWorkSize;
    size_t m_EmitterWorkSize;
    size_t m_EmitterPtrWorkSize;
    size_t m_EmitterCalculatorWorkSize;
    size_t m_EmitterConstantBufferWorkSize;
    size_t m_EmitterSetSortWorkSize;
    s32 m_ProcessingEmitterSetNum;
    s32 m_ProcessingCount[12];
    u8 _1294[0x1298 - 0x1294];
    u64 m_ProcessingAllocatedSize;
    u64 m_12a0;
    Emitter* m_ComputeShaderEmitterHead;
    Emitter* m_ComputeShaderEmitterTail;
    s32 m_ParticleSortBufferNum;
    u8 _12bc[0x12c0 - 0x12bc];
    size_t m_ParticleSortWorkSize;
    void* m_CustomConstantBuffer[CustomConstantBufferMax];
    size_t m_CustomConstantBufferSize[CustomConstantBufferMax];
    detail::Buffer m_EmitterConstantBuffer;
    detail::BufferAllocator m_GpuBufferAllocator;
    void* m_pGpuBufferAllocatorWork;
    void* m_pGpuBufferAllocatorPool;
    os::MutexType m_DelayKillMutex;
    os::MutexType m_DelayCreateMutex;
    bool m_1700;
};

struct RenderingInfo {
    u32 m_ViewFlag;
    u32 m_DrawPathFlag;
    u32 m_RenderingEmitterFlag[64];
    s32 m_108;
    u8 _10c[0x110 - 0x10c];
    TemporaryBuffer m_TemporaryBuffer;
    ViewParam m_ViewParam;
    gfx::GpuAddress m_ViewParamGpuAddress;
    gfx::DescriptorSlot m_TextureSlot[2];
    u32 m_RenderingFlag0[64];
    u32 m_RenderingFlag1[64];
    detail::Shader* m_pCurrentShader;
    s32 m_9c0;
    u8 _9c4[0x9c8 - 0x9c4];
    System::SortEmitterSetData* m_SortEmitterSets;
    s32 m_SortEmitterSetNum;
    u8 _9d4[0x9d8 - 0x9d4];
    detail::SortData* m_ParticleSortBuffer;
    bool m_IsViewParamBound;
};

static_assert(sizeof(System) == 0x1708);

inline const RenderingInfo& System::GetRenderingInfo(s32 index) const {
    return m_RenderingInfos[index];
}

inline void System::SetTextureSlot(s32 processingIndex, TextureSlotId id,
                                   gfx::DescriptorSlot slot) {
    if (processingIndex < m_RenderingInfoNum) {
        m_RenderingInfos[processingIndex].m_TextureSlot[id] = slot;
    }
}

inline detail::Shader* System::GetCurrentShader(s32 processingIndex) const {
    return m_RenderingInfos[processingIndex].m_pCurrentShader;
}

inline void System::SetCurrentShader(s32 processingIndex, detail::Shader* pShader) {
    m_RenderingInfos[processingIndex].m_pCurrentShader = pShader;
    m_RenderingInfos[processingIndex].m_9c0 = 6;
}
static_assert(sizeof(RenderingInfo) == 0x9e8);
}  // namespace vfx
}  // namespace nn
