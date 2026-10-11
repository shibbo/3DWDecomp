#include <nn/vfx/vfx_EmitterCalc.h>

#include <attributes.h>
#include <cmath>
#include <cstring>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_Constants.h>
#include <nn/util/util_MatrixApi.h>
#include <nn/util/util_VectorApi.h>
#include <nn/vfx/System.h>
#include <nn/vfx/vfx_ParticleBehavior.h>
#include <nn/vfx/vfx_System.h>

namespace nn {
namespace vfx {
namespace detail {

namespace {

/** Emitter animations, by their index in the emitter animation arrays. */
enum EmitterAnimKind {
    EmitterAnimKind_Scale,
    EmitterAnimKind_Rotate,
    EmitterAnimKind_Translate,
    EmitterAnimKind_Color0,
    EmitterAnimKind_Color1,
    EmitterAnimKind_EmissionRate,
    EmitterAnimKind_ParticleLife,
    EmitterAnimKind_Alpha0,
    EmitterAnimKind_Alpha1,
    EmitterAnimKind_AllDirectionalVel,
    EmitterAnimKind_DirectionalVel,
    EmitterAnimKind_ParticleScale,
    EmitterAnimKind_EmitterVolumeScale,
    EmitterAnimKind_GravityScale,
    EmitterAnimKind_Max,
};

/** Ways the particles of an emitter follow the emitter matrix. */
enum FollowType {
    FollowType_All,
    FollowType_None,
    FollowType_PosOnly,
};

/** Emitter plugins changing the life of one-time emitters. */
enum EmitterPluginIndex {
    EmitterPluginIndex_ConnectionStripe = 1,
    EmitterPluginIndex_Stripe = 2,
    EmitterPluginIndex_SuperStripe = 3,
};

/** Emitter volume shapes with a special handling. */
enum VolumeType {
    VolumeType_CircleEquallyDivided = 2,
    VolumeType_Primitive = 5,
    VolumeType_PrimitiveEquallyDivided = 6,
    VolumeType_SphereEquallyDivided = 13,
};

/** Value of the life of a particle that never dies. */
const f32 InfiniteLife = 268435456.0f;

/** Smallest difference between 1 and the next float. */
const f32 FloatEpsilon = 1.1920929e-7f;

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
    return vfmsq_f32(vmulq_f32(ShuffleYzx(lhs), ShuffleZxy(rhs)), ShuffleYzx(rhs), ShuffleZxy(lhs));
}

/**
 * Calculates the dot product of two vectors.
 * @param lhs the first vector
 * @param rhs the second vector
 * @return the dot product, in every lane
 */
inline float32x4_t VectorDot(float32x4_t lhs, float32x4_t rhs) {
    float32x4_t product = vmulq_f32(lhs, rhs);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    sum = vpadd_f32(sum, sum);
    return vcombine_f32(sum, sum);
}

/**
 * Calculates the length of a vector.
 * @param vector the vector
 * @return the length, in every lane
 */
inline float32x4_t VectorLength(float32x4_t vector) {
    return vsqrtq_f32(VectorDot(vector, vector));
}

/**
 * Scales a vector to unit length with the reciprocal square root estimate.
 * A zero vector stays zero.
 * @param vector the vector
 * @param lengthSquared the squared length of the vector, in every lane
 * @return the normalized vector
 */
inline float32x4_t VectorNormalizeEst(float32x4_t vector, float32x4_t lengthSquared) {
    float32x4_t estimate = vrsqrteq_f32(lengthSquared);
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(estimate, lengthSquared)));
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(lengthSquared, estimate)));
    uint32x4_t mask = vmvnq_u32(vceqzq_f32(lengthSquared));
    return vreinterpretq_f32_u32(
        vandq_u32(vreinterpretq_u32_f32(vmulq_f32(vector, estimate)), mask));
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
 * Multiplies two matrices.
 * @param pOut the product, which may be one of the operands
 * @param rLhs the matrix applied first
 * @param rRhs the matrix applied second
 */
