#include <nn/vfx/vfx_ParticleBehavior.h>

#include <attributes.h>
#include <cmath>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_Constants.h>
#include <nn/util/util_MatrixApi.h>
#include <nn/util/util_VectorApi.h>
#include <nn/vfx/System.h>

namespace nn {
namespace vfx {
namespace detail {

namespace {

/**
 * Builds a vector from its components, starting from a zero vector.
 * @param pOut the vector to write
 * @param x the x component
 * @param y the y component
 * @param z the z component
 */
inline void SetVector(util::Vector3fType* pOut, f32 x, f32 y, f32 z) {
    float32x4_t v = vdupq_n_f32(0.0f);
    v = vsetq_lane_f32(x, v, 0);
    v = vsetq_lane_f32(y, v, 1);
    v = vsetq_lane_f32(z, v, 2);
    pOut->_v = v;
}

/**
 * Gets the x component of a vector.
 * @param rVector the vector
 * @return the x component
 */
template <typename T>
inline f32 GetX(const T& rVector) {
    return vgetq_lane_f32(rVector._v, 0);
}

/**
 * Gets the y component of a vector.
 * @param rVector the vector
 * @return the y component
 */
template <typename T>
inline f32 GetY(const T& rVector) {
    return vgetq_lane_f32(rVector._v, 1);
}

/**
 * Gets the z component of a vector.
 * @param rVector the vector
 * @return the z component
 */
template <typename T>
inline f32 GetZ(const T& rVector) {
    return vgetq_lane_f32(rVector._v, 2);
}

/**
 * Gets the w component of a vector.
 * @param rVector the vector
 * @return the w component
 */
inline f32 GetW(const util::Vector4fType& rVector) {
    return vgetq_lane_f32(rVector._v, 3);
}

/**
 * Sets the x component of a vector.
 * @param pOut the vector
 * @param x the new x component
 */
template <typename T>
inline void SetX(T* pOut, f32 x) {
    pOut->_v = vsetq_lane_f32(x, pOut->_v, 0);
}

/**
 * Sets the y component of a vector.
 * @param pOut the vector
 * @param y the new y component
 */
template <typename T>
inline void SetY(T* pOut, f32 y) {
    pOut->_v = vsetq_lane_f32(y, pOut->_v, 1);
}

/**
 * Sets the z component of a three component vector and clears its unused w component.
 * @param pOut the vector
 * @param z the new z component
 */
inline void SetZ(util::Vector3fType* pOut, f32 z) {
    pOut->_v = vsetq_lane_f32(z, pOut->_v, 2);
    pOut->_v = vsetq_lane_f32(0.0f, pOut->_v, 3);
}

/**
 * Sets the z component of a four component vector.
 * @param pOut the vector
 * @param z the new z component
 */
inline void SetZ(util::Vector4fType* pOut, f32 z) {
    pOut->_v = vsetq_lane_f32(z, pOut->_v, 2);
}

/**
 * Sets the w component of a vector.
 * @param pOut the vector
 * @param w the new w component
 */
inline void SetW(util::Vector4fType* pOut, f32 w) {
    pOut->_v = vsetq_lane_f32(w, pOut->_v, 3);
}

/**
 * Stores a vector to three floats.
 * @param pOut the destination
 * @param rVector the vector
 */
inline void StoreVector(util::Float3* pOut, const util::Vector3fType& rVector) {
    vst1_f32(pOut->v, vget_low_f32(rVector._v));
    vst1q_lane_f32(&pOut->v[2], rVector._v, 2);
}

/**
 * Calculates the length of a vector.
 * @param rVector the vector
 * @return the length, in every lane
 */
inline float32x4_t VectorLength(float32x4_t vector) {
    float32x4_t square = vmulq_f32(vector, vector);
    float32x2_t sum = vadd_f32(vget_high_f32(square), vget_low_f32(square));
    sum = vpadd_f32(sum, sum);
    return vsqrtq_f32(vcombine_f32(sum, sum));
}

/**
 * Scales a vector to unit length, returning zero for a zero length vector.
 * @param vector the vector
 * @param length the length of the vector
 * @return the normalized vector
 */
inline float32x4_t VectorNormalize(float32x4_t vector, f32 length) {
    if (length > 0.0f) {
        return vmulq_n_f32(vector, 1.0f / length);
    }

    return vdupq_n_f32(0.0f);
}

/**
 * Interpolates between the two keys around a time.
 * @param pOut the interpolated value
 * @param pKeys the keys
 * @param keyNum the number of keys
 * @param time the time
 */
inline void CalculateKeyLerp(util::Float3* pOut, const ResAnimKey* pKeys, int keyNum, f32 time) {
    for (int i = 0; i < keyNum; i++) {
        f32 startTime = pKeys[i].time;
        f32 endTime = pKeys[i + 1].time;

        if (startTime <= time && time < endTime) {
            f32 rate = (time - startTime) / (endTime - startTime);
            util::Vector3fType start;
            util::Vector3fType end;
            util::VectorLoad(&start, pKeys[i].GetValue());
            util::VectorLoad(&end, pKeys[i + 1].GetValue());

            util::Vector3fType value;
            value._v = vaddq_f32(start._v, vmulq_n_f32(vsubq_f32(end._v, start._v), rate));
            StoreVector(pOut, value);
            return;
        }
    }

    pOut->x = 0.0f;
    pOut->y = 0.0f;
    pOut->z = 0.0f;
}

/**
 * Evaluates keys at a time between the first and the last key.
 * @param pOut the interpolated value
 * @param pKeys the keys
 * @param rLastKey the last key
 * @param keyNum the number of keys
 * @param time the time, in [0, 1]
 */
inline void CalculateKeyValue(util::Float3* pOut, const ResAnimKey* pKeys,
                              const ResAnimKey& rLastKey, int keyNum, f32 time) {
    if (rLastKey.time <= time) {
        pOut->x = rLastKey.x;
        pOut->y = rLastKey.y;
        pOut->z = rLastKey.z;
    } else if (time < pKeys[0].time) {
        pOut->x = pKeys[0].x;
        pOut->y = pKeys[0].y;
        pOut->z = pKeys[0].z;
    } else {
        CalculateKeyLerp(pOut, pKeys, keyNum, time);
    }
}

/**
 * Rotates the components of a vector to (y, z, x).
 * @param vector the vector
 * @return the rotated vector
 */
inline float32x4_t ShuffleYzx(float32x4_t vector) {
    const uint8x8_t indexYz = {4, 5, 6, 7, 8, 9, 10, 11};
    const uint8x8_t indexXw = {0, 1, 2, 3, 12, 13, 14, 15};
    uint8x8x2_t table = {{vreinterpret_u8_f32(vget_low_f32(vector)),
                           vreinterpret_u8_f32(vget_high_f32(vector))}};
    return vreinterpretq_f32_u8(vcombine_u8(vtbl2_u8(table, indexYz), vtbl2_u8(table, indexXw)));
}

/**
 * Rotates the components of a vector to (z, x, y).
 * @param vector the vector
 * @return the rotated vector
 */
inline float32x4_t ShuffleZxy(float32x4_t vector) {
    const uint8x8_t indexZx = {8, 9, 10, 11, 0, 1, 2, 3};
    const uint8x8_t indexYw = {4, 5, 6, 7, 12, 13, 14, 15};
    uint8x8x2_t table = {{vreinterpret_u8_f32(vget_low_f32(vector)),
                           vreinterpret_u8_f32(vget_high_f32(vector))}};
    return vreinterpretq_f32_u8(vcombine_u8(vtbl2_u8(table, indexZx), vtbl2_u8(table, indexYw)));
}

/**
 * Calculates the cross product of two vectors.
 * @param lhs the first vector
 * @param rhs the second vector
 * @return the cross product
 */
inline float32x4_t VectorCross(float32x4_t lhs, float32x4_t rhs) {
    return vfmsq_f32(vmulq_f32(ShuffleYzx(lhs), ShuffleZxy(rhs)), ShuffleZxy(lhs), ShuffleYzx(rhs));
}

/**
 * Inverts a matrix. A singular matrix gives a zero matrix.
 * @param pOut the inverse
 * @param rMatrix the matrix
 */
inline void MatrixInverse(util::Matrix4x3fType* pOut, const util::Matrix4x3fType& rMatrix) {
    float32x4_t row0 = rMatrix._m.val[0];
    float32x4_t row1 = rMatrix._m.val[1];
    float32x4_t row2 = rMatrix._m.val[2];
    float32x4_t row3 = rMatrix._m.val[3];

    float32x4_t product = vsubq_f32(vmulq_f32(vmulq_f32(row0, ShuffleYzx(row1)), ShuffleZxy(row2)),
                                    vmulq_f32(vmulq_f32(row0, ShuffleZxy(row1)), ShuffleYzx(row2)));
    float32x2_t sum = vpadd_f32(vget_low_f32(product), vget_high_f32(product));
    float32x4_t determinant = vdupq_n_f32(vpadds_f32(sum));

    float32x4_t inverseDeterminant = vrecpeq_f32(determinant);
    inverseDeterminant =
        vmulq_f32(inverseDeterminant, vrecpsq_f32(inverseDeterminant, determinant));
    inverseDeterminant =
        vmulq_f32(inverseDeterminant, vrecpsq_f32(inverseDeterminant, determinant));

    float32x4x4_t matrix;
    matrix.val[0] = row0;
    matrix.val[1] = row1;
    matrix.val[2] = row2;
    matrix.val[3] = vdupq_n_f32(0.0f);
    float32x4x4_t transposed = util::detail::Matrix4x4fTranspose(matrix);

    float32x4_t inverse0 =
        vmulq_f32(inverseDeterminant, VectorCross(transposed.val[1], transposed.val[2]));
    float32x4_t inverse1 =
        vmulq_f32(inverseDeterminant, VectorCross(transposed.val[2], transposed.val[0]));
    float32x4_t inverse2 =
        vmulq_f32(inverseDeterminant, VectorCross(transposed.val[0], transposed.val[1]));
    float32x4_t inverse3 = vnegq_f32(vmulq_laneq_f32(inverse0, row3, 0));
    inverse3 = vfmsq_laneq_f32(inverse3, inverse1, row3, 1);
    inverse3 = vfmsq_laneq_f32(inverse3, inverse2, row3, 2);

    uint32x4_t mask = vmvnq_u32(vceqzq_f32(determinant));
    pOut->_m.val[0] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse0), mask));
    pOut->_m.val[1] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse1), mask));
    pOut->_m.val[2] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse2), mask));
    pOut->_m.val[3] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse3), mask));
}

