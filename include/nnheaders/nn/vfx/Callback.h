/**
 * @file Callback.h
 * @brief VFX callback arguments and callback sets.
 */

#pragma once

#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_GpuAddress.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/vfx_Buffer.h>

namespace nn {
namespace vfx {

class Emitter;
class System;
struct EmitterResource;
struct EmitterAnimValue;
struct EmitReservationInfo;

namespace detail {
class EmitterCalculator;
class TextureSampler;
struct Particle;
struct ParentParticleData;
class Shader;
struct ParticleProperty;
struct ResFieldCustom;
struct ResAnim8KeyParamSet;
struct ResAnim8KeyParam;
struct ParticleData;
}  // namespace detail

/** How the buffers of an emitter are swapped before it is calculated. */
enum BufferSwapMode {
    BufferSwapMode_None = 0,
    BufferSwapMode_Swap = 1,
    BufferSwapMode_Auto = 2,
};

/** Number of per-frame copies kept of multi-buffered GPU data. */
enum BufferingMode {
    BufferingMode_Single = 1,
    BufferingMode_Double = 2,
    BufferingMode_Triple = 3,
};

class TemporaryBuffer {
public:
    TemporaryBuffer() { Invalidate(); }

    void Initialize(gfx::Device* pDevice, size_t size, BufferingMode bufferingMode);
    void Invalidate();
    void Finalize(gfx::Device* pDevice);
    void Swap();
    void* Map(gfx::GpuAddress* pAddress, size_t size);
    void Unmap();

    u8 _0[0x38];
    detail::Buffer m_Buffer[3];
};

static_assert(sizeof(TemporaryBuffer) == 0x548);

enum CustomShaderConstantBufferIndex {
    CustomShaderConstantBufferIndex_0,
    CustomShaderConstantBufferIndex_1,
    CustomShaderConstantBufferIndex_2,
    CustomShaderConstantBufferIndex_3,
};

enum CustomShaderTextureType {
    CustomShaderTextureType_0,
    CustomShaderTextureType_1,
    CustomShaderTextureType_2,
    CustomShaderTextureType_3,
    CustomShaderTextureType_4,
    CustomShaderTextureType_5,
    CustomShaderTextureType_6,
    CustomShaderTextureType_7,
    CustomShaderTextureType_8,
    CustomShaderTextureType_9,
    CustomShaderTextureType_10,
    CustomShaderTextureType_11,
    CustomShaderTextureType_12,
    CustomShaderTextureType_28 = 28,
    CustomShaderTextureType_29,
};

enum DrawPathCallbackId {
    DrawPathCallbackId_0,
    DrawPathCallbackId_1,
};

enum DrawPathFlag : u64 {
    DrawPathFlag_None = 0,
};

struct ViewParam;

namespace detail {
struct SortData;
}  // namespace detail

struct DrawParameterArg {
    s32 m_ProcessingIndex;
    ViewParam* m_pViewParam;
    u32 m_ViewFlag;
    s32 m_14;
    gfx::DescriptorSlot m_FrameBufferTextureSlot;
    gfx::DescriptorSlot m_DepthBufferTextureSlot;
    gfx::GpuAddress* m_pViewGpuAddress;
    TemporaryBuffer* m_pTemporaryBuffer;
    detail::SortData* m_pParticleSortBuffer;
};

struct EndianFlipArg;

struct EmitterInitializeArg {
    Emitter* pEmitter;
};

struct EmitterPreCalculateArg {
    Emitter* pEmitter;
    bool isBufferSwapped;
};

struct EmitterPostCalculateArg {
    Emitter* pEmitter;
};

struct EmitterMatrixSetArg {
    Emitter* pEmitter;
};

struct EmitterFinalizeArg {
    Emitter* pEmitter;
};

enum ShaderType {
    ShaderType_Normal,
    ShaderType_User1,
    ShaderType_User2,
    ShaderType_Compute,
};

struct EmitterDrawArg {
    gfx::CommandBuffer* pCommandBuffer;
    Emitter* pEmitter;
    ShaderType shaderType;
    void* pUserParam;
    DrawParameterArg* pDrawParameterArg;
};

struct ParticleCalculateArgImpl {
    void* pUserData;
    void* pUserData2;
    Emitter* pEmitter;
    f32 time;
    f32 life;
    s32 particleIndex;
};

class RenderStateSetArg {
public:
    detail::Shader* GetShader();
    System* GetSystem();