inline void MatrixMultiply(util::Matrix4x3fType* pOut, const util::Matrix4x3fType& rLhs,
                           const util::Matrix4x3fType& rRhs) {
    float32x4x4_t lhs = rLhs._m;
    float32x4x4_t rhs = rRhs._m;
    float32x4_t row0 = vmulq_laneq_f32(rhs.val[0], lhs.val[0], 0);
    row0 = vfmaq_laneq_f32(row0, rhs.val[1], lhs.val[0], 1);
    row0 = vfmaq_laneq_f32(row0, rhs.val[2], lhs.val[0], 2);
    float32x4_t row1 = vmulq_laneq_f32(rhs.val[0], lhs.val[1], 0);
    row1 = vfmaq_laneq_f32(row1, rhs.val[1], lhs.val[1], 1);
    row1 = vfmaq_laneq_f32(row1, rhs.val[2], lhs.val[1], 2);
    float32x4_t row2 = vmulq_laneq_f32(rhs.val[0], lhs.val[2], 0);
    row2 = vfmaq_laneq_f32(row2, rhs.val[1], lhs.val[2], 1);
    row2 = vfmaq_laneq_f32(row2, rhs.val[2], lhs.val[2], 2);
    float32x4_t row3 = vmulq_laneq_f32(rhs.val[0], lhs.val[3], 0);
    row3 = vfmaq_laneq_f32(row3, rhs.val[1], lhs.val[3], 1);
    row3 = vfmaq_laneq_f32(row3, rhs.val[2], lhs.val[3], 2);
    pOut->_m.val[0] = row0;
    pOut->_m.val[1] = row1;
    pOut->_m.val[2] = row2;
    pOut->_m.val[3] = vaddq_f32(rhs.val[3], row3);
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
 * Stores the emitter matrix of a particle.
 * @param pProperty the particle arrays
 * @param particleIndex the index of the particle
 * @param rMatrix the matrix
 */
inline void StoreParticleEmitterMatrix(ParticleProperty* pProperty, int particleIndex,
                                       const util::Matrix4x3fType& rMatrix) {
    pProperty->pEmitterMatrixRow[0][particleIndex].x = vgetq_lane_f32(rMatrix._m.val[0], 0);
    pProperty->pEmitterMatrixRow[0][particleIndex].y = vgetq_lane_f32(rMatrix._m.val[1], 0);
    pProperty->pEmitterMatrixRow[0][particleIndex].z = vgetq_lane_f32(rMatrix._m.val[2], 0);
    pProperty->pEmitterMatrixRow[0][particleIndex].w = vgetq_lane_f32(rMatrix._m.val[3], 0);
    pProperty->pEmitterMatrixRow[1][particleIndex].x = vgetq_lane_f32(rMatrix._m.val[0], 1);
    pProperty->pEmitterMatrixRow[1][particleIndex].y = vgetq_lane_f32(rMatrix._m.val[1], 1);
    pProperty->pEmitterMatrixRow[1][particleIndex].z = vgetq_lane_f32(rMatrix._m.val[2], 1);
    pProperty->pEmitterMatrixRow[1][particleIndex].w = vgetq_lane_f32(rMatrix._m.val[3], 1);
    pProperty->pEmitterMatrixRow[2][particleIndex].x = vgetq_lane_f32(rMatrix._m.val[0], 2);
    pProperty->pEmitterMatrixRow[2][particleIndex].y = vgetq_lane_f32(rMatrix._m.val[1], 2);
    pProperty->pEmitterMatrixRow[2][particleIndex].z = vgetq_lane_f32(rMatrix._m.val[2], 2);
    pProperty->pEmitterMatrixRow[2][particleIndex].w = vgetq_lane_f32(rMatrix._m.val[3], 2);
}

/**
 * Stores the first three components of a vector.
 * @param pOut the destination
 * @param rVector the vector
 */
template <typename T>
inline void StoreVector(T* pOut, const util::Vector3fType& rVector) {
    vst1_f32(pOut->v, vget_low_f32(rVector._v));
    vst1q_lane_f32(&pOut->v[2], rVector._v, 2);
}

/**
 * Loads the first three components of a vector, one lane at a time.
 * @param rSource the source
 * @return the vector, with a zero w component
 */
template <typename T>
inline float32x4_t LoadVectorLanes(const T& rSource) {
    float32x4_t value = vdupq_n_f32(0.0f);
    value = vld1q_lane_f32(&rSource.v[0], value, 0);
    value = vld1q_lane_f32(&rSource.v[1], value, 1);
    return vld1q_lane_f32(&rSource.v[2], value, 2);
}

/**
 * Builds a vector from its components, starting from a zero vector.
 * @param pOut the vector to write
 * @param x the x component
 * @param y the y component
 * @param z the z component
 */
inline void SetVector(float32x4_t* pOut, f32 x, f32 y, f32 z) {
    float32x4_t v = vdupq_n_f32(0.0f);
    v = vsetq_lane_f32(x, v, 0);
    v = vsetq_lane_f32(y, v, 1);
    v = vsetq_lane_f32(z, v, 2);
    *pOut = v;
}

/**
 * Sets the x component of a vector.
 * @param pOut the vector
 * @param x the new x component
 */
inline void SetX(util::Vector3fType* pOut, f32 x) {
    pOut->_v = vsetq_lane_f32(x, pOut->_v, 0);
}

/**
 * Sets the y component of a vector.
 * @param pOut the vector
 * @param y the new y component
 */
inline void SetY(util::Vector3fType* pOut, f32 y) {
    pOut->_v = vsetq_lane_f32(y, pOut->_v, 1);
}

/**
 * Sets the z component of a vector and clears its unused w component.
 * @param pOut the vector
 * @param z the new z component
 */
inline void SetZ(util::Vector3fType* pOut, f32 z) {
    pOut->_v = vsetq_lane_f32(z, pOut->_v, 2);
    pOut->_v = vsetq_lane_f32(0.0f, pOut->_v, 3);
}

/**
 * Splits the translation of a reserved emission between the particle and its emitter matrix.
 * @param pMatrix the emitter matrix of the particle
 * @param pPos the position of the particle
 * @param followType how the particle follows the emitter
 * @param translate the translation of the reserved emission
 */
inline void ApplyFollowType(util::Matrix4x3fType* pMatrix, util::Vector3fType* pPos, u8 followType,
                            float32x4_t translate) {
    if (followType == FollowType_PosOnly) {
        pPos->_v = vaddq_f32(translate, pPos->_v);
        pMatrix->_m.val[3] = vdupq_n_f32(0.0f);
    } else {
        pMatrix->_m.val[3] = vaddq_f32(translate, vdupq_n_f32(0.0f));
    }
}

/**
 * Extends a row of a 4x3 matrix to four components.
 * @param row the row
 * @param w the fourth component
 * @return the extended row
 */
inline float32x4_t ExtendMatrixRow(float32x4_t row, f32 w) {
    return vcombine_f32(vget_low_f32(row), vset_lane_f32(vgetq_lane_f32(row, 2), vdup_n_f32(w), 0));
}

/**
 * Stores a 4x3 matrix as a column-major 4x4 matrix, as read by the shaders.
 * @param pOut the four columns
 * @param rMatrix the matrix
 */
inline void StoreMatrix4x4(util::Float4* pOut, const util::Matrix4x3fType& rMatrix) {
    float32x4x4_t matrix;
    matrix.val[0] = ExtendMatrixRow(rMatrix._m.val[0], 0.0f);
    matrix.val[1] = ExtendMatrixRow(rMatrix._m.val[1], 0.0f);
    matrix.val[2] = ExtendMatrixRow(rMatrix._m.val[2], 0.0f);
    matrix.val[3] = ExtendMatrixRow(rMatrix._m.val[3], 1.0f);
    float32x4x4_t transposed = util::detail::Matrix4x4fTranspose(matrix);
    vst1q_f32(pOut[0].v, transposed.val[0]);
    vst1q_f32(pOut[1].v, transposed.val[1]);
    vst1q_f32(pOut[2].v, transposed.val[2]);
    vst1q_f32(pOut[3].v, transposed.val[3]);
}

/**
 * Gets how many points of an equally divided shape emit, some of them skipped at random.
 * @param divisionNum the number of points
 * @param divisionRandom the percentage of points that may be skipped
 * @param random the random value of this emission
 * @return the number of emitting points
 */
inline int GetEmitDivisionNum(u32 divisionNum, u32 divisionRandom, f32 random) {
    f32 skipRatio = random * static_cast<f32>(divisionRandom) * 0.01f;
    return divisionNum - static_cast<int>(skipRatio * static_cast<f32>(divisionNum));
}

/**
 * Gets the elapsed time of an emitter, as used by the shaders and the emitter animations.
 * @param pEmitter the emitter
 * @return the time, without the delay when the system asks for it
 */
inline f32 GetEmitterTime(const Emitter* pEmitter) {
    return pEmitter->m_EmitterSet->m_System->m_1700 ? pEmitter->m_SystemFrame :
                                                       pEmitter->m_Frame;
}

/**
 * Estimates the sine and cosine of four angles with polynomials.
 * @param pSin the sines
 * @param pCos the cosines
 * @param angles the angles in radians
 */
inline void SinCosEst(float32x4_t* pSin, float32x4_t* pCos, float32x4_t angles) {
    using namespace util::detail;

    float32x4_t turns = vmulq_n_f32(angles, Float1Divided2Pi);
    float32x4_t rounding = vbslq_f32(vcgezq_f32(turns), vdupq_n_f32(0.5f), vdupq_n_f32(-0.5f));
    turns = vcvtq_f32_s32(vcvtq_s32_f32(vaddq_f32(turns, rounding)));
    angles = vfmsq_n_f32(angles, turns, Float2Pi);
    uint32x4_t upper = vcgtq_f32(angles, vdupq_n_f32(FloatPiDivided2));
    angles = vbslq_f32(upper, vsubq_f32(vdupq_n_f32(FloatPi), angles), angles);
    uint32x4_t lower = vcltq_f32(angles, vdupq_n_f32(-FloatPiDivided2));
    angles = vbslq_f32(lower, vsubq_f32(vdupq_n_f32(-FloatPi), angles), angles);
    float32x4_t sign = vbslq_f32(vorrq_u32(upper, lower), vdupq_n_f32(-1.0f), vdupq_n_f32(1.0f));
    float32x4_t square = vmulq_f32(angles, angles);
    float32x4_t sine = vfmsq_n_f32(vdupq_n_f32(SinCoefficients[1]), square, SinCoefficients[0]);
    float32x4_t cosine = vfmsq_n_f32(vdupq_n_f32(CosCoefficients[1]), square, CosCoefficients[0]);
    sine = vfmaq_f32(vdupq_n_f32(-SinCoefficients[2]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(-CosCoefficients[2]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(SinCoefficients[3]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(CosCoefficients[3]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(-SinCoefficients[4]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(-CosCoefficients[4]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(1.0f), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(1.0f), square, cosine);
    *pSin = vmulq_f32(angles, sine);
    *pCos = vmulq_f32(sign, cosine);
}

/**
 * Reduces an angle to the range [-pi, pi].
 * @param radian the angle in radians
 * @return the equivalent angle in [-pi, pi]
 */
inline f32 ModTwoPi(f32 radian) {
    using namespace util::detail;

    f32 quotient = Float1Divided2Pi * radian + (radian >= 0.0f ? 0.5f : -0.5f);
    return radian - Float2Pi * static_cast<f32>(static_cast<s32>(quotient));
}

/**
 * Estimates the sine and the cosine of an angle with polynomials.
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

/**
 * Makes the rotation matrix of the angles of the x, y and z axes, applied in that order.
 * @param pOut the matrix, without translation
 * @param rRotate the angles in radians
 */
inline void MakeRotateXyzMatrix(util::Matrix4x3fType* pOut, const util::Vector3fType& rRotate) {
    float32x4_t sin;
    float32x4_t cos;
    SinCosEst(&sin, &cos, rRotate._v);

    float32x2_t sinZw = vget_high_f32(sin);
    float32x2_t cosZw = vget_high_f32(cos);
    float32x2_t cosSinZ = vzip1_f32(cosZw, sinZw);
    float32x2_t sinCosZ = vzip1_f32(sinZw, cosZw);
    const float32x2_t signX = {1.0f, 0.0f};
    const float32x2_t signY = {-1.0f, 0.0f};
    const float32x4_t signRow1 = {-1.0f, 1.0f, 0.0f, 0.0f};
    const float32x4_t signRow2 = {1.0f, -1.0f, 0.0f, 0.0f};

    float32x2_t sinY = vset_lane_f32(0.0f, vdup_laneq_f32(sin, 1), 1);
    float32x2_t cosY = vset_lane_f32(0.0f, vdup_laneq_f32(cos, 1), 1);
    float32x4_t row0 = vcombine_f32(vmul_f32(cosSinZ, vdup_laneq_f32(cos, 1)),
                                    vmul_f32(sinY, signY));
    float32x4_t rowY = vcombine_f32(vmul_f32(cosSinZ, vdup_laneq_f32(sin, 1)),
                                    vmul_f32(cosY, signX));
    float32x4_t rowZ = vcombine_f32(sinCosZ, vdup_n_f32(0.0f));

    pOut->_m.val[0] = row0;
    pOut->_m.val[1] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(rowZ, cos, 0), signRow1),
                                vmulq_laneq_f32(rowY, sin, 0));
    pOut->_m.val[2] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(rowZ, sin, 0), signRow2),
                                vmulq_laneq_f32(rowY, cos, 0));
}

/**
 * Makes the shortest rotation from one unit vector to another.
 * @param pOut the rotation quaternion
 * @param rFrom the first unit vector
 * @param rTo the second unit vector
 */
inline void QuaternionMakeVectorRotation(util::Vector4fType* pOut, const util::Vector3fType& rFrom,
                                         const util::Vector3fType& rTo) {
    float32x4_t cross = VectorCross(rFrom._v, rTo._v);
    float32x4_t dot = vaddq_f32(VectorDot(rTo._v, rFrom._v), vdupq_n_f32(1.0f));
    float32x4_t scale = vsqrtq_f32(vaddq_f32(dot, dot));
    float32x4_t quaternion = vdivq_f32(cross, scale);
    quaternion = vsetq_lane_f32(vgetq_lane_f32(vmulq_f32(scale, vdupq_n_f32(0.5f)), 3),
                                quaternion, 3);
    uint32x4_t isOpposite = vcgeq_f32(vdupq_n_f32(util::detail::FloatUlp), dot);
    const float32x4_t halfTurn = {1.0f, 0.0f, 0.0f, 0.0f};
    pOut->_v = vbslq_f32(isOpposite, halfTurn, quaternion);
}

}  // namespace

/**
 * Creates the emitter calculator and the default texture samplers.
 * @param pSystem the system owning the calculator
 */
EmitterCalculator::EmitterCalculator(System* pSystem) {
    m_pSystem = pSystem;

    m_DefaultSamplerRes.filter = 1;
    m_DefaultSamplerRes.wrapU = 1;
    m_DefaultSamplerRes.wrapV = 0;
    m_DefaultSamplerRes.wrapW = 0;
    m_DefaultSamplerRes.maxLod = 15.99f;
    m_DefaultSamplerRes.lodBias = 0.0f;
    m_pDefaultSampler = TextureSampler::GetSamplerFromTable(&m_DefaultSamplerRes);

    m_MirrorSamplerRes.filter = 2;
    m_MirrorSamplerRes.wrapU = 2;
    m_MirrorSamplerRes.wrapV = 0;
    m_MirrorSamplerRes.wrapW = 0;
    m_MirrorSamplerRes.maxLod = 15.99f;
    m_MirrorSamplerRes.lodBias = 0.0f;
    m_pMirrorSampler = TextureSampler::GetSamplerFromTable(&m_MirrorSamplerRes);
}

/**
 * Destroys the emitter calculator.
 */
EmitterCalculator::~EmitterCalculator() {}

/**
 * Builds the emitter matrices of the emitter animation.
 * @param pEmitter the emitter
 * @param pOutMatrixSrt the scale, rotation and translation matrix
 * @param pOutMatrixRt the rotation and translation matrix
 */
void EmitterCalculator::ApplyEmitterAnimation(Emitter* pEmitter,
                                              util::Matrix4x3fType* pOutMatrixSrt,
                                              util::Matrix4x3fType* pOutMatrixRt) {
    util::MatrixIdentity(pOutMatrixSrt);
    util::MatrixIdentity(pOutMatrixRt);

    const EmitterAnimValue& rAnim = pEmitter->m_EmitterAnimValue;
    util::Vector3fType scale;
    util::VectorLoad(&scale, rAnim.scale);
    util::Vector3fType rotate;

    if (pEmitter->m_pEmitterRes->m_EmitterAnimArray[EmitterAnimKind_Rotate] != nullptr) {
        util::VectorSet(&rotate, util::DegreeToRadian(rAnim.rotate.x),
                        util::DegreeToRadian(rAnim.rotate.y),
                        util::DegreeToRadian(rAnim.rotate.z));
    } else {
        util::VectorLoad(&rotate, rAnim.rotate);
    }

    util::Matrix4x3fType rotateMatrix;
    MakeRotateXyzMatrix(&rotateMatrix, rotate);
    util::Vector3fType translate;
    util::VectorLoad(&translate, rAnim.translate);

    pOutMatrixRt->_m.val[0] = rotateMatrix._m.val[0];
    pOutMatrixRt->_m.val[1] = rotateMatrix._m.val[1];
    pOutMatrixRt->_m.val[2] = rotateMatrix._m.val[2];
    pOutMatrixRt->_m.val[3] = translate._v;
    pOutMatrixSrt->_m.val[0] = vmulq_laneq_f32(rotateMatrix._m.val[0], scale._v, 0);
    pOutMatrixSrt->_m.val[1] = vmulq_laneq_f32(rotateMatrix._m.val[1], scale._v, 1);
    pOutMatrixSrt->_m.val[2] = vmulq_laneq_f32(rotateMatrix._m.val[2], scale._v, 2);
    pOutMatrixSrt->_m.val[3] = translate._v;
}

/**
 * Emits the particles due this frame, by time or by travelled distance.
 * @param pIntervalCounter the time since the last emission
 * @param pEmitCounter the fractional amount of particles carried to the next emission
 * @param pInterval the time between two emissions
 * @param pIsEmitted set when a particle is emitted
 * @param pEmitter the emitter
 * @param frameRate the length of the frame
 * @param isSearchFreeSlot whether to look for a free particle slot before reusing the oldest one
 */
void EmitterCalculator::TryEmitParticle(f32* pIntervalCounter, f32* pEmitCounter, f32* pInterval,
                                        u8* pIsEmitted, Emitter* pEmitter, f32 frameRate,
                                        bool isSearchFreeSlot) {
    const ResEmitter* pRes = pEmitter->m_pEmitterData;
    EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;

    if (pRes->isEmitDistance) {
        if (!pEmitterSet->m_IsEmitDistanceEnabled) {
            return;
        }

        f32 length = vgetq_lane_f32(VectorLength(pEmitter->m_EmitterLocalVec._v), 0);
        f32 distance = *pEmitCounter;

        if (length < pRes->emitDistanceMargin) {
            length = 0.0f;
        }

        f32 virtualLength;

        if (length == 0.0f) {
            virtualLength = pRes->emitDistanceMin;
        } else if (length < pRes->emitDistanceMin) {
            virtualLength = length * pRes->emitDistanceMin / length;
        } else if (pRes->emitDistanceMax < length) {
            virtualLength = length * pRes->emitDistanceMax / length;
        } else {
            virtualLength = length;
        }

        distance += virtualLength;

        if (pRes->emitDistanceUnit != 0.0f) {
            int emitCount = static_cast<int>(distance / pRes->emitDistanceUnit);

            if (emitCount >= 1) {
                util::Vector3fType currentPos;
                currentPos._v = pEmitter->m_MatrixSrt._m.val[3];
                float32x4_t moveVec = vsubq_f32(pEmitter->m_EmitterPrevPos._v, currentPos._v);

                for (int i = 0; i < emitCount; i++) {
                    distance -= pEmitter->m_pEmitterData->emitDistanceUnit;
                    f32 ratio = virtualLength != 0.0f ? distance / virtualLength : 0.0f;
                    float32x4_t pos = vfmaq_n_f32(currentPos._v, moveVec, ratio);
                    pEmitter->m_MatrixRt._m.val[3] = pos;
                    pEmitter->m_MatrixSrt._m.val[3] = pos;
                    Emit(pIsEmitted, pEmitter, 1, isSearchFreeSlot, nullptr);
                    pEmitter->m_MatrixRt._m.val[3] = currentPos._v;
                    pEmitter->m_MatrixSrt._m.val[3] = currentPos._v;
                }

                pEmitter->m_LastEmitFrame = pEmitter->m_Frame;
            }
        }

        *pEmitCounter = distance;
        return;
    }

    f32 intervalCounter = *pIntervalCounter;
    f32 interval = *pInterval;

    if (!(intervalCounter >= interval)) {
        *pIntervalCounter = intervalCounter + frameRate;
        return;
    }

    f32 randomRate = pRes->emitRate * (static_cast<f32>(pRes->emitRateRandom) / -100.0f);
    f32 random = pEmitter->m_Random.GetF32();
    randomRate *= random;
    bool isPrimitive = pRes->volumeType == VolumeType_Primitive ||
                       pRes->volumeType == VolumeType_PrimitiveEquallyDivided;
    const f32* pEmitRate =
        isPrimitive ? &pRes->emitRate : &pEmitter->m_EmitterAnimValue.emissionRate.x;
    f32 emitRate = (*pEmitRate + randomRate) * pEmitter->m_EmitRatio;
    f32 count = *pEmitCounter + emitRate * pEmitterSet->m_EmissionRatioScale;
    count = count < 0.0f ? 0.0f : count;
    int emitCount = static_cast<int>(count);
    *pEmitCounter = count;

    if (emitCount == 0) {
        *pIntervalCounter = 0.0f;
        return;
    }

    f32 rest = intervalCounter - interval;
    Emit(pIsEmitted, pEmitter, emitCount, isSearchFreeSlot, nullptr);
    pEmitter->UpdateByEmit(pInterval);
    pEmitter->m_EmitterSet->m_IsMatrixUpdated = 1;
    *pEmitCounter -= static_cast<f32>(emitCount);
    *pIntervalCounter = rest + frameRate;
    pEmitter->m_LastEmitFrame = pEmitter->m_Frame;
}

/**
 * Calculates the move of the emitter since the previous frame, in emitter space.
 * @param pOut the move
 * @param pEmitter the emitter
 */
void EmitterCalculator::UpdateEmitterLocalVec(util::Vector3fType* pOut, Emitter* pEmitter) const {
    util::Matrix4x3fType inverse;
    MatrixInverse(&inverse, pEmitter->m_MatrixSrt);

    util::Vector3fType currentPos;
    currentPos._v = pEmitter->m_MatrixSrt._m.val[3];
    util::Vector3fType localCurrentPos;
    VectorTransform(&localCurrentPos, currentPos, inverse);
    util::Vector3fType localPrevPos;
    VectorTransform(&localPrevPos, pEmitter->m_EmitterPrevPos, inverse);
    pOut->_v = vsubq_f32(localCurrentPos._v, localPrevPos._v);
}

/**
 * Copies the particles of a CPU emitter to the buffers read by the GPU.
 * @param pEmitter the emitter
 */
void EmitterCalculator::UpdateCurrentParticleGpuBufferForCpuEmitter(Emitter* pEmitter) {
    if (pEmitter->m_pEmitterData->calcType != 0) {
        return;
    }

    ParticleProperty* pGpu = pEmitter->m_pGpuParticleProperty;
    const ParticleProperty* pCpu = pEmitter->GetCpuParticleProperty();
    size_t size = pEmitter->m_ParticleNum * sizeof(util::Float4);
    std::memcpy(pGpu->pPos, pCpu->pPos, size);
    std::memcpy(pGpu->pVec, pCpu->pVec, size);
    std::memcpy(pGpu->pPosDelta, pCpu->pPosDelta, size);
    std::memcpy(pGpu->pAnimRandom, pCpu->pAnimRandom, size);
    std::memcpy(pGpu->pScale, pCpu->pScale, size);
    std::memcpy(pGpu->pRotate, pCpu->pRotate, size);

    if (pGpu->pColor0 != nullptr) {
        std::memcpy(pGpu->pColor0, pCpu->pColor0, size);
        std::memcpy(pGpu->pColor1, pCpu->pColor1, size);
    }

    if (pGpu->pEmitterMatrixRow[0] != nullptr) {
        std::memcpy(pGpu->pEmitterMatrixRow[0], pCpu->pEmitterMatrixRow[0], size);
        std::memcpy(pGpu->pEmitterMatrixRow[1], pCpu->pEmitterMatrixRow[1], size);
        std::memcpy(pGpu->pEmitterMatrixRow[2], pCpu->pEmitterMatrixRow[2], size);
    }
}

/**
 * Sets the emitter matrices to the local matrices of the emitter, animated or not.
 * @param pEmitter the emitter
 */
inline void EmitterCalculator::SetEmitterLocalMatrix(Emitter* pEmitter) {
    if (pEmitter->m_pEmitterRes->m_IsUseEmitterMatrixAnim) {
        util::Matrix4x3fType matrixSrt;
        util::Matrix4x3fType matrixRt;
        ApplyEmitterAnimation(pEmitter, &matrixSrt, &matrixRt);
        pEmitter->m_MatrixSrt = matrixSrt;
        pEmitter->m_MatrixRt = matrixRt;
    } else {
        pEmitter->m_MatrixSrt = pEmitter->m_ResMatrixSrt;
        pEmitter->m_MatrixRt = pEmitter->m_ResMatrixRt;
    }
}

/**
 * Moves the emitter matrices of a child emitter into the space of its parent emitter.
 * @param pEmitter the child emitter
 */
inline void EmitterCalculator::ApplyParentEmitterMatrix(Emitter* pEmitter) {
    Emitter* pParent = pEmitter->m_pParentEmitter;

    if (pParent->m_pEmitterData->followType != FollowType_None) {
        MatrixMultiply(&pEmitter->m_MatrixSrt, pEmitter->m_MatrixSrt, pParent->m_MatrixSrt);
        MatrixMultiply(&pEmitter->m_MatrixRt, pEmitter->m_MatrixRt, pParent->m_MatrixRt);
    }
}

/**
 * Updates the emitter matrices from the emitter set or the parent emitter.
 * @param pEmitter the emitter
 */
void EmitterCalculator::UpdateEmitterMatrix(Emitter* pEmitter) {
    if (pEmitter->m_IsChildEmitter) {
        bool isIndependentChild = pEmitter->m_pEmitterData->isIndependentChild;
        SetEmitterLocalMatrix(pEmitter);

        if (isIndependentChild) {
            pEmitter->m_MatrixSrt._m.val[3] = vaddq_f32(pEmitter->m_MatrixSrt._m.val[3],
                                                        pEmitter->m_ParentParticleWorldPos._v);
            pEmitter->m_MatrixRt._m.val[3] = pEmitter->m_MatrixSrt._m.val[3];
        }

        ApplyParentEmitterMatrix(pEmitter);
    } else {
        EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;

        if (pEmitter->m_pEmitterRes->m_IsUseEmitterMatrixAnim) {
            util::Matrix4x3fType matrixSrt;
            util::Matrix4x3fType matrixRt;
            ApplyEmitterAnimation(pEmitter, &matrixSrt, &matrixRt);
            MatrixMultiply(&pEmitter->m_MatrixSrt, matrixSrt, pEmitterSet->m_MatrixSrt);
            MatrixMultiply(&pEmitter->m_MatrixRt, matrixRt, pEmitterSet->m_MatrixRt);
        } else if (pEmitterSet->m_IsMatrixUpdated != 0) {
            MatrixMultiply(&pEmitter->m_MatrixSrt, pEmitter->m_ResMatrixSrt,
                           pEmitterSet->m_MatrixSrt);
            MatrixMultiply(&pEmitter->m_MatrixRt, pEmitter->m_ResMatrixRt,
                           pEmitterSet->m_MatrixRt);
        }
    }

    EmitterMatrixSetArg arg;
    arg.pEmitter = pEmitter;
    CallbackSet* pCallbackSet2 = pEmitter->m_pCallbackSet[2];
    CallbackSet* pCallbackSet0 = pEmitter->m_pCallbackSet[0];
    CallbackSet* pCallbackSet1 = pEmitter->m_pCallbackSet[1];

    if (pCallbackSet2 != nullptr && pCallbackSet2->emitterMatrixSet != nullptr) {
        pCallbackSet2->emitterMatrixSet(arg);
    }

    if (pCallbackSet0 != nullptr && pCallbackSet0->emitterMatrixSet != nullptr) {
        pCallbackSet0->emitterMatrixSet(arg);
    }

    if (pCallbackSet1 != nullptr && pCallbackSet1->emitterMatrixSet != nullptr) {
        pCallbackSet1->emitterMatrixSet(arg);
    }
}

/**
 * Advances an emitter by one frame: animations, matrices, emission and particles.
 * @param pEmitter the emitter
 * @param frameRate the length of the frame
 * @param swapMode how the buffers of the emitter are swapped
 * @param isFade whether the emitter set fades out
 * @param isEmit whether the emitter may emit particles
 * @param isCalculateParticle whether a GPU emitter with CPU particles calculates them
 * @return whether the emitter is still alive
 */
bool EmitterCalculator::Calculate(Emitter* pEmitter, f32 frameRate, BufferSwapMode swapMode,
                                  bool isFade, bool isEmit, bool isCalculateParticle) {
    const ResEmitter* pRes = pEmitter->m_pEmitterData;
    f32 time = pEmitter->m_Frame;
    EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;
    u32 emitStartFrame = pRes->emitStartFrame;
    u32 emitDuration = pRes->emitDuration;

    bool isSwapped = pEmitter->SwapBuffer(swapMode);

    if (isSwapped) {
        pEmitter->m_AccumulatedFrameRate = frameRate;
    } else if (pEmitter->m_IsEmitted) {
        pEmitter->m_AccumulatedFrameRate += frameRate;
    }

    if (pEmitter->m_IsDead) {
        return false;
    }

    if (time > 0.0f && pEmitter->m_EmitterSet->m_System->m_1700) {
        pEmitter->m_SystemFrame += frameRate;
    }

    {
        EmitterPreCalculateArg arg;
        arg.pEmitter = pEmitter;
        arg.isBufferSwapped = isSwapped;
        CallbackSet* pCallbackSet2 = pEmitter->m_pCallbackSet[2];
        CallbackSet* pCallbackSet0 = pEmitter->m_pCallbackSet[0];
        CallbackSet* pCallbackSet1 = pEmitter->m_pCallbackSet[1];

        if (pCallbackSet2 != nullptr && pCallbackSet2->emitterPreCalculate != nullptr) {
            pCallbackSet2->emitterPreCalculate(arg);
        }

        if (pCallbackSet0 != nullptr && pCallbackSet0->emitterPreCalculate != nullptr) {
            pCallbackSet0->emitterPreCalculate(arg);
        }

        if (pCallbackSet1 != nullptr && pCallbackSet1->emitterPreCalculate != nullptr) {
            pCallbackSet1->emitterPreCalculate(arg);
        }
    }

    f32 emitStart = static_cast<f32>(emitStartFrame);
    f32 emitEnd = static_cast<f32>(emitDuration + emitStartFrame);
    pEmitter->m_FrameRate = frameRate;

    if (pEmitter->m_IsChildEmitter && pEmitter->m_pEmitterData->isIndependentChild) {
        emitEnd = pEmitter->m_ParentParticleLife;
        emitStart = emitEnd * (static_cast<f32>(pRes->emitStartRatio) / 100.0f);

        if (pRes->isOneTime) {
            emitEnd = emitStart + static_cast<f32>(pRes->emitDuration);
        }
    }

    EmitterResource* pEmitterRes = pEmitter->m_pEmitterRes;
    pEmitter->m_UpdatedAnimNum = 0;

    if (pEmitterRes->m_IsUseEmitterAnim &&
        (!pEmitter->m_IsChildEmitter || pEmitter->m_pEmitterData->isIndependentChild)) {
        f32 animTime = GetEmitterTime(pEmitter);

        for (int i = 0; i < EmitterAnimKind_Max; i++) {
            const ResAnimEmitterKeyParamSet* pAnim = pEmitterRes->m_EmitterAnimArray[i];

            if (pAnim != nullptr && pAnim->enable &&
                (!pEmitter->m_IsEmitterAnimEnd[i] || pAnim->loop)) {
                CalculateEmitterKeyFrameAnimation(&pEmitter->m_EmitterAnimValue.values[i],
                                                  &pEmitter->m_IsEmitterAnimEnd[i], pAnim,
                                                  animTime);
                pEmitter->m_UpdatedAnimNum++;
                pEmitterRes = pEmitter->m_pEmitterRes;
            }
        }

        const ResAnimEmitterKeyParamSet* pAnim =
            pEmitterRes->m_EmitterAnimArray[EmitterAnimKind_AllDirectionalVel];

        if (pAnim != nullptr && pAnim->enable &&
            (!pEmitter->m_IsEmitterAnimEnd[EmitterAnimKind_AllDirectionalVel] || pAnim->loop)) {
            pEmitter->m_EmitterAnimValue.allDirectionalVel.x *= pEmitterSet->m_EmissionSpeedScale;
        }

        pAnim = pEmitterRes->m_EmitterAnimArray[EmitterAnimKind_EmitterVolumeScale];

        if (pAnim != nullptr && pAnim->enable &&
            (!pEmitter->m_IsEmitterAnimEnd[EmitterAnimKind_EmitterVolumeScale] || pAnim->loop)) {
            util::Float3& rVolumeScale = pEmitter->m_EmitterAnimValue.emitterVolumeScale;
            rVolumeScale.x *= vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 0);
            rVolumeScale.y *= vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 1);
            rVolumeScale.z *= vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 2);
        }

        if (pEmitter->m_UpdatedAnimNum >= 1) {
            f32 life = static_cast<f32>(pEmitterRes->m_pResEmitter->particleLife);

            if (pEmitter->m_EmitterAnimValue.particleLife.x > life) {
                pEmitter->m_EmitterAnimValue.particleLife.x = life;
            }
        }

        UpdateEmitterMatrix(pEmitter);
    } else {
        pEmitter->m_EmitterAnimValue.allDirectionalVel.x =
            pEmitterRes->m_pResEmitter->allDirectionalVel * pEmitterSet->m_EmissionSpeedScale;
        const util::Float3& rResVolumeScale = pEmitterRes->m_pResEmitter->volumeScale;
        util::Float3& rVolumeScale = pEmitter->m_EmitterAnimValue.emitterVolumeScale;
        rVolumeScale.x =
            rResVolumeScale.x * vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 0);
        rVolumeScale.y =
            rResVolumeScale.y * vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 1);
        rVolumeScale.z =
            rResVolumeScale.z * vgetq_lane_f32(pEmitterSet->m_EmitterVolumeScale._v, 2);

        UpdateEmitterMatrix(pEmitter);
    }

    if (time == 0.0f) {
        pEmitter->m_EmitterPrevPos._v = pEmitter->m_MatrixSrt._m.val[3];
    }

    if (pEmitter->m_Frame != 0.0f) {
        UpdateEmitterLocalVec(&pEmitter->m_EmitterLocalVec, pEmitter);
    }

    if (pRes->isFadeInAlpha || pRes->isFadeInScale) {
        if (pEmitter->m_FadeInRatio < 1.0f && pRes->fadeInFrame >= 1) {
            pEmitter->m_FadeInRatio += frameRate / static_cast<f32>(pRes->fadeInFrame);

            if (pEmitter->m_FadeInRatio > 1.0f) {
                pEmitter->m_FadeInRatio = 1.0f;
            }
        }
    }

    if (isFade || pEmitter->m_IsSoloFade) {
        isEmit &= pRes->isStopEmitInFade == 0;

        if (pRes->isFadeOutAlpha || pRes->isFadeOutScale) {
            if (pRes->fadeOutFrame <= 0) {
                pEmitter->m_FadeOutRatio = 0.0f;
                return false;
            }

            pEmitter->m_FadeOutRatio -= frameRate / static_cast<f32>(pRes->fadeOutFrame);

            if (pEmitter->m_FadeOutRatio < 0.0f) {
                pEmitter->m_FadeOutRatio = 0.0f;
                return false;
            }
        }
    }

    pEmitter->m_IsParticleFull = false;

    if (!pEmitter->m_IsChildEmitter && pEmitterSet->m_IsManualEmission &&
        pEmitterSet->m_EmitReservationNum >= 1) {
        pEmitter->m_LastEmitFrame = pEmitter->m_Frame;
        const ResEmitter* pResEmitter = pEmitter->m_pEmitterData;
        f32 randomRatio = static_cast<f32>(pResEmitter->emitRateRandom);
        f32 emitRate = pResEmitter->emitRate;
        f32 random = pEmitter->m_Random.GetF32();

        if (pEmitterSet->m_EmitReservationNum >= 1) {
            f32 randomRate = emitRate * (randomRatio / 100.0f) * random;

            for (int i = 0; i < pEmitterSet->m_EmitReservationNum; i++) {
                const EmitReservationInfo* pInfo = &pEmitterSet->m_pEmitReservationInfo[i];
                f32 count = (pEmitter->m_pEmitterData->emitRate - randomRate) *
                            pEmitter->m_EmitRatio * pInfo->emitRatio;
                Emit(&pEmitter->m_IsEmitted, pEmitter, static_cast<int>(count), false, pInfo);
            }
        }
    }

    if (isEmit && pEmitter->m_IsEmitEnabled &&
        (!pEmitter->m_IsChildEmitter || pEmitter->m_pEmitterData->isIndependentChild) &&
        emitStart <= time) {
        bool isEmitFinished;

        if (pRes->isOneTime) {
            isEmitFinished = emitEnd <= time && pEmitter->m_IsEmitted;
        } else {
            isEmitFinished =
                emitEnd <= time && pEmitter->m_IsChildEmitter && pEmitter->m_IsEmitted;
        }

        if (!isEmitFinished) {
            TryEmitParticle(&pEmitter->m_EmitIntervalCounter, &pEmitter->m_EmitCounter,
                            &pEmitter->m_EmitInterval, &pEmitter->m_IsEmitted, pEmitter, frameRate,
                            false);
        }
    }

    if (pEmitter->m_pEmitterRes->m_ChildEmitterResNum >= 1) {
        for (int i = 0; i < pEmitter->m_ParticleNum; i++) {
            if (pEmitter->GetParticleData()[i].createId != 0) {
                pEmitter->m_pParentParticleData[i].time += frameRate;
            }
        }
    }

    u8 calcType = pRes->calcType;

    if (calcType == 0 || (isCalculateParticle && calcType == 2)) {
        pEmitter->m_EmitterCalculator->CalculateParticle(pEmitter);
    } else {
        pEmitter->m_AliveParticleNum = pEmitter->m_ParticleNum;
    }

    {
        EmitterPostCalculateArg arg;
        arg.pEmitter = pEmitter;
        CallbackSet* pCallbackSet2 = pEmitter->m_pCallbackSet[2];
        CallbackSet* pCallbackSet0 = pEmitter->m_pCallbackSet[0];
        CallbackSet* pCallbackSet1 = pEmitter->m_pCallbackSet[1];

        if (pCallbackSet2 != nullptr && pCallbackSet2->emitterPostCalculate != nullptr) {
            pCallbackSet2->emitterPostCalculate(arg);
        }

        if (pCallbackSet0 != nullptr && pCallbackSet0->emitterPostCalculate != nullptr) {
            pCallbackSet0->emitterPostCalculate(arg);
        }

        if (pCallbackSet1 != nullptr && pCallbackSet1->emitterPostCalculate != nullptr) {
            pCallbackSet1->emitterPostCalculate(arg);
        }
    }

    if (pEmitter->m_ParticleNum > 0 || pEmitter->m_pEmitterRes->m_pEmitterPluginData != nullptr) {
        MakeDynamicConstantBuffer(pEmitter, pEmitter->m_FrameRate,
                                  pEmitter->m_AccumulatedFrameRate);
    }

    pEmitter->m_EmitterPrevPos._v = pEmitter->m_MatrixSrt._m.val[3];
    pEmitter->m_Frame += frameRate;

    if (pEmitter->m_IsChildEmitter && !pEmitter->m_pEmitterData->isIndependentChild) {
        if (pEmitter->m_IsParentFinished == 1 &&
            time > emitStart + pEmitter->m_LastEmitFrame + static_cast<f32>(pRes->particleLife)) {
            return false;
        }

        return true;
    }

    if (pRes->isOneTime && !pEmitterSet->m_IsManualEmission) {
        if (pRes->isInfinityLife == 1) {
            return true;
        }

        f32 endFrame = emitEnd + static_cast<f32>(pRes->particleLife);

        if (pRes->isFadeOutAlpha || pRes->isFadeOutScale) {
            endFrame += static_cast<f32>(static_cast<u32>(pRes->fadeOutFrame));
        }

        s32 pluginIndex = pEmitter->m_pEmitterRes->m_EmitterPluginIndex;

        if (pluginIndex > 0) {
            if (pluginIndex == EmitterPluginIndex_Stripe) {
                endFrame +=
                    static_cast<f32>(StripeSystem::GetExtendedEndTimeForOneTimeEmitter(pEmitter));
            } else if (pluginIndex == EmitterPluginIndex_SuperStripe) {
                endFrame += static_cast<f32>(
                    SuperStripeSystem::GetExtendedEndTimeForOneTimeEmitter(pEmitter));
            }
        }

        if (time > endFrame) {
            return false;
        }

        return true;
    }

    if (isFade || pEmitter->m_IsSoloFade || pEmitter->m_IsChildEmitter) {
        if (time > emitStart + pEmitter->m_LastEmitFrame + static_cast<f32>(pRes->particleLife)) {
            return false;
        }
    }

    if (pEmitterSet->m_IsManualEmission && pEmitter->IsManualEmitterReadyToExit()) {
        return false;
    }

    return true;
}