/**
 * Transposes the rotation of a matrix, dropping its translation.
 * @param pOut the transposed matrix
 * @param rMatrix the matrix
 */
inline void MatrixTranspose(util::Matrix4x3fType* pOut, const util::Matrix4x3fType& rMatrix) {
    float32x4x4_t matrix;
    matrix.val[0] = rMatrix._m.val[0];
    matrix.val[1] = rMatrix._m.val[1];
    matrix.val[2] = rMatrix._m.val[2];
    matrix.val[3] = vdupq_n_f32(0.0f);
    pOut->_m = util::detail::Matrix4x4fTranspose(matrix);
}

/**
 * Transforms a point by a matrix.
 * @param pOut the transformed point
 * @param rVector the point
 * @param rMatrix the matrix
 */
inline void VectorTransform(util::Vector3fType* pOut, const util::Vector3fType& rVector,
                            const util::Matrix4x3fType& rMatrix) {
    float32x4_t value = vmulq_laneq_f32(rMatrix._m.val[0], rVector._v, 0);
    value = vfmaq_laneq_f32(value, rMatrix._m.val[1], rVector._v, 1);
    value = vfmaq_laneq_f32(value, rMatrix._m.val[2], rVector._v, 2);
    pOut->_v = vaddq_f32(rMatrix._m.val[3], value);
}

/**
 * Transforms a direction by a matrix, ignoring its translation.
 * @param pOut the transformed direction
 * @param rVector the direction
 * @param rMatrix the matrix
 */
inline void VectorTransformNormal(util::Vector3fType* pOut, const util::Vector3fType& rVector,
                                  const util::Matrix4x3fType& rMatrix) {
    float32x4_t value = vmulq_laneq_f32(rMatrix._m.val[0], rVector._v, 0);
    value = vfmaq_laneq_f32(value, rMatrix._m.val[1], rVector._v, 1);
    pOut->_v = vfmaq_laneq_f32(value, rMatrix._m.val[2], rVector._v, 2);
}

/**
 * Loads the emitter matrix a particle was emitted with.
 * @param pOut the matrix
 * @param pRows the three per-particle matrix row arrays
 * @param particleIndex the index of the particle
 */
inline void LoadParticleEmitterMatrix(util::Matrix4x3fType* pOut, util::Float4* const* pRows,
                                      int particleIndex) {
    const util::Float4& rRow0 = pRows[0][particleIndex];
    const util::Float4& rRow1 = pRows[1][particleIndex];
    const util::Float4& rRow2 = pRows[2][particleIndex];
    util::Vector3fType axis;
    util::VectorSet(&axis, rRow0.x, rRow1.x, rRow2.x);
    pOut->_m.val[0] = axis._v;
    util::VectorSet(&axis, rRow0.y, rRow1.y, rRow2.y);
    pOut->_m.val[1] = axis._v;
    util::VectorSet(&axis, rRow0.z, rRow1.z, rRow2.z);
    pOut->_m.val[2] = axis._v;
    util::VectorSet(&axis, rRow0.w, rRow1.w, rRow2.w);
    pOut->_m.val[3] = axis._v;
}

/**
 * Gets the emitter matrix a particle was emitted with.
 * @param pOut the matrix, left untouched when the emitter keeps no per-particle matrices
 * @param pEmitter the emitter of the particle
 * @param particleIndex the index of the particle
 */
inline void GetParticleEmitterMatrix(util::Matrix4x3fType* pOut, const Emitter* pEmitter,
                                     int particleIndex) {
    if (pEmitter->m_ParticleEmitterMatrixRow[0] != nullptr &&
        particleIndex <= pEmitter->m_ParticleNum) {
        LoadParticleEmitterMatrix(pOut, pEmitter->m_ParticleEmitterMatrixRow, particleIndex);
    }
}

/**
 * Gets the rotation of the emitter matrix a particle was emitted with.
 * @param pOut the rotation and translation matrix
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 */
inline void GetParticleEmitterMatrixRt(util::Matrix4x3fType* pOut,
                                       const ParticleProperty* pProperty, int particleIndex) {
    const util::Float4& rRow0 = pProperty->pEmitterMatrixRow[0][particleIndex];
    const util::Float4& rRow1 = pProperty->pEmitterMatrixRow[1][particleIndex];
    const util::Float4& rRow2 = pProperty->pEmitterMatrixRow[2][particleIndex];
    util::Matrix4x3fType matrix;
    util::Vector3fType axis;
    SetVector(&axis, rRow0.x, rRow1.x, rRow2.x);
    matrix._m.val[0] = axis._v;
    SetVector(&axis, rRow0.y, rRow1.y, rRow2.y);
    matrix._m.val[1] = axis._v;
    SetVector(&axis, rRow0.z, rRow1.z, rRow2.z);
    matrix._m.val[2] = axis._v;
    SetVector(&axis, rRow0.w, rRow1.w, rRow2.w);
    matrix._m.val[3] = axis._v;
    MakrRtMatrix(pOut, matrix);
}

/**
 * Reduces an angle to the range [-pi, pi].
 * @param radian the angle in radians
 * @return the equivalent angle in [-pi, pi]
 */
