/**
 * @file EmitterSet.h
 * @brief VFX emitter set.
 */

#pragma once

#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/Callback.h>
#include <nn/vfx/vfx_VertexBuffer.h>

namespace nn {
namespace vfx {
class EmitterSet;
class System;
class Heap;
struct EmitterResource;
struct EmitReservationInfo;
struct EmitterCalculateLodArg;
struct EmitterDrawCullArg;
struct DrawEmitterProfilerArg;

enum EmitterCalculationResult {
    EmitterCalculationResult_Continue,
    EmitterCalculationResult_Skip,
};

typedef EmitterCalculationResult (*EmitterCalculateLodCallback)(EmitterCalculateLodArg& rArg);
typedef bool (*EmitterDrawCullingCallback)(EmitterDrawCullArg& rArg);
typedef void (*DrawEmitterProfileCallback)(DrawEmitterProfilerArg& rArg);

namespace detail {
struct ResEmitterSet {
    u8 _0[0x10];
    char m_Name[0x48];
    u32 m_ClipRadius;
};
}  // namespace detail

struct EmitterSetResource {
    const char* GetName() const { return m_ResEmitterSet->m_Name; }

    s32 m_EmitterNum;
    u8 _4[0x10 - 0x4];
    detail::ResEmitterSet* m_ResEmitterSet;
    u8 _18[0x29 - 0x18];
    bool m_IsLoop;
    bool m_IsInfinity;
    u8 _2b[0x38 - 0x2b];
};

namespace detail {
class EmitterCalculator;
struct ResEmitter;
struct ParticleAttribute;

/** Per-emitter random number generator state. */
class Random {
public:
    Random();

    static void Initialize();
    static void Finalize();

    /** @return the next entry of the shared random vector table */
    const util::Vector3fType& GetVec3() { return g_Vec3Table[m_Vec3RndIdx++ & 0x1ff]; }

    /** @return the next entry of the shared random unit vector table */
    const util::Vector3fType& GetNormalizedVec3() {
        return g_NormalizedVec3Table[m_NormalizedVec3RndIdx++ & 0x1ff];
    }

    /** @return a random value in [0, 1) */
    f32 GetF32() {
        u32 value = m_RandomSeed;
        m_RandomSeed = value * 1103515245 + 12345;
        return static_cast<f32>(value) * (1.0f / 4294967296.0f);
    }

    /**
     * @param min the smallest value
     * @param max the largest value
     * @return a random value in [min, max)
     */
    f32 GetF32Range(f32 min, f32 max) { return GetF32() * (max - min) + min; }

    /**
     * @param max the end of the range
     * @return a random integer in [0, max)
     */
    s32 GetS32(s32 max) {
        u32 value = m_RandomSeed;
        m_RandomSeed = value * 1103515245 + 12345;
        return static_cast<s32>((static_cast<u64>(value) * static_cast<s64>(max)) >> 32);
    }

    static util::Vector3fType* g_Vec3Table;
    static util::Vector3fType* g_NormalizedVec3Table;