/**
 * Moves the particles of a CPU emitter and emits the particles of its child emitters.
 * @param pEmitter the emitter
 */
void EmitterCalculator::CalculateParticle(Emitter* pEmitter) {
    CallbackSet* pCallbackSet2 = pEmitter->m_pCallbackSet[2];
    ParticleRemoveCallback pRemoveCallback2 = nullptr;
    ParticleCalculateCallback pCalculateCallback2 = nullptr;

    if (pCallbackSet2 != nullptr) {
        pRemoveCallback2 = pCallbackSet2->particleRemove;
        pCalculateCallback2 = pCallbackSet2->particleCalculate;
    }

    CallbackSet* pCallbackSet0 = pEmitter->m_pCallbackSet[0];
    ParticleRemoveCallback pRemoveCallback0 = nullptr;
    ParticleCalculateCallback pCalculateCallback0 = nullptr;

    if (pCallbackSet0 != nullptr) {
        pRemoveCallback0 = pCallbackSet0->particleRemove;
        pCalculateCallback0 = pCallbackSet0->particleCalculate;
    }

    CallbackSet* pCallbackSet1 = pEmitter->m_pCallbackSet[1];
    ParticleRemoveCallback pRemoveCallback1 = nullptr;
    ParticleCalculateCallback pCalculateCallback1 = nullptr;

    if (pCallbackSet1 != nullptr) {
        pRemoveCallback1 = pCallbackSet1->particleRemove;
        pCalculateCallback1 = pCallbackSet1->particleCalculate;
    }

    bool isRemoveCallback = pRemoveCallback2 != nullptr || pRemoveCallback0 != nullptr ||
                            pRemoveCallback1 != nullptr;
    bool isCalculateCallback = pCalculateCallback2 != nullptr || pCalculateCallback0 != nullptr ||
                               pCalculateCallback1 != nullptr;
    int aliveNum = 0;

    for (int i = 0; i < pEmitter->m_ParticleNum; i++) {
        ParticleData* pData = &pEmitter->GetParticleData()[i];

        if (pData->createId == 0) {
            continue;
        }

        ParentParticleData* pParentData = pEmitter->m_pParentParticleData;
        f32 time = pEmitter->m_Frame - pData->createTime;
        f32 life = pData->life;

        if (time >= life) {
            if (isRemoveCallback) {
                ParticleCalculateArgImpl arg;
                arg.pUserData = nullptr;
                arg.pUserData2 = nullptr;
                arg.pEmitter = pEmitter;
                arg.time = time;
                arg.life = life;
                arg.particleIndex = i;
                arg.pUserData = pEmitter->GetParticleData()[i].pUserData;
                arg.pUserData2 = pEmitter->GetParticleData()[i].pUserData2;

                if (pRemoveCallback2 != nullptr) {
                    pRemoveCallback2(arg);
                }

                if (pRemoveCallback0 != nullptr) {
                    pRemoveCallback0(arg);
                }

                if (pRemoveCallback1 != nullptr) {
                    pRemoveCallback1(arg);
                }
            }

            pEmitter->GetParticleData()[i].pUserData2 = nullptr;
            pEmitter->GetParticleData()[i].pUserData = nullptr;
            pData->createId = 0;
        } else {
            util::Vector3fType pos;
            util::VectorLoad(&pos, reinterpret_cast<util::Float3&>(pEmitter->m_ParticlePos[i]));
            util::Vector3fType prevPos = pos;
            f32 createTime = pData->createTime;
            f32 newLife = pData->life;
            util::Vector3fType vec;
            util::VectorLoad(&vec, reinterpret_cast<util::Float3&>(pEmitter->m_ParticleVec[i]));
            CalculateParticleBehavior(&pos, &vec, &createTime, &newLife, pEmitter, i, time, pos,
                                      vec);
            StoreVector(&pEmitter->m_ParticlePos[i], pos);
            StoreVector(&pEmitter->m_ParticleVec[i], vec);
            pData->createTime = createTime;
            pData->life = newLife;

            if (isCalculateCallback) {
                ParticleCalculateArgImpl arg;
                arg.pUserData = nullptr;
                arg.pUserData2 = nullptr;
                arg.pEmitter = pEmitter;
                arg.time = time;
                arg.life = life;
                arg.particleIndex = i;
                arg.pUserData = pEmitter->GetParticleData()[i].pUserData;
                arg.pUserData2 = pEmitter->GetParticleData()[i].pUserData2;

                if (pCalculateCallback2 != nullptr) {
                    pCalculateCallback2(arg);
                }

                if (pCalculateCallback0 != nullptr) {
                    pCalculateCallback0(arg);
                }

                if (pCalculateCallback1 != nullptr) {
                    pCalculateCallback1(arg);
                }

                pEmitter->GetParticleData()[i].pUserData = arg.pUserData;
            }

            pEmitter->m_ParticlePos[i].w = pData->life;
            pEmitter->m_ParticleVec[i].w = pData->createTime;

            float32x4_t delta = vsubq_f32(pos._v, prevPos._v);

            if (std::fabs(vgetq_lane_f32(delta, 0)) > 0.0001f ||
                std::fabs(vgetq_lane_f32(delta, 1)) > 0.0001f ||
                std::fabs(vgetq_lane_f32(delta, 2)) > 0.0001f) {
                util::Vector3fType deltaVector;
                deltaVector._v = delta;
                StoreVector(&pEmitter->m_ParticlePosDelta[i], deltaVector);
            }

            for (int j = 0; j < pEmitter->m_pEmitterRes->m_ChildEmitterResNum; j++) {
                Emitter* pChild = pParentData[i].pChildEmitter[j];

                if (pChild == nullptr) {
                    break;
                }

                if (pChild->m_EmitterCalculator == nullptr) {
                    continue;
                }

                const ResEmitter* pChildRes = pChild->m_pEmitterData;

                if (pChildRes->isIndependentChild) {
                    continue;
                }

                if (pData->createId == 0) {
                    continue;
                }

                f32 particleTime = pParentData[i].time;

                if (pChild->m_Frame == 0.0f) {
                    if (pChild->m_pEmitterRes->m_IsUseEmitterMatrixAnim) {
                        util::Matrix4x3fType matrixSrt;
                        util::Matrix4x3fType matrixRt;
                        ApplyEmitterAnimation(pChild, &matrixSrt, &matrixRt);
                        pChild->m_MatrixSrt = matrixSrt;
                        pChild->m_MatrixRt = matrixRt;
                    } else {
                        pChild->m_MatrixSrt = pChild->m_ResMatrixSrt;
                        pChild->m_MatrixRt = pChild->m_ResMatrixRt;
                    }
                }

                f32 emitStart =
                    pData->life * (static_cast<f32>(pChildRes->emitStartRatio) / 100.0f);

                if (!(emitStart <= particleTime)) {
                    continue;
                }

                u8* pIsEmitted = &pParentData[i].isEmitted[j];

                if (pChildRes->isOneTime) {
                    if (emitStart + static_cast<f32>(pChildRes->emitDuration) <= particleTime &&
                        *pIsEmitted) {
                        continue;
                    }
                } else if (pData->life <= particleTime && pChild->m_IsChildEmitter &&
                           *pIsEmitted) {
                    continue;
                }

                util::Vector3fType particlePos;
                util::VectorLoad(&particlePos,
                                 reinterpret_cast<util::Float3&>(pEmitter->m_ParticlePos[i]));
                util::Vector3fType particleVec;
                util::VectorLoad(&particleVec,
                                 reinterpret_cast<util::Float3&>(pEmitter->m_ParticleVec[i]));

                if (pEmitter->m_pEmitterData->followType == FollowType_None) {
                    util::Matrix4x3fType matrix;
                    LoadParticleEmitterMatrix(&matrix, pEmitter->m_ParticleEmitterMatrixRow, i);
                    VectorTransform(&particlePos, particlePos, matrix);
                    util::Matrix4x3fType matrixRt;
                    LoadParticleEmitterMatrix(&matrixRt, pEmitter->m_ParticleEmitterMatrixRow, i);
                    matrixRt._m.val[3] = vdupq_n_f32(0.0f);
                    VectorTransform(&particleVec, particleVec, matrixRt);
                }

                if (time != 0.0f && pChildRes->isEmitDistance) {
                    util::VectorLoad(&pChild->m_EmitterLocalVec,
                                     reinterpret_cast<util::Float3&>(
                                         pEmitter->m_ParticlePosDelta[i]));
                }

                pChild->m_pParentParticlePos = &particlePos;
                pChild->m_pParentParticleVec = &particleVec;
                pChild->m_ParentParticleIndexForEmit = i;

                if (pChild->m_pEmitterData->followType == FollowType_None &&
                    pEmitter->m_pEmitterData->followType != FollowType_None) {
                    pChild->m_MatrixSrt = pChild->m_ResMatrixSrt;
                    pChild->m_MatrixRt = pChild->m_ResMatrixRt;
                    MatrixMultiply(&pChild->m_MatrixSrt, pChild->m_MatrixSrt,
                                   pEmitter->m_MatrixSrt);
                    MatrixMultiply(&pChild->m_MatrixRt, pChild->m_MatrixRt, pEmitter->m_MatrixRt);
                }

                pChild->m_EmitterCalculator->TryEmitParticle(
                    &pParentData[i].emitIntervalCounter[j], &pParentData[i].emitCounter[j],
                    &pParentData[i].emitInterval[j], pIsEmitted, pChild, pEmitter->m_FrameRate,
                    false);
                pChild->m_ParentParticleIndexForEmit = -1;
                pChild->m_pParentParticlePos = nullptr;
                pChild->m_pParentParticleVec = nullptr;
            }

            aliveNum++;
        }

        if (pEmitter->m_ChildEmitterRes[0] == nullptr || pData->createId == 0) {
            continue;
        }

        for (int j = 0; j < pEmitter->m_pEmitterRes->m_ChildEmitterResNum; j++) {
            Emitter* pChild = pParentData[i].pChildEmitter[j];

            if (pChild == nullptr) {
                continue;
            }

            if (!pEmitter->m_ChildEmitterRes[j]->m_pResEmitter->isIndependentChild) {
                continue;
            }

            if (pChild->m_EmitterCalculator == nullptr ||
                pChild->m_ParentEmitterCreateId != pEmitter->m_EmitterCreateId) {
                continue;
            }

            pChild->m_ParentParticleTime = pEmitter->m_Frame - pData->createTime;
            pChild->m_ParentParticleLife = pData->life;
            u8 followType = pEmitter->m_pEmitterData->followType;

            if (followType != FollowType_All) {
                util::Matrix4x3fType matrix;
                GetParticleEmitterMatrix(&matrix, pEmitter, i);
                util::Vector3fType pos;
                util::VectorLoad(&pos,
                                 reinterpret_cast<util::Float3&>(pEmitter->m_ParticlePos[i]));

                if (followType == FollowType_None) {
                    VectorTransform(&pos, pos, matrix);
                }

                pChild->m_ParentParticleWorldPos = pos;
            } else {
                util::VectorLoad(&pChild->m_ParentParticleWorldPos,
                                 reinterpret_cast<util::Float3&>(pEmitter->m_ParticlePos[i]));
            }

            util::Vector3fType vec;
            util::VectorLoad(&vec, reinterpret_cast<util::Float3&>(pEmitter->m_ParticleVec[i]));
            VectorTransformNormal(&pChild->m_ParentParticleWorldVec, vec, pEmitter->m_MatrixRt);
        }
    }

    pEmitter->m_AliveParticleNum = aliveNum;

    if (aliveNum == 0) {
        pEmitter->m_ParticleNum = 0;
        pEmitter->m_ParticleHead = 0;
    }

    UpdateCurrentParticleGpuBufferForCpuEmitter(pEmitter);
}

