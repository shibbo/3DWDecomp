/**
 * @file EmitterRes.h
 * @brief VFX emitter resources and per-particle data.
 */

#pragma once

#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/util/util_MathTypes.h>

namespace nn {
namespace vfx {

namespace detail {
class Shader;
class ComputeShader;
struct EmitterStaticUniformBlock;
struct ResFieldRandom;
struct ResFieldRandomSimple;
struct ResFieldMagnet;
struct ResFieldSpin;
struct ResFieldCollision;
struct ResFieldConvergence;
struct ResFieldPosAdd;
struct ResFieldCurlNoise;
struct ResFieldCustom;

struct ResEmitter {
    u8 _0[0x10];
    char name[0x40];
    u8 _50[0x752 - 0x50];
    u8 calcType;
    u8 followType;
    u8 isStopEmitInFade;
    bool isFadeOutAlpha;
    bool isFadeOutScale;
    u8 _757[0x75b - 0x757];
    bool isFadeInAlpha;
    bool isFadeInScale;
    u8 _75d[0x768 - 0x75d];
    s32 fadeOutFrame;
    s32 fadeInFrame;
    u8 _770[0x7d8 - 0x770];
    bool isInheritParentVel;
    bool isInheritParentScale;
    bool isInheritParentRotate;
    u8 _7db[0x7dc - 0x7db];
    bool isInheritParentColor0;
    bool isInheritParentColor1;
    bool isInheritParentAlpha0;
    bool isInheritParentAlpha1;
    u8 _7e0[0x7e2 - 0x7e0];
    bool isInheritParentEmitterAlpha0;
    bool isInheritParentEmitterAlpha1;
    bool isIndependentChild;
    u8 _7e5[0x7e8 - 0x7e5];
    f32 inheritParentVelRate;
    f32 inheritParentScaleRate;
    union {
        bool isOneTime;
        bool isLoop;
    };
    bool isWorldGravity;
    bool isEmitDistance;
    bool isEmitDirectionLocal;
    union {
        u32 emitStartFrame;
        u32 emitEndFrame;
    };
    u32 emitStartRatio;
    u32 emitDuration;
    f32 emitRate;
    s32 emitRateRandom;
    u8 _808[0x810 - 0x808];
    f32 emitPosRandom;
    u8 _814[0x818 - 0x814];
    util::Float3 gravity;
    f32 emitDistanceUnit;
    f32 emitDistanceMin;
    f32 emitDistanceMax;
    f32 emitDistanceMargin;
    u8 _834[0x838 - 0x834];
    u8 volumeType;
    bool isArcStartRandom;
    u8 _83a[0x840 - 0x83a];
    f32 arcLength;
    u8 _844[0x848 - 0x844];
    f32 arcStart;
    u8 _84c[0x85c - 0x84c];
    util::Float3 volumeRadius;
    util::Float3 volumeScale;
    u32 divisionEmitMode;
    u8 _878[0x880 - 0x878];
    u32 circleDivisionNum;
    u32 circleDivisionRandom;
    u32 sphereDivisionNum;
    u32 sphereDivisionRandom;
    u8 _890[0x89b - 0x890];
    bool isAlphaMaskEnable;
    u8 _89c[0x89f - 0x89c];
    u8 maskType;
    u8 _8a0[0x8a8 - 0x8a0];
    u8 isInfinityLife;
    u8 _8a9[0x8ad - 0x8a9];
    bool isRotateDirRandom[3];
    u8 _8b0[0x8b2 - 0x8b0];
    u8 rotType;
    u8 _8b3[0x8b8 - 0x8b3];
    s32 particleLife;
    s32 particleLifeRandom;
    f32 momentumRandom;
    u8 _8c4[0x8d8 - 0x8c4];
    bool isAnimLoop[5];
    u8 isAnimStartRandom[5];
    u8 _8e2[0x8e4 - 0x8e2];
    s32 animLoopRate[5];
    u8 _8f8[0x924 - 0x8f8];
    u32 drawPath;
    u64 customShaderFlag;
    u64 customShaderSwitch;
    u8 _938[0x964 - 0x938];
    f32 allDirectionalVel;
    u8 _968[0x96c - 0x968];
    util::Float3 emitDirection;
    f32 dispersionAngle;
    f32 xzDiffusionVel;
    util::Float3 randomVel;
    f32 velocityRandom;
    f32 emitterVelInherit;
    u8 _994[0x99c - 0x994];
    u8 colorCalcType[2];
    u8 alphaCalcType[2];
    util::Float4 color[2];
    f32 particleScaleX;
    f32 particleScaleY;
    u8 _9c8[0x9cc - 0x9c8];
    util::Float3 scaleRandom;
    u8 _9d8[0x9e4 - 0x9d8];
    bool isAlphaFluctuation;
    bool isScaleFluctuation;
    bool isScaleFluctuationAxisSeparate;
    u8 fluctuationFlag;
};

struct ResAnimKey {
    f32 x;
    f32 y;
    f32 z;
    f32 time;