    u16 m_Vec3RndIdx;
    u16 m_NormalizedVec3RndIdx;
    u32 m_RandomSeed;
};

/** Arrays holding the per-particle simulation data. */
struct ParticleProperty {
    util::Float4* pPos;
    util::Float4* pVec;
    util::Float4* pPosDelta;
    union {
        util::Float4* pRandom;
        util::Float4* pScale;
    };
    util::Float4* pAnimRandom;
    util::Float4* pRotate;
    util::Float4* pColor0;
    util::Float4* pColor1;
    util::Float4* pEmitterMatrixRow[3];
};
}  // namespace detail

/** Current values of the emitter animations. */
struct EmitterAnimValue {
    union {
        util::Float3 values[14];
        struct {
            util::Float3 scale;
            util::Float3 rotate;
            util::Float3 translate;
            util::Float3 color0;
            util::Float3 color1;
            util::Float3 emissionRate;
            util::Float3 particleLife;
            util::Float3 alpha0;
            util::Float3 alpha1;
            util::Float3 allDirectionalVel;
            util::Float3 directionalVel;
            util::Float3 particleScale;
            util::Float3 emitterVolumeScale;
            util::Float3 gravityScale;
        };
    };
};

static_assert(sizeof(EmitterAnimValue) == 0xa8);

/** Emission requested ahead of time by a manual emitter set. */
struct EmitReservationInfo {
    bool isUseMatrix;
    u8 _1[0x10 - 0x1];
    util::Matrix4x3fType matrix;
    void* pUserData;
    f32 emitRatio;
    u8 _5c[0x60 - 0x5c];
};

static_assert(sizeof(EmitReservationInfo) == 0x60);

class Emitter {
public:
    void Reset();
    bool SwapBuffer(BufferSwapMode swapMode);
    void UpdateByEmit(f32* pInterval);
    bool IsManualEmitterReadyToExit() const;
    bool InitializeCustomConstantBuffer(int index, size_t size);
    bool InitializeCustomAttribute(int bufferCount, size_t size);

    /** @return the heap the per-emitter allocations are made from */
    Heap* GetDynamicHeap() { return reinterpret_cast<Heap*>(m_DynamicHeap); }

    /** @return the CPU state of the particles */
    detail::ParticleData* GetParticleData() const {
        return reinterpret_cast<detail::ParticleData*>(m_ParticleAttr);
    }

    f32 GetFrame() const { return m_Frame; }
    Emitter* GetNextEmitter() const { return m_Next; }

    /** @return the rotation and translation matrix (its translation is m_EmitterLocalPos) */
    const util::Matrix4x3fType& GetMatrixRt() const {
        return *reinterpret_cast<const util::Matrix4x3fType*>(m_MatrixRtAxis);
    }

    /** @return the particle arrays kept in CPU memory */
    detail::ParticleProperty* GetCpuParticleProperty() {
        return reinterpret_cast<detail::ParticleProperty*>(&m_ParticlePos);
    }