inline f32 ModTwoPi(f32 radian) {
    using namespace util::detail;

    f32 quotient = Float1Divided2Pi * radian + (radian >= 0.0f ? 0.5f : -0.5f);
    return radian - Float2Pi * static_cast<int>(quotient);
}

/**
 * Estimates the cosine with a polynomial.
 * @param radian the angle in radians
 * @return the cosine of the angle
 */
inline f32 CosEst(f32 radian) {
    using namespace util::detail;

    f32 value = ModTwoPi(radian);
    f32 sign;

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
        sign = -1.0f;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
        sign = -1.0f;
    } else {
        sign = 1.0f;
    }

    f32 square = value * value;
    return sign * (((((CosCoefficients[1] - CosCoefficients[0] * square) * square -
                      CosCoefficients[2]) * square + CosCoefficients[3]) * square -
                    CosCoefficients[4]) * square + 1.0f);
}

/**
 * Estimates the sine with a polynomial.
 * @param radian the angle in radians
 * @return the sine of the angle
 */
inline f32 SinEst(f32 radian) {
    using namespace util::detail;

    f32 value = ModTwoPi(radian);

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
    }

    f32 square = value * value;
    return value * (((((SinCoefficients[1] - SinCoefficients[0] * square) * square -
                       SinCoefficients[2]) * square + SinCoefficients[3]) * square -
                     SinCoefficients[4]) * square + 1.0f);
}

/**
 * Estimates the sine and the cosine with polynomials.
 * @param pSin the sine of the angle
 * @param pCos the cosine of the angle
 * @param radian the angle in radians
 */
inline void SinCosEst(f32* pSin, f32* pCos, f32 radian) {
    using namespace util::detail;

    f32 value = ModTwoPi(radian);
    f32 sign;

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
        sign = -1.0f;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
        sign = -1.0f;
    } else {
        sign = 1.0f;
    }

    f32 square = value * value;
    *pSin = value * (((((SinCoefficients[1] - SinCoefficients[0] * square) * square -
                        SinCoefficients[2]) * square + SinCoefficients[3]) * square -
                      SinCoefficients[4]) * square + 1.0f);
    *pCos = sign * (((((CosCoefficients[1] - CosCoefficients[0] * square) * square -
                       CosCoefficients[2]) * square + CosCoefficients[3]) * square -
                     CosCoefficients[4]) * square + 1.0f);
}

/** Indices of the animations of an emitter. */
enum AnimKind {
    AnimKind_Color0,
    AnimKind_Alpha0,
    AnimKind_Color1,
    AnimKind_Alpha1,
    AnimKind_Scale,
};

/** How a color of the particles is calculated. */
enum ColorCalcType {
    ColorCalcType_Fixed = 0,
    ColorCalcType_Anim = 2,
    ColorCalcType_Random = 3,
};

/**
 * Evaluates the fluctuation wave of an emitter.
 * @param waveType the shape of the wave
 * @param time the age of the particle
 * @param amplitude the amplitude of the wave
 * @param cycle the length of one period
 * @param phaseInit the initial phase
 * @param phaseRandom the random phase range
 * @param rRandom the random values of the particle
 * @return the fluctuation factor
 */
ALWAYS_INLINE inline f32 CalculateFluctuation(int waveType, f32 time, f32 amplitude, f32 cycle,
                                              f32 phaseInit, f32 phaseRandom,
                                              const util::Vector4fType& rRandom) {
    switch (waveType) {
    case 0:
        return CalculateFluctuationSineWave(time, amplitude, cycle, phaseInit, phaseRandom,
                                            rRandom);
    case 1:
        return CalculateFluctuationSawToothWave(time, amplitude, cycle, phaseInit, phaseRandom,
                                                rRandom);
    case 2:
        return CalculateFluctuationRectangleWave(time, amplitude, cycle, phaseInit, phaseRandom,
                                                 rRandom);
    default:
        return 1.0f;
    }
}

/**
 * Applies the scale fluctuation of an emitter.
 * @param pOut the scale
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param time the age of the particle
 */
ALWAYS_INLINE inline void ApplyScaleFluctuation(util::Vector3fType* pOut,
                                                const EmitterResource* pEmitterRes,
                                                const util::Vector4fType& rRandom, f32 time) {
    const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;

    if (!pResEmitter->isScaleFluctuation) {
        return;
    }

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;
    int waveType = pResEmitter->fluctuationFlag >> 4;
    util::Float2 amplitude = pUbo->fluctuationAmplitude;
    util::Float2 cycle = pUbo->fluctuationCycle;
    util::Float2 phaseRandom = pUbo->fluctuationPhaseRandom;
    util::Float2 phaseInit = pUbo->fluctuationPhaseInit;

    f32 waveX = CalculateFluctuation(waveType, time, amplitude.x, cycle.x, phaseInit.x,
                                     phaseRandom.x, rRandom);
    f32 waveY = waveX;

    if (pResEmitter->isScaleFluctuationAxisSeparate) {
        waveY = CalculateFluctuation(waveType, time, amplitude.y, cycle.y, phaseInit.y,
                                     phaseRandom.y, rRandom);
    }

    SetX(pOut, waveX * GetX(*pOut));
    SetY(pOut, waveY * GetY(*pOut));
}

/**
 * Applies the alpha fluctuation of an emitter.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param time the age of the particle
 */
ALWAYS_INLINE inline void ApplyAlphaFluctuation(util::Vector4fType* pOut,
                                                const EmitterResource* pEmitterRes,
                                                const util::Vector4fType& rRandom, f32 time) {
    const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;

    if (!pResEmitter->isAlphaFluctuation) {
        return;
    }

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;
    f32 wave = CalculateFluctuation(pResEmitter->fluctuationFlag >> 4, time,
                                    pUbo->fluctuationAmplitude.x, pUbo->fluctuationCycle.x,
                                    pUbo->fluctuationPhaseInit.x, pUbo->fluctuationPhaseRandom.x,
                                    rRandom);
    SetW(pOut, GetW(*pOut) * wave);

    if (GetW(*pOut) < 0.0f) {
        SetW(pOut, 0.0f);
    }

    if (GetW(*pOut) > 1.0f) {
        SetW(pOut, 1.0f);
    }
}

/**
 * Multiplies a particle color by the colors of the emitter set and the emitter.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rColor the color of the emitter set
 * @param rEmitterColor the color of the emitter
 * @param alpha the alpha of the emitter
 */
ALWAYS_INLINE inline void MultiplyEmitterColor(util::Vector4fType* pOut,
                                               const EmitterResource* pEmitterRes,
                                               const util::Vector4fType& rColor,
                                               const util::Vector3fType& rEmitterColor, f32 alpha) {
    f32 colorScale = pEmitterRes->m_pEmitterStaticUbo->colorScale;
    SetX(pOut, GetX(rColor) * GetX(rEmitterColor) * colorScale * GetX(*pOut));
    SetY(pOut, GetY(rColor) * GetY(rEmitterColor) * colorScale * GetY(*pOut));
    SetZ(pOut, GetZ(rColor) * GetZ(rEmitterColor) * colorScale * GetZ(*pOut));
    SetW(pOut, GetW(rColor) * alpha * GetW(*pOut));
}

/**
 * Calculates one of the two colors of a particle.
 * @param pCalculator the emitter calculator
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param colorIndex the color to calculate, 0 or 1
 * @param life the life of the particle
 * @param time the age of the particle
 * @return whether the emitter has animation parameters
 */
