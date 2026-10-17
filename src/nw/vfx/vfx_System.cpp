#include <nn/vfx/vfx_System.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/lmem.h>
#include <nn/os/os_Mutex.h>
#include <nn/util.h>
#include <nn/util/util_VectorApi.h>
#include <nn/vfx/Handle.h>
#include <nn/vfx/vfx_System_dup2.h>

namespace nn {
namespace vfx {

namespace {

typedef detail::SortCompareLessUInt<System::SortEmitterSetData> CompareEmitterSetZ;
typedef detail::SortCompareLessIndexStable<detail::SortData> CompareAscendingOrder;
typedef detail::SortCompareGreaterIndexStable<detail::SortData> CompareDescendingOrder;
typedef detail::SortCompareViewInvZ<detail::SortData> CompareViewInvZ;
typedef detail::SortCompareViewZ<detail::SortData> CompareViewZ;

/** Memory pool property of the GPU buffer allocator (CPU uncached, GPU cached). */
const int GpuBufferMemoryPoolProperty = 0x12;

/** Size of the constant buffer every emitter cuts out of the emitter constant buffer. */
const size_t EmitterConstantBufferSize = 0xC0;

/**
 * Rounds a size up to a power of two alignment.
 * @param size the size
 * @param alignment the alignment
 * @return the aligned size
 */
inline size_t AlignUp(size_t size, size_t alignment) {
    return (size + alignment - 1) & -alignment;
}

/**
 * Gets the alignment required for the memory of a memory pool.
 * @param pDevice the device
 * @param property the memory pool property
 * @return the alignment
 */
inline size_t GetMemoryPoolAlignment(gfx::Device* pDevice, int property) {
    gfx::MemoryPoolInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetMemoryPoolProperty(property);
    return gfx::MemoryPool::GetPoolMemoryAlignment(pDevice, info);
}

/**
 * Rounds a memory pool size up to the size granularity of the device.
 * @param pDevice the device
 * @param size the requested size
 * @param property the memory pool property
 * @return the aligned size
 */
inline size_t AlignMemoryPoolSize(gfx::Device* pDevice, size_t size, int property) {
    gfx::MemoryPoolInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetMemoryPoolProperty(property);
    return AlignUp(size, gfx::MemoryPool::GetPoolMemorySizeGranularity(pDevice, info));
}

/**
 * Gets the alignment required for a buffer.
 * @param pDevice the device
 * @param gpuAccessFlag how the GPU accesses the buffer
 * @return the alignment
 */
inline size_t GetBufferAlignment(gfx::Device* pDevice, int gpuAccessFlag) {
    gfx::BufferInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetGpuAccessFlags(gpuAccessFlag);
    return gfx::Buffer::GetBufferAlignment(pDevice, info);
}

/**
 * Converts a float into the 24-bit float (1 sign, 7 exponent, 16 mantissa bits) used as the
 * low part of the emitter set sort key.
 * @param value the float to convert
 * @return the converted value
 */
inline u32 ConvertF32ToF24(f32 value) {
    u32 bits = *reinterpret_cast<u32*>(&value);
    u32 sign = (bits >> 8) & 0x800000;
    s32 exponent;

    if ((bits & 0x7FFFFFFF) == 0) {
        exponent = 0;
    } else {
        exponent = static_cast<s32>((bits >> 23) & 0xFF) - 64;

        if (exponent < 0) {
            return sign;
        }

        if (exponent > 0x7F) {
            return 0x7F0000;
        }
    }

    return sign | ((bits >> 7) & 0xFFFF) | ((exponent & 0x7F) << 16);
}

/**
 * Computes the view depth of a point.
 * @param rViewParam the view the depth is computed in
 * @param rPos the point
 * @return the depth of the point
 */
inline f32 GetViewDepth(const ViewParam& rViewParam, const util::Vector3fType& rPos) {
    const util::Float4& rRow = rViewParam.viewMatrix[2];
    util::Vector3fType axisZ;
    util::VectorSet(&axisZ, rRow.x, rRow.y, rRow.z);

    float32x4_t product = vmulq_f32(rPos._v, axisZ._v);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    return vget_lane_f32(vpadd_f32(sum, sum), 0) + rRow.w;
}

/**
 * Gets the matrix a particle is transformed with, depending on how it follows its emitter.
 * @param pOutMatrix receives the matrix
 * @param pEmitter the emitter
 * @param index index of the particle
 */
inline void GetParticleMatrix(util::Matrix4x3fType* pOutMatrix, const Emitter* pEmitter,
                              s32 index) {
    const util::Float4* pRow0 = pEmitter->m_ParticleEmitterMatrixRow[0];
    const util::Float4* pRow1 = pEmitter->m_ParticleEmitterMatrixRow[1];
    const util::Float4* pRow2 = pEmitter->m_ParticleEmitterMatrixRow[2];
    util::Vector3fType axis;

    switch (pEmitter->m_pEmitterData->followType) {
    case 0:
        *pOutMatrix = pEmitter->m_MatrixSrt;
        break;
    case 2: {
        const float32x4_t& rTranslate = pEmitter->m_MatrixSrt._m.val[3];
        util::VectorSet(&axis, pRow0[index].x, pRow1[index].x, pRow2[index].x);
        pOutMatrix->_m.val[0] = axis._v;
        util::VectorSet(&axis, pRow0[index].y, pRow1[index].y, pRow2[index].y);
        pOutMatrix->_m.val[1] = axis._v;
        util::VectorSet(&axis, pRow0[index].z, pRow1[index].z, pRow2[index].z);
        pOutMatrix->_m.val[2] = axis._v;
        util::VectorSet(&axis, vgetq_lane_f32(rTranslate, 0), vgetq_lane_f32(rTranslate, 1),
                        vgetq_lane_f32(rTranslate, 2));
        pOutMatrix->_m.val[3] = axis._v;
        break;
    }
    default:
        util::VectorSet(&axis, pRow0[index].x, pRow1[index].x, pRow2[index].x);
        pOutMatrix->_m.val[0] = axis._v;
        util::VectorSet(&axis, pRow0[index].y, pRow1[index].y, pRow2[index].y);
        pOutMatrix->_m.val[1] = axis._v;
        util::VectorSet(&axis, pRow0[index].z, pRow1[index].z, pRow2[index].z);
        pOutMatrix->_m.val[2] = axis._v;
        util::VectorSet(&axis, pRow0[index].w, pRow1[index].w, pRow2[index].w);
        pOutMatrix->_m.val[3] = axis._v;
        break;
    }
}

/**
 * Transforms a point by a matrix.
 * @param pOutValue receives the transformed point
 * @param rVector the point
 * @param rMatrix the matrix
 */
inline void TransformVector(util::Vector3fType* pOutValue, const util::Vector3fType& rVector,
                            const util::Matrix4x3fType& rMatrix) {
    float32x4_t result = vmulq_n_f32(rMatrix._m.val[0], vgetq_lane_f32(rVector._v, 0));
    result = vmlaq_n_f32(result, rMatrix._m.val[1], vgetq_lane_f32(rVector._v, 1));
    result = vmlaq_n_f32(result, rMatrix._m.val[2], vgetq_lane_f32(rVector._v, 2));
    pOutValue->_v = vaddq_f32(rMatrix._m.val[3], result);
}

}  // namespace

/**
 * Constructs the system and initializes it with the heaps of the configuration.
 * @param rConfig the system configuration
 */
System::System(const Config& rConfig) {
    os::InitializeMutex(&m_DelayKillMutex, false, 0);
    os::InitializeMutex(&m_DelayCreateMutex, false, 0);
    util::ReferSymbol("SDK MW+Nintendo+NintendoWare_Vfx-10_4_0-Release");

    m_Resources = nullptr;
    m_EmitterSets = nullptr;
    m_Emitters = nullptr;
    m_ComputeShaderEmitterHead = nullptr;
    m_ComputeShaderEmitterTail = nullptr;

    Initialize(rConfig.m_SystemHeap, rConfig.m_DynamicHeap, rConfig);
}

/**
 * Allocates every work buffer of the system and initializes the sub systems.
 * @param pStaticHeap heap the resident work memory is allocated from
 * @param pDynamicHeap heap the dynamic allocations are made from
 * @param rConfig the system configuration
 */
void System::Initialize(Heap* pStaticHeap, Heap* pDynamicHeap, const Config& rConfig) {
    m_pDevice = static_cast<gfx::Device*>(rConfig.m_GfxDevice);
    m_ResourceNum = rConfig.m_ResourceNum;
    m_TotalResourceNum = rConfig.m_ResourceNum + 32;
    m_EmitterSetNum = rConfig.m_EmitterNum;
    m_EmitterNum = rConfig.m_EmitterSetNum;
    m_RenderingInfoNum = rConfig.m_MultiBufferNum;
    m_StripeNum = rConfig.m_StripeNum;
    m_SuperStripeNum = rConfig.m_SuperStripeNum;
    m_StaticHeap.SetHeap(pStaticHeap);
    m_IsTripleBuffer = rConfig.m_EnableDebugMode;

    detail::SetStaticHeap(&m_StaticHeap);
    detail::SetDynamicHeap(pDynamicHeap);

    detail::BufferAllocator::InitializeArg arg;
    arg.pDevice = m_pDevice;
    arg.memoryPoolProperty = GpuBufferMemoryPoolProperty;
    arg.gpuAccessFlag = 0x4C;
    arg.workMemorySize = rConfig.m_EmitterSetNum * 0x70 + 0x20;
    m_pGpuBufferAllocatorWork = pDynamicHeap->Alloc(arg.workMemorySize, 0x80);
    arg.pWorkMemory = m_pGpuBufferAllocatorWork;
    size_t poolAlignment = GetMemoryPoolAlignment(m_pDevice, GpuBufferMemoryPoolProperty);
    arg.poolMemorySize = AlignMemoryPoolSize(m_pDevice, rConfig.m_TemporaryBufferSize,
                                             GpuBufferMemoryPoolProperty);
    m_pGpuBufferAllocatorPool = pDynamicHeap->Alloc(arg.poolMemorySize, poolAlignment);
    arg.pPoolMemory = m_pGpuBufferAllocatorPool;
    m_GpuBufferAllocator.Initialize(arg);

    m_BufferingMode = m_IsTripleBuffer ? BufferingMode_Triple : BufferingMode_Double;
    detail::TextureSampler::InitializeSamplerTable(m_pDevice, &m_StaticHeap);

    u32 allocatedSize = detail::GetAllocatedSizeFromStaticHeap();
    detail::InitializeDelayFreeList(m_EmitterNum * 2);
    m_DelayFreeWorkSize = detail::GetAllocatedSizeFromStaticHeap() - allocatedSize;

    detail::SetSuppressOutputLog(rConfig.m_EnableDoubleBuffer);

    m_IsEnableCalculation = true;
    m_IsEnableDraw = true;
    m_IsEnableComputeShader = true;
    m_EmitterSetIndex = 0;
    m_EmitterIndex = 0;
    m_EmitterSetCreateId = 0;
    m_FreeEmitterNum = m_EmitterNum;
    m_FrameCount = 0;
    m_CalculatedGroupFlag = 0;
    m_ProcessingEmitterSetNum = 0;

    for (s32 i = 0; i < 12; i++) {
        m_ProcessingCount[i] = 0;
    }

    m_ProcessingAllocatedSize = 0;
    m_12a0 = 0;
    m_IsBatchProcessComputeShaderEmitter = rConfig.m_EnableGpuBufferFixed;
    m_ParticleSortBufferNum = rConfig.m_ParticleSortBufferNum;
    m_1700 = false;

    for (s32 i = 0; i < GroupMax; i++) {
        m_EmitterSetHead[i] = nullptr;
        m_EmitterSetTail[i] = nullptr;
        m_DelayCreateEmitterSetHead[i] = nullptr;
        m_DelayCreateEmitterSetTail[i] = nullptr;
    }

    m_DelayKillEmitterSets = nullptr;
    m_DelayKillEmitterSetNum = 0;

    m_EmitterWorkSize = sizeof(Emitter) * m_EmitterNum;
    m_EmitterPtrWorkSize = (sizeof(Emitter*) * m_EmitterNum + 0xF) & ~0xF;
    m_EmitterSetWorkSize = sizeof(EmitterSet) * m_EmitterSetNum;
    m_ResourceWorkSize = (sizeof(Resource*) * m_TotalResourceNum + 0xF) & ~0xF;

    m_1178 = nullptr;
    m_1180 = nullptr;
    m_DrawEmitterProfileCallback = nullptr;
    m_EmitterCalculateLodCallback = nullptr;
    m_EmitterDrawCullingCallback = nullptr;
    m_CustomFieldCallback = nullptr;

    for (s32 i = 0; i < CustomConstantBufferMax; i++) {
        m_CustomConstantBuffer[i] = nullptr;
        m_CustomConstantBufferSize[i] = 0;
    }

    m_DelayKillEmitterSetWorkSize = (sizeof(EmitterSet*) * m_EmitterSetNum + 0xF) & ~0xF;
    m_EmitterCalculatorWorkSize = sizeof(detail::EmitterCalculator);
    m_ParticleSortWorkSize =
        ((sizeof(detail::SortData) * m_ParticleSortBufferNum + 0xF) & ~0xF) * m_RenderingInfoNum;
    m_EmitterSetSortWorkSize =
        sizeof(SortEmitterSetData) * m_EmitterSetNum * m_RenderingInfoNum;
    m_RenderingInfoWorkSize = (sizeof(RenderingInfo) + 8) * m_RenderingInfoNum;
    m_DrawParameterArgWorkSize = sizeof(DrawParameterArg) * m_RenderingInfoNum;

    size_t residentSize = m_EmitterWorkSize + m_EmitterPtrWorkSize + m_EmitterSetWorkSize +
                          m_ResourceWorkSize + m_DelayKillEmitterSetWorkSize +
                          m_ParticleSortWorkSize + m_EmitterSetSortWorkSize +
                          m_RenderingInfoWorkSize + m_DrawParameterArgWorkSize +
                          m_EmitterCalculatorWorkSize + 0x100;
    residentSize = (residentSize + 0xF) & ~0xF;
    m_pResidentMemory = m_StaticHeap.Alloc(residentSize, 0x100);

    lmem::HeapHandle frameHeap = lmem::CreateFrameHeap(m_pResidentMemory, residentSize, 1);

    m_ResourceWorkSize = sizeof(Resource*) * m_TotalResourceNum;
    m_Resources =
        static_cast<Resource**>(lmem::AllocateFromFrameHeap(frameHeap, m_ResourceWorkSize, 0x10));
    std::memset(m_Resources, 0, m_ResourceWorkSize);

    m_EmitterSets = static_cast<EmitterSet*>(
        lmem::AllocateFromFrameHeap(frameHeap, m_EmitterSetWorkSize, 0x10));
    std::memset(m_EmitterSets, 0, m_EmitterSetWorkSize);

    for (s32 i = 0; i < m_EmitterSetNum; i++) {
        m_EmitterSets[i].m_System = this;
        m_EmitterSets[i].m_IsAlive = false;
    }

    m_DelayKillEmitterSets = static_cast<EmitterSet**>(
        lmem::AllocateFromFrameHeap(frameHeap, sizeof(EmitterSet*) * m_EmitterSetNum, 0x10));

    s32 bufferCount = m_BufferingMode == BufferingMode_Triple ? 3 : 2;

    m_Emitters =
        static_cast<Emitter*>(lmem::AllocateFromFrameHeap(frameHeap, m_EmitterWorkSize, 0x10));
    std::memset(m_Emitters, 0, m_EmitterWorkSize);
    Emitter* pEmitter = m_Emitters;

    size_t alignment = GetBufferAlignment(m_pDevice, gfx::GpuAccess_ConstantBuffer);
    s32 constantBufferNum = m_EmitterNum * bufferCount;
    size_t constantBufferSize =
        AlignUp(EmitterConstantBufferSize, alignment) * constantBufferNum;

    if (m_EmitterConstantBuffer.Initialize(m_pDevice, &m_StaticHeap, gfx::GpuAccess_ConstantBuffer,
                                           constantBufferSize)) {
        m_EmitterConstantBuffer.UpdateGpuAddress();
    }

    m_EmitterConstantBufferWorkSize = constantBufferSize;
    m_EmitterConstantBuffer.Begin();

    for (s32 i = 0; i < m_EmitterNum; i++, pEmitter++) {
        new (&pEmitter->m_Random) detail::Random();
        new (&pEmitter->m_Attribute) detail::Attribute();
        pEmitter->m_ConstantBuffer[0] = nullptr;
        pEmitter->m_ConstantBuffer[1] = nullptr;
        pEmitter->m_ConstantBuffer[2] = nullptr;
        new (pEmitter->m_DynamicHeap) DynamicHeap();
        pEmitter->m_pBufferAllocator = &m_GpuBufferAllocator;
        pEmitter->m_pConstantBuffer = &m_EmitterConstantBuffer;

        pEmitter->m_ConstantBuffer[0] = m_EmitterConstantBuffer.Cut(EmitterConstantBufferSize);
        pEmitter->m_ConstantBuffer[1] = m_EmitterConstantBuffer.Cut(EmitterConstantBufferSize);

        if (m_IsTripleBuffer) {
            pEmitter->m_ConstantBuffer[2] = m_EmitterConstantBuffer.Cut(EmitterConstantBufferSize);
        }
    }

    m_EmitterConstantBuffer.End();

    void* pCalculatorMemory =
        lmem::AllocateFromFrameHeap(frameHeap, m_EmitterCalculatorWorkSize, 0x10);
    m_pEmitterCalculator = new (pCalculatorMemory) detail::EmitterCalculator(this);

    allocatedSize = detail::GetAllocatedSizeFromStaticHeap();
    detail::Random::Initialize();
    m_RandomWorkSize = detail::GetAllocatedSizeFromStaticHeap() - allocatedSize;

    detail::InitializeCurlNoise(m_pDevice, &m_StaticHeap);

    for (s32 i = 0; i < CallbackIdMax; i++) {
        m_IsEnableCallback[i] = false;
        m_CallbackSet[i].endianFlip = EndianFlipCallbackImpl;
        m_CallbackSet[i].renderStateSet =
            reinterpret_cast<RenderStateSetCallback>(BindReservedCustomShaderConstantBuffer);
    }

    size_t stripeAllocatedSize = m_StaticHeap.GetAllocatedSize();
    detail::StripeSystem::InitializeSystem(&m_StaticHeap, this, m_BufferingMode, m_StripeNum);
    detail::ConnectionStripeSystem::InitializeSystem(&m_StaticHeap, this, m_BufferingMode);
    detail::SuperStripeSystem::InitializeSystem(&m_StaticHeap, this, m_BufferingMode,
                                                m_SuperStripeNum);
    size_t stripeWorkSize = m_StaticHeap.GetAllocatedSize() - stripeAllocatedSize;

    detail::AreaLoopSystem::Initialize(this);

    m_RenderingInfos = static_cast<RenderingInfo*>(
        lmem::AllocateFromFrameHeap(frameHeap, m_RenderingInfoWorkSize, 0x10));
    m_DrawParameterArgs = static_cast<DrawParameterArg*>(
        lmem::AllocateFromFrameHeap(frameHeap, m_DrawParameterArgWorkSize, 0x10));

    size_t temporaryBufferSize = rConfig.m_GpuBufferSize;
    size_t sortEmitterSetSize = sizeof(SortEmitterSetData) * m_EmitterSetNum;
    size_t particleSortSize = sizeof(detail::SortData) * m_ParticleSortBufferNum;

    for (s32 i = 0; i < m_RenderingInfoNum; i++) {
        m_RenderingInfos[i].m_ViewFlag = 0xFFFFFFFF;
        m_RenderingInfos[i].m_DrawPathFlag = 0;

        std::memset(m_RenderingInfos[i].m_RenderingEmitterFlag, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingEmitterFlag));
        m_RenderingInfos[i].m_108 = 0;
        new (&m_RenderingInfos[i].m_TemporaryBuffer) TemporaryBuffer();
        m_RenderingInfos[i].m_TemporaryBuffer.Initialize(m_pDevice, temporaryBufferSize,
                                                         m_BufferingMode);
        m_RenderingInfos[i].m_TextureSlot[0].Invalidate();
        m_RenderingInfos[i].m_TextureSlot[1].Invalidate();

        std::memset(m_RenderingInfos[i].m_RenderingFlag0, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingFlag0));
        std::memset(m_RenderingInfos[i].m_RenderingFlag1, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingFlag1));