    gfx::CommandBuffer* pCommandBuffer;
    Emitter* pEmitter;
    s32 shaderType;
    void* pUserParam;
    bool isComputeShader;
    DrawParameterArg* pDrawParameterArg;
};

bool InvokeRenderStateSetCallback(RenderStateSetArg& rArg);

typedef void (*EndianFlipCallback)(EndianFlipArg& rArg);
typedef bool (*EmitterInitializeCallback)(EmitterInitializeArg& rArg);
typedef void (*EmitterPreCalculateCallback)(EmitterPreCalculateArg& rArg);
typedef void (*EmitterMatrixSetCallback)(EmitterMatrixSetArg& rArg);
typedef void (*EmitterPostCalculateCallback)(EmitterPostCalculateArg& rArg);
typedef bool (*EmitterDrawCallback)(EmitterDrawArg& rArg);
typedef void (*EmitterFinalizeCallback)(EmitterFinalizeArg& rArg);
typedef bool (*ParticleEmitCallback)(ParticleCalculateArgImpl& rArg);
typedef bool (*ParticleRemoveCallback)(ParticleCalculateArgImpl& rArg);
typedef void (*ParticleCalculateCallback)(ParticleCalculateArgImpl& rArg);
typedef bool (*RenderStateSetCallback)(RenderStateSetArg& rArg);
typedef void (*DrawPathRenderStateSetCallback)(RenderStateSetArg& rArg);
typedef bool (*CustomFieldCallback)(util::neon::Vector3fType* pPos, util::neon::Vector3fType* pVel,
                                    f32* pTime, f32* pLife, Emitter* pEmitter,
                                    const detail::ParticleProperty* pProperty,
                                    const detail::ResFieldCustom* pField, int particleIndex);

void EndianFlipCallbackImpl(EndianFlipArg& rArg);
void BindReservedCustomShaderConstantBuffer(RenderStateSetArg& rArg);

struct CallbackSet {
    CallbackSet()
        : endianFlip(nullptr), emitterInitialize(nullptr), emitterPreCalculate(nullptr),
          emitterMatrixSet(nullptr), emitterPostCalculate(nullptr), emitterDraw(nullptr),
          emitterFinalize(nullptr), particleEmit(nullptr), particleRemove(nullptr),
          particleCalculate(nullptr), renderStateSet(nullptr) {}

    EndianFlipCallback endianFlip;
    EmitterInitializeCallback emitterInitialize;
    EmitterPreCalculateCallback emitterPreCalculate;
    EmitterMatrixSetCallback emitterMatrixSet;
    EmitterPostCalculateCallback emitterPostCalculate;
    EmitterDrawCallback emitterDraw;
    EmitterFinalizeCallback emitterFinalize;
    ParticleEmitCallback particleEmit;
    ParticleRemoveCallback particleRemove;
    ParticleCalculateCallback particleCalculate;
    RenderStateSetCallback renderStateSet;
};

static_assert(sizeof(CallbackSet) == 0x58);

namespace detail {

class Shader {
public:
    void BindCustomShaderUniformBlock(gfx::CommandBuffer* pCommandBuffer,
                                      CustomShaderConstantBufferIndex index,
                                      gfx::GpuAddress* pAddress, size_t size);

    s32 GetCustomTexturePixelLocation(CustomShaderTextureType index) const {
        return m_CustomTextureLocation[index][0];
    }

    s32 GetCustomTextureVertexLocation(CustomShaderTextureType index) const {
        return m_CustomTextureLocation[index][1];
    }

    s32 GetCustomTexturePixelLocationByIndex(s32 index) const {
        return m_CustomTextureLocation[index][0];
    }

    s32 GetCustomTextureVertexLocationByIndex(s32 index) const {
        return m_CustomTextureLocation[index][1];
    }

    const gfx::Shader* GetGfxShader() const { return m_pGfxShader; }

    u8 _0[0x8];
    gfx::Shader* m_pGfxShader;
    u8 _10[0x14 - 0x10];
    s32 m_CustomTextureLocation[32][2];
    u8 _114[0x144 - 0x114];
    s32 m_VertexEmitterPluginConstantBufferLocation;
    s32 m_PixelEmitterPluginConstantBufferLocation;
};

/** Compute shader of a GPU stream-out emitter. */
class ComputeShader {
public:
    s32 GetViewParamLocation() const { return m_ViewParamLocation; }