/**
 * Fills the per-frame constant buffer of an emitter.
 * @param pEmitter the emitter
 * @param frameRate the length of the frame
 * @param accumulatedFrameRate the frames passed since the buffers were last swapped
 */
void EmitterCalculator::MakeDynamicConstantBuffer(Emitter* pEmitter, f32 frameRate,
                                                  f32 accumulatedFrameRate) {
    const EmitterAnimValue& rAnim = pEmitter->m_EmitterAnimValue;
    const EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;
    float32x4_t setColor = pEmitterSet->m_Color._v;
    float32x4_t color0 = pEmitter->m_Color0._v;
    float32x4_t color1 = pEmitter->m_Color1._v;

    float32x4_t emitterColor0 = vdupq_n_f32(0.0f);
    emitterColor0 = vsetq_lane_f32(rAnim.color0.x * vgetq_lane_f32(color0, 0) *
                                       vgetq_lane_f32(setColor, 0),
                                   emitterColor0, 0);
    emitterColor0 = vsetq_lane_f32(rAnim.color0.y * vgetq_lane_f32(color0, 1) *
                                       vgetq_lane_f32(setColor, 1),
                                   emitterColor0, 1);
    emitterColor0 = vsetq_lane_f32(rAnim.color0.z * vgetq_lane_f32(color0, 2) *
                                       vgetq_lane_f32(setColor, 2),
                                   emitterColor0, 2);
    emitterColor0 = vsetq_lane_f32(rAnim.alpha0.x * vgetq_lane_f32(color0, 3), emitterColor0, 3);

    float32x4_t emitterColor1 = vdupq_n_f32(0.0f);
    emitterColor1 = vsetq_lane_f32(rAnim.color1.x * vgetq_lane_f32(color1, 0) *
                                       vgetq_lane_f32(setColor, 0),
                                   emitterColor1, 0);
    emitterColor1 = vsetq_lane_f32(rAnim.color1.y * vgetq_lane_f32(color1, 1) *
                                       vgetq_lane_f32(setColor, 1),
                                   emitterColor1, 1);
    emitterColor1 = vsetq_lane_f32(rAnim.color1.z * vgetq_lane_f32(color1, 2) *
                                       vgetq_lane_f32(setColor, 2),
                                   emitterColor1, 2);
    emitterColor1 = vsetq_lane_f32(rAnim.alpha1.x * vgetq_lane_f32(color1, 3), emitterColor1, 3);

    EmitterDynamicUniformBlock* pBlock = static_cast<EmitterDynamicUniformBlock*>(
        pEmitter->m_ConstantBuffer[pEmitter->m_BufferIndex]);

    if (pEmitter->m_IsChildEmitter) {
        const ResEmitter* pResEmitter = pEmitter->m_pEmitterRes->m_pResEmitter;
        const Emitter* pParent = pEmitter->m_pParentEmitter;

        if (pResEmitter->isInheritParentAlpha0 && pResEmitter->isInheritParentEmitterAlpha0) {
            emitterColor0 = vsetq_lane_f32(vgetq_lane_f32(emitterColor0, 3) *
                                               pParent->m_EmitterAnimValue.alpha0.x,
                                           emitterColor0, 3);
        }

        if (pResEmitter->isInheritParentAlpha1 && pResEmitter->isInheritParentEmitterAlpha1) {
            emitterColor1 = vsetq_lane_f32(vgetq_lane_f32(emitterColor1, 3) *
                                               pParent->m_EmitterAnimValue.alpha1.x,
                                           emitterColor1, 3);
        }
    }

    vst1q_f32(pBlock->emitterColor0.v, emitterColor0);
    vst1q_f32(pBlock->emitterColor1.v, emitterColor1);

    const ResEmitter* pResEmitter = pEmitter->m_pEmitterRes->m_pResEmitter;
    f32 alpha = vgetq_lane_f32(pEmitterSet->m_Color._v, 3);
    f32 fadeAlpha = pResEmitter->isFadeInAlpha ? pEmitter->m_FadeInRatio : 1.0f;

    if (pResEmitter->isFadeOutAlpha) {
        fadeAlpha *= pEmitter->m_FadeOutRatio;
    }

    alpha *= fadeAlpha;

    if ((pResEmitter->isInheritParentAlpha0 && pResEmitter->isInheritParentEmitterAlpha0) ||
        (pResEmitter->isInheritParentAlpha1 && pResEmitter->isInheritParentEmitterAlpha1)) {
        const Emitter* pParent = pEmitter->m_pParentEmitter;
        const ResEmitter* pParentRes = pParent->m_pEmitterRes->m_pResEmitter;
        f32 parentFadeAlpha = pParentRes->isFadeInAlpha ? pParent->m_FadeInRatio : 1.0f;

        if (pParentRes->isFadeOutAlpha) {
            parentFadeAlpha *= pParent->m_FadeOutRatio;
        }

        alpha *= parentFadeAlpha;
    }

    f32 fadeScale = pResEmitter->isFadeInScale ? pEmitter->m_FadeInRatio : 1.0f;

    if (pResEmitter->isFadeOutScale) {
        fadeScale *= pEmitter->m_FadeOutRatio;
    }

    pBlock->time = GetEmitterTime(pEmitter);
    pBlock->particleNum = static_cast<f32>(pEmitter->m_ParticleNum);
    pBlock->accumulatedFrameRate = accumulatedFrameRate;
    pBlock->frameRate = frameRate;

    if (frameRate == 0.0f && pEmitter->m_Frame > 0.0f &&
        !pEmitter->m_EmitterSet->m_System->m_1700) {
        pBlock->time = pEmitter->m_Frame - pEmitter->m_FrameRate;
        pBlock->frameRate = pEmitter->m_FrameRate;
    }

    pBlock->alpha = alpha;
    pBlock->particleScale.x = fadeScale * vgetq_lane_f32(pEmitterSet->m_ParticleScaleForCalc._v, 0);
    pBlock->particleScale.y = fadeScale * vgetq_lane_f32(pEmitterSet->m_ParticleScaleForCalc._v, 1);
    pBlock->particleScale.z = fadeScale * vgetq_lane_f32(pEmitterSet->m_ParticleScaleForCalc._v, 2);

    StoreMatrix4x4(pBlock->emitterMatrixSrt, pEmitter->m_MatrixSrt);
    StoreMatrix4x4(pBlock->emitterMatrixRt, pEmitter->m_MatrixRt);
}

