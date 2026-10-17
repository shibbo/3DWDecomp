/**
 * @file vfx_SuperStripe.h
 * @brief Super stripe emitter plugin: ribbons drawn along the recent positions of every particle.
 */

#pragma once

#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/Callback.h>
#include <nn/vfx/vfx_VertexBuffer.h>

namespace nn {
namespace vfx {

class Emitter;
class Heap;
class System;

namespace detail {

struct SuperStripeInstance;

/** Meshes a stripe is drawn with. */
enum StripeMeshType {
    StripeMeshType_Normal,
    StripeMeshType_Cross,
};

/** Ways the polygon of a stripe is oriented. */
enum SuperStripeType {
    SuperStripeType_Billboard,
    SuperStripeType_EmitterMatrix,
    SuperStripeType_EmitterUpDown,
    SuperStripeType_Ribbon,
};

/** Ways the texture coordinates of a stripe are calculated. */
enum StripeTexCoordType {
    StripeTexCoordType_Uniform,
    StripeTexCoordType_DistanceBased,
};

/** Emitter plugin data of a super stripe emitter. */
struct ResStripeSuper {
    u32 type;
    u32 isEmitterCoord;
    u32 meshType;
    s32 texCoordSource[3];
    f32 historyNum;
    u8 _1c[0x20 - 0x1c];
    f32 shaderParam0[2];
    s32 divideNum;
    u8 _2c[0x34 - 0x2c];
    f32 historyAirRegist;
    util::Float3 historyAcceleration;
    f32 turbulenceSpeed;
    f32 turbulenceAmplitude;
    util::Float3 turbulenceFrequency;
    u32 texCoordType;
    f32 shaderParam1[2];
};

/**
 * Emitter plugin drawing a ribbon along the history of the positions of every particle.
 */
class SuperStripeSystem {
public:
    /** One recorded position of a stripe. The records of a stripe form a ring. */
    struct History {
        util::Vector3fType pos;
        util::Vector3fType vec;
        util::Vector3fType dir;
        util::Vector3fType outer;
        util::Vector3fType emitterAxis;
        f32 scale;
        History* pNext;
        History* pPrev;
        u8 _68[0x70 - 0x68];
    };

    /** Per-emitter state of the plugin. */
    struct EmitterPluginUserData {
        util::Float4 shaderParam;
        bool isBufferAllocated;
        SuperStripeInstance* pDelayedStripeHead;
        s32 vertexNumPerStripe;
        s32 vertexNum;
        s32 stripeNum;
        s32 stripeIndex;
        u32 maxEmitRate;
        s32 maxParticleLife;
        BufferSide bufferSide;
    };

    /** One vertex of a stripe polygon. */
    struct VertexAttribute {
        util::Float4 pos;
        util::Float4 dir;
        util::Float4 outer;
        util::Float4 texCoord;
        util::Float4 emitterAxis;
    };

    /** Constant buffer bound when a stripe is drawn. */
    struct ConstantBufferObject {
        util::Vector4fType random;
        util::Float4 shaderParam;
        util::Vector4fType color0;
        util::Vector4fType color1;
        f32 time;
        f32 vertexNum;
        f32 meshType;
        f32 life;
        util::Float4 pos;
    };

    typedef ResStripeSuper ResourceType;
    typedef SuperStripeInstance StripeInstance;

    SuperStripeSystem(Heap* pHeap, System* pSystem, BufferingMode bufferingMode, int stripeNum);
    virtual ~SuperStripeSystem();

    static bool InitializeStripeEmitter(EmitterInitializeArg& rArg);
    static bool EmitStripe(ParticleCalculateArgImpl& rArg);
    static bool KillStripe(ParticleCalculateArgImpl& rArg);
    static void ParticleCalculateCallback(ParticleCalculateArgImpl& rArg);
    static void EmitterPreCalculateCallback(EmitterPreCalculateArg& rArg);
    static void EmitterPostCalculateCallback(EmitterPostCalculateArg& rArg);
    static bool EmitterDrawCallback(EmitterDrawArg& rArg);
    static void FinalizeStripeEmitter(EmitterFinalizeArg& rArg);