    u8 _0[0x8];
    s32 m_ViewParamLocation;
};

/** Sampler key read from the emitter resource. */
struct ResTextureSampler {
    u8 _0[0x8];
    u8 filter;
    u8 wrapU;
    u8 wrapV;
    u8 wrapW;
    f32 maxLod;
    f32 lodBias;
    u8 _14[0x20 - 0x14];
};

class EmitterCalculator {
public:
    /** Emits from a shape: position and velocity of one particle. */
    typedef bool (*EmitFunction)(util::Vector3fType* pOutPos, util::Vector3fType* pOutVec,
                                 Emitter* pEmitter, int emitIndex, int emitCount, f32 random,
                                 EmitterAnimValue* pAnimValue);

    explicit EmitterCalculator(System* pSystem);
    ~EmitterCalculator();

    void ApplyEmitterAnimation(Emitter* pEmitter, util::Matrix4x3fType* pOutMatrixSrt,
                               util::Matrix4x3fType* pOutMatrixRt);
    void TryEmitParticle(f32* pIntervalCounter, f32* pEmitCounter, f32* pInterval,
                         u8* pIsEmitted, Emitter* pEmitter, f32 frameRate, bool isSearchFreeSlot);
    void UpdateEmitterLocalVec(util::Vector3fType* pOut, Emitter* pEmitter) const;
    void UpdateCurrentParticleGpuBufferForCpuEmitter(Emitter* pEmitter);
    void UpdateEmitterMatrix(Emitter* pEmitter);
    bool Calculate(Emitter* pEmitter, f32 frameRate, BufferSwapMode swapMode, bool isFade,
                   bool isEmit, bool isCalculateParticle);
    void CalculateParticle(Emitter* pEmitter);
    void MakeDynamicConstantBuffer(Emitter* pEmitter, f32 frameRate, f32 accumulatedFrameRate);
    void CalculateParticleInfo(Particle* pParticle, Emitter* pEmitter, f32 time, f32 life,
                               int particleIndex);
    void InheritParentParticleInfo(Emitter* pEmitter, int particleIndex);
    int EmitBySearchOrder(u8* pIsEmitted, Emitter* pEmitter, int emitCount,
                          bool isSearchFreeSlot, const EmitReservationInfo* pReservationInfo);
    bool InitializeParticle(Emitter* pEmitter, int particleIndex, ParticleData* pData,
                            ParentParticleData* pParentData, ParticleProperty* pProperty,
                            int propertyIndex, int emitIndex, int emitCount, f32 random,
                            const EmitReservationInfo* pReservationInfo);
    int Emit(u8* pIsEmitted, Emitter* pEmitter, int emitCount, bool isSearchFreeSlot,
             const EmitReservationInfo* pReservationInfo);

    static bool CalculateEmitPoint(util::Vector3fType* pOutPos, util::Vector3fType* pOutVec,
                                   Emitter* pEmitter, int emitIndex, int emitCount, f32 random,
                                   EmitterAnimValue* pAnimValue);
    static bool CalculateEmitCircle(util::Vector3fType* pOutPos, util::Vector3fType* pOutVec,
                                    Emitter* pEmitter, int emitIndex, int emitCount, f32 random,
                                    EmitterAnimValue* pAnimValue);

    static EmitFunction g_EmitFunctions[];

private:
    void SetEmitterLocalMatrix(Emitter* pEmitter);
    void ApplyParentEmitterMatrix(Emitter* pEmitter);

public:

    void CalculateComputeShader(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                                const ComputeShader* pComputeShader, int bufferIndex,
                                bool isDoComputeShaderProcess, void* pUserParam, bool isBatch);
    void Draw(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter, void* pUserParam,
              DrawParameterArg* pDrawParameterArg);

    void DrawEmitterUsingBoundShader(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                                     Shader* pShader, void* pUserParam,
                                     DrawParameterArg* pDrawParameterArg);

    void CalculateField8KeyAnim(util::Float3* pOut, const Emitter* pEmitter,
                                const ResAnim8KeyParamSet& rAnim, int particleIndex,
                                f32 time);
    void Calculate8KeyAnim(util::Float3* pOut, const ResAnim8KeyParam& rAnim, int keyNum,
                           f32 random, f32 time, f32 loopRate, f32 startRandom, f32 life);