/**
 * Calculates the information of a particle. Nothing is calculated on this platform.
 * @param pParticle the particle
 * @param pEmitter the emitter of the particle
 * @param time the age of the particle
 * @param life the life of the particle
 * @param particleIndex the index of the particle
 */
void EmitterCalculator::CalculateParticleInfo(Particle* pParticle, Emitter* pEmitter, f32 time,
                                              f32 life, int particleIndex) {}

/**
 * Copies the inherited values of the parent particle to a particle of a child emitter.
 * @param pEmitter the child emitter
 * @param particleIndex the index of the particle
 */
void EmitterCalculator::InheritParentParticleInfo(Emitter* pEmitter, int particleIndex) {
    const ResEmitter* pRes = pEmitter->m_pEmitterData;
    ParticleProperty* pProperty =
        pRes->calcType == 0 ? pEmitter->GetCpuParticleProperty() : pEmitter->m_pGpuParticleProperty;
    Particle particle;
    util::Vector4fType scale;
    util::Vector4fType rotate;
    util::Vector4fType random;
    f32 life;
    f32 time;
    util::Vector3fType parentVec;

    if (pRes->isIndependentChild) {
        parentVec = pEmitter->m_ParentParticleWorldVec;
        scale = pEmitter->m_ParentParticleScale;
        rotate = pEmitter->m_ParentParticleRotate;
        random = pEmitter->m_ParentParticleRandom;
        time = pEmitter->m_ParentParticleTime;
        life = pEmitter->m_ParentParticleLife;
    } else {
        const Emitter* pParent = pEmitter->m_pParentEmitter;
        int parentIndex = pEmitter->m_ParentParticleIndexForEmit;
        parentVec = *pEmitter->m_pParentParticleVec;
        scale._v = vld1q_f32(pParent->m_ParticleScale[parentIndex].v);
        rotate._v = vld1q_f32(pParent->m_ParticleRotate[parentIndex].v);
        random._v = vld1q_f32(pParent->m_ParticleAnimRandom[parentIndex].v);
        const ParticleData& rParentData = pParent->GetParticleData()[parentIndex];
        time = pParent->m_Frame - rParentData.createTime - pParent->m_FrameRate;
        life = rParentData.life;

        util::Vector3fType pos;
        util::VectorLoad(&pos, reinterpret_cast<util::Float3&>(pProperty->pPos[particleIndex]));
        pos._v = vaddq_f32(pEmitter->m_pParentParticlePos->_v, pos._v);
        StoreVector(&pProperty->pPos[particleIndex], pos);
    }

    if (pEmitter->m_pEmitterData->isInheritParentVel) {
        util::Float4& rVec = pProperty->pVec[particleIndex];
        util::Vector3fType vec;
        vec._v = vaddq_f32(vmulq_n_f32(parentVec._v, pRes->inheritParentVelRate),
                           LoadVectorLanes(rVec));
        StoreVector(&rVec, vec);
    }

    if (pEmitter->m_pEmitterData->isInheritParentScale) {
        util::Vector3fType& parentScale = particle.scale;
        CalculateParticleScaleVecFromTime(&parentScale,
                                          pEmitter->m_pParentEmitter->m_pEmitterRes, scale,
                                          random, life, time);
        parentScale._v =
            vmulq_n_f32(parentScale._v, pEmitter->m_pEmitterData->inheritParentScaleRate);
        pProperty->pScale[particleIndex].x = vgetq_lane_f32(parentScale._v, 0);
        pProperty->pScale[particleIndex].y = vgetq_lane_f32(parentScale._v, 1);
        pProperty->pScale[particleIndex].z = vgetq_lane_f32(parentScale._v, 2);
    }

    if (pEmitter->m_pEmitterData->isInheritParentRotate) {
        util::Vector3fType& parentRotate = particle.rotate;
        CalculateRotationMatrix(&parentRotate, pEmitter->m_pParentEmitter->m_pEmitterRes, rotate,
                                random, time);
        pProperty->pRotate[particleIndex].x = vgetq_lane_f32(parentRotate._v, 0);
        pProperty->pRotate[particleIndex].y = vgetq_lane_f32(parentRotate._v, 1);
        pProperty->pRotate[particleIndex].z = vgetq_lane_f32(parentRotate._v, 2);
    }

    if (pProperty->pColor0 != nullptr && (pEmitter->m_pEmitterData->isInheritParentColor0 ||
                                          pEmitter->m_pEmitterData->isInheritParentAlpha0)) {
        const Emitter* pParent = pEmitter->m_pParentEmitter;
        util::Vector3fType emitterColor;
        util::VectorLoad(&emitterColor, pParent->m_EmitterAnimValue.color0);
        util::Vector4fType& color = particle.color0;
        CalculateParticleColor0VecFromTime(&color, pParent->m_pEmitterRes, random,
                                           pParent->m_Color0, emitterColor,
                                           pParent->m_EmitterAnimValue.alpha0.x, life, time);

        if (pEmitter->m_pEmitterData->isInheritParentColor0) {
            pProperty->pColor0[particleIndex].x = vgetq_lane_f32(color._v, 0);
            pProperty->pColor0[particleIndex].y = vgetq_lane_f32(color._v, 1);
            pProperty->pColor0[particleIndex].z = vgetq_lane_f32(color._v, 2);
        }

        if (pEmitter->m_pEmitterData->isInheritParentAlpha0) {
            pProperty->pColor0[particleIndex].w = vgetq_lane_f32(color._v, 3);
        }
    }

    if (pProperty->pColor1 != nullptr && (pEmitter->m_pEmitterData->isInheritParentColor1 ||
                                          pEmitter->m_pEmitterData->isInheritParentAlpha1)) {
        const Emitter* pParent = pEmitter->m_pParentEmitter;
        util::Vector3fType emitterColor;
        util::VectorLoad(&emitterColor, pParent->m_EmitterAnimValue.color1);
        util::Vector4fType& color = particle.color1;
        CalculateParticleColor1VecFromTime(&color, pParent->m_pEmitterRes, random,
                                           pParent->m_Color1, emitterColor,
                                           pParent->m_EmitterAnimValue.alpha1.x, life, time);

        if (pEmitter->m_pEmitterData->isInheritParentColor1) {
            pProperty->pColor1[particleIndex].x = vgetq_lane_f32(color._v, 0);
            pProperty->pColor1[particleIndex].y = vgetq_lane_f32(color._v, 1);
            pProperty->pColor1[particleIndex].z = vgetq_lane_f32(color._v, 2);
        }

        if (pEmitter->m_pEmitterData->isInheritParentAlpha1) {
            pProperty->pColor1[particleIndex].w = vgetq_lane_f32(color._v, 3);
        }
    }
}