ALWAYS_INLINE inline bool CalculateParticleColor(EmitterCalculator* pCalculator,
                                                 util::Vector4fType* pOut,
                                                 const EmitterResource* pEmitterRes,
                                                 const util::Vector4fType& rRandom, int colorIndex,
                                                 f32 life, f32 time) {
    SetX(pOut, pEmitterRes->m_pResEmitter->color[colorIndex].x);
    SetY(pOut, pEmitterRes->m_pResEmitter->color[colorIndex].y);
    SetZ(pOut, pEmitterRes->m_pResEmitter->color[colorIndex].z);
    SetW(pOut, pEmitterRes->m_pResEmitter->color[colorIndex].w);

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;

    if (pUbo == nullptr) {
        return false;
    }

    int colorKind = AnimKind_Color0 + colorIndex * 2;
    int alphaKind = AnimKind_Alpha0 + colorIndex * 2;

    if (pEmitterRes->m_pResEmitter->colorCalcType[colorIndex] == ColorCalcType_Random) {
        int keyIndex = static_cast<int>(GetX(rRandom) * pUbo->animKeyNum[colorKind]);
        SetX(pOut, pEmitterRes->m_pEmitterStaticUbo->colorAnim[colorKind].keys[keyIndex].x);
        SetY(pOut, pEmitterRes->m_pEmitterStaticUbo->colorAnim[colorKind].keys[keyIndex].y);
        SetZ(pOut, pEmitterRes->m_pEmitterStaticUbo->colorAnim[colorKind].keys[keyIndex].z);
    }

    const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;

    if (pResEmitter->colorCalcType[colorIndex] == ColorCalcType_Anim) {
        pUbo = pEmitterRes->m_pEmitterStaticUbo;
        int keyNum = pUbo->animKeyNum[colorKind];

        if (keyNum > 0) {
            f32 loopRate = pResEmitter->isAnimLoop[colorKind] ?
                               static_cast<f32>(pResEmitter->animLoopRate[colorKind]) :
                               0.0f;
            util::Float3 color;
            pCalculator->Calculate8KeyAnim(&color, pUbo->colorAnim[colorKind], keyNum,
                                           GetX(rRandom), time, loopRate,
                                           pResEmitter->isAnimStartRandom[colorKind], life);
            SetX(pOut, color.x);
            SetY(pOut, color.y);
            SetZ(pOut, color.z);
        }
    }

    pResEmitter = pEmitterRes->m_pResEmitter;

    if (pResEmitter->alphaCalcType[colorIndex] == ColorCalcType_Anim) {
        pUbo = pEmitterRes->m_pEmitterStaticUbo;
        int keyNum = pUbo->animKeyNum[alphaKind];

        if (keyNum > 0) {
            f32 loopRate = pResEmitter->isAnimLoop[alphaKind] ?
                               static_cast<f32>(pResEmitter->animLoopRate[alphaKind]) :
                               0.0f;
            util::Float3 alpha;
            pCalculator->Calculate8KeyAnim(&alpha, pUbo->colorAnim[alphaKind], keyNum,
                                           GetX(rRandom), time, loopRate,
                                           pResEmitter->isAnimStartRandom[alphaKind], life);
            SetW(pOut, alpha.x);
        }
    }

    return true;
}

}  // namespace

/**
 * Removes the scale from a matrix by normalizing its axes.
 * @param pOutMatrix the rotation and translation matrix
 * @param rSrcMatrix the scale, rotation and translation matrix
 */
void MakrRtMatrix(util::Matrix4x3fType* pOutMatrix, const util::Matrix4x3fType& rSrcMatrix) {
    f32 lengthX = vgetq_lane_f32(VectorLength(rSrcMatrix._m.val[0]), 0);
    f32 lengthY = vgetq_lane_f32(VectorLength(rSrcMatrix._m.val[1]), 0);
    f32 lengthZ = vgetq_lane_f32(VectorLength(rSrcMatrix._m.val[2]), 0);

    pOutMatrix->_m.val[0] = VectorNormalize(rSrcMatrix._m.val[0], lengthX);
    pOutMatrix->_m.val[1] = VectorNormalize(rSrcMatrix._m.val[1], lengthY);
    pOutMatrix->_m.val[2] = VectorNormalize(rSrcMatrix._m.val[2], lengthZ);
    pOutMatrix->_m.val[3] = rSrcMatrix._m.val[3];
}

/**
 * Evaluates the 8-key animation of a field parameter.
 * @param pOut the animated value
 * @param pEmitter the emitter of the particle
 * @param rAnim the animation
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateField8KeyAnim(util::Float3* pOut, const Emitter* pEmitter,
                                               const ResAnim8KeyParamSet& rAnim, int particleIndex,
                                               f32 time) {
    int keyNum = rAnim.keyNum;

    if (keyNum == 1) {
        pOut->x = rAnim.keys[0].x;
        pOut->y = rAnim.keys[0].y;
        pOut->z = rAnim.keys[0].z;
        return;
    }

    if (keyNum != 0) {
        int lastIndex = keyNum - 1;
        f32 rate;

        if (rAnim.loop) {
            f32 random = pEmitter->m_ParticleAnimRandom[particleIndex].x;
            rate = fmodf(time + random * rAnim.startRandom * rAnim.loopRate, rAnim.loopRate) /
                   rAnim.loopRate;
        } else {
            rate = time / pEmitter->m_pEmitterData->particleLife;
        }

        CalculateKeyValue(pOut, rAnim.keys, rAnim.keys[lastIndex], rAnim.keyNum, rate);
        return;
    }

    pOut->x = pOut->y = pOut->z = 0.0f;
}

/**
 * Evaluates an 8-key animation.
 * @param pOut the animated value
 * @param rAnim the keys
 * @param keyNum the number of keys
 * @param random the random value of the particle
 * @param time the age of the particle
 * @param loopRate the length of one loop, or 0 to stretch the animation over the life
 * @param startRandom whether the loop starts at a random time
 * @param life the life of the particle
 */
void EmitterCalculator::Calculate8KeyAnim(util::Float3* pOut, const ResAnim8KeyParam& rAnim,
                                          int keyNum, f32 random, f32 time, f32 loopRate,
                                          f32 startRandom, f32 life) {
    if (keyNum == 1) {
        pOut->x = rAnim.keys[0].x;
        pOut->y = rAnim.keys[0].y;
        pOut->z = rAnim.keys[0].z;
        return;
    }

    if (keyNum != 0) {
        int lastIndex = keyNum - 1;
        f32 rate;

        if (loopRate > 0.0f) {
            rate = fmodf(time + random * startRandom * loopRate, loopRate) / loopRate;
        } else {
            rate = time / life;
        }

        CalculateKeyValue(pOut, rAnim.keys, rAnim.keys[lastIndex], keyNum, rate);
        return;
    }

    pOut->x = 1.0f;
    pOut->y = 1.0f;
    pOut->z = 1.0f;
}

/**
 * Applies the custom field callback of the system.
 * @param pPos the position of the particle
 * @param pVec the velocity of the particle
 * @param pTime the age of the particle
 * @param pLife the life of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 */
void CalculateParticleBehaviorCustomField(util::Vector3fType* pPos, util::Vector3fType* pVec,
                                          f32* pTime, f32* pLife, Emitter* pEmitter,
                                          const ParticleProperty* pProperty, int particleIndex) {
    CustomFieldCallback callback = pEmitter->m_EmitterSet->m_System->m_CustomFieldCallback;

    if (callback != nullptr) {
        callback(pPos, pVec, pTime, pLife, pEmitter, pProperty,
                 pEmitter->m_pEmitterRes->m_pFieldCustomData, particleIndex);
    }
}