        m_RenderingInfos[i].m_pCurrentShader = nullptr;
        m_RenderingInfos[i].m_9c0 = 6;
        m_RenderingInfos[i].m_SortEmitterSetNum = 0;
        m_RenderingInfos[i].m_SortEmitterSets =
            static_cast<SortEmitterSetData*>(lmem::AllocateFromFrameHeap(
                frameHeap, sortEmitterSetSize, 0x10));
        m_RenderingInfos[i].m_ParticleSortBuffer =
            static_cast<detail::SortData*>(lmem::AllocateFromFrameHeap(
                frameHeap, particleSortSize, 0x10));

        m_DrawParameterArgs[i].m_ProcessingIndex = i;
        m_DrawParameterArgs[i].m_pViewParam = &m_RenderingInfos[i].m_ViewParam;
        m_DrawParameterArgs[i].m_ViewFlag = 0xFFFFFFFF;
        m_DrawParameterArgs[i].m_14 = 0;
        m_DrawParameterArgs[i].m_pTemporaryBuffer = &m_RenderingInfos[i].m_TemporaryBuffer;
        m_DrawParameterArgs[i].m_pViewGpuAddress = &m_RenderingInfos[i].m_ViewParamGpuAddress;
        m_DrawParameterArgs[i].m_FrameBufferTextureSlot.Invalidate();
        m_DrawParameterArgs[i].m_DepthBufferTextureSlot.Invalidate();
        m_DrawParameterArgs[i].m_pParticleSortBuffer = m_RenderingInfos[i].m_ParticleSortBuffer;
    }

    lmem::DestroyFrameHeap(frameHeap);

    for (s32 i = 0; i < DrawPathCallbackMax; i++) {
        m_DrawPathFlag[i] = 0;
        m_DrawPathCallback[i] = nullptr;
    }

    detail::OutputLog("\n");
    detail::OutputLog("System Static     WorkSize : %d \n", m_StaticHeap.GetAllocatedSize());
    detail::OutputLog("System Static     AlcCount : %d \n", m_StaticHeap.GetAllocatedCount());
    detail::OutputLog("  DelayFree       WorkSize : %d \n", m_DelayFreeWorkSize);
    detail::OutputLog("  Random          WorkSize : %d \n", m_RandomWorkSize);
    detail::OutputLog("  Resource        WorkSize : %d \n", m_ResourceWorkSize);
    detail::OutputLog("  EmitterSet      WorkSize : %d ( EmitterSetNum:%d )\n",
                      m_EmitterSetWorkSize, m_EmitterSetNum);
    detail::OutputLog("  Emitter         WorkSize : %d ( EmitterNum:%d )\n", m_EmitterWorkSize,
                      m_EmitterNum);
    detail::OutputLog("  EmitterCalc     WorkSize : %d \n", m_EmitterCalculatorWorkSize);
    detail::OutputLog("  EmitterConstBuf WorkSize : %d \n", m_EmitterConstantBufferWorkSize);
    detail::OutputLog("  EmitterSetSort  WorkSize : %d \n", m_EmitterSetSortWorkSize);
    detail::OutputLog("  ParticleSort    WorkSize : %d \n", m_ParticleSortWorkSize);
    detail::OutputLog("  NoiseTexture    WorkSize : %d \n",
                      detail::GetCurlNoiseTextureAllocatedSize());
    detail::OutputLog("  TempBuffer      WorkSize : %d \n", temporaryBufferSize);
    detail::OutputLog("-------------------------------\n");
    detail::OutputLog("Stripe            WorkSize : %d \n", stripeWorkSize);
    detail::OutputLog("-------------------------------\n");
}