/**
 * Emits particles, placing each one in a free slot or in the oldest one.
 * @param pIsEmitted set when a particle is emitted
 * @param pEmitter the emitter
 * @param emitCount the number of particles to emit
 * @param isSearchFreeSlot whether to look for a free slot before reusing the oldest one
 * @param pReservationInfo the reserved emission, if any
 * @return the index of the last emitted particle, or -1
 */
int EmitterCalculator::EmitBySearchOrder(u8* pIsEmitted, Emitter* pEmitter, int emitCount,
                                         bool isSearchFreeSlot,
                                         const EmitReservationInfo* pReservationInfo) {
    f32 random = pEmitter->m_Random.GetF32();
    CallbackSet* pCallbackSet2 = pEmitter->m_pCallbackSet[2];
    ParticleEmitCallback pEmitCallback2 =
        pCallbackSet2 != nullptr ? pCallbackSet2->particleEmit : nullptr;
    CallbackSet* pCallbackSet0 = pEmitter->m_pCallbackSet[0];
    ParticleEmitCallback pEmitCallback0 =
        pCallbackSet0 != nullptr ? pCallbackSet0->particleEmit : nullptr;
    CallbackSet* pCallbackSet1 = pEmitter->m_pCallbackSet[1];
    ParticleEmitCallback pEmitCallback1 =
        pCallbackSet1 != nullptr ? pCallbackSet1->particleEmit : nullptr;

    const ResEmitter* pResEmitter = pEmitter->m_pEmitterRes->m_pResEmitter;

    if (pResEmitter->divisionEmitMode == 0) {
        u8 volumeType = pResEmitter->volumeType;
        const ResEmitter* pRes = pEmitter->m_pEmitterData;

        if (volumeType == VolumeType_CircleEquallyDivided) {
            emitCount *= GetEmitDivisionNum(pRes->circleDivisionNum, pRes->circleDivisionRandom,
                                            random);
        } else if (volumeType == VolumeType_SphereEquallyDivided) {
            emitCount *= GetEmitDivisionNum(pRes->sphereDivisionNum, pRes->sphereDivisionRandom,
                                            random);
        }
    }

    if (pEmitter->m_IsSequentialEmit && *pIsEmitted != 0) {
        const ParticleData& rData = pEmitter->GetParticleData()[0];

        if (pEmitter->m_Frame - rData.createTime - rData.life > 0.0f) {
            pEmitter->m_IsSequentialEmit = false;
        }
    }

    int lastIndex = -1;

    for (int i = 0; i < emitCount; i++) {
        int index;
        bool isAdvanceHead;

        if (pEmitter->m_IsSequentialEmit) {
            index = pEmitter->m_ParticleHead;
            isAdvanceHead = true;
        } else {
            bool isSearch = !pEmitter->m_IsParticleFull && isSearchFreeSlot;
            index = pEmitter->m_ParticleHead;

            if (!isSearch) {
                isAdvanceHead = true;
            } else {
                int particleNum = pEmitter->m_ParticleNum;
                int freeIndex = -1;
                isAdvanceHead = true;

                if (particleNum == 0) {
                    index = 0;
                } else if (pEmitter->m_pEmitterData->calcType != 0) {
                    f32 time = pEmitter->m_Frame;

                    for (int j = 0; j < particleNum; j++) {
                        const ParticleData& rData = pEmitter->GetParticleData()[j];

                        if (time - rData.createTime - rData.life > 0.0f) {
                            freeIndex = j;
                            isAdvanceHead = false;
                            break;
                        }
                    }
                } else {
                    for (int j = 0; j < particleNum; j++) {
                        if (pEmitter->GetParticleData()[j].createId == 0) {
                            freeIndex = j;
                            isAdvanceHead = false;
                            break;
                        }
                    }
                }

                if (freeIndex != -1) {
                    index = freeIndex;
                } else {
                    pEmitter->m_IsParticleFull = true;
                }
            }
        }

        ParticleData* pData = &pEmitter->GetParticleData()[index];

        if (pData->pUserData2 != nullptr || pData->pUserData != nullptr) {
            Warning(pEmitter, RuntimeWarningId_ParticleUserDataInUse);
            return lastIndex;
        }

        *pIsEmitted = 1;
        u8 calcType = pEmitter->m_pEmitterData->calcType;
        ParticleProperty* pGpuProperty = pEmitter->m_pGpuParticleProperty;
        ParticleProperty* pProperty =
            calcType == 0 ? pEmitter->GetCpuParticleProperty() : pGpuProperty;
        ParentParticleData* pParentData = pEmitter->m_pEmitterRes->m_ChildEmitterResNum != 0 ?
                                              &pEmitter->m_pParentParticleData[index] :
                                              nullptr;

        if (!InitializeParticle(pEmitter, index, &pEmitter->GetParticleData()[index], pParentData,
                                pProperty, index, i, emitCount, random, pReservationInfo)) {
            continue;
        }

        pEmitter->m_LastEmitIndex = index;
        const util::Float4& rVec = pProperty->pVec[index];
        util::Float4& rPosDelta = pProperty->pPosDelta[index];
        rPosDelta.x = rVec.x;
        rPosDelta.y = rVec.y;
        rPosDelta.z = rVec.z;

        if (pEmitter->m_pParentEmitter != nullptr) {
            InheritParentParticleInfo(pEmitter, index);
        }

        ParticleCalculateArgImpl arg;
        arg.pUserData = pReservationInfo != nullptr ? pReservationInfo->pUserData : nullptr;
        arg.pUserData2 = nullptr;
        arg.pEmitter = pEmitter;
        arg.time = 0.0f;
        arg.life = pEmitter->GetParticleData()[index].life;
        arg.particleIndex = index;

        if (pEmitCallback2 != nullptr) {
            pEmitCallback2(arg);
        }

        if (pEmitCallback0 != nullptr) {
            pEmitCallback0(arg);
        } else if (pEmitCallback1 != nullptr) {
            pEmitCallback1(arg);
        }

        pEmitter->GetParticleData()[index].pUserData2 = arg.pUserData2;
        pEmitter->GetParticleData()[index].pUserData = arg.pUserData;

        if (isAdvanceHead) {
            int maxNum = pEmitter->m_MaxParticleNum;
            pEmitter->m_ParticleHead = index + 1 == maxNum ? 0 : index + 1;

            if (pEmitter->m_ParticleNum < maxNum) {
                pEmitter->m_ParticleNum++;
            }
        }

        lastIndex = index;
    }

    return lastIndex;
}

