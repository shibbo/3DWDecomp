/**
 * @file vfx_EmitterCalc.h
 * @brief Per-frame emitter calculation: emitter matrices, emission and CPU particles.
 */

#pragma once

#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/Callback.h>
#include <nn/vfx/EmitterRes.h>
#include <nn/vfx/EmitterSet.h>

namespace nn {
namespace vfx {
namespace detail {

/** State kept for the child emitters of one particle. */
struct ParentParticleData {
    static const int ChildEmitterMax = 16;

    Emitter* pChildEmitter[ChildEmitterMax];
    f32 time;
    f32 life;
    f32 emitIntervalCounter[ChildEmitterMax];
    f32 emitCounter[ChildEmitterMax];
    f32 emitInterval[ChildEmitterMax];
    u8 isEmitted[ChildEmitterMax];
};

static_assert(sizeof(ParentParticleData) == 0x158);

/** Values of one particle, as calculated for the CPU. */
struct Particle {
    u8 _0[0x60];
    util::Vector3fType rotate;
    util::Vector3fType scale;
    u8 _80[0x90 - 0x80];
    util::Vector4fType color0;
    util::Vector4fType color1;
    u8 _b0[0xc0 - 0xb0];
};

static_assert(sizeof(Particle) == 0xc0);

/** Per-frame values uploaded to the shaders of an emitter. */
struct EmitterDynamicUniformBlock {
    util::Float4 emitterColor0;
    util::Float4 emitterColor1;
    f32 time;
    f32 particleNum;
    f32 accumulatedFrameRate;
    f32 frameRate;
    f32 alpha;
    util::Float3 particleScale;
    util::Float4 emitterMatrixSrt[4];
    util::Float4 emitterMatrixRt[4];
};

static_assert(sizeof(EmitterDynamicUniformBlock) == 0xc0);

/** Warnings reported while the effects run. */
enum RuntimeWarningId {
    RuntimeWarningId_ParticleUserDataInUse = 4,
    RuntimeWarningId_NoAvailableStripeInstance = 0x2000,
    RuntimeWarningId_StripeHistoryAllocationFailed = 0x4000,
    RuntimeWarningId_TemporaryBufferAllocationFailed = 0x8000,
};

void Warning(void* pContext, RuntimeWarningId id);

void CalculateEmitterKeyFrameAnimation(util::Float3* pOut, bool* pIsEnd,
                                       const ResAnimEmitterKeyParamSet* pAnim, f32 time);

void _MatrixFromQuaternion(util::Matrix4x3fType* pOut, const util::Vector4fType& rQuaternion);

}  // namespace detail
}  // namespace vfx
}  // namespace nn