/**
 * Finalizes every emitter set, resource and sub system and releases the work memory.
 */
System::~System() {
    for (s32 i = 0; i < m_EmitterSetNum; i++) {
        m_EmitterSets[i].Finalize();
    }

    for (s32 i = 0; i < m_TotalResourceNum; i++) {
        if (m_Resources[i] != nullptr) {
            m_Resources[i]->Finalize(nullptr);
            detail::OutputWarning("vfx system deleted the registered binary. resource id : %d.",
                                  i);
        }
    }

    m_GpuBufferAllocator.Finalize(m_pDevice);
    detail::GetDynamicHeap()->Free(m_pGpuBufferAllocatorWork);
    detail::GetDynamicHeap()->Free(m_pGpuBufferAllocatorPool);
    m_EmitterConstantBuffer.Finalize(m_pDevice, &m_StaticHeap);

    for (s32 i = 0; i < m_EmitterNum; i++) {
        reinterpret_cast<DynamicHeap*>(m_Emitters[i].m_DynamicHeap)->DynamicHeap::~DynamicHeap();
    }

    if (m_pEmitterCalculator != nullptr) {
        m_pEmitterCalculator->~EmitterCalculator();
    }

    for (s32 i = 0; i < m_RenderingInfoNum; i++) {
        m_RenderingInfos[i].m_TemporaryBuffer.Finalize(m_pDevice);
    }

    m_StaticHeap.Free(m_pResidentMemory);
    detail::FinalizeCurlNoise(m_pDevice);
    detail::Random::Finalize();
    detail::FinalizeDelayFreeList();
    detail::TextureSampler::FinalizeSamplerTable(m_pDevice, &m_StaticHeap);
    detail::SetStaticHeap(nullptr);
    detail::SetDynamicHeap(nullptr);
    detail::StripeSystem::FinalizeSystem(&m_StaticHeap);
    detail::SuperStripeSystem::FinalizeSystem(&m_StaticHeap);
    detail::ConnectionStripeSystem::FinalizeSystem(&m_StaticHeap);
    os::FinalizeMutex(&m_DelayCreateMutex);
    os::FinalizeMutex(&m_DelayKillMutex);
}

/**
 * Registers a binary resource that uses an external memory pool.
 * @param pHeap heap the resource is allocated from
 * @param pData the resource binary
 * @param pMemoryPool memory pool holding the binary
 * @param memoryPoolOffset offset of the binary in the memory pool
 * @param memoryPoolSize size of the binary in the memory pool
 * @param resourceId the id the resource is registered with
 * @param isDelay whether the GPU resources are created later
 * @param pResource resource the new one shares its data with
 * @return true on success
 */
bool System::EntryResource(Heap* pHeap, void* pData, gfx::MemoryPool* pMemoryPool,
                           size_t memoryPoolOffset, size_t memoryPoolSize, s32 resourceId,
                           bool isDelay, Resource* pResource) {
    if (m_Resources[resourceId] != nullptr) {
        m_Resources[resourceId]->Finalize(pHeap);
        pHeap->Free(m_Resources[resourceId]);
        m_Resources[resourceId] = nullptr;
        detail::OutputWarning("vfx system deleted the registered binary. resource id : %d.\n",
                              resourceId);
    }

    void* pMemory = pHeap->Alloc(sizeof(Resource), 0x80);

    if (pMemory == nullptr) {
        return false;
    }

    m_Resources[resourceId] =
        new (pMemory) Resource(m_pDevice, pHeap, pData, pMemoryPool, memoryPoolOffset,
                               memoryPoolSize, resourceId, this, isDelay, pResource);
    return true;
}

/**
 * Unregisters a binary resource.
 * @param pHeap heap the resource was allocated from
 * @param resourceId the id of the resource
 */
void System::ClearResource(Heap* pHeap, s32 resourceId) {
    if (m_Resources[resourceId] == nullptr) {
        detail::OutputWarning("The Resource to be cleared does not exist. ResourceId : %d.\n",
                              resourceId);
        return;
    }

    m_Resources[resourceId]->Finalize(pHeap);
    pHeap->Free(m_Resources[resourceId]);
    m_Resources[resourceId] = nullptr;
}

/**
 * Registers a binary resource.
 * @param pHeap heap the resource is allocated from
 * @param pData the resource binary
 * @param resourceId the id the resource is registered with
 * @param isDelay whether the GPU resources are created later
 * @param pResource resource the new one shares its data with
 * @return true on success
 */
bool System::EntryResource(Heap* pHeap, void* pData, s32 resourceId, bool isDelay,
                           Resource* pResource) {
    if (m_Resources[resourceId] != nullptr) {
        m_Resources[resourceId]->Finalize(pHeap);
        pHeap->Free(m_Resources[resourceId]);
        m_Resources[resourceId] = nullptr;
        detail::OutputWarning("vfx system deleted the registered binary. resource id : %d.\n",
                              resourceId);
    }

    void* pMemory = pHeap->Alloc(sizeof(Resource), 0x80);

    if (pMemory == nullptr) {
        return false;
    }

    m_Resources[resourceId] = new (pMemory)
        Resource(m_pDevice, pHeap, pData, nullptr, 0, 0, resourceId, this, isDelay, pResource);
    return true;
}

/**
 * Finds an emitter set instance that is not in use.
 * @return the emitter set, or nullptr when every instance is in use
 */
EmitterSet* System::AllocEmitterSet() {
    EmitterSet* pEmitterSet = nullptr;
    s32 i = 0;

    do {
        m_EmitterSetIndex = m_EmitterSetIndex + 1 >= m_EmitterSetNum ? 0 : m_EmitterSetIndex + 1;

        if (!m_EmitterSets[m_EmitterSetIndex].m_IsAlive) {
            pEmitterSet = &m_EmitterSets[m_EmitterSetIndex];
            break;
        }
    } while (++i < m_EmitterSetNum);

    if (pEmitterSet == nullptr) {
        detail::OutputWarning("There is no available EmitterSet instance.\n");
        return nullptr;
    }

    return pEmitterSet;
}

/**
 * Finds an emitter instance that is not in use.
 * @return the emitter, or nullptr when every instance is in use
 */
Emitter* System::AllocEmitter() {
    Emitter* pEmitter = nullptr;
    s32 i = 0;

    do {
        m_EmitterIndex = m_EmitterIndex + 1 >= m_EmitterNum ? 0 : m_EmitterIndex + 1;

        if (m_Emitters[m_EmitterIndex].m_EmitterCalculator == nullptr) {
            pEmitter = &m_Emitters[m_EmitterIndex];
            break;
        }
    } while (++i < m_EmitterNum);

    if (pEmitter == nullptr) {
        detail::OutputWarning("There is no available Emitter instance.\n");
        return nullptr;
    }

    m_FreeEmitterNum--;
    return pEmitter;
}