    bool m_IsChildEmitter;
    u8 m_IsEmitted;
    bool m_IsCalculated;
    bool m_IsEmitEnabled;
    u8 _4[0x5 - 0x4];
    u8 m_IsParentFinished;
    u8 _6[0x7 - 0x6];
    bool m_IsDead;
    u8 _8[0xc - 0x8];
    s32 m_UpdatedAnimNum;
    u8 _10[0x14 - 0x10];
    s32 m_EmitterCreateId;
    u8 _18[0x1c - 0x18];
    s32 m_ParticleCreateId;
    u8 _20[0x28 - 0x20];
    s32 m_ParticleNum;
    s32 m_AliveParticleNum;
    u32 m_BufferIndex;
    u8 _34[0x44 - 0x34];
    f32 m_Frame;
    f32 m_SystemFrame;
    f32 m_FrameRate;
    f32 m_AccumulatedFrameRate;
    f32 m_EmitIntervalCounter;
    f32 m_EmitCounter;
    f32 m_EmitInterval;
    f32 m_EmitRatio;
    u8 _64[0x68 - 0x64];
    f32 m_LastEmitFrame;
    f32 m_ParticleLifeScale;
    union {
        f32 m_FadeOutRatio;
        f32 m_EmitterAnimScale;
    };
    union {
        f32 m_FadeInRatio;
        f32 m_EmitterSetScale;
    };
    EmitterSet* m_EmitterSet;
    Emitter* m_Next;
    u8 _88[0x90 - 0x88];
    Emitter* m_NextComputeShaderEmitter;
    detail::EmitterCalculator* m_EmitterCalculator;
    detail::ResEmitter* m_pEmitterData;
    detail::Random m_Random;
    detail::ParticleAttribute* m_ParticleAttr;
    detail::ParentParticleData* m_pParentParticleData;
    detail::ParticleProperty* m_pGpuParticleProperty;
    u8 _c8[0x1d0 - 0xc8];
    util::Float4* m_ParticlePos;
    util::Float4* m_ParticleVec;
    util::Float4* m_ParticlePosDelta;
    union {
        util::Float4* m_ParticleRandom;
        util::Float4* m_ParticleScale;
    };
    util::Float4* m_ParticleAnimRandom;
    util::Float4* m_ParticleRotate;
    util::Float4* m_ParticleColor0;
    util::Float4* m_ParticleColor1;
    util::Float4* m_ParticleEmitterMatrixRow[3];
    u8 _228[0x230 - 0x228];
    s32 m_MaxParticleNum;
    s32 m_ParticleHead;
    EmitterResource* m_pEmitterRes;
    EmitterResource* m_ChildEmitterRes[16];
    util::Matrix4x3fType m_ResMatrixSrt;
    util::Matrix4x3fType m_ResMatrixRt;
    util::Matrix4x3fType m_MatrixSrt;
    union {
        util::Matrix4x3fType m_MatrixRt;
        struct {
            util::Vector3fType m_MatrixRtAxis[3];
            util::Vector3fType m_EmitterLocalPos;
        };
    };
    util::Vector3fType m_EmitterPrevPos;
    util::Vector3fType m_EmitterLocalVec;
    CallbackSet* m_pCallbackSet[3];
    DrawPathRenderStateSetCallback m_RenderStateSetCallback;
    u8 _400[0x408 - 0x400];
    void* m_UserData;
    void* m_UserData2;
    void* m_pEmitterPluginUserData;
    util::Vector4fType m_Color0;
    util::Vector4fType m_Color1;
    Emitter* m_ChildEmitter[16];
    u8 _4c0[0x548 - 0x4c0];
    Emitter* m_pParentEmitter;
    s32 m_ParentEmitterCreateId;
    s32 m_ParentParticleCreateId;
    s32 m_ParentParticleIndex;
    f32 m_ParentParticleLife;
    f32 m_ParentParticleBirthTime;
    f32 m_ParentParticleTime;
    u8 _568[0x570 - 0x568];
    util::Vector3fType m_ParentParticleLocalPos;
    util::Vector3fType m_ParentParticleLocalVec;
    util::Vector4fType m_ParentParticleScale;
    util::Vector4fType m_ParentParticleRotate;
    util::Vector4fType m_ParentParticleRandom;
    util::Vector3fType m_ParentParticleWorldPos;
    util::Vector3fType m_ParentParticleWorldVec;
    u8 _5e0[0x5e4 - 0x5e0];
    u32 m_GroupBitFlag;
    util::Vector3fType* m_pParentParticlePos;
    util::Vector3fType* m_pParentParticleVec;
    s32 m_ParentParticleIndexForEmit;
    bool m_IsSoloFade;
    u8 _5fd[0x600 - 0x5fd];
    union {
        EmitterAnimValue m_EmitterAnimValue;
        struct {
            u8 _600[0x69c - 0x600];
            f32 m_GravityScale;
        };
    };
    bool m_IsEmitterAnimEnd[14];
    u8 _6b6[0x6b8 - 0x6b6];
    s32 m_LastEmitIndex;
    bool m_IsParticleFull;
    bool m_IsSequentialEmit;
    u8 _6be[0x6c0 - 0x6be];
    void* m_ConstantBuffer[3];
    detail::Buffer* m_pConstantBuffer;
    u8 _6e0[0x700 - 0x6e0];
    detail::BufferAllocator* m_pBufferAllocator;
    u8 _708[0x7a8 - 0x708];
    detail::Attribute m_Attribute;
    u8 m_DynamicHeap[0x18];
};

static_assert(sizeof(Emitter) == 0x840);

class EmitterSet {
public:
    bool Initialize(s32 emitterSetId, s32 createId, s32 resourceId, s32 groupId,
                    s32 maxParticleCount, Heap* pHeap);
    void Finalize();
    void Reset();
    void Calculate(f32 frameRate, BufferSwapMode swapMode, bool isForceCalculate,
                   EmitterCalculateLodCallback pLodCallback);
    void Draw(gfx::CommandBuffer* pCommandBuffer, u32 drawPathFlag, bool isDoComputeShaderProcess,
              void* pUserParam, DrawParameterArg* pDrawParameterArg,
              EmitterDrawCullingCallback pCullingCallback,
              DrawEmitterProfileCallback pProfileCallback);
    bool UpdateFromResource(EmitterResource* pEmitterResource);
    void OverwriteCustomActionCallbackSet(CallbackSet* pCallbackSet);
    void Kill(bool isImmediate);
    void Fade();
    Emitter* GetAliveEmitter(s32 index) const;
    Emitter* CreateEmitter(const EmitterResource* pEmitterRes, int resourceIndex, Emitter* pParent,
                           int childIndex);
    void SetMatrix(const util::Matrix4x3fType& rMatrix);
    void ForceCalculate(s32 frame);