/**
 * Applies the GPU noise field.
 * @param pVec the velocity of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldGpuNoise(util::Vector3fType* pVec,
                                                               Emitter* pEmitter,
                                                               const ParticleProperty* pProperty,
                                                               int particleIndex, f32 time) {
    const ResFieldRandom* pField = pEmitter->m_pEmitterRes->m_pFieldRandomData;
    util::Float3 randomVel;

    if (pField->randomVelAnim.enable) {
        CalculateField8KeyAnim(&randomVel, pEmitter, pField->randomVelAnim, particleIndex, time);
    } else {
        randomVel = pField->randomVel;
    }

    util::Vector3fType vel;
    SetVector(&vel, randomVel.x, randomVel.y, randomVel.z);
    CalculateGpuNoise(pVec, pEmitter, pProperty, particleIndex, vel, time);
}

/**
 * Applies the simple random field.
 * @param pVec the velocity of the particle
 * @param pEmitter the emitter of the particle
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldRandomSimple(util::Vector3fType* pVec,
                                                                   Emitter* pEmitter,
                                                                   int particleIndex, f32 time) {
    const ResFieldRandomSimple* pField = pEmitter->m_pEmitterRes->m_pFieldRandomSimpleData;
    util::Float3 randomVel;

    if (pField->randomVelAnim.enable) {
        CalculateField8KeyAnim(&randomVel, pEmitter, pField->randomVelAnim, particleIndex, time);
    } else {
        randomVel = pField->randomVel;
    }

    if (static_cast<u32>(time) % pField->blank == 0) {
        const util::Vector3fType& rRandom = pEmitter->m_Random.GetVec3();
        SetX(pVec, GetX(*pVec) + GetX(rRandom) * randomVel.x);
        SetY(pVec, GetY(*pVec) + GetY(rRandom) * randomVel.y);
        SetZ(pVec, GetZ(*pVec) + GetZ(rRandom) * randomVel.z);
    }
}


/**
 * Applies the magnet field, pulling the velocity towards the magnet.
 * @param pPos the position of the particle
 * @param pVec the velocity of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldMagnet(util::Vector3fType* pPos,
                                                             util::Vector3fType* pVec,
                                                             Emitter* pEmitter,
                                                             const ParticleProperty* pProperty,
                                                             int particleIndex, f32 time) {
    const ResFieldMagnet* pField = pEmitter->m_pEmitterRes->m_pFieldMagnetData;
    util::Float3 power;

    if (pField->powerAnim.enable) {
        CalculateField8KeyAnim(&power, pEmitter, pField->powerAnim, particleIndex, time);
    } else {
        power.x = pField->power;
    }

    f32 magnetPower = power.x;

    if (pField->isFollowEmitter) {
        util::Vector3fType emitterPos;
        emitterPos._v = pEmitter->m_MatrixSrt._m.val[3];

        util::Matrix4x3fType inverse;

        if (pEmitter->m_pEmitterData->followType == 1) {
            util::Matrix4x3fType matrix;
            GetParticleEmitterMatrix(&matrix, pEmitter, particleIndex);
            MatrixInverse(&inverse, matrix);
        } else {
            MatrixInverse(&inverse, pEmitter->m_MatrixSrt);
        }

        util::Vector3fType localPos;
        VectorTransform(&localPos, emitterPos, inverse);

        if (pField->isEnableX) {
            f32 diff = GetX(localPos) + pField->pos.x - GetX(*pPos) -
                       GetX(*pVec);
            SetX(pVec, GetX(*pVec) + magnetPower * diff);
        }

        if (pField->isEnableY) {
            f32 diff = GetY(localPos) + pField->pos.y - GetY(*pPos) -
                       GetY(*pVec);
            SetY(pVec, GetY(*pVec) + magnetPower * diff);
        }

        if (pField->isEnableZ) {
            f32 diff = GetZ(localPos) + pField->pos.z - GetZ(*pPos) -
                       GetZ(*pVec);
            SetZ(pVec, GetZ(*pVec) + magnetPower * diff);
        }
    } else {
        if (pField->isEnableX) {
            f32 diff = pField->pos.x - GetX(*pPos) - GetX(*pVec);
            SetX(pVec, GetX(*pVec) + magnetPower * diff);
        }

        if (pField->isEnableY) {
            f32 diff = pField->pos.y - GetY(*pPos) - GetY(*pVec);
            SetY(pVec, GetY(*pVec) + magnetPower * diff);
        }

        if (pField->isEnableZ) {
            f32 diff = pField->pos.z - GetZ(*pPos) - GetZ(*pVec);
            SetZ(pVec, GetZ(*pVec) + magnetPower * diff);
        }
    }
}

/**
 * Applies the spin field, rotating the position around an axis of the emitter.
 * @param pPos the position of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldSpin(util::Vector3fType* pPos,
                                                           const Emitter* pEmitter,
                                                           ParticleProperty* pProperty,
                                                           int particleIndex, f32 time) {
    const ResFieldSpin* pField = pEmitter->m_pEmitterRes->m_pFieldSpinData;
    f32 frameRate = pEmitter->m_FrameRate;
    f32 random = pProperty->pRandom[particleIndex].w;

    util::Float3 rotate;

    if (pField->rotateAnim.enable) {
        CalculateField8KeyAnim(&rotate, pEmitter, pField->rotateAnim, particleIndex, time);
        rotate.x = util::DegreeToRadian(rotate.x);
    } else {
        rotate.x = pField->rotate;
    }

    pField = pEmitter->m_pEmitterRes->m_pFieldSpinData;
    util::Float3 diffusionVel;

    if (pField->diffusionVelAnim.enable) {
        CalculateField8KeyAnim(&diffusionVel, pEmitter, pField->diffusionVelAnim, particleIndex,
                               time);
        diffusionVel.x = util::DegreeToRadian(diffusionVel.x);
    } else {
        diffusionVel.x = pField->diffusionVel;
    }

    f32 sinValue;
    f32 cosValue;
    SinCosEst(&sinValue, &cosValue, rotate.x * random * frameRate);

    switch (pField->axis) {
    case 0: {
        f32 y = cosValue * GetY(*pPos) + sinValue * GetZ(*pPos);
        f32 z = cosValue * GetZ(*pPos) - sinValue * GetY(*pPos);
        SetY(pPos, y);
        SetZ(pPos, z);

        if (diffusionVel.x != 0.0f) {
            f32 lengthSq = y * y + z * z;

            if (lengthSq > 0.0f) {
                f32 scale = 1.0f / std::sqrt(lengthSq) * diffusionVel.x * random * frameRate;
                SetY(pPos, GetY(*pPos) + y * scale);
                SetZ(pPos, GetZ(*pPos) + z * scale);
            }
        }

        break;
    }
    case 1: {
        f32 x = cosValue * GetX(*pPos) - sinValue * GetZ(*pPos);
        f32 z = cosValue * GetZ(*pPos) + sinValue * GetX(*pPos);
        SetX(pPos, x);
        SetZ(pPos, z);

        if (diffusionVel.x != 0.0f) {
            f32 lengthSq = z * z + x * x;

            if (lengthSq > 0.0f) {
                f32 scale = 1.0f / std::sqrt(lengthSq) * diffusionVel.x * random * frameRate;
                SetX(pPos, GetX(*pPos) + x * scale);
                SetZ(pPos, GetZ(*pPos) + z * scale);
            }
        }

        break;
    }
    case 2: {
        f32 x = cosValue * GetX(*pPos) + sinValue * GetY(*pPos);
        f32 y = cosValue * GetY(*pPos) - sinValue * GetX(*pPos);
        SetX(pPos, x);
        SetY(pPos, y);

        if (diffusionVel.x != 0.0f) {
            f32 lengthSq = x * x + y * y;

            if (lengthSq > 0.0f) {
                f32 scale = 1.0f / std::sqrt(lengthSq) * diffusionVel.x * random * frameRate;
                SetX(pPos, GetX(*pPos) + x * scale);
                SetY(pPos, GetY(*pPos) + y * scale);
            }
        }

        break;
    }
    default:
        break;
    }
}

/**
 * Applies the collision field, bouncing or killing particles that cross a plane.
 * @param pPos the position of the particle
 * @param pVec the velocity of the particle
 * @param pLife the life of the particle, set to its age to kill it
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param pParticleData the CPU state of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldCollision(
    util::Vector3fType* pPos, util::Vector3fType* pVec, f32* pLife, Emitter* pEmitter,
    ParticleProperty* pProperty, int particleIndex, ParticleData* pParticleData, f32 time) {
    const ResFieldCollision* pField = pEmitter->m_pEmitterRes->m_pFieldCollisionData;

    if (pField->count != -1 && pField->count <= pParticleData->collisionCount) {
        return;
    }

    f32 coord = pField->coord;

    if (pField->isWorld) {
        util::Matrix4x3fType matrix;

        if (pEmitter->m_pEmitterData->followType != 0) {
            LoadParticleEmitterMatrix(&matrix, pProperty->pEmitterMatrixRow, particleIndex);
        } else {
            matrix = pEmitter->m_MatrixSrt;
        }

        util::Vector3fType worldPos;
        VectorTransform(&worldPos, *pPos, matrix);

        if (pField->type == 1) {
            if (GetY(worldPos) < coord) {
                *pLife = time;
            }

            return;
        }

        if (pField->type == 0 && GetY(worldPos) < coord) {
            util::Vector3fType worldVec;
            VectorTransformNormal(&worldVec, *pVec, matrix);
            SetY(&worldVec, -(pField->coef * GetY(worldVec)));

            util::Matrix4x3fType inverse;
            MatrixInverse(&inverse, matrix);
            inverse._m.val[3] = vdupq_n_f32(0.0f);

            util::Vector3fType localVec;
            VectorTransform(&localVec, worldVec, inverse);
            SetX(pVec, pField->friction * GetX(localVec));
            SetY(pVec, pField->friction * GetY(localVec));
            SetZ(pVec, pField->friction * GetZ(localVec));
            pParticleData->collisionCount++;
        }
    } else {
        if (pField->type == 1) {
            if (GetY(*pPos) < coord) {
                SetY(pPos, coord);
                *pLife = time;
            }

            return;
        }

        if (pField->type == 0 && GetY(*pPos) < coord) {
            SetY(pPos, coord);
            SetY(pVec, -(pField->coef * GetY(*pVec)));
            SetX(pVec, pField->friction * GetX(*pVec));
            SetY(pVec, pField->friction * GetY(*pVec));
            SetZ(pVec, pField->friction * GetZ(*pVec));
            pParticleData->collisionCount++;
        }
    }
}

/**
 * Applies the convergence field, pulling the position towards a point.
 * @param pPos the position of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldConvergence(
    util::Vector3fType* pPos, Emitter* pEmitter, const ParticleProperty* pProperty,
    int particleIndex, f32 time) {
    const ResFieldConvergence* pField = pEmitter->m_pEmitterRes->m_pFieldConvergenceData;
    f32 frameRate = pEmitter->m_FrameRate;
    f32 random = pProperty->pRandom[particleIndex].w;
    util::Float3 ratio;

    if (pField->ratioAnim.enable) {
        CalculateField8KeyAnim(&ratio, pEmitter, pField->ratioAnim, particleIndex, time);
    } else {
        ratio.x = pField->ratio;
    }

    f32 convergenceRatio = ratio.x;

    if (pField->type) {
        util::Vector3fType emitterPos = pEmitter->m_EmitterLocalPos;
        util::Matrix4x3fType inverse;

        if (pEmitter->m_pEmitterData->followType == 1) {
            util::Matrix4x3fType matrix;
            GetParticleEmitterMatrix(&matrix, pEmitter, particleIndex);
            MatrixInverse(&inverse, matrix);
        } else {
            MatrixInverse(&inverse, pEmitter->m_MatrixSrt);
        }

        util::Vector3fType localPos;
        VectorTransform(&localPos, emitterPos, inverse);

        SetX(pPos, GetX(*pPos) +
                       (pField->pos.x + GetX(localPos) - GetX(*pPos)) *
                           convergenceRatio * random * frameRate);
        SetY(pPos, GetY(*pPos) +
                       (pField->pos.y + GetY(localPos) - GetY(*pPos)) *
                           convergenceRatio * random * frameRate);
        SetZ(pPos, GetZ(*pPos) +
                       (pField->pos.z + GetZ(localPos) - GetZ(*pPos)) *
                           convergenceRatio * random * frameRate);
    } else {
        SetX(pPos, GetX(*pPos) + (pField->pos.x - GetX(*pPos)) *
                                                 convergenceRatio * random * frameRate);
        SetY(pPos, GetY(*pPos) + (pField->pos.y - GetY(*pPos)) *
                                                 convergenceRatio * random * frameRate);
        SetZ(pPos, GetZ(*pPos) + (pField->pos.z - GetZ(*pPos)) *
                                                 convergenceRatio * random * frameRate);
    }
}

/**
 * Applies the position add field.
 * @param pPos the position of the particle
 * @param pEmitter the emitter of the particle
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleBehaviorFieldPosAdd(util::Vector3fType* pPos,
                                                             Emitter* pEmitter,
                                                             const ParticleProperty* pProperty,
                                                             int particleIndex, f32 time) {
    const ResFieldPosAdd* pField = pEmitter->m_pEmitterRes->m_pFieldPosAddData;
    f32 frameRate = pEmitter->m_FrameRate;
    f32 random = pProperty->pRandom[particleIndex].w;
    util::Float3 posAdd;

    if (pField->posAddAnim.enable) {
        CalculateField8KeyAnim(&posAdd, pEmitter, pField->posAddAnim, particleIndex, time);
    } else {
        posAdd = pField->posAdd;
    }

    if (pField->isWorld) {
        util::Vector3fType worldAdd;
        SetVector(&worldAdd, posAdd.x * random * frameRate, posAdd.y * random * frameRate,
                  posAdd.z * random * frameRate);

        util::Matrix4x3fType matrix;

        if (pEmitter->m_pEmitterData->followType == 1) {
            GetParticleEmitterMatrixRt(&matrix, pProperty, particleIndex);
        } else {
            matrix = pEmitter->GetMatrixRt();
        }

        util::Matrix4x3fType inverse;
        MatrixTranspose(&inverse, matrix);

        util::Vector3fType localAdd;
        VectorTransformNormal(&localAdd, worldAdd, inverse);
        SetVector(pPos, GetX(*pPos) + GetX(localAdd),
                  GetY(*pPos) + GetY(localAdd),
                  GetZ(*pPos) + GetZ(localAdd));
    } else {
        SetVector(pPos, GetX(*pPos) + posAdd.x * random * frameRate,
                  GetY(*pPos) + posAdd.y * random * frameRate,
                  GetZ(*pPos) + posAdd.z * random * frameRate);
    }
}


/**
 * Evaluates a sine wave fluctuation, oscillating between 1 and 1 - amplitude.
 * @param time the age of the particle
 * @param amplitude the amplitude of the wave
 * @param cycle the length of one period
 * @param phaseInit the initial phase
 * @param phaseRandom the random phase range
 * @param rRandom the random values of the particle
 * @return the fluctuation factor
 */