/**
 * Attaches the emitter calculator to an emitter.
 * @param pEmitter the emitter
 */
void System::InitializeEmitter(Emitter* pEmitter) {
    pEmitter->m_EmitterCalculator = m_pEmitterCalculator;
}

/**
 * Resets an emitter and returns it to the free pool.
 * @param pEmitter the emitter
 */
void System::FinalizeEmitter(Emitter* pEmitter) {
    pEmitter->Reset();
    m_FreeEmitterNum++;
}

/**
 * Creates an emitter set.
 * @param pHandle receives the handle of the created emitter set
 * @param emitterSetId id of the emitter set in the resource
 * @param resourceId id of the resource
 * @param groupId group the emitter set is added to
 * @param maxParticleCount particle count override for manual emitter sets
 * @param pHeap heap the emitter set allocates from
 * @param isDelay whether the emitter set is only added to the group on the next frame
 * @return true on success
 */
bool System::CreateEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId, s32 groupId,
                                s32 maxParticleCount, Heap* pHeap, bool isDelay) {
    Resource* pResource = GetResource(resourceId);

    if (pResource == nullptr) {
        return false;
    }

    s32 emitterNum = 0;
    EmitterSetResource* pEmitterSetResource = emitterSetId < pResource->GetEmitterSetNum() ?
                                                  pResource->GetEmitterSetResource(emitterSetId) :
                                                  nullptr;

    if (pEmitterSetResource != nullptr) {
        emitterNum = pEmitterSetResource->m_EmitterNum;
    }

    if (emitterNum > m_FreeEmitterNum) {
        pHandle->Invalidate();
        detail::OutputWarning("There is no available Emitter instance.\n");
        return false;
    }

    EmitterSet* pEmitterSet = AllocEmitterSet();

    if (pEmitterSet == nullptr) {
        return false;
    }

    pHandle->m_EmitterSet = pEmitterSet;
    s32 createId = m_EmitterSetCreateId++;
    pHandle->m_CreateId = createId;

    if (!pEmitterSet->Initialize(emitterSetId, createId, resourceId, groupId, maxParticleCount,
                                 pHeap)) {
        pEmitterSet->Finalize();
        return false;
    }

    if (isDelay) {
        os::LockMutex(&m_DelayCreateMutex);
        AddDelayCreateEmitterSetList(pEmitterSet, groupId);
        os::UnlockMutex(&m_DelayCreateMutex);
        pEmitterSet->m_IsDelayCreate = true;
    } else {
        AddEmitterSetList(pEmitterSet, groupId);
    }

    return true;
}

/**
 * Appends an emitter set to the list of its group.
 * @param pEmitterSet the emitter set
 * @param groupId the group
 */
void System::AddEmitterSetList(EmitterSet* pEmitterSet, s32 groupId) {
    if (m_EmitterSetHead[groupId] == nullptr) {
        m_EmitterSetHead[groupId] = pEmitterSet;
        pEmitterSet->m_Prev = nullptr;
    } else {
        m_EmitterSetTail[groupId]->m_Next = pEmitterSet;
        pEmitterSet->m_Prev = m_EmitterSetTail[groupId];
    }

    pEmitterSet->m_Next = nullptr;
    m_EmitterSetTail[groupId] = pEmitterSet;
}

/**
 * Appends an emitter set to the list of emitter sets added to their group on the next frame.
 * @param pEmitterSet the emitter set
 * @param groupId the group
 */
void System::AddDelayCreateEmitterSetList(EmitterSet* pEmitterSet, s32 groupId) {
    os::LockMutex(&m_DelayCreateMutex);

    if (m_DelayCreateEmitterSetHead[groupId] == nullptr) {
        m_DelayCreateEmitterSetHead[groupId] = pEmitterSet;
        pEmitterSet->m_Prev = nullptr;
    } else {
        m_DelayCreateEmitterSetTail[groupId]->m_Next = pEmitterSet;
        pEmitterSet->m_Prev = m_DelayCreateEmitterSetTail[groupId];
    }

    pEmitterSet->m_Next = nullptr;
    m_DelayCreateEmitterSetTail[groupId] = pEmitterSet;

    os::UnlockMutex(&m_DelayCreateMutex);
}

/**
 * Creates an emitter set.
 * @param pHandle receives the handle of the created emitter set
 * @param emitterSetId id of the emitter set in the resource
 * @param resourceId id of the resource
 * @param groupId group the emitter set is added to
 * @param pHeap heap the emitter set allocates from
 * @param isDelay whether the emitter set is only added to the group on the next frame
 * @return true on success
 */
bool System::CreateEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId, s32 groupId,
                                Heap* pHeap, bool isDelay) {
    return CreateEmitterSetId(pHandle, emitterSetId, resourceId, groupId, 0, pHeap, isDelay);
}

/**
 * Creates an emitter set whose particles are emitted manually.
 * @param pHandle receives the handle of the created emitter set
 * @param emitterSetId id of the emitter set in the resource
 * @param resourceId id of the resource
 * @param groupId group the emitter set is added to
 * @param maxParticleCount maximum number of particles
 * @param maxEmitCountPerFrame maximum number of particles emitted per frame
 * @param pEmitReservationInfo buffer of reserved emissions
 * @param pHeap heap the emitter set allocates from
 * @param pCallbackSet custom action callbacks to use, or nullptr
 * @param emitterSetLife life of the emitter set
 * @return true on success
 */
bool System::CreateManualEmitterSetId(Handle* pHandle, s32 emitterSetId, s32 resourceId,
                                      s32 groupId, s32 maxParticleCount,
                                      s32 maxEmitCountPerFrame,
                                      EmitReservationInfo* pEmitReservationInfo, Heap* pHeap,
                                      CallbackSet* pCallbackSet, s32 emitterSetLife) {
    if (maxParticleCount < maxEmitCountPerFrame) {
        detail::OutputWarning("maxParticleCount must be larger than maxEmitCountPerFrame.\n");
        return false;
    }

    Resource* pResource = GetResource(resourceId);

    if (pResource == nullptr) {
        detail::OutputWarning("ResourceID: %d does not exist.\n", resourceId);
        return false;
    }

    if (pResource->IsExistChildEmitter(emitterSetId)) {
        detail::OutputWarning("Manual EmitterSet must not to have Child Emitter.\n");
        return false;
    }

    if (!CreateEmitterSetId(pHandle, emitterSetId, resourceId, groupId, maxParticleCount, pHeap,
                            false) ||
        !pHandle->IsValid()) {
        detail::OutputWarning("CreateEmitterSetId has failed.\n");
        return false;
    }

    pHandle->GetEmitterSet()->m_IsManualEmission = true;
    pHandle->GetEmitterSet()->m_pEmitReservationInfo = pEmitReservationInfo;
    pHandle->GetEmitterSet()->m_MaxEmitCountPerFrame = maxEmitCountPerFrame;
    pHandle->GetEmitterSet()->m_ManualEmitterSetLife = emitterSetLife;

    if (pCallbackSet != nullptr) {
        pHandle->GetEmitterSet()->OverwriteCustomActionCallbackSet(pCallbackSet);
    }

    return true;
}

/**
 * Restarts every emitter set created from an emitter set resource.
 * @param resourceId id of the resource
 * @param emitterSetId id of the emitter set in the resource
 * @return true if any emitter set was restarted
 */
bool System::ReCreateEmitterSet(s32 resourceId, s32 emitterSetId) {
    bool isReCreated = false;

    for (s32 i = 0; i < GroupMax; i++) {
        EmitterSet* pEmitterSet = m_EmitterSetHead[i];

        while (pEmitterSet != nullptr) {
            EmitterSet* pNext = pEmitterSet->m_Next;

            if (pEmitterSet->m_ResourceId == resourceId &&
                pEmitterSet->m_EmitterSetId == emitterSetId) {
                if (pEmitterSet->m_IsFadeRequest) {
                    pEmitterSet->Kill(true);
                } else {
                    pEmitterSet->Reset();
                    isReCreated = true;
                }
            }

            pEmitterSet = pNext;
        }
    }

    return isReCreated;
}

/**
 * Moves every emitter set with a name to another resource and restarts it.
 * @param pEmitterSetName name of the emitter sets
 * @param oldResourceId id of the resource they currently use
 * @param newResourceId id of the resource they are moved to
 */
void System::RecreateEmitterSet2(const char* pEmitterSetName, s32 oldResourceId,
                                 s32 newResourceId) {
    for (s32 i = 0; i < GroupMax; i++) {
        EmitterSet* pEmitterSet = m_EmitterSetHead[i];

        while (pEmitterSet != nullptr) {
            EmitterSet* pNext = pEmitterSet->m_Next;

            if (std::strcmp(pEmitterSet->GetEmitterSetResource()->GetName(), pEmitterSetName) ==
                    0 &&
                pEmitterSet->m_ResourceId == oldResourceId) {
                pEmitterSet->m_ResourceId = newResourceId;
                pEmitterSet->Reset();
            }

            pEmitterSet = pNext;
        }
    }
}

/**
 * Requests an emitter set to be deleted at the beginning of the next frame.
 * @param pEmitterSet the emitter set
 * @param isImmediate unused
 */
void System::KillEmitterSet(EmitterSet* pEmitterSet, bool isImmediate) {
    if (pEmitterSet->m_IsAlive) {
        os::LockMutex(&m_DelayKillMutex);
        bool isRequested = false;

        for (s32 i = 0; i < m_DelayKillEmitterSetNum; i++) {
            if (m_DelayKillEmitterSets[i] == pEmitterSet) {
                isRequested = true;
                break;
            }
        }

        if (!isRequested) {
            m_DelayKillEmitterSets[m_DelayKillEmitterSetNum] = pEmitterSet;
            m_DelayKillEmitterSetNum++;
        }

        os::UnlockMutex(&m_DelayKillMutex);
    } else {
        detail::OutputWarning("EmitterSet has been already deleted.\n");
    }
}

/**
 * Deletes every emitter set with a name.
 * @param pEmitterSetName name of the emitter sets
 * @param resourceId id of the resource they use
 */
