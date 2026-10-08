/**
 * @file vfx_ParticleBehavior.h
 * @brief Per-particle behavior: fields, gravity, and the scale / color / rotation animations.
 */

#pragma once

#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/EmitterRes.h>
#include <nn/vfx/EmitterSet.h>

namespace nn {
namespace vfx {
namespace detail {

/** Eight animation keys, evaluated with an explicit key count. */
struct ResAnim8KeyParam {
    ResAnimKey keys[8];
};

/** Emitter parameters uploaded to the shaders, also read by the CPU particle code. */
struct EmitterStaticUniformBlock {
    u8 _0[0x10];
    s32 animKeyNum[5];
    u8 _24[0x70 - 0x24];
    f32 airRegist;
    u8 _74[0x90 - 0x74];
    util::Float2 fluctuationAmplitude;
    util::Float2 fluctuationCycle;
    util::Float2 fluctuationPhaseRandom;
    util::Float2 fluctuationPhaseInit;
    u8 _b0[0x360 - 0xb0];
    f32 colorScale;
    u8 _364[0x370 - 0x364];
    ResAnim8KeyParam colorAnim[4];
    u8 _570[0x5b0 - 0x570];
    ResAnim8KeyParam scaleAnim;
    u8 _630[0x6b0 - 0x630];
    util::Float3 rotateInit;
    u8 _6bc[0x6c0 - 0x6bc];
    util::Float3 rotateInitRandom;
    u8 _6cc[0x6d0 - 0x6cc];
    util::Float3 rotateAdd;
    f32 rotateRegist;
    util::Float3 rotateAddRandom;
};

/** GPU noise field. */
struct ResFieldRandom {
    u8 _0[0x4];
    util::Float3 randomVel;
    u8 _10[0x3c - 0x10];
    ResAnim8KeyParamSet randomVelAnim;
};

/** Random field that kicks the velocity every few frames. */
struct ResFieldRandomSimple {
    util::Float3 randomVel;
    u32 blank;
    ResAnim8KeyParamSet randomVelAnim;
};

/** Magnet field that pulls particles towards a point. */
struct ResFieldMagnet {
    bool isFollowEmitter;
    bool isEnableX;
    bool isEnableY;
    bool isEnableZ;
    f32 power;
    util::Float3 pos;
    ResAnim8KeyParamSet powerAnim;
};

/** Spin field that rotates particles around an axis. */
struct ResFieldSpin {
    f32 rotate;
    s32 axis;
    f32 diffusionVel;
    ResAnim8KeyParamSet rotateAnim;
    ResAnim8KeyParamSet diffusionVelAnim;
};

/** Collision against a horizontal plane. */
struct ResFieldCollision {
    u8 type;
    bool isWorld;
    u8 _2[0x4 - 0x2];
    f32 coord;
    f32 coef;
    s32 count;
    f32 friction;
};

/** Convergence field that pulls particles towards a point. */
struct ResFieldConvergence {
    u8 type;
    u8 _1[0x4 - 0x1];
    util::Float3 pos;
    f32 ratio;
    ResAnim8KeyParamSet ratioAnim;
};

/** Field adding a fixed offset to the position. */
struct ResFieldPosAdd {
    bool isWorld;
    u8 _1[0x4 - 0x1];
    util::Float3 posAdd;
    ResAnim8KeyParamSet posAddAnim;
};

/** CPU-side state of one particle. */
struct ParticleData {
    s32 createId;
    s32 collisionCount;
    f32 createTime;
    f32 life;
    void* pUserData;
    void* pUserData2;
};

void MakrRtMatrix(util::Matrix4x3fType* pOutMatrix, const util::Matrix4x3fType& rSrcMatrix);

void CalculateParticleBehaviorCustomField(util::Vector3fType* pPos, util::Vector3fType* pVec,
                                          f32* pTime, f32* pLife, Emitter* pEmitter,
                                          const ParticleProperty* pProperty, int particleIndex);

void CalculateGpuNoise(util::Vector3fType* pVec, Emitter* pEmitter,
                       const ParticleProperty* pProperty, int particleIndex,
                       const util::Vector3fType& rRandomVel, f32 time);

void CalculateParticleBehavior_FieldCurlNoise(util::Vector3fType* pPos, util::Vector3fType* pVec,
                                              Emitter* pEmitter, const ParticleProperty* pProperty,
                                              int particleIndex);

f32 CalculateFluctuationSineWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                 f32 phaseRandom, const util::Vector4fType& rRandom);
f32 CalculateFluctuationSawToothWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                     f32 phaseRandom, const util::Vector4fType& rRandom);
f32 CalculateFluctuationRectangleWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                      f32 phaseRandom, const util::Vector4fType& rRandom);
}  // namespace detail
}  // namespace vfx
}  // namespace nn