f32 CalculateFluctuationSineWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                 f32 phaseRandom, const util::Vector4fType& rRandom) {
    f32 phase = (time + phaseInit) / cycle + GetX(rRandom) * phaseRandom;
    return 1.0f - (CosEst(util::FloatPi * phase * 2.0f) + 1.0f) * 0.5f * amplitude;
}

/**
 * Evaluates a saw tooth wave fluctuation.
 * @param time the age of the particle
 * @param amplitude the amplitude of the wave
 * @param cycle the length of one period
 * @param phaseInit the initial phase
 * @param phaseRandom the random phase range
 * @param rRandom the random values of the particle
 * @return the fluctuation factor
 */
f32 CalculateFluctuationSawToothWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                     f32 phaseRandom, const util::Vector4fType& rRandom) {
    f32 phase = (time + phaseInit) / cycle + GetX(rRandom) * phaseRandom;
    f32 rate = phase - std::floor(phase);
    return std::fabs(1.0f - rate * amplitude);
}

/**
 * Evaluates a rectangle wave fluctuation.
 * @param time the age of the particle
 * @param amplitude the amplitude of the wave
 * @param cycle the length of one period
 * @param phaseInit the initial phase
 * @param phaseRandom the random phase range
 * @param rRandom the random values of the particle
 * @return the fluctuation factor
 */