void System::KillEmitterSet(const char* pEmitterSetName, s32 resourceId) {
    for (s32 i = 0; i < GroupMax; i++) {
        EmitterSet* pEmitterSet = m_EmitterSetHead[i];

        while (pEmitterSet != nullptr) {
            EmitterSet* pNext = pEmitterSet->m_Next;

            if (std::strcmp(pEmitterSet->GetEmitterSetResource()->GetName(), pEmitterSetName) ==
                    0 &&
                pEmitterSet->m_ResourceId == resourceId) {
                KillEmitterSet(pEmitterSet, false);
            }

            pEmitterSet = pNext;
        }

        pEmitterSet = m_DelayCreateEmitterSetHead[i];

        while (pEmitterSet != nullptr) {
            EmitterSet* pNext = pEmitterSet->m_Next;

            if (std::strcmp(pEmitterSet->GetEmitterSetResource()->GetName(), pEmitterSetName) ==
                    0 &&
                pEmitterSet->m_ResourceId == resourceId) {
                RemoveDelayCreateEmitterSetList(pEmitterSet);
                KillEmitterSet(pEmitterSet, false);
            }

            pEmitterSet = pNext;
        }
    }
}

/**
 * Removes an emitter set from the list of emitter sets waiting to be added to their group.
 * @param pEmitterSet the emitter set
 */
void System::RemoveDelayCreateEmitterSetList(EmitterSet* pEmitterSet) {
    if (m_DelayCreateEmitterSetHead[pEmitterSet->m_GroupId] == pEmitterSet) {
        m_DelayCreateEmitterSetHead[pEmitterSet->m_GroupId] = pEmitterSet->m_Next;

        if (m_DelayCreateEmitterSetHead[pEmitterSet->m_GroupId] != nullptr) {
            m_DelayCreateEmitterSetHead[pEmitterSet->m_GroupId]->m_Prev = nullptr;
        }

        if (m_DelayCreateEmitterSetTail[pEmitterSet->m_GroupId] == pEmitterSet) {
            m_DelayCreateEmitterSetTail[pEmitterSet->m_GroupId] = nullptr;
        }
    } else {
        if (m_DelayCreateEmitterSetTail[pEmitterSet->m_GroupId] == pEmitterSet) {
            m_DelayCreateEmitterSetTail[pEmitterSet->m_GroupId] = pEmitterSet->m_Prev;
        }

        if (pEmitterSet->m_Next != nullptr) {
            pEmitterSet->m_Next->m_Prev = pEmitterSet->m_Prev;
        }

        if (pEmitterSet->m_Prev != nullptr) {
            pEmitterSet->m_Prev->m_Next = pEmitterSet->m_Next;
        } else {
            detail::OutputError("EmitterSet Remove Failed.\n");
        }
    }

    pEmitterSet->m_IsDelayCreate = false;
    pEmitterSet->m_Next = nullptr;
    pEmitterSet->m_Prev = nullptr;
}

/**
 * Deletes every emitter set of a group.
 * @param groupId the group
 */
void System::KillEmitterSetGroup(s32 groupId) {
    EmitterSet* pEmitterSet = m_EmitterSetHead[groupId];

    while (pEmitterSet != nullptr) {
        EmitterSet* pNext = pEmitterSet->m_Next;
        KillEmitterSet(pEmitterSet, false);
        pEmitterSet = pNext;
    }

    pEmitterSet = m_DelayCreateEmitterSetHead[groupId];

    while (pEmitterSet != nullptr) {
        EmitterSet* pNext = pEmitterSet->m_Next;
        KillEmitterSet(pEmitterSet, false);
        pEmitterSet->m_IsDelayCreate = false;
        pEmitterSet = pNext;
    }
}

/**
 * Deletes every emitter set.
 */
void System::KillAllEmitterSet() {
    for (s32 i = 0; i < GroupMax; i++) {
        KillEmitterSetGroup(i);
    }

    m_ComputeShaderEmitterHead = nullptr;
    m_ComputeShaderEmitterTail = nullptr;
}

/**
 * Removes an emitter set from the list of its group.
 * @param pEmitterSet the emitter set
 */
void System::RemoveEmitterSetList(EmitterSet* pEmitterSet) {
    if (pEmitterSet->m_Next == nullptr && pEmitterSet->m_Prev == nullptr &&
        pEmitterSet->m_IsDelayCreate) {
        RemoveDelayCreateEmitterSetList(pEmitterSet);
        return;
    }

    if (m_EmitterSetHead[pEmitterSet->m_GroupId] == pEmitterSet) {
        m_EmitterSetHead[pEmitterSet->m_GroupId] = pEmitterSet->m_Next;

        if (m_EmitterSetHead[pEmitterSet->m_GroupId] != nullptr) {
            m_EmitterSetHead[pEmitterSet->m_GroupId]->m_Prev = nullptr;
        }

        if (m_EmitterSetTail[pEmitterSet->m_GroupId] == pEmitterSet) {
            m_EmitterSetTail[pEmitterSet->m_GroupId] = nullptr;
        }
    } else {
        if (m_EmitterSetTail[pEmitterSet->m_GroupId] == pEmitterSet) {
            m_EmitterSetTail[pEmitterSet->m_GroupId] = pEmitterSet->m_Prev;
        }

        if (pEmitterSet->m_Next != nullptr) {
            pEmitterSet->m_Next->m_Prev = pEmitterSet->m_Prev;
        }

        if (pEmitterSet->m_Prev != nullptr) {
            pEmitterSet->m_Prev->m_Next = pEmitterSet->m_Next;
        } else {
            detail::OutputError("EmitterSet Remove Failed.\n");
        }
    }

    pEmitterSet->m_Next = nullptr;
    pEmitterSet->m_Prev = nullptr;
}

/**
 * Starts a new frame: deletes the emitter sets requested to be killed and adds the delayed
 * emitter sets to their groups.
 */
void System::BeginFrame() {
    if (!m_IsEnableCalculation) {
        return;
    }

    m_ProcessingEmitterSetNum = 0;

    for (s32 i = 0; i < 12; i++) {
        m_ProcessingCount[i] = 0;
    }

    m_ComputeShaderEmitterTail = nullptr;
    m_ComputeShaderEmitterHead = nullptr;
    m_12a0 = 0;
    m_ProcessingAllocatedSize = 0;
    m_FrameCount++;
    m_CalculatedGroupFlag = 0;

    detail::FlushDelayFreeList();
    m_GpuBufferAllocator.FlushFreeList();

    for (s32 i = 0; i < m_RenderingInfoNum; i++) {
        std::memset(m_RenderingInfos[i].m_RenderingEmitterFlag, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingEmitterFlag));
        std::memset(m_RenderingInfos[i].m_RenderingFlag0, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingFlag0));
        std::memset(m_RenderingInfos[i].m_RenderingFlag1, 0,
                    sizeof(m_RenderingInfos[i].m_RenderingFlag1));
    }

    os::LockMutex(&m_DelayKillMutex);

    for (s32 i = 0; i < m_DelayKillEmitterSetNum; i++) {
        m_DelayKillEmitterSets[i]->Finalize();
        RemoveEmitterSetList(m_DelayKillEmitterSets[i]);
        m_DelayKillEmitterSets[i] = nullptr;
    }

    m_DelayKillEmitterSetNum = 0;
    os::UnlockMutex(&m_DelayKillMutex);

    os::LockMutex(&m_DelayCreateMutex);
    bool isAdded = false;

    for (s32 i = 0; i < GroupMax; i++) {
        EmitterSet* pEmitterSet = m_DelayCreateEmitterSetHead[i];

        if (pEmitterSet != nullptr) {
            while (pEmitterSet != nullptr) {
                EmitterSet* pNext = pEmitterSet->m_Next;
                AddEmitterSetList(pEmitterSet, i);
                pEmitterSet->m_IsDelayCreate = false;
                pEmitterSet = pNext;
            }

            isAdded = true;
        }
    }

    os::UnlockMutex(&m_DelayCreateMutex);

    if (isAdded) {
        for (s32 i = 0; i < GroupMax; i++) {
            m_DelayCreateEmitterSetHead[i] = nullptr;
            m_DelayCreateEmitterSetTail[i] = nullptr;
        }
    }
}

/**
 * Updates an emitter set.
 * @param pEmitterSet the emitter set
 * @param frameRate the number of frames to advance
 * @param swapMode how the GPU buffers are swapped
 * @return true if the emitter set is still alive
 */
bool System::Calculate(EmitterSet* pEmitterSet, f32 frameRate, BufferSwapMode swapMode) {
    s64 groupId = pEmitterSet->m_GroupId;

    if (frameRate != 0.0f) {
        m_CalculatedGroupFlag |= static_cast<u32>(1ULL << groupId);
    }

    if (!pEmitterSet->IsCalcEnable()) {
        frameRate = 0.0f;
    }

    pEmitterSet->Calculate(frameRate, swapMode, false, m_EmitterCalculateLodCallback);

    if (pEmitterSet->IsAlive()) {
        m_ProcessingEmitterSetNum++;
        m_ProcessingCount[0] += pEmitterSet->m_EmitterNum;
        m_ProcessingCount[1] += pEmitterSet->m_ProcessingCount[0];
        m_ProcessingCount[3] += pEmitterSet->m_ProcessingCount[2];
        m_ProcessingCount[4] += pEmitterSet->m_ProcessingCount[3];
        m_ProcessingCount[5] += pEmitterSet->m_ProcessingCount[4];
        m_ProcessingCount[2] += pEmitterSet->m_ProcessingCount[1];
        m_ProcessingCount[6] += pEmitterSet->m_ProcessingCount[5];
        m_ProcessingCount[7] += pEmitterSet->m_ProcessingCount[6];
        m_ProcessingCount[8] += pEmitterSet->m_ProcessingCount[7];
        m_ProcessingCount[9] += pEmitterSet->m_ProcessingCount[8];
        m_ProcessingCount[10] += pEmitterSet->m_ProcessingCount[9];
        m_ProcessingAllocatedSize += pEmitterSet->m_AllocatedSize;
        m_ProcessingCount[11] += pEmitterSet->m_ProcessingCount[10];
        m_ProcessingAllocatedSize += pEmitterSet->m_AllocatedSize;

        m_RenderingInfos->m_RenderingEmitterFlag[groupId] |= pEmitterSet->m_DrawPathFlag;
        m_RenderingInfos->m_RenderingFlag0[groupId] |= pEmitterSet->m_RenderingFlag0;
        m_RenderingInfos->m_RenderingFlag1[groupId] |= pEmitterSet->m_RenderingFlag1;
        return true;
    }

    if (pEmitterSet->m_IsAlive) {
        KillEmitterSet(pEmitterSet, false);
        return false;
    }

    detail::OutputWarning("Invalid EmitterSet has been detected. Invalidates handle.");
    pEmitterSet->m_CreateId = -1;
    return false;
}