/**
 * Initializes a newly emitted particle.
 * @param pEmitter the emitter
 * @param particleIndex the index of the particle
 * @param pData the state of the particle
 * @param pParentData the state of the child emitters of the particle, if any
 * @param pProperty the particle arrays
 * @param propertyIndex the index of the particle in the particle arrays
 * @param emitIndex the index of the particle in this emission
 * @param emitCount the number of particles of this emission
 * @param random the random value of this emission
 * @param pReservationInfo the reserved emission, if any
 * @return whether the particle was emitted
 */
bool EmitterCalculator::InitializeParticle(Emitter* pEmitter, int particleIndex,
                                           ParticleData* pData, ParentParticleData* pParentData,
                                           ParticleProperty* pProperty, int propertyIndex,
                                           int emitIndex, int emitCount, f32 random,
                                           const EmitReservationInfo* pReservationInfo) {
    const ResEmitter* pRes = pEmitter->m_pEmitterData;
    EmitterSet* pEmitterSet = pEmitter->m_EmitterSet;
    util::Vector3fType localPos;
    util::Vector3fType localVec;

    if (!g_EmitFunctions[pRes->volumeType](&localPos, &localVec, pEmitter, emitIndex, emitCount,
                                           random, &pEmitter->m_EmitterAnimValue)) {
        return false;
    }

    pData->createId = ++pEmitter->m_ParticleCreateId;

    const ResEmitter* pResEmitter = pEmitter->m_pEmitterData;

    if (pResEmitter->xzDiffusionVel != 0.0f) {
        float32x4_t zero = vdupq_n_f32(0.0f);
        float32x4_t direction = vtrn2q_f32(vtrn1q_f32(zero, localPos._v), zero);
        float32x4_t lengthSquared = VectorDot(direction, direction);

        if (!(vgetq_lane_f32(lengthSquared, 0) > FloatEpsilon)) {
            direction = vsetq_lane_f32(pEmitter->m_Random.GetF32Range(-1.0f, 1.0f), direction, 0);
            direction = vsetq_lane_f32(pEmitter->m_Random.GetF32Range(-1.0f, 1.0f), direction, 2);
            direction = vsetq_lane_f32(0.0f, direction, 3);
            lengthSquared = VectorDot(direction, direction);
        }

        localVec._v = vaddq_f32(localVec._v,
                                vmulq_n_f32(VectorNormalizeEst(direction, lengthSquared),
                                            pResEmitter->xzDiffusionVel));
    }

    f32 velocityRandom = -pEmitter->m_Random.GetF32();
    f32 velocityRandomRatio = pResEmitter->velocityRandom / 100.0f;
    f32 setVelocityRandom = pEmitterSet->m_VelocityRandomScale;
    f32 directionalVelScale = pEmitterSet->m_DirectionalVel;
    f32 directionalVel = pEmitter->m_EmitterAnimValue.directionalVel.x;
    f32 dispersionAngle = pResEmitter->dispersionAngle;

    if (pResEmitter->emitPosRandom != 0.0f) {
        localPos._v = vaddq_f32(localPos._v, vmulq_n_f32(pEmitter->m_Random.GetNormalizedVec3()._v,
                                                         pResEmitter->emitPosRandom));
    }

    velocityRandom *= velocityRandomRatio;

    util::Matrix4x3fType emitterMatrix;
    bool isStoreMatrix = pProperty->pEmitterMatrixRow[0] != nullptr;

    if (pReservationInfo != nullptr) {
        if (pReservationInfo->isUseMatrix) {
            if (isStoreMatrix) {
                emitterMatrix = pReservationInfo->matrix;
                ApplyFollowType(&emitterMatrix, &localPos, pResEmitter->followType,
                                emitterMatrix._m.val[3]);
            } else {
                VectorTransform(&localPos, localPos, pReservationInfo->matrix);
            }
        } else {
            float32x4_t translate = pReservationInfo->matrix._m.val[3];

            if (isStoreMatrix) {
                util::MatrixIdentity(&emitterMatrix);
                ApplyFollowType(&emitterMatrix, &localPos, pResEmitter->followType, translate);
            } else {
                localPos._v = vaddq_f32(translate, localPos._v);
            }
        }
    } else if (isStoreMatrix) {
        emitterMatrix = pEmitter->m_MatrixSrt;
    }

    if (isStoreMatrix) {
        StoreParticleEmitterMatrix(pProperty, propertyIndex, emitterMatrix);
    }

    StoreVector(&pProperty->pPos[propertyIndex], localPos);

    pResEmitter = pEmitter->m_pEmitterData;
    util::Vector3fType direction;
    util::VectorLoad(&direction, pResEmitter->emitDirection);

    if (pResEmitter->isEmitDirectionLocal) {
        util::Matrix4x3fType matrix;

        if (pResEmitter->followType == FollowType_PosOnly ||
            pResEmitter->followType != FollowType_All) {
            LoadParticleEmitterMatrix(&matrix, pEmitter->m_ParticleEmitterMatrixRow,
                                      propertyIndex);
        } else {
            matrix = pEmitter->m_MatrixSrt;
        }

        matrix._m.val[3] = vdupq_n_f32(0.0f);
        util::Matrix4x3fType inverse;
        MatrixInverse(&inverse, matrix);
        VectorTransform(&direction, direction, inverse);
    }

    f32 velocityScale = setVelocityRandom * velocityRandom + 1.0f;
    f32 directionalSpeed = directionalVel * directionalVelScale;

    if (dispersionAngle == 0.0f) {
        SetVector(&localVec._v,
                  velocityScale * (directionalSpeed * util::VectorGetX(direction) +
                                   util::VectorGetX(localVec)),
                  velocityScale * (directionalSpeed * util::VectorGetY(direction) +
                                   util::VectorGetY(localVec)),
                  velocityScale * (directionalSpeed * util::VectorGetZ(direction) +
                                   util::VectorGetZ(localVec)));
    } else {
        f32 cosAngle = dispersionAngle / -90.0f + 1.0f;
        f32 rotate = pEmitter->m_Random.GetF32() * 2.0f * util::FloatPi;
        f32 sinValue;
        f32 cosValue;
        SinCosEst(&sinValue, &cosValue, rotate);
        f32 y = cosAngle + (1.0f - cosAngle) * pEmitter->m_Random.GetF32();
        f32 radiusSquared = 1.0f - y * y;
        f32 radius;

        if (radiusSquared <= 0.0f) {
            radius = 0.0f;
        } else {
            radius = std::sqrt(radiusSquared);
        }

        util::Vector3fType coneVec;
        SetVector(&coneVec._v, cosValue * radius, y, sinValue * radius);
        util::Vector3fType up;
        util::VectorSet(&up, 0.0f, 1.0f, 0.0f);
        util::Vector4fType quaternion;
        QuaternionMakeVectorRotation(&quaternion, up, direction);
        util::Matrix4x3fType rotateMatrix;
        _MatrixFromQuaternion(&rotateMatrix, quaternion);
        VectorTransform(&coneVec, coneVec, rotateMatrix);
        localVec._v = vmulq_n_f32(
            vaddq_f32(localVec._v, vmulq_n_f32(coneVec._v, directionalSpeed)), velocityScale);
    }

    const util::Vector3fType& rRandomVec = pEmitter->m_Random.GetVec3();
    const ResEmitter* pRandomRes = pEmitter->m_pEmitterData;
    SetX(&localVec, util::VectorGetX(localVec) +
                        util::VectorGetX(rRandomVec) * pRandomRes->randomVel.x);
    SetY(&localVec, util::VectorGetY(localVec) +
                        util::VectorGetY(rRandomVec) * pRandomRes->randomVel.y);
    SetZ(&localVec, util::VectorGetZ(localVec) +
                        util::VectorGetZ(rRandomVec) * pRandomRes->randomVel.z);
    localVec._v = vaddq_f32(localVec._v, vmulq_n_f32(pEmitter->m_EmitterLocalVec._v,
                                                     pRandomRes->emitterVelInherit));

    util::Matrix4x3fType inverseRt;
    MatrixInverse(&inverseRt, pEmitter->m_MatrixRt);
    util::Vector3fType addVel;
    VectorTransformNormal(&addVel, pEmitterSet->m_AddVelocity, inverseRt);
    localVec._v = vaddq_f32(localVec._v, addVel._v);
    StoreVector(&pProperty->pVec[propertyIndex], localVec);

    util::Float4& rScale = pProperty->pScale[propertyIndex];
    const util::Float3& rScaleRandom = pRes->scaleRandom;

    if (rScaleRandom.x != rScaleRandom.y) {
        rScale.x = pEmitterSet->m_EmissionParticleScale.x *
                   (pEmitter->m_EmitterAnimValue.particleScale.x *
                    (1.0f - rScaleRandom.x / 100.0f * pEmitter->m_Random.GetF32()));
        pProperty->pScale[propertyIndex].y =
            pEmitterSet->m_EmissionParticleScale.y *
            (pEmitter->m_EmitterAnimValue.particleScale.y *
             (1.0f - rScaleRandom.y / 100.0f * pEmitter->m_Random.GetF32()));
        rScale.z = pEmitterSet->m_EmissionParticleScale.z *
                   (pEmitter->m_EmitterAnimValue.particleScale.z *
                    (1.0f - rScaleRandom.z / 100.0f * pEmitter->m_Random.GetF32()));
    } else {
        f32 scaleRandom = 1.0f - rScaleRandom.x / 100.0f * pEmitter->m_Random.GetF32();
        rScale.x = pEmitterSet->m_EmissionParticleScale.x *
                   (pEmitter->m_EmitterAnimValue.particleScale.x * scaleRandom);
        pProperty->pScale[propertyIndex].y = scaleRandom *
                                             pEmitter->m_EmitterAnimValue.particleScale.y *
                                             pEmitterSet->m_EmissionParticleScale.y;
        rScale.z = scaleRandom * pEmitter->m_EmitterAnimValue.particleScale.z *
                   pEmitterSet->m_EmissionParticleScale.z;
    }

    pData->collisionCount = 0;

    f32 momentumRandom = pRes->momentumRandom;
    pProperty->pScale[propertyIndex].w =
        (momentumRandom + 1.0f) + momentumRandom * -pEmitter->m_Random.GetF32() * 2.0f;

    pData->createTime = pEmitter->m_Frame;
    pProperty->pVec[propertyIndex].w = pEmitter->m_Frame;

    f32 life;

    if (pRes->isInfinityLife) {
        life = InfiniteLife;
    } else {
        f32 animLife = pEmitter->m_EmitterAnimValue.particleLife.x;
        f32 lifeRandom = static_cast<f32>(pEmitter->m_Random.GetS32(pRes->particleLifeRandom));
        life = pEmitterSet->m_ParticleLifeScale *
               (pEmitter->m_ParticleLifeScale * (animLife - animLife * (lifeRandom * 0.01f)));
    }

    pData->life = life;
    pProperty->pPos[propertyIndex].w = life;

    if (pParentData != nullptr) {
        pParentData->time = 0.0f;
        std::memset(pParentData->emitIntervalCounter, 0,
                    sizeof(ParentParticleData) - offsetof(ParentParticleData, emitIntervalCounter));
        pParentData->life = pData->life;
    }

    pProperty->pAnimRandom[propertyIndex].x = pEmitter->m_Random.GetF32();
    pProperty->pAnimRandom[propertyIndex].y = pEmitter->m_Random.GetF32();
    pProperty->pAnimRandom[propertyIndex].z = pEmitter->m_Random.GetF32();
    pProperty->pAnimRandom[propertyIndex].w = pEmitter->m_Random.GetF32();

    pProperty->pRotate[propertyIndex].x =
        pEmitter->m_pEmitterRes->m_InitRotate.x + pEmitter->m_EmitterSet->m_ParticleInitRotate.x;
    pProperty->pRotate[propertyIndex].y =
        pEmitter->m_pEmitterRes->m_InitRotate.y + pEmitter->m_EmitterSet->m_ParticleInitRotate.y;
    pProperty->pRotate[propertyIndex].z =
        pEmitter->m_pEmitterRes->m_InitRotate.z + pEmitter->m_EmitterSet->m_ParticleInitRotate.z;
    pProperty->pRotate[propertyIndex].w = 0.0f;

    if (pProperty->pColor0 != nullptr) {
        const util::Float4 white = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
        pProperty->pColor0[propertyIndex] = white;
        pProperty->pColor1[propertyIndex] = white;
    }

    for (int i = 0; i < pEmitter->m_pEmitterRes->m_ChildEmitterResNum; i++) {
        EmitterResource* pChildRes = pEmitter->m_ChildEmitterRes[i];

        if (pChildRes == nullptr) {
            pParentData->pChildEmitter[i] = nullptr;
            continue;
        }

        if (!pChildRes->m_pResEmitter->isIndependentChild) {
            pParentData->pChildEmitter[i] = pEmitter->m_ChildEmitter[i];
            continue;
        }

        Emitter* pChild = pEmitterSet->CreateEmitter(pChildRes, 0, pEmitter, i);

        if (pChild == nullptr) {
            continue;
        }

        pParentData->pChildEmitter[i] = pChild;
        pChild->m_ParentEmitterCreateId = pEmitter->m_EmitterCreateId;
        pChild->m_ParentParticleLife = life;
        pChild->m_ParentParticleCreateId = pEmitter->m_ParticleCreateId;
        pChild->m_ParentParticleIndex = particleIndex;
        pChild->m_ParentParticleBirthTime = pEmitter->m_Frame;
        util::VectorLoad(&pChild->m_ParentParticleLocalPos,
                         reinterpret_cast<util::Float3&>(pProperty->pPos[propertyIndex]));
        util::VectorLoad(&pChild->m_ParentParticleLocalVec,
                         reinterpret_cast<util::Float3&>(pProperty->pVec[propertyIndex]));
        pChild->m_ParentParticleScale._v = vld1q_f32(pProperty->pScale[propertyIndex].v);
        pChild->m_ParentParticleRotate._v = vld1q_f32(pProperty->pRotate[propertyIndex].v);
        pChild->m_ParentParticleRandom._v = vld1q_f32(pProperty->pAnimRandom[propertyIndex].v);
        util::MatrixIdentity(&pChild->m_MatrixSrt);
        util::MatrixIdentity(&pChild->m_MatrixRt);
    }

    return true;
}