    s32 GetCreateId() const { return m_CreateId; }
    void SetDirectionalVel(f32 vel) { m_DirectionalVel = vel; }
    bool IsFadeRequest() const { return m_IsFadeRequest; }
    bool IsAlive() const { return m_EmitterNum > 0 && m_IsAlive; }
    bool IsCalcEnable() const { return m_IsCalcEnable; }
    bool IsDrawEnable() const { return m_IsDrawEnable; }
    void SetCalcEnable(bool isEnable) { m_IsCalcEnable = isEnable; }
    void SetDrawEnable(bool isEnable) { m_IsDrawEnable = isEnable; }
    void SetUserData(u64 userData) { m_UserData = userData; }
    u64 GetUserData() const { return m_UserData; }
    EmitterSet* GetNext() const { return m_Next; }
    u32 GetDrawPathFlag() const { return m_DrawPathFlag; }
    const EmitterSetResource* GetEmitterSetResource() const { return m_EmitterSetResource; }
    const util::Vector3fType& GetClipPos() const {
        return reinterpret_cast<const util::Vector3fType&>(m_MatrixSrt._m.val[3]);
    }

    void SetEmissionRatioScale(f32 ratio);
    void SetParticleLifeScale(f32 scale);
    void SetEmitterColor0(const util::Vector4fType& rColor);
    void SetEmitterColor1(const util::Vector4fType& rColor);

    s32 GetEmitterNum() const { return m_EmitterNum; }
    const util::Matrix4x3fType& GetMatrixRt() const { return m_MatrixRt; }
    const util::Vector3fType& GetAutoCalcScale() const { return m_AutoCalcScale; }
    const util::Vector3fType& GetParticleScale() const { return m_ParticleScale; }
    const util::Vector3fType& GetParticleScaleForCalc() const { return m_ParticleScaleForCalc; }

    void SetMatrixAndScale(const util::Matrix4x3fType& rMatrix, const util::Vector3fType& rScale) {
        m_MatrixSrt._m.val[0] = vmulq_n_f32(rMatrix._m.val[0], vgetq_lane_f32(rScale._v, 0));
        m_MatrixSrt._m.val[1] = vmulq_n_f32(rMatrix._m.val[1], vgetq_lane_f32(rScale._v, 1));
        m_MatrixSrt._m.val[2] = vmulq_n_f32(rMatrix._m.val[2], vgetq_lane_f32(rScale._v, 2));
        m_MatrixSrt._m.val[3] = rMatrix._m.val[3];
        m_MatrixRt = rMatrix;
        m_IsMatrixUpdated = 1;
        m_AutoCalcScale = rScale;
        m_ParticleScaleForCalc._v = vmulq_f32(m_AutoCalcScale._v, m_ParticleScale._v);
    }

    void SetEmitterScale(const util::Vector3fType& rScale) { m_EmitterVolumeScale = rScale; }