/**
 * Updates every emitter set of a group.
 * @param groupId the group
 * @param frameRate the number of frames to advance
 * @param swapMode how the GPU buffers are swapped
 */
void System::Calculate(s32 groupId, f32 frameRate, BufferSwapMode swapMode) {
    if (!m_IsEnableCalculation) {
        return;
    }

    EmitterSet* pEmitterSet = m_EmitterSetHead[groupId];

    while (pEmitterSet != nullptr) {
        EmitterSet* pNext = pEmitterSet->m_Next;
        Calculate(pEmitterSet, frameRate, swapMode);
        pEmitterSet = pNext;
    }
}

/**
 * Flushes the CPU cache of the GPU buffers. Nothing to do on this platform.
 */
void System::FlushGpuCache() {}

/**
 * Sets the view of a processing index and uploads it to the temporary buffer.
 * @param processingIndex the processing index
 * @param pViewParam the view parameters
 */
void System::SetViewParam(s32 processingIndex, ViewParam* pViewParam) {
    m_RenderingInfos[processingIndex].m_ViewParam = *pViewParam;

    void* pBuffer = m_RenderingInfos[processingIndex].m_TemporaryBuffer.Map(
        &m_RenderingInfos[processingIndex].m_ViewParamGpuAddress, sizeof(ViewParam));

    if (pBuffer != nullptr) {
        std::memcpy(pBuffer, pViewParam, sizeof(ViewParam));
        m_RenderingInfos[processingIndex].m_TemporaryBuffer.Unmap();
    }

    m_RenderingInfos[processingIndex].m_IsViewParamBound = false;
}

/**
 * Sets the view of a processing index and binds it and the custom constant buffers.
 * @param processingIndex the processing index
 * @param pCommandBuffer the command buffer the constant buffers are bound to
 * @param pViewParam the view parameters
 */
void System::SetViewParam(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                          ViewParam* pViewParam) {
    m_RenderingInfos[processingIndex].m_ViewParam = *pViewParam;

    void* pBuffer = m_RenderingInfos[processingIndex].m_TemporaryBuffer.Map(
        &m_RenderingInfos[processingIndex].m_ViewParamGpuAddress, sizeof(ViewParam));

    if (pBuffer != nullptr) {
        std::memcpy(pBuffer, pViewParam, sizeof(ViewParam));
        pCommandBuffer->SetConstantBuffer(5, gfx::ShaderStage_Vertex,
                                          m_RenderingInfos[processingIndex].m_ViewParamGpuAddress,
                                          sizeof(ViewParam));
        pCommandBuffer->SetConstantBuffer(5, gfx::ShaderStage_Pixel,
                                          m_RenderingInfos[processingIndex].m_ViewParamGpuAddress,
                                          sizeof(ViewParam));
        pCommandBuffer->SetConstantBuffer(5, gfx::ShaderStage_Compute,
                                          m_RenderingInfos[processingIndex].m_ViewParamGpuAddress,
                                          sizeof(ViewParam));
        m_RenderingInfos[processingIndex].m_TemporaryBuffer.Unmap();
    }

    m_RenderingInfos[processingIndex].m_IsViewParamBound = true;

    for (s32 i = 0; i < CustomConstantBufferMax; i++) {
        if (m_CustomConstantBuffer[i] == nullptr) {
            continue;
        }

        gfx::GpuAddress address;
        address.ToData()->value = 0;
        address.ToData()->impl = 0;

        void* pCustomBuffer = m_RenderingInfos[processingIndex].m_TemporaryBuffer.Map(
            &address, m_CustomConstantBufferSize[i]);

        if (pCustomBuffer == nullptr) {
            continue;
        }

        std::memcpy(pCustomBuffer, m_CustomConstantBuffer[i], m_CustomConstantBufferSize[i]);
        pCommandBuffer->SetConstantBuffer(11 + i, gfx::ShaderStage_Vertex, address,
                                          m_CustomConstantBufferSize[i]);
        pCommandBuffer->SetConstantBuffer(11 + i, gfx::ShaderStage_Pixel, address,
                                          m_CustomConstantBufferSize[i]);
        pCommandBuffer->SetConstantBuffer(11 + i, gfx::ShaderStage_Compute, address,
                                          m_CustomConstantBufferSize[i]);
        m_RenderingInfos[processingIndex].m_TemporaryBuffer.Unmap();
    }
}

/**
 * Gets the draw parameters of a processing index, updated from its rendering info.
 * @param processingIndex the processing index
 * @return the draw parameters, or nullptr if the index is out of range
 */
inline DrawParameterArg* System::GetDrawParameterArg(s32 processingIndex) {
    if (processingIndex >= m_RenderingInfoNum) {
        return nullptr;
    }

    m_DrawParameterArgs[processingIndex].m_ViewFlag = m_RenderingInfos[processingIndex].m_ViewFlag;
    m_DrawParameterArgs[processingIndex].m_14 = m_RenderingInfos[processingIndex].m_108;
    m_DrawParameterArgs[processingIndex].m_FrameBufferTextureSlot =
        m_RenderingInfos[processingIndex].m_TextureSlot[TextureSlotId_FrameBuffer];
    m_DrawParameterArgs[processingIndex].m_DepthBufferTextureSlot =
        m_RenderingInfos[processingIndex].m_TextureSlot[TextureSlotId_DepthBuffer];
    m_DrawParameterArgs[processingIndex].m_pViewGpuAddress =
        !m_RenderingInfos[processingIndex].m_IsViewParamBound ?
            &m_RenderingInfos[processingIndex].m_ViewParamGpuAddress :
            nullptr;
    return &m_DrawParameterArgs[processingIndex];
}

/**
 * Draws the emitter sets of a group.
 * @param processingIndex the processing index
 * @param pCommandBuffer the command buffer
 * @param groupId the group
 * @param drawPathFlag the draw paths to draw
 * @param isEmitterSetSort whether the emitter sets are sorted by depth first
 * @param isDoComputeShaderProcess whether compute shader emitters are processed
 * @param pUserParam user parameter passed to the callbacks
 */
void System::Draw(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer, s32 groupId,
                  u32 drawPathFlag, bool isEmitterSetSort, bool isDoComputeShaderProcess,
                  void* pUserParam) {
    EmitterSet* pEmitterSet = m_EmitterSetHead[groupId];

    if (pEmitterSet == nullptr) {
        return;
    }

    if (isEmitterSetSort) {
        AddSortBuffer(processingIndex, groupId, drawPathFlag);
        DrawSortBuffer(processingIndex, pCommandBuffer, isDoComputeShaderProcess, pUserParam);
        return;
    }

    DrawParameterArg* pDrawParameterArg = GetDrawParameterArg(processingIndex);

    while (pEmitterSet != nullptr) {
        if ((pEmitterSet->m_ViewFlag & m_RenderingInfos[processingIndex].m_ViewFlag) != 0 &&
            (pEmitterSet->m_DrawPathFlag & drawPathFlag) != 0 && pEmitterSet->m_IsDrawEnable) {
            pEmitterSet->Draw(pCommandBuffer, drawPathFlag, isDoComputeShaderProcess, pUserParam,
                              pDrawParameterArg, m_EmitterDrawCullingCallback,
                              m_DrawEmitterProfileCallback);
        }

        pEmitterSet = pEmitterSet->m_Next;
    }
}

/**
 * Adds the emitter sets of a group to the sort buffer of a processing index.
 * @param processingIndex the processing index
 * @param groupId the group
 * @param drawPathFlag the draw paths to draw
 */
void System::AddSortBuffer(s32 processingIndex, s32 groupId, u32 drawPathFlag) {
    EmitterSet* pEmitterSet = m_EmitterSetHead[groupId];

    if (pEmitterSet == nullptr) {
        return;
    }

    RenderingInfo& rInfo = m_RenderingInfos[processingIndex];

    while (pEmitterSet != nullptr) {
        if ((pEmitterSet->m_ViewFlag & rInfo.m_ViewFlag) != 0 &&
            (pEmitterSet->m_DrawPathFlag & drawPathFlag) != 0 && pEmitterSet->m_IsDrawEnable) {
            util::Vector3fType clipPos = pEmitterSet->GetClipPos();
            rInfo.m_SortEmitterSets[rInfo.m_SortEmitterSetNum].pEmitterSet = pEmitterSet;
            f32 depth = GetViewDepth(rInfo.m_ViewParam, clipPos);
            rInfo.m_SortEmitterSets[rInfo.m_SortEmitterSetNum].z =
                (pEmitterSet->m_DrawPriority << 24) | ConvertF32ToF24(depth);
            rInfo.m_SortEmitterSetNum++;
        }

        pEmitterSet = pEmitterSet->m_Next;
    }

    rInfo.m_DrawPathFlag |= drawPathFlag;
}

/**
 * Sorts and draws the emitter sets in the sort buffer of a processing index.
 * @param processingIndex the processing index
 * @param pCommandBuffer the command buffer
 * @param isDoComputeShaderProcess whether compute shader emitters are processed
 * @param pUserParam user parameter passed to the callbacks
 */
void System::DrawSortBuffer(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                            bool isDoComputeShaderProcess, void* pUserParam) {
    RenderingInfo& rInfo = m_RenderingInfos[processingIndex];

    if (rInfo.m_SortEmitterSetNum != 0) {
        CompareEmitterSetZ compare;
        std::sort(rInfo.m_SortEmitterSets, rInfo.m_SortEmitterSets + rInfo.m_SortEmitterSetNum,
                  compare);

        DrawParameterArg* pDrawParameterArg = GetDrawParameterArg(processingIndex);

        for (s32 i = 0; i < rInfo.m_SortEmitterSetNum; i++) {
            rInfo.m_SortEmitterSets[i].pEmitterSet->Draw(
                pCommandBuffer, rInfo.m_DrawPathFlag, isDoComputeShaderProcess, pUserParam,
                pDrawParameterArg, m_EmitterDrawCullingCallback, m_DrawEmitterProfileCallback);
        }

        rInfo.m_SortEmitterSetNum = 0;
    }

    rInfo.m_DrawPathFlag = 0;
}