/**
 * Emits particles.
 * @param pIsEmitted set when a particle is emitted
 * @param pEmitter the emitter
 * @param emitCount the number of particles to emit
 * @param isSearchFreeSlot whether to look for a free slot before reusing the oldest one
 * @param pReservationInfo the reserved emission, if any
 * @return the index of the last emitted particle, or -1
 */
int EmitterCalculator::Emit(u8* pIsEmitted, Emitter* pEmitter, int emitCount,
                            bool isSearchFreeSlot, const EmitReservationInfo* pReservationInfo) {
    return EmitBySearchOrder(pIsEmitted, pEmitter, emitCount, isSearchFreeSlot, pReservationInfo);
}

/**
 * Makes the rotation matrix of a quaternion.
 * @param pOut the matrix, without translation
 * @param rQuaternion the quaternion
 */
void _MatrixFromQuaternion(util::Matrix4x3fType* pOut, const util::Vector4fType& rQuaternion) {
    f32 x = vgetq_lane_f32(rQuaternion._v, 0);
    f32 y = vgetq_lane_f32(rQuaternion._v, 1);
    f32 z = vgetq_lane_f32(rQuaternion._v, 2);
    f32 w = vgetq_lane_f32(rQuaternion._v, 3);
    f32 scale = 2.0f / (x * x + y * y + z * z + w * w);
    f32 xs = scale * x;
    f32 ys = scale * y;
    f32 zs = scale * z;
    f32 wx = xs * w;
    f32 wy = ys * w;
    f32 wz = zs * w;
    f32 xx = xs * x;
    f32 xy = ys * x;
    f32 xz = zs * x;
    f32 yy = ys * y;
    f32 yz = zs * y;
    f32 zz = zs * z;

    float32x4_t row0 = {1.0f - (yy + zz), xy + wz, xz - wy, 0.0f};
    float32x4_t row1 = {xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f};
    float32x4_t row2 = {xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f};
    pOut->_m.val[0] = row0;
    pOut->_m.val[1] = row1;
    pOut->_m.val[2] = row2;
    pOut->_m.val[3] = vdupq_n_f32(0.0f);
}

/**
 * Emits from a point.
 * @param pOutPos the emission position
 * @param pOutVec the emission velocity
 * @param pEmitter the emitter
 * @param emitIndex the index of the particle in this emission
 * @param emitCount the number of particles of this emission
 * @param random the random value of this emission
 * @param pAnimValue the emitter animation values
 * @return true
 */
bool EmitterCalculator::CalculateEmitPoint(util::Vector3fType* pOutPos,
                                           util::Vector3fType* pOutVec, Emitter* pEmitter,
                                           int emitIndex, int emitCount, f32 random,
                                           EmitterAnimValue* pAnimValue) {
    pOutPos->_v = vdupq_n_f32(0.0f);
    pOutVec->_v = vmulq_n_f32(pEmitter->m_Random.GetNormalizedVec3()._v,
                              pAnimValue->allDirectionalVel.x);
    return true;
}

/**
 * Emits from a random point of a circle.
 * @param pOutPos the emission position
 * @param pOutVec the emission velocity
 * @param pEmitter the emitter
 * @param emitIndex the index of the particle in this emission
 * @param emitCount the number of particles of this emission
 * @param random the random value of this emission
 * @param pAnimValue the emitter animation values
 * @return true
 */
bool EmitterCalculator::CalculateEmitCircle(util::Vector3fType* pOutPos,
                                            util::Vector3fType* pOutVec, Emitter* pEmitter,
                                            int emitIndex, int emitCount, f32 random,
                                            EmitterAnimValue* pAnimValue) {
    const ResEmitter* pRes = pEmitter->m_pEmitterData;
    f32 arcLength = pRes->arcLength;
    f32 arcStart;

    if (pRes->isArcStartRandom) {
        arcStart = util::FloatPi * random * 2.0f;
    } else {
        arcStart = pRes->arcStart;
    }

    f32 rotate = arcStart + arcLength * pEmitter->m_Random.GetF32() - arcLength * 0.5f;
    f32 sinValue;
    f32 cosValue;
    SinCosEst(&sinValue, &cosValue, rotate);

    f32 radiusX = pRes->volumeRadius.x * pAnimValue->emitterVolumeScale.x;
    f32 radiusZ = pRes->volumeRadius.z * pAnimValue->emitterVolumeScale.z;
    util::VectorSet(pOutPos, sinValue * radiusX, 0.0f, cosValue * radiusZ);
    f32 speed = pAnimValue->allDirectionalVel.x;
    util::VectorSet(pOutVec, sinValue * speed, 0.0f, speed * cosValue);
    return true;
}

}  // namespace detail
}  // namespace vfx
}  // namespace nn