f32 CalculateFluctuationRectangleWave(f32 time, f32 amplitude, f32 cycle, f32 phaseInit,
                                      f32 phaseRandom, const util::Vector4fType& rRandom) {
    f32 phase = (time + phaseInit) / cycle + GetX(rRandom) * phaseRandom;
    f32 rate = phase - std::floor(phase);
    f32 value = rate < 0.5f ? 1.0f : 0.0f;
    return std::fabs(1.0f - value * amplitude);
}

/**
 * Moves a particle by one frame: integrates the velocity and applies air resistance, gravity
 * and every field of the emitter.
 * @param pPos the new position of the particle
 * @param pVec the new velocity of the particle
 * @param pTime the age of the particle
 * @param pLife the life of the particle
 * @param pEmitter the emitter of the particle
 * @param particleIndex the index of the particle
 * @param time the age of the particle
 * @param rPos the position of the particle
 * @param rVec the velocity of the particle
 */
void EmitterCalculator::CalculateParticleBehavior(util::Vector3fType* pPos,
                                                  util::Vector3fType* pVec, f32* pTime,
                                                  f32* pLife, Emitter* pEmitter,
                                                  int particleIndex, f32 time,
                                                  const util::Vector3fType& rPos,
                                                  const util::Vector3fType& rVec) {
    f32 frameRate = pEmitter->m_FrameRate;
    f32 random = pEmitter->m_ParticleRandom[particleIndex].w;
    pPos->_v = vaddq_f32(rPos._v, vmulq_n_f32(rVec._v, frameRate * random));

    f32 airRegist = pEmitter->m_pEmitterRes->m_pEmitterStaticUbo->airRegist;

    if (airRegist != 1.0f) {
        pVec->_v = vmulq_n_f32(rVec._v, std::pow(airRegist, frameRate));
    } else {
        *pVec = rVec;
    }

    f32 gravityScale = pEmitter->m_GravityScale;

    if (gravityScale > 0.0f) {
        const ResEmitter* pResEmitter = pEmitter->m_pEmitterData;
        f32 gravityX = gravityScale * pResEmitter->gravity.x;
        f32 gravityY = gravityScale * pResEmitter->gravity.y;
        f32 gravityZ = gravityScale * pResEmitter->gravity.z;

        if (pResEmitter->isWorldGravity) {
            util::Vector3fType gravity;
            SetVector(&gravity, gravityX, gravityY, gravityZ);

            util::Matrix4x3fType matrix;

            if (pResEmitter->followType == 1) {
                GetParticleEmitterMatrixRt(&matrix, pEmitter->GetCpuParticleProperty(),
                                           particleIndex);
            } else {
                matrix = pEmitter->GetMatrixRt();
            }

            util::Matrix4x3fType inverse;
            MatrixTranspose(&inverse, matrix);

            util::Vector3fType localGravity;
            VectorTransformNormal(&localGravity, gravity, inverse);
            localGravity._v = vmulq_n_f32(localGravity._v, frameRate);
            SetVector(pVec, GetX(*pVec) + GetX(localGravity), GetY(*pVec) + GetY(localGravity),
                      GetZ(*pVec) + GetZ(localGravity));
        } else {
            SetVector(pVec, GetX(*pVec) + gravityX * frameRate, GetY(*pVec) + gravityY * frameRate,
                      GetZ(*pVec) + gravityZ * frameRate);
        }
    }

    if (!pEmitter->m_pEmitterRes->m_IsUseField) {
        return;
    }

    ParticleProperty* pProperty = pEmitter->GetCpuParticleProperty();

    if (pEmitter->m_pEmitterRes->m_pFieldCollisionData != nullptr) {
        ParticleData* pParticleData = reinterpret_cast<ParticleData*>(pEmitter->m_ParticleAttr);
        CalculateParticleBehaviorFieldCollision(pPos, pVec, pLife, pEmitter, pProperty,
                                                particleIndex, &pParticleData[particleIndex],
                                                time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldRandomData != nullptr) {
        CalculateParticleBehaviorFieldGpuNoise(pVec, pEmitter, pProperty, particleIndex, time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldRandomSimpleData != nullptr) {
        CalculateParticleBehaviorFieldRandomSimple(pVec, pEmitter, particleIndex, time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldMagnetData != nullptr) {
        CalculateParticleBehaviorFieldMagnet(pPos, pVec, pEmitter, pProperty, particleIndex,
                                             time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldSpinData != nullptr) {
        CalculateParticleBehaviorFieldSpin(pPos, pEmitter, pProperty, particleIndex, time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldConvergenceData != nullptr) {
        CalculateParticleBehaviorFieldConvergence(pPos, pEmitter, pProperty, particleIndex,
                                                  time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldCurlNoiseData != nullptr) {
        CalculateParticleBehavior_FieldCurlNoise(pPos, pVec, pEmitter, pProperty,
                                                 particleIndex);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldPosAddData != nullptr) {
        CalculateParticleBehaviorFieldPosAdd(pPos, pEmitter, pProperty, particleIndex, time);
    }

    if (pEmitter->m_pEmitterRes->m_pFieldCustomData != nullptr) {
        CalculateParticleBehaviorCustomField(pPos, pVec, pLife, pTime, pEmitter, pProperty,
                                             particleIndex);
    }
}

/**
 * Calculates the scale of a particle from its age.
 * @param pOut the scale
 * @param pEmitterRes the emitter resource
 * @param rScale the initial scale of the particle
 * @param rRandom the random values of the particle
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleScaleVecFromTime(util::Vector3fType* pOut,
                                                          const EmitterResource* pEmitterRes,
                                                          const util::Vector4fType& rScale,
                                                          const util::Vector4fType& rRandom,
                                                          f32 life, f32 time) {
    SetX(pOut, GetX(rScale));
    SetY(pOut, GetY(rScale));
    SetZ(pOut, GetZ(rScale));

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;

    if (pUbo == nullptr) {
        return;
    }

    int keyNum = pUbo->animKeyNum[AnimKind_Scale];

    if (keyNum >= 2) {
        const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;
        f32 loopRate = pResEmitter->isAnimLoop[AnimKind_Scale] ?
                           static_cast<f32>(pResEmitter->animLoopRate[AnimKind_Scale]) :
                           0.0f;
        util::Float3 scale;
        Calculate8KeyAnim(&scale, pUbo->scaleAnim, keyNum, GetX(rRandom), time, loopRate,
                          pResEmitter->isAnimStartRandom[AnimKind_Scale], life);
        SetVector(pOut, scale.x * GetX(*pOut), scale.y * GetY(*pOut), scale.z * GetZ(*pOut));
    } else {
        SetX(pOut, pEmitterRes->m_pEmitterStaticUbo->scaleAnim.keys[0].x * GetX(*pOut));
        SetY(pOut, pEmitterRes->m_pEmitterStaticUbo->scaleAnim.keys[0].y * GetY(*pOut));
        SetZ(pOut, pEmitterRes->m_pEmitterStaticUbo->scaleAnim.keys[0].z * GetZ(*pOut));
    }

    ApplyScaleFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates the scale of a particle from its age in frames.
 * @param pOut the scale
 * @param pEmitterRes the emitter resource
 * @param rScale the initial scale of the particle
 * @param rRandom the random values of the particle
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleScaleVecFromFrame(util::Vector3fType* pOut,
                                                           const EmitterResource* pEmitterRes,
                                                           const util::Vector4fType& rScale,
                                                           const util::Vector4fType& rRandom,
                                                           f32 life, f32 time) {
    SetX(pOut, GetX(rScale));
    SetY(pOut, GetY(rScale));
    SetZ(pOut, GetZ(rScale));

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;

    if (pUbo == nullptr) {
        return;
    }

    int keyNum = pUbo->animKeyNum[AnimKind_Scale];

    if (keyNum >= 2) {
        const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;
        f32 loopRate = pResEmitter->isAnimLoop[AnimKind_Scale] ?
                           static_cast<f32>(pResEmitter->animLoopRate[AnimKind_Scale]) :
                           0.0f;
        util::Float3 scale;
        Calculate8KeyAnim(&scale, pUbo->scaleAnim, keyNum, GetX(rRandom), time, loopRate,
                          pResEmitter->isAnimStartRandom[AnimKind_Scale], life);
        SetVector(pOut, scale.x * GetX(*pOut), scale.y * GetY(*pOut), scale.z * GetZ(*pOut));
    }

    ApplyScaleFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates color 0 of a particle and multiplies it by the emitter colors.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param rColor the color of the emitter set
 * @param rEmitterColor the color of the emitter
 * @param alpha the alpha of the emitter
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleColor0VecFromTime(
    util::Vector4fType* pOut, const EmitterResource* pEmitterRes, const util::Vector4fType& rRandom,
    const util::Vector4fType& rColor, const util::Vector3fType& rEmitterColor, f32 alpha, f32 life,
    f32 time) {
    if (!CalculateParticleColor(this, pOut, pEmitterRes, rRandom, 0, life, time)) {
        return;
    }

    MultiplyEmitterColor(pOut, pEmitterRes, rColor, rEmitterColor, alpha);
    ApplyAlphaFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates color 0 of a particle.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleColor0RawValue(util::Vector4fType* pOut,
                                                        const EmitterResource* pEmitterRes,
                                                        const util::Vector4fType& rRandom,
                                                        f32 life, f32 time) {
    if (!CalculateParticleColor(this, pOut, pEmitterRes, rRandom, 0, life, time)) {
        return;
    }

    ApplyAlphaFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates color 1 of a particle and multiplies it by the emitter colors.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param rColor the color of the emitter set
 * @param rEmitterColor the color of the emitter
 * @param alpha the alpha of the emitter
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleColor1VecFromTime(
    util::Vector4fType* pOut, const EmitterResource* pEmitterRes, const util::Vector4fType& rRandom,
    const util::Vector4fType& rColor, const util::Vector3fType& rEmitterColor, f32 alpha, f32 life,
    f32 time) {
    if (!CalculateParticleColor(this, pOut, pEmitterRes, rRandom, 1, life, time)) {
        return;
    }

    MultiplyEmitterColor(pOut, pEmitterRes, rColor, rEmitterColor, alpha);
    ApplyAlphaFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates color 1 of a particle.
 * @param pOut the color
 * @param pEmitterRes the emitter resource
 * @param rRandom the random values of the particle
 * @param life the life of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateParticleColor1RawValue(util::Vector4fType* pOut,
                                                        const EmitterResource* pEmitterRes,
                                                        const util::Vector4fType& rRandom,
                                                        f32 life, f32 time) {
    if (!CalculateParticleColor(this, pOut, pEmitterRes, rRandom, 1, life, time)) {
        return;
    }

    ApplyAlphaFluctuation(pOut, pEmitterRes, rRandom, time);
}

/**
 * Calculates the rotation of a particle from its age.
 * @param pOut the rotation
 * @param pEmitterRes the emitter resource
 * @param rRotate the initial rotation of the particle
 * @param rRandom the random values of the particle
 * @param time the age of the particle
 */
void EmitterCalculator::CalculateRotationMatrix(util::Vector3fType* pOut,
                                                const EmitterResource* pEmitterRes,
                                                const util::Vector4fType& rRotate,
                                                const util::Vector4fType& rRandom, f32 time) {
    SetX(pOut, GetX(rRotate));
    SetY(pOut, GetY(rRotate));
    SetZ(pOut, GetZ(rRotate));

    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;

    if (pUbo == nullptr) {
        return;
    }

    const ResEmitter* pResEmitter = pEmitterRes->m_pResEmitter;
    f32 velX = pUbo->rotateAdd.x +
               pUbo->rotateAddRandom.x * ((GetX(rRandom) + GetY(rRandom)) * 0.5f);
    f32 velY = pUbo->rotateAdd.y +
               pUbo->rotateAddRandom.y * ((GetY(rRandom) + GetZ(rRandom)) * 0.5f);
    f32 velZ = pUbo->rotateAdd.z +
               pUbo->rotateAddRandom.z * ((GetZ(rRandom) + GetX(rRandom)) * 0.5f);
    f32 rotateX = GetX(rRotate);
    f32 rotateY = GetY(rRotate);
    f32 rotateZ = GetZ(rRotate);

    if (pResEmitter->isRotateDirRandom[0] && GetY(rRandom) >= 0.5f) {
        rotateX = -rotateX;
        velX = -velX;
    }

    if (pResEmitter->isRotateDirRandom[1] && GetZ(rRandom) >= 0.5f) {
        rotateY = -rotateY;
        velY = -velY;
    }

    if (pResEmitter->isRotateDirRandom[2] && GetX(rRandom) >= 0.5f) {
        rotateZ = -rotateZ;
        velZ = -velZ;
    }

    rotateX = pUbo->rotateInitRandom.x * GetX(rRandom) + rotateX;
    rotateY = pUbo->rotateInitRandom.y * GetY(rRandom) + rotateY;
    rotateZ = pUbo->rotateInitRandom.z * GetZ(rRandom) + rotateZ;

    f32 regist = pUbo->rotateRegist;
    f32 rate = (1.0f - std::pow(regist, time)) / (1.0f - regist);

    if (regist == 1.0f) {
        rate = time;
    }

    SetVector(pOut, rotateX + velX * rate, rotateY + velY * rate, rotateZ + velZ * rate);
}

/**
 * Gets the constant rotation speed of the particles.
 * @param pOut the rotation speed
 * @param pEmitterRes the emitter resource
 * @param rRotate the rotation of the particle (unused)
 */
void EmitterCalculator::CalculateRotationMatrix(util::Vector3fType* pOut,
                                                const EmitterResource* pEmitterRes,
                                                const util::Vector4fType& rRotate) {
    const EmitterStaticUniformBlock* pUbo = pEmitterRes->m_pEmitterStaticUbo;
    SetVector(pOut, pUbo->rotateAdd.x, pUbo->rotateAdd.y, pUbo->rotateAdd.z);
}

/**
 * Builds a rotation matrix from euler angles, rotating around x, then y, then z.
 * @param pOutMatrix the rotation matrix
 * @param rRotate the euler angles in radians
 */
void EmitterCalculator::MakeRotationMatrixXYZ(util::neon::MatrixRowMajor4x4fType* pOutMatrix,
                                              const util::Vector3fType& rRotate) {
    f32 sinX = SinEst(GetX(rRotate));
    f32 cosX = CosEst(GetX(rRotate));
    f32 sinY = SinEst(GetY(rRotate));
    f32 cosY = CosEst(GetY(rRotate));
    f32 sinZ = SinEst(GetZ(rRotate));
    f32 cosZ = CosEst(GetZ(rRotate));

    f32 sinXSinZ = sinX * sinZ;
    f32 sinXCosZ = sinX * cosZ;
    f32 cosXSinZ = cosX * sinZ;
    f32 cosXCosZ = cosX * cosZ;

    pOutMatrix->_m.val[0] = float32x4_t{cosY * cosZ, cosY * sinZ, -sinY, 0.0f};
    pOutMatrix->_m.val[1] = float32x4_t{sinY * sinXCosZ - cosXSinZ, sinY * sinXSinZ + cosXCosZ,
                                        sinX * cosY, 0.0f};
    pOutMatrix->_m.val[2] = float32x4_t{sinXSinZ + sinY * cosXCosZ, sinY * cosXSinZ - sinXCosZ,
                                        cosX * cosY, 0.0f};
    pOutMatrix->_m.val[3] = float32x4_t{0.0f, 0.0f, 0.0f, 1.0f};
}

}  // namespace detail
}  // namespace vfx
}  // namespace nn