/**
 * Draws an emitter, running its compute shader first if it is not batched.
 * @param pCommandBuffer the command buffer
 * @param pEmitter the emitter
 * @param isDoComputeShaderProcess whether the compute shader is run
 * @param pUserParam user parameter passed to the callbacks
 * @param pDrawParameterArg the draw parameters
 */
void System::DrawEmitter(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                         bool isDoComputeShaderProcess, void* pUserParam,
                         DrawParameterArg* pDrawParameterArg) {
    if (pEmitter->m_pEmitterData->calcType == 2 && !m_IsBatchProcessComputeShaderEmitter &&
        m_IsEnableComputeShader) {
        s32 bufferIndex = pEmitter->m_EmitterSet->m_BufferIndex;
        detail::ComputeShader* pComputeShader = pEmitter->m_pEmitterRes->GetComputeShader();

        if (pDrawParameterArg->m_pViewGpuAddress != nullptr &&
            pComputeShader->GetViewParamLocation() != -1) {
            pCommandBuffer->SetConstantBuffer(pComputeShader->GetViewParamLocation(),
                                              gfx::ShaderStage_Compute,
                                              *pDrawParameterArg->m_pViewGpuAddress,
                                              sizeof(ViewParam));
        }

        pEmitter->m_EmitterCalculator->CalculateComputeShader(
            pCommandBuffer, pEmitter, pComputeShader, bufferIndex, isDoComputeShaderProcess,
            pUserParam, true);
    }

    if (m_IsEnableDraw) {
        pEmitter->m_EmitterCalculator->Draw(pCommandBuffer, pEmitter, pUserParam,
                                            pDrawParameterArg);
    }
}

/**
 * Swaps the temporary buffers of every processing index.
 */
void System::SwapBuffer() {
    for (s32 i = 0; i < m_RenderingInfoNum; i++) {
        m_RenderingInfos[i].m_TemporaryBuffer.Swap();
    }
}

/**
 * Allocates memory from the temporary buffer of a processing index.
 * @param processingIndex the processing index
 * @param pAddress receives the GPU address of the memory
 * @param size the number of bytes to allocate
 * @return the memory, or nullptr on failure
 */
void* System::AllocFromTempBuffer(s32 processingIndex, gfx::GpuAddress* pAddress, size_t size) {
    if (processingIndex >= m_RenderingInfoNum) {
        return nullptr;
    }

    return m_RenderingInfos[processingIndex].m_TemporaryBuffer.Map(pAddress, size);
}

/**
 * Flushes the temporary buffers. Nothing to do on this platform.
 */
void System::FlushTempBuffer() {}

/**
 * Applies an edited emitter resource to the emitter sets using it.
 * @param pEmitterResource the edited emitter resource
 * @param isResetEmitterSet whether every emitter set is restarted
 */
void System::UpdateFromResource(EmitterResource* pEmitterResource, bool isResetEmitterSet) {
    if (pEmitterResource != nullptr) {
        pEmitterResource->FinalizeRenderState(m_pDevice);
        pEmitterResource->InitializeRenderState(m_pDevice);

        for (s32 i = 0; i < pEmitterResource->m_ChildEmitterResNum; i++) {
            if (pEmitterResource->m_ChildEmitterResSet[i] != nullptr) {
                pEmitterResource->m_ChildEmitterResSet[i]->FinalizeRenderState(m_pDevice);
                pEmitterResource->m_ChildEmitterResSet[i]->InitializeRenderState(m_pDevice);
            }
        }
    }

    for (s32 i = 0; i < GroupMax; i++) {
        EmitterSet* pEmitterSet = m_EmitterSetHead[i];

        if (pEmitterSet == nullptr) {
            continue;
        }

        if (isResetEmitterSet) {
            while (pEmitterSet != nullptr) {
                pEmitterSet->Reset();
                pEmitterSet = pEmitterSet->m_Next;
            }
        } else {
            while (pEmitterSet != nullptr) {
                EmitterSet* pCurrent = pEmitterSet;
                bool isUpdated = pCurrent->UpdateFromResource(pEmitterResource);
                pEmitterSet = pCurrent->m_Next;

                if (!isUpdated) {
                    KillEmitterSet(pCurrent, false);
                }
            }
        }
    }
}

/**
 * Prints the names of an emitter and its emitter set.
 * @param pEmitter the emitter
 * @param isOutputGroupId whether the group of the emitter set is printed too
 */
void DumpEmitterInformation(Emitter* pEmitter, int isOutputGroupId) {
    EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;
    const char* pEmitterSetName = pEmitterSet->GetEmitterSetResource()->GetName();
    const char* pEmitterName = pEmitter->m_pEmitterData->name;

    if (isOutputGroupId != 0) {
        detail::OutputWarning("  %s-%s-GroupId:%d\n", pEmitterSetName, pEmitterName,
                              pEmitterSet->m_GroupId);
    } else {
        detail::OutputWarning("  %s - %s\n", pEmitterSetName, pEmitterName);
    }
}

/**
 * Finds the id of an emitter set from its name.
 * @param pEmitterSetName name of the emitter set
 * @param resourceId id of the resource
 * @return the id of the emitter set, or -1 if it does not exist
 */
s32 System::SearchEmitterSetId(const char* pEmitterSetName, s32 resourceId) const {
    Resource* pResource = GetResource(resourceId);

    if (pResource == nullptr) {
        return -1;
    }

    return pResource->SearchEmitterSetId(pEmitterSetName);
}

/**
 * Gets the name of an emitter set.
 * @param emitterSetId id of the emitter set
 * @param resourceId id of the resource
 * @return the name, or nullptr if the emitter set does not exist
 */
const char* System::SearchEmitterSetName(s32 emitterSetId, s32 resourceId) const {
    Resource* pResource = GetResource(resourceId);

    if (pResource == nullptr) {
        return nullptr;
    }

    if (emitterSetId >= pResource->GetEmitterSetNum()) {
        return nullptr;
    }

    return pResource->GetEmitterSetName(emitterSetId);
}

/**
 * Sets the render state callback of a draw path.
 * @param id slot of the callback
 * @param flag the draw paths the callback is used for
 * @param callback the callback
 */
void System::SetDrawPathRenderStateSetCallback(DrawPathCallbackId id, DrawPathFlag flag,
                                               DrawPathRenderStateSetCallback callback) {
    if (id < DrawPathCallbackMax) {
        m_DrawPathFlag[id] = flag;
        m_DrawPathCallback[id] = callback;
    }
}

/**
 * Sets the render state callback of a draw path in the first slot.
 * @param flag the draw paths the callback is used for
 * @param callback the callback
 */
void System::SetDrawPathRenderStateSetCallback(DrawPathFlag flag,
                                               DrawPathRenderStateSetCallback callback) {
    SetDrawPathRenderStateSetCallback(DrawPathCallbackId_0, flag, callback);
}

/**
 * Gets the render state callback of a draw path.
 * @param flag the draw path
 * @return the callback, or nullptr if none is set
 */
DrawPathRenderStateSetCallback System::GetDrawPathRenderStateSetCallback(DrawPathFlag flag) {
    if (flag == DrawPathFlag_None) {
        return nullptr;
    }

    for (s32 i = 0; i < DrawPathCallbackMax; i++) {
        if ((m_DrawPathFlag[i] & flag) != 0) {
            return m_DrawPathCallback[i];
        }
    }

    return nullptr;
}

/**
 * Adds an emitter to the list of emitters whose compute shader is run in a batch.
 * @param pEmitter the emitter
 */
void System::AddComputeShaderEmitterList(Emitter* pEmitter) {
    for (Emitter* pCurrent = m_ComputeShaderEmitterHead; pCurrent != nullptr;
         pCurrent = pCurrent->m_NextComputeShaderEmitter) {
        if (pCurrent == pEmitter) {
            return;
        }
    }

    if (m_ComputeShaderEmitterHead == nullptr) {
        m_ComputeShaderEmitterHead = pEmitter;
        m_ComputeShaderEmitterTail = pEmitter;
    } else {
        m_ComputeShaderEmitterTail->m_NextComputeShaderEmitter = pEmitter;
        m_ComputeShaderEmitterTail = pEmitter;
    }

    pEmitter->m_NextComputeShaderEmitter = nullptr;
}

/**
 * Runs the compute shaders of the batched emitters.
 * @param pCommandBuffer the command buffer
 * @param pUserParam user parameter passed to the callbacks
 * @param groupBitFlag the groups to process
 */
void System::BatchCalculationComputeShaderEmitter(gfx::CommandBuffer* pCommandBuffer,
                                                  void* pUserParam, u32 groupBitFlag) {
    Emitter* pEmitter = m_ComputeShaderEmitterHead;

    if (pEmitter == nullptr || !m_IsEnableComputeShader) {
        return;
    }

    while (pEmitter != nullptr) {
        if (pEmitter->m_EmitterSet != nullptr && (pEmitter->m_GroupBitFlag & groupBitFlag) != 0 &&
            pEmitter->m_pEmitterData->calcType == 2) {
            s32 bufferIndex = pEmitter->m_EmitterSet->m_BufferIndex;
            detail::ComputeShader* pComputeShader = pEmitter->m_pEmitterRes->GetComputeShader();
            pEmitter->m_EmitterCalculator->CalculateComputeShader(
                pCommandBuffer, pEmitter, pComputeShader, bufferIndex, true, pUserParam, false);
        }

        pEmitter = pEmitter->m_NextComputeShaderEmitter;
    }

    pCommandBuffer->FlushMemory(gfx::GpuAccess_UnorderedAccessBuffer);
}

/**
 * Runs the compute shaders of the batched emitters with the view of a processing index.
 * @param processingIndex the processing index, or a negative value for no view
 * @param pCommandBuffer the command buffer
 * @param pUserParam user parameter passed to the callbacks
 * @param groupBitFlag the groups to process
 */