    /** @return the key value */
    const util::Float3& GetValue() const { return *reinterpret_cast<const util::Float3*>(&x); }
};

struct ResAnim8KeyParamSet {
    u32 enable;
    u32 loop;
    u32 startRandom;
    s32 keyNum;
    s32 loopRate;
    ResAnimKey keys[8];
};

/** Key frame animation of an emitter value. */
struct ResAnimEmitterKeyParamSet {
    bool enable;
    bool loop;
    u8 _2[0x4 - 0x2];
    s32 keyNum;
    u8 _8[0xc - 0x8];
    ResAnimKey keys[8];
};

struct ParticleAttribute {
    u8 _0[0x8];
    f32 createTime;
    f32 life;
    u8 _10[0x20 - 0x10];
};

}  // namespace detail

struct EmitterResource {
    void InitializeRenderState(gfx::Device* pDevice);
    void FinalizeRenderState(gfx::Device* pDevice);

    /** @return the compute shader, stored in the slot of ShaderType_Compute */
    detail::ComputeShader* GetComputeShader() const {
        return reinterpret_cast<detail::ComputeShader*>(m_Shader[3]);
    }

    u8 _0[0x1];
    bool m_IsUseEmitterAnim;
    bool m_IsUseEmitterMatrixAnim;
    bool m_IsUseField;
    u8 _4[0x10 - 0x4];
    detail::ResEmitter* m_pResEmitter;
    detail::EmitterStaticUniformBlock* m_pEmitterStaticUbo;
    u8 _20[0x88 - 0x20];
    s32 m_ChildEmitterResNum;
    EmitterResource* m_ChildEmitterResSet[16];
    u8 _110[0x248 - 0x110];
    detail::ResFieldRandom* m_pFieldRandomData;
    detail::ResFieldRandomSimple* m_pFieldRandomSimpleData;
    detail::ResFieldMagnet* m_pFieldMagnetData;
    detail::ResFieldSpin* m_pFieldSpinData;
    detail::ResFieldCollision* m_pFieldCollisionData;
    detail::ResFieldConvergence* m_pFieldConvergenceData;
    detail::ResFieldPosAdd* m_pFieldPosAddData;
    detail::ResFieldCurlNoise* m_pFieldCurlNoiseData;
    detail::ResFieldCustom* m_pFieldCustomData;
    const detail::ResAnimEmitterKeyParamSet* m_EmitterAnimArray[14];
    void* m_CustomShaderParam;
    size_t m_CustomShaderParamSize;
    void* m_CustomActionParam;
    s32* m_CustomDataParam;
    util::Float3 m_InitRotate;
    u8 _32c[0x330 - 0x32c];
    s32 m_EmitterPluginIndex;
    u8 _334[0x338 - 0x334];
    void* m_pEmitterPluginData;
    detail::Shader* m_Shader[8];
    u8 _380[0x3d0 - 0x380];
    s32 m_CustomAttributeBufferSlot;
};

}  // namespace vfx
}  // namespace nn