    void SetEmitterVolumeScale(const util::Vector3fType& rScale) { m_EmitterVolumeScale = rScale; }

    void SetParticleScale(const util::Vector3fType& rScale) {
        m_ParticleScale = rScale;
        m_ParticleScaleForCalc._v = vmulq_f32(m_AutoCalcScale._v, rScale._v);
    }

    void SetColor(const util::Vector4fType& rColor) { m_Color = rColor; }

    void SetColor(f32 r, f32 g, f32 b) {
        m_Color._v = vsetq_lane_f32(r, m_Color._v, 0);
        m_Color._v = vsetq_lane_f32(g, m_Color._v, 1);
        m_Color._v = vsetq_lane_f32(b, m_Color._v, 2);
    }

    void SetAlpha(f32 alpha) { m_Color._v = vsetq_lane_f32(alpha, m_Color._v, 3); }

    u8 _0[0x9];
    bool m_IsFadeRequest;
    bool m_IsCalcEnable;
    bool m_IsDrawEnable;
    bool m_IsAlive;
    u8 _d[0xe - 0xd];
    bool m_IsManualEmission;
    bool m_IsDelayCreate;
    u8 m_DrawPriority;
    u8 _11[0x14 - 0x11];
    s32 m_GroupId;
    s32 m_IsMatrixUpdated;
    u8 _1c[0x20 - 0x1c];
    System* m_System;
    s32 m_EmitterSetId;
    s32 m_CreateId;
    s32 m_ResourceId;
    u8 _34[0x3c - 0x34];
    u32 m_RenderingFlag0;
    u32 m_RenderingFlag1;
    u8 _44[0x48 - 0x44];
    u32 m_ViewFlag;
    u8 _4c[0x50 - 0x4c];
    f32 m_EmissionRatioScale;
    u8 _54[0x58 - 0x54];
    f32 m_ParticleLifeScale;
    u8 _5c[0x60 - 0x5c];
    util::Matrix4x3fType m_MatrixSrt;
    util::Matrix4x3fType m_MatrixRt;
    util::Vector3fType m_EmitterVolumeScale;
    util::Vector3fType m_AutoCalcScale;
    util::Vector4fType m_Color;
    util::Float3 m_ParticleInitRotate;
    u8 _11c[0x120 - 0x11c];
    util::Vector3fType m_ParticleScale;
    util::Float3 m_EmissionParticleScale;
    u8 _13c[0x140 - 0x13c];
    util::Vector3fType m_ParticleScaleForCalc;
    u8 _150[0x174 - 0x150];
    s32 m_BufferIndex;
    u8 _178[0x17c - 0x178];
    s32 m_EmitterNum;
    s32 m_ProcessingCount[11];
    u8 _1ac[0x1b0 - 0x1ac];
    u64 m_AllocatedSize;
    u8 _1b8[0x1c0 - 0x1b8];
    u64 m_UserData;
    u8 _1c8[0x1d0 - 0x1c8];
    EmitterSet* m_Next;
    EmitterSet* m_Prev;
    u8 _1e0[0x1e8 - 0x1e0];
    f32 m_EmissionSpeedScale;
    f32 m_VelocityRandomScale;
    util::Vector3fType m_AddVelocity;
    u8 _200[0x210 - 0x200];
    f32 m_DirectionalVel;
    u8 _214[0x230 - 0x214];
    u32 m_DrawPathFlag;
    EmitterSetResource* m_EmitterSetResource;
    u8 _240[0x250 - 0x240];
    EmitReservationInfo* m_pEmitReservationInfo;
    s32 m_MaxEmitCountPerFrame;
    s32 m_EmitReservationNum;
    s32 m_ManualEmitterSetLife;
    u8 _264[0x265 - 0x264];
    bool m_IsEmitDistanceEnabled;
    u8 _266[0x270 - 0x266];
};

static_assert(sizeof(EmitterSet) == 0x270);
}  // namespace vfx
}  // namespace nn