void System::BatchCalculationComputeShaderEmitter(s32 processingIndex,
                                                  gfx::CommandBuffer* pCommandBuffer,
                                                  void* pUserParam, u32 groupBitFlag) {
    Emitter* pEmitter = m_ComputeShaderEmitterHead;

    if (pEmitter == nullptr || !m_IsEnableComputeShader) {
        return;
    }

    while (pEmitter != nullptr) {
        if (pEmitter->m_EmitterSet != nullptr && (pEmitter->m_GroupBitFlag & groupBitFlag) != 0 &&
            pEmitter->m_pEmitterData->calcType == 2) {
            s32 bufferIndex = pEmitter->m_EmitterSet->m_BufferIndex;
            detail::ComputeShader* pComputeShader = pEmitter->m_pEmitterRes->GetComputeShader();

            if (processingIndex >= 0 && !m_RenderingInfos[processingIndex].m_IsViewParamBound &&
                pComputeShader->GetViewParamLocation() != -1) {
                pCommandBuffer->SetConstantBuffer(
                    pComputeShader->GetViewParamLocation(), gfx::ShaderStage_Compute,
                    m_RenderingInfos[processingIndex].m_ViewParamGpuAddress, sizeof(ViewParam));
            }

            pEmitter->m_EmitterCalculator->CalculateComputeShader(
                pCommandBuffer, pEmitter, pComputeShader, bufferIndex, true, pUserParam, false);
        }

        pEmitter = pEmitter->m_NextComputeShaderEmitter;
    }

    pCommandBuffer->FlushMemory(gfx::GpuAccess_UnorderedAccessBuffer);
}

/**
 * Collects the living particles of an emitter and sorts them.
 * @param ppSortData receives the sorted particle list
 * @param pSortDataNum receives the number of particles in the list
 * @param pEmitter the emitter
 * @param sortType how the particles are sorted
 * @param time the current time of the emitter
 * @param processingIndex the processing index whose view and buffer are used
 * @return true on success
 */
bool System::GetSortedParticleList(detail::SortData** ppSortData, s32* pSortDataNum,
                                   Emitter* pEmitter, detail::ParticleSortType sortType, f32 time,
                                   s32 processingIndex) const {
    if (pEmitter->m_ParticleNum > m_ParticleSortBufferNum) {
        detail::OutputWarning("Particle sort has failed. More buffer size is needed. \n");
        *ppSortData = nullptr;
        *pSortDataNum = 0;
        return false;
    }

    *ppSortData = m_RenderingInfos[processingIndex].m_ParticleSortBuffer;
    *pSortDataNum = 0;

    detail::ParticleProperty* pProperty = pEmitter->m_pGpuParticleProperty;

    if (pEmitter->m_pEmitterData->calcType == 0) {
        pProperty = pEmitter->GetCpuParticleProperty();
    }

    if (sortType == detail::ParticleSortType_DescendingOrder) {
        for (s32 i = 0; i < pEmitter->m_ParticleNum; i++) {
            if (time - pEmitter->m_ParticleAttr[i].createTime < pEmitter->m_ParticleAttr[i].life) {
                (*ppSortData)[*pSortDataNum].z = pProperty->pVec[i].w;
                (*ppSortData)[*pSortDataNum].index = i;
                (*pSortDataNum)++;
            }
        }

        CompareDescendingOrder compare;
        std::sort(*ppSortData, *ppSortData + *pSortDataNum, compare);
    } else if (sortType == detail::ParticleSortType_ZSort) {
        const ViewParam& rViewParam = m_RenderingInfos[processingIndex].m_ViewParam;

        for (s32 i = 0; i < pEmitter->m_ParticleNum; i++) {
            if (time - pEmitter->m_ParticleAttr[i].createTime < pEmitter->m_ParticleAttr[i].life) {
                util::Vector3fType localPos;
                util::VectorLoad(&localPos, reinterpret_cast<const util::Float3&>(pProperty->pPos[i]));
                util::Matrix4x3fType matrix;
                GetParticleMatrix(&matrix, pEmitter, i);

                util::Vector3fType worldPos;
                TransformVector(&worldPos, localPos, matrix);

                const util::Float4& rRow = rViewParam.viewMatrix[2];
                f32 depth = rRow.x * util::VectorGetX(worldPos) + rRow.y * util::VectorGetY(worldPos);
                depth = rRow.z * util::VectorGetZ(worldPos) + depth;
                depth = rRow.w + depth;

                (*ppSortData)[*pSortDataNum].z = depth;
                (*ppSortData)[*pSortDataNum].index = i;
                (*pSortDataNum)++;
            }
        }

        if (pEmitter->m_pEmitterRes->m_pResEmitter->isAlphaMaskEnable) {
            CompareViewInvZ compare;
            std::sort(*ppSortData, *ppSortData + *pSortDataNum, compare);
        } else {
            CompareViewZ compare;
            std::sort(*ppSortData, *ppSortData + *pSortDataNum, compare);
        }
    } else if (sortType == detail::ParticleSortType_AscendingOrder) {
        for (s32 i = 0; i < pEmitter->m_ParticleNum; i++) {
            if (time - pEmitter->m_ParticleAttr[i].createTime < pEmitter->m_ParticleAttr[i].life) {
                (*ppSortData)[*pSortDataNum].z = pProperty->pVec[i].w;
                (*ppSortData)[*pSortDataNum].index = i;
                (*pSortDataNum)++;
            }
        }

        CompareAscendingOrder compare;
        std::sort(*ppSortData, *ppSortData + *pSortDataNum, compare);
    }

    return true;
}

/**
 * Calls the render state callbacks before an emitter is drawn.
 * @param pCommandBuffer the command buffer
 * @param pEmitter the emitter
 * @param shaderType the shader the emitter is drawn with
 * @param pUserParam user parameter passed to the callbacks
 * @param pDrawParameterArg the draw parameters
 * @return false if a callback cancelled the draw
 */
bool System::InvokeBeforeRenderCallbacks(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                                         ShaderType shaderType, void* pUserParam,
                                         DrawParameterArg* pDrawParameterArg) {
    RenderStateSetArg arg;
    arg.pUserParam = pUserParam;
    arg.pCommandBuffer = pCommandBuffer;
    arg.pEmitter = pEmitter;
    arg.shaderType = shaderType;
    arg.isComputeShader = shaderType == ShaderType_Compute;
    arg.pDrawParameterArg = pDrawParameterArg;

    SetCurrentShader(pDrawParameterArg->m_ProcessingIndex,
                     shaderType == ShaderType_Compute ?
                         nullptr :
                         pEmitter->m_pEmitterRes->m_Shader[shaderType]);

    if (!InvokeRenderStateSetCallback(arg)) {
        return false;
    }

    if (pEmitter->m_RenderStateSetCallback != nullptr) {
        pEmitter->m_RenderStateSetCallback(arg);
    }

    SetCurrentShader(pDrawParameterArg->m_ProcessingIndex, nullptr);
    return true;
}

/**
 * Binds a texture to a custom shader texture slot of the shader being drawn with.
 * @param processingIndex the processing index
 * @param pCommandBuffer the command buffer
 * @param textureType the custom texture slot
 * @param slot descriptor slot of the texture
 * @return true if the texture was bound to any stage
 */
bool System::BindCustomShaderTexture(s32 processingIndex, gfx::CommandBuffer* pCommandBuffer,
                                     CustomShaderTextureType textureType,
                                     gfx::DescriptorSlot slot) {
    detail::Shader* pShader = m_RenderingInfos[processingIndex].m_pCurrentShader;

    if (pShader == nullptr) {
        return false;
    }

    s32 pixelLocation = pShader->GetCustomTexturePixelLocation(textureType);
    s32 vertexLocation = pShader->GetCustomTextureVertexLocation(textureType);

    detail::ResTextureSampler resSampler;
    resSampler.wrapV = 0;
    resSampler.filter = 0;
    resSampler.wrapU = 0;
    detail::TextureSampler* pSampler = detail::TextureSampler::GetSamplerFromTable(&resSampler);

    if (pSampler == nullptr) {
        return false;
    }

    bool isBound = false;

    if (vertexLocation != -1) {
        pCommandBuffer->SetTextureAndSampler(vertexLocation, gfx::ShaderStage_Vertex, slot,
                                             pSampler->GetDescriptorSlot());
        isBound = true;
    }

    if (pixelLocation != -1) {
        pCommandBuffer->SetTextureAndSampler(pixelLocation, gfx::ShaderStage_Pixel, slot,
                                             pSampler->GetDescriptorSlot());
        isBound = true;
    }

    return isBound;
}

/**
 * Registers the system textures to a descriptor pool.
 * @param pFunc function registering one texture view
 * @param pUserData user data passed to the function
 */
void System::RegisterTextureViewToDescriptorPool(RegisterTextureViewSlot pFunc,
                                                 void* pUserData) {
    detail::RegisterCurlNoiseTextureViewToDescriptorPool(pFunc, pUserData);
}

/**
 * Unregisters the system textures from a descriptor pool.
 * @param pFunc function unregistering one texture view
 * @param pUserData user data passed to the function
 */
void System::UnregisterTextureViewFromDescriptorPool(UnregisterTextureViewSlot pFunc,
                                                     void* pUserData) {
    detail::UnRegisterCurlNoiseTextureViewToDescriptorPool(pFunc, pUserData);
}

/**
 * Registers the system samplers to a descriptor pool.
 * @param pFunc function registering one sampler
 * @param pUserData user data passed to the function
 */
void System::RegisterSamplerToDescriptorPool(RegisterSamplerSlot pFunc, void* pUserData) {
    detail::TextureSampler::RegisterSamplerToDescriptorPool(reinterpret_cast<void*>(pFunc),
                                                            pUserData);
}

/**
 * Unregisters the system samplers from a descriptor pool.
 * @param pFunc function unregistering one sampler
 * @param pUserData user data passed to the function
 */
void System::UnregisterSamplerFromDescriptorPool(UnregisterSamplerSlot pFunc, void* pUserData) {
    detail::TextureSampler::UnregisterSamplerFromDescriptorPool(reinterpret_cast<void*>(pFunc),
                                                                pUserData);
}

}  // namespace vfx
}  // namespace nn