    static void InitializeSystem(Heap* pHeap, System* pSystem, BufferingMode bufferingMode,
                                 int stripeNum);
    static void FinalizeSystem(Heap* pHeap);

    bool AllocStripeSystemVertexBuffer(Emitter* pEmitter);

    static void UpdateStripeColor(ParticleCalculateArgImpl& rArg, SuperStripeInstance* pInstance);
    void CalculateStripe(ParticleCalculateArgImpl& rArg, SuperStripeInstance* pInstance);
    void UpdateHistory(ParticleCalculateArgImpl& rArg, SuperStripeInstance* pInstance,
                       ResStripeSuper* pRes, bool isHistoryUpdated);
    void MakeStripePolygon(VertexAttribute* pVertex, SuperStripeInstance* pInstance,
                           const Emitter* pEmitter, const ResStripeSuper* pRes,
                           f32 texCoordOffset);
    void MakeStripePolygonWithDivision(VertexAttribute* pVertex, SuperStripeInstance* pInstance,
                                       const Emitter* pEmitter, const ResStripeSuper* pRes,
                                       f32 texCoordOffset);
    void CalculateDelayedStripe(Emitter* pEmitter);
    void CalculateTextureOffsetUniform(util::Vector3fType* pOut, const ResStripeSuper* pRes,
                                       int index, int vertexNum, int historyNum,
                                       f32 texCoordOffset);
    void CalculateTextureOffsetDistanceBased(util::Vector3fType* pOut, const ResStripeSuper* pRes,
                                             int index, f32 length, f32 totalLength,
                                             f32 invTotalLength,
                                             const util::Float4& rPrevTexCoord);
    void MakeDefaultVertexAttribute(VertexAttribute* pVertex, int index,
                                    const util::Vector3fType& rPos,
                                    const util::Vector3fType& rDir,
                                    const util::Vector3fType& rOuter,
                                    const util::Vector3fType& rTexCoord, f32 width);

    static int GetExtendedEndTimeForOneTimeEmitter(Emitter* pEmitter);
    static int GetActualSuperStripeCalclationCount(const Emitter* pEmitter);
    static void MakeConstantBufferObject(ConstantBufferObject* pOut, const ResStripeSuper* pRes,
                                         const EmitterPluginUserData* pUserData,
                                         const SuperStripeInstance* pInstance,
                                         const Emitter* pEmitter, StripeMeshType meshType);
    static size_t GetWorkSize();
    static int GetProcessingStripeCount();

    static SuperStripeSystem* g_pStripeSystem;

    System* m_pSystem;
    Heap* m_pHeap;
    BufferingMode m_BufferingMode;
    s32 m_StripeNum;
    size_t m_StripeWorkSize;
    s32 m_StripeIndex;
    s32 m_ProcessingStripeCount;
    SuperStripeInstance* m_pStripeArray;
};

static_assert(sizeof(SuperStripeSystem) == 0x38);
static_assert(sizeof(SuperStripeSystem::History) == 0x70);
static_assert(sizeof(SuperStripeSystem::EmitterPluginUserData) == 0x40);
static_assert(sizeof(SuperStripeSystem::VertexAttribute) == 0x50);
static_assert(sizeof(SuperStripeSystem::ConstantBufferObject) == 0x60);

/** One stripe, following one particle. */
struct SuperStripeInstance {
    util::Vector4fType color0;
    util::Vector4fType color1;
    util::Vector4fType random;
    u8 _30[0x40 - 0x30];
    util::Vector3fType pos;
    SuperStripeSystem::History* pHistoryBuffer;
    SuperStripeSystem::History* pHistoryHead;
    SuperStripeSystem::History* pHistoryTail;
    SuperStripeInstance* pNext;
    s32 historyNum;
    s32 historyNumAtKill;
    s32 vertexNum;
    s32 index;
    f32 time;
    f32 life;
    f32 prevTime;
    f32 totalLength;
    bool isUsed;
};

static_assert(sizeof(SuperStripeInstance) == 0xa0);

}  // namespace detail
}  // namespace vfx
}  // namespace nn