    void CalculateParticleBehaviorFieldGpuNoise(util::Vector3fType* pVec, Emitter* pEmitter,
                                                const ParticleProperty* pProperty,
                                                int particleIndex, f32 time);
    void CalculateParticleBehaviorFieldRandomSimple(util::Vector3fType* pVec,
                                                    Emitter* pEmitter, int particleIndex,
                                                    f32 time);
    void CalculateParticleBehaviorFieldMagnet(util::Vector3fType* pPos,
                                              util::Vector3fType* pVec, Emitter* pEmitter,
                                              const ParticleProperty* pProperty,
                                              int particleIndex, f32 time);
    void CalculateParticleBehaviorFieldSpin(util::Vector3fType* pPos,
                                            const Emitter* pEmitter,
                                            ParticleProperty* pProperty, int particleIndex,
                                            f32 time);
    void CalculateParticleBehaviorFieldCollision(util::Vector3fType* pPos,
                                                 util::Vector3fType* pVec, f32* pLife,
                                                 Emitter* pEmitter,
                                                 ParticleProperty* pProperty,
                                                 int particleIndex,
                                                 ParticleData* pParticleData, f32 time);
    void CalculateParticleBehaviorFieldConvergence(util::Vector3fType* pPos,
                                                   Emitter* pEmitter,
                                                   const ParticleProperty* pProperty,
                                                   int particleIndex, f32 time);
    void CalculateParticleBehaviorFieldPosAdd(util::Vector3fType* pPos, Emitter* pEmitter,
                                              const ParticleProperty* pProperty,
                                              int particleIndex, f32 time);
    void CalculateParticleBehavior(util::Vector3fType* pPos, util::Vector3fType* pVec,
                                   f32* pTime, f32* pLife, Emitter* pEmitter,
                                   int particleIndex, f32 time,
                                   const util::Vector3fType& rPos,
                                   const util::Vector3fType& rVec);

    void CalculateParticleScaleVecFromTime(util::Vector3fType* pOut,
                                           const EmitterResource* pEmitterRes,
                                           const util::Vector4fType& rScale,
                                           const util::Vector4fType& rRandom, f32 life,
                                           f32 time);
    void CalculateParticleScaleVecFromFrame(util::Vector3fType* pOut,
                                            const EmitterResource* pEmitterRes,
                                            const util::Vector4fType& rScale,
                                            const util::Vector4fType& rRandom, f32 life,
                                            f32 time);
    void CalculateParticleColor0VecFromTime(util::Vector4fType* pOut,
                                            const EmitterResource* pEmitterRes,
                                            const util::Vector4fType& rRandom,
                                            const util::Vector4fType& rColor,
                                            const util::Vector3fType& rEmitterColor,
                                            f32 alpha, f32 life, f32 time);
    void CalculateParticleColor0RawValue(util::Vector4fType* pOut,
                                         const EmitterResource* pEmitterRes,
                                         const util::Vector4fType& rRandom, f32 life,
                                         f32 time);
    void CalculateParticleColor1VecFromTime(util::Vector4fType* pOut,
                                            const EmitterResource* pEmitterRes,
                                            const util::Vector4fType& rRandom,
                                            const util::Vector4fType& rColor,
                                            const util::Vector3fType& rEmitterColor,
                                            f32 alpha, f32 life, f32 time);
    void CalculateParticleColor1RawValue(util::Vector4fType* pOut,
                                         const EmitterResource* pEmitterRes,
                                         const util::Vector4fType& rRandom, f32 life,
                                         f32 time);
    void CalculateRotationMatrix(util::Vector3fType* pOut,
                                 const EmitterResource* pEmitterRes,
                                 const util::Vector4fType& rRotate,
                                 const util::Vector4fType& rRandom, f32 time);
    void CalculateRotationMatrix(util::Vector3fType* pOut,
                                 const EmitterResource* pEmitterRes,
                                 const util::Vector4fType& rRotate);
    static void MakeRotationMatrixXYZ(util::neon::MatrixRowMajor4x4fType* pOutMatrix,
                                      const util::Vector3fType& rRotate);

    System* m_pSystem;
    TextureSampler* m_pDefaultSampler;
    TextureSampler* m_pMirrorSampler;
    ResTextureSampler m_DefaultSamplerRes;
    ResTextureSampler m_MirrorSamplerRes;
    u8 _58[0x60 - 0x58];
};

static_assert(sizeof(EmitterCalculator) == 0x60);

}  // namespace detail

}  // namespace vfx
}  // namespace nn
