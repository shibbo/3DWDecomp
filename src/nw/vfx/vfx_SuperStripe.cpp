#include <nn/vfx/vfx_SuperStripe.h>

#include <attributes.h>
#include <cmath>
#include <cstring>
#include <new>
#include <nn/util/detail/util_ArithmeticImpl.h>
#include <nn/util/util_VectorApi.h>
#include <nn/vfx/System.h>
#include <nn/vfx/vfx_EmitterCalc.h>
#include <nn/vfx/vfx_ParticleBehavior.h>
#include <nn/vfx/vfx_StripeUtility.h>
#include <nn/vfx/vfx_System.h>
#include <nn/vfx/vfx_System_dup2.h>

namespace nn {
namespace vfx {
namespace detail {

namespace {

typedef SuperStripeSystem::History History;
typedef SuperStripeSystem::EmitterPluginUserData EmitterPluginUserData;
typedef SuperStripeSystem::VertexAttribute VertexAttribute;
typedef SuperStripeSystem::ConstantBufferObject ConstantBufferObject;

/** Slot of the system callback sets used by the super stripe emitter plugin. */
const int SuperStripeCallbackId = 19;

/** Ways the particles of an emitter follow the emitter matrix. */
enum FollowType {
    FollowType_All,
    FollowType_None,
    FollowType_PosOnly,
};

/** Emitter animations, by their index in the emitter animation arrays. */
enum EmitterAnimKind {
    EmitterAnimKind_EmissionRate = 5,
    EmitterAnimKind_ParticleLife = 6,
};

/** The y axis. */
const util::Vector3fType AxisY = {{0.0f, 1.0f, 0.0f, 0.0f}};

/** The x axis. */
const util::Vector3fType AxisX = {{1.0f, 0.0f, 0.0f, 0.0f}};

/** Direction of a stripe whose particle and emitter both stand still. */
const util::Vector3fType DefaultDir = {{0.0f, 0.001f, 0.0f, 0.0f}};

/** Squared length under which a vector is considered to have no direction. */
const f32 DirectionEpsilon = 1e-5f;

/** Texture coordinate change for every unit of length, for distance based coordinates. */
const f32 TexCoordPerLength = 0.1f;

/**
 * Gets the plugin state of an emitter.
 * @param pEmitter the emitter
 * @return the plugin state
 */
inline EmitterPluginUserData* GetUserData(const Emitter* pEmitter) {
    return static_cast<EmitterPluginUserData*>(pEmitter->m_pEmitterPluginUserData);
}

/**
 * Gets the plugin data of an emitter.
 * @param pEmitter the emitter
 * @return the plugin data
 */
inline ResStripeSuper* GetResource(const Emitter* pEmitter) {
    return static_cast<ResStripeSuper*>(pEmitter->m_pEmitterRes->m_pEmitterPluginData);
}

/**
 * Gets the dynamic heap of an emitter.
 * @param pEmitter the emitter
 * @return the dynamic heap
 */
inline DynamicHeap* GetDynamicHeap(Emitter* pEmitter) {
    return reinterpret_cast<DynamicHeap*>(pEmitter->m_DynamicHeap);
}

/**
 * Loads a four component vector.
 * @param rSource the source
 * @return the vector
 */
inline util::Vector4fType LoadVector4(const util::Float4& rSource) {
    util::Vector4fType vector;
    vector._v = vld1q_f32(rSource.v);
    return vector;
}

/**
 * Gets the animation random values of a particle.
 * @param rArg the particle
 * @return the random values
 */
inline util::Vector4fType GetAnimRandom(const ParticleCalculateArgImpl& rArg) {
    return LoadVector4(rArg.pEmitter->m_ParticleAnimRandom[rArg.particleIndex]);
}

/**
 * Gets the scale of a particle.
 * @param rArg the particle
 * @return the scale
 */
inline util::Vector4fType GetParticleScale(const ParticleCalculateArgImpl& rArg) {
    return LoadVector4(rArg.pEmitter->m_ParticleScale[rArg.particleIndex]);
}

/**
 * Gets the rotation of a particle.
 * @param rArg the particle
 * @return the rotation
 */
inline util::Vector4fType GetParticleRotate(const ParticleCalculateArgImpl& rArg) {
    return LoadVector4(rArg.pEmitter->m_ParticleRotate[rArg.particleIndex]);
}

/**
 * Loads one of the emitter animation values.
 * @param rValue the animation value
 * @return the vector
 */
inline util::Vector3fType LoadAnimValue(const util::Float3& rValue) {
    util::Vector3fType vector;
    util::VectorLoad(&vector, rValue);
    return vector;
}

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
 * Calculates the dot product of two vectors.
 * @param lhs the first vector
 * @param rhs the second vector
 * @return the dot product
 */
inline f32 VectorDot(float32x4_t lhs, float32x4_t rhs) {
    float32x4_t product = vmulq_f32(lhs, rhs);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    return vget_lane_f32(vpadd_f32(sum, sum), 0);
}

/**
 * Calculates the length of a vector.
 * @param vector the vector
 * @return the length
 */
inline f32 VectorLength(float32x4_t vector) {
    float32x4_t product = vmulq_f32(vector, vector);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    sum = vpadd_f32(sum, sum);
    return vgetq_lane_f32(vsqrtq_f32(vcombine_f32(sum, sum)), 0);
}

/**
 * Scales a vector to unit length with the reciprocal square root estimate.
 * A zero vector stays zero.
 * @param vector the vector
 * @return the normalized vector
 */
inline float32x4_t VectorNormalize(float32x4_t vector) {
    float32x4_t product = vmulq_f32(vector, vector);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    sum = vpadd_f32(sum, sum);
    float32x4_t lengthSquared = vcombine_f32(sum, sum);
    float32x4_t estimate = vrsqrteq_f32(lengthSquared);
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(estimate, lengthSquared)));
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(lengthSquared, estimate)));
    uint32x4_t mask = vmvnq_u32(vceqzq_f32(lengthSquared));
    return vreinterpretq_f32_u32(
        vandq_u32(vreinterpretq_u32_f32(vmulq_f32(vector, estimate)), mask));
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
    return vfmsq_f32(vmulq_f32(ShuffleYzx(lhs), ShuffleZxy(rhs)), ShuffleYzx(rhs), ShuffleZxy(lhs));
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
 * Gets the matrix moving a particle into the world, depending on how it follows its emitter.
 * @param pOut the matrix
 * @param pEmitter the emitter of the particle
 * @param particleIndex the index of the particle
 */
inline void GetParticleWorldMatrix(util::Matrix4x3fType* pOut, const Emitter* pEmitter,
                                   int particleIndex) {
    const util::Float4& rRow0 = pEmitter->m_ParticleEmitterMatrixRow[0][particleIndex];
    const util::Float4& rRow1 = pEmitter->m_ParticleEmitterMatrixRow[1][particleIndex];
    const util::Float4& rRow2 = pEmitter->m_ParticleEmitterMatrixRow[2][particleIndex];
    util::Vector3fType row;

    switch (pEmitter->m_pEmitterData->followType) {
    case FollowType_All:
        *pOut = pEmitter->m_MatrixSrt;
        break;
    default:
        util::VectorSet(&row, rRow0.x, rRow1.x, rRow2.x);
        pOut->_m.val[0] = row._v;
        util::VectorSet(&row, rRow0.y, rRow1.y, rRow2.y);
        pOut->_m.val[1] = row._v;
        util::VectorSet(&row, rRow0.z, rRow1.z, rRow2.z);
        pOut->_m.val[2] = row._v;
        util::VectorSet(&row, rRow0.w, rRow1.w, rRow2.w);
        pOut->_m.val[3] = row._v;
        break;
    case FollowType_PosOnly: {
        util::VectorSet(&row, rRow0.x, rRow1.x, rRow2.x);
        pOut->_m.val[0] = row._v;
        util::VectorSet(&row, rRow0.y, rRow1.y, rRow2.y);
        pOut->_m.val[1] = row._v;
        util::VectorSet(&row, rRow0.z, rRow1.z, rRow2.z);
        pOut->_m.val[2] = row._v;
        float32x4_t translate = pEmitter->m_MatrixSrt._m.val[3];
        util::VectorSet(&row, vgetq_lane_f32(translate, 0), vgetq_lane_f32(translate, 1),
                        vgetq_lane_f32(translate, 2));
        pOut->_m.val[3] = row._v;
        break;
    }
    }
}

/**
 * Calculates the position of a particle as recorded in the stripe history.
 * @param pOut the position
 * @param rArg the particle
 * @param pRes the plugin data
 */
inline void CalculateHistoryPos(util::Vector3fType* pOut, const ParticleCalculateArgImpl& rArg,
                                const ResStripeSuper* pRes) {
    util::VectorLoad(pOut, reinterpret_cast<const util::Float3&>(
                               rArg.pEmitter->m_ParticlePos[rArg.particleIndex]));

    if (pRes->isEmitterCoord == 0) {
        util::Matrix4x3fType matrix;
        GetParticleWorldMatrix(&matrix, rArg.pEmitter, rArg.particleIndex);
        VectorTransform(pOut, *pOut, matrix);
    }
}

/**
 * Calculates the velocity of a particle in the coordinates of the stripe history.
 * @param pOut the velocity
 * @param rArg the particle
 * @param pRes the plugin data
 */
inline void CalculateHistoryVec(util::Vector3fType* pOut, const ParticleCalculateArgImpl& rArg,
                                const ResStripeSuper* pRes) {
    util::Vector3fType vec;
    util::VectorLoad(&vec, reinterpret_cast<const util::Float3&>(
                               rArg.pEmitter->m_ParticleVec[rArg.particleIndex]));

    if (pRes->isEmitterCoord == 0) {
        util::Matrix4x3fType matrix;
        GetParticleWorldMatrix(&matrix, rArg.pEmitter, rArg.particleIndex);
        matrix._m.val[3] = vdupq_n_f32(0.0f);
        VectorTransform(&vec, vec, matrix);
    }

    pOut->_v = vec._v;
}

/**
 * Gets the axis the emitter matrix keeps up, as recorded in the stripe history.
 * @param pOut the axis
 * @param pEmitter the emitter
 * @param pRes the plugin data
 */
inline void GetEmitterAxis(util::Vector3fType* pOut, const Emitter* pEmitter,
                           const ResStripeSuper* pRes) {
    if (pRes->isEmitterCoord != 0) {
        pOut->_v = AxisY._v;
    } else {
        pOut->_v = vsetq_lane_f32(0.0f, pEmitter->m_MatrixSrt._m.val[1], 3);
    }
}

/**
 * Converts degrees to a table angle index.
 * @param degree the angle in degrees
 * @return the angle index
 */
inline util::AngleIndex DegreeToAngleIndex(f32 degree) {
    return static_cast<int64_t>(degree * (util::detail::AngleIndexHalfRound /
                                          util::detail::FloatDegree180));
}

/**
 * Evaluates the sine through the interpolated sample table.
 * @param degree the angle in degrees
 * @return the sine of the angle
 */
inline f32 SinTableDegree(f32 degree) {
    util::AngleIndex angleIndex = DegreeToAngleIndex(degree);
    u32 sampleTableIndex = (angleIndex >> 24) & 0xFF;
    f32 rest = static_cast<f32>(angleIndex & 0xFFFFFF) / 0x1000000;
    const util::detail::SinCosSample* pSample = &util::detail::SinCosSampleTable[sampleTableIndex];
    return pSample->sinValue + pSample->sinDelta * rest;
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
 * Builds a matrix rotating around an axis.
 * @param pOut the matrix
 * @param rAxis the unit axis
 * @param angle the angle in radians
 */
inline void MakeAxisRotationMatrix(util::Matrix4x3fType* pOut, const util::Vector3fType& rAxis,
                                   f32 angle) {
    f32 x = vgetq_lane_f32(rAxis._v, 0);
    f32 y = vgetq_lane_f32(rAxis._v, 1);
    f32 z = vgetq_lane_f32(rAxis._v, 2);
    f32 c = CosEst(angle);
    f32 s = SinEst(angle);
    f32 t = 1.0f - c;

    util::Vector3fType row;
    SetVector(&row, c + x * x * t, x * y * t - z * s, z * x * t + y * s);
    pOut->_m.val[0] = row._v;
    SetVector(&row, x * y * t + z * s, c + y * y * t, y * z * t - x * s);
    pOut->_m.val[1] = row._v;
    SetVector(&row, z * x * t - y * s, y * z * t + x * s, c + z * z * t);
    pOut->_m.val[2] = row._v;
    pOut->_m.val[3] = vdupq_n_f32(0.0f);
}

/**
 * Calculates the outer vector of a stripe from its unit direction.
 * @param pOut the outer vector
 * @param rDir the unit direction
 */
inline void CalculateOuterFromDir(util::Vector3fType* pOut, const util::Vector3fType& rDir) {
    f32 dot = VectorDot(rDir._v, AxisY._v);

    if (std::fabs(std::fabs(dot) - 1.0f) < DirectionEpsilon) {
        pOut->_v = VectorNormalize(vmulq_n_f32(AxisX._v, dot));
    } else {
        pOut->_v = VectorNormalize(VectorCross(rDir._v, AxisY._v));
    }
}

/**
 * Moves the older records of a stripe by their velocity.
 * @param pHistory the newest record
 * @param pInstance the stripe
 * @param pRes the plugin data
 * @param airRegist the slow down of the velocity
 * @param frameRate the frame rate
 */
inline void MoveHistory(History* pHistory, const SuperStripeInstance* pInstance,
                        const ResStripeSuper* pRes, f32 airRegist, f32 frameRate) {
    f32 scale = frameRate * airRegist;
    util::Vector3fType acceleration;
    SetVector(&acceleration, scale * pRes->historyAcceleration.x,
              scale * pRes->historyAcceleration.y, scale * pRes->historyAcceleration.z);

    History* const* ppNext = &pHistory->pNext;

    for (int i = 1; i < pInstance->historyNum; i++) {
        History* pOlder = *ppNext;
        pOlder->vec._v = vaddq_f32(acceleration._v, vmulq_n_f32(pOlder->vec._v, airRegist));
        pOlder->pos._v = vaddq_f32(pOlder->pos._v, vmulq_n_f32(pOlder->vec._v, frameRate));
        ppNext = &pOlder->pNext;
    }
}

/**
 * Calculates the turbulence velocity of the newest record of a stripe.
 * @param pHistory the newest record
 * @param rArg the particle
 * @param pInstance the stripe
 * @param pRes the plugin data
 */
ALWAYS_INLINE void CalculateTurbulence(History* pHistory, const ParticleCalculateArgImpl& rArg,
                                const SuperStripeInstance* pInstance,
                                const ResStripeSuper* pRes) {
    f32 speed = pRes->turbulenceSpeed;
    f32 amplitude = pRes->turbulenceAmplitude;
    f32 speedX = pRes->turbulenceFrequency.x * speed;
    f32 speedY = speed * pRes->turbulenceFrequency.y;
    f32 speedZ = speed * pRes->turbulenceFrequency.z;
    f32 time = pInstance->time;
    util::Vector4fType random = GetAnimRandom(rArg);
    f32 randomX = 360.0f * vgetq_lane_f32(random._v, 0);
    f32 randomY = 360.0f * vgetq_lane_f32(random._v, 1);
    f32 randomZ = 360.0f * vgetq_lane_f32(random._v, 2);
    f32 randomW = 360.0f * vgetq_lane_f32(random._v, 3);
    f32 halfTime = time * 0.5f;

    pHistory->vec._v = vsetq_lane_f32(
        SinTableDegree(randomX + time * speedX) + SinTableDegree(randomY + halfTime),
        pHistory->vec._v, 0);
    pHistory->vec._v = vsetq_lane_f32(
        SinTableDegree(randomY + time * speedY) + SinTableDegree(randomZ + halfTime),
        pHistory->vec._v, 1);
    pHistory->vec._v = vsetq_lane_f32(
        SinTableDegree(randomZ + time * speedZ) + SinTableDegree(randomW + halfTime),
        pHistory->vec._v, 2);
    pHistory->vec._v = vsetq_lane_f32(0.0f, pHistory->vec._v, 3);
    pHistory->vec._v = vmulq_n_f32(pHistory->vec._v, amplitude);
}

/**
 * Stores the first three components of a vector.
 * @param pOut the destination
 * @param rVector the vector
 */
inline void StoreVector(util::Float4* pOut, const util::Vector3fType& rVector) {
    float32x4_t vector = rVector._v;
    vst1_f32(pOut->v, vget_low_f32(vector));
    vst1q_lane_f32(&pOut->v[2], vector, 2);
}

/**
 * Sums the lengths of the segments of a stripe.
 * @param pHistory the newest record of the stripe
 * @param historyNum the number of records of the stripe
 * @param pRes the plugin data
 * @return the inverse of the length for distance based texture coordinates, or 0
 */
inline f32 CalculateInvTotalLength(const History* pHistory, int historyNum,
                                   const ResStripeSuper* pRes) {
    if (pRes->texCoordType != StripeTexCoordType_DistanceBased) {
        return 0.0f;
    }

    f32 totalLength = 0.0f;

    for (int i = 0; i < historyNum - 1; i++) {
        pHistory = pHistory->pNext;
        totalLength += VectorLength(vsubq_f32(pHistory->pos._v, pHistory->pPrev->pos._v));
    }

    return 1.0f / totalLength;
}

/**
 * Calculates the width scale applied by the fade of an emitter.
 * @param pEmitter the emitter
 * @return the width scale
 */
inline f32 CalculateFadeScale(const Emitter* pEmitter) {
    const ResEmitter* pResEmitter = pEmitter->m_pEmitterRes->m_pResEmitter;
    f32 scale = pResEmitter->isFadeInScale ? pEmitter->m_FadeInRatio : 1.0f;

    if (pResEmitter->isFadeOutScale) {
        scale *= pEmitter->m_FadeOutRatio;
    }

    return scale;
}

/**
 * Picks the texture coordinate of each of the three textures.
 * @param pOut the texture coordinates of the three textures
 * @param pTexCoords the stretched and the scrolled coordinates
 * @param pRes the plugin data, telling which coordinate each texture uses
 */
inline void SelectTexCoord(util::Vector3fType* pOut, const util::Float3* pTexCoords,
                           const ResStripeSuper* pRes) {
    util::Float3 texCoord;
    texCoord.x = pTexCoords[pRes->texCoordSource[0]].x;
    texCoord.y = pTexCoords[pRes->texCoordSource[1]].y;
    texCoord.z = pTexCoords[pRes->texCoordSource[2]].z;
    util::VectorLoad(pOut, texCoord);
}

/**
 * Gives a stripe back to the stripe system.
 * @param pSystem the stripe system
 * @param pEmitter the emitter of the stripe
 * @param pInstance the stripe
 */
inline void FreeStripe(SuperStripeSystem* pSystem, Emitter* pEmitter,
                       SuperStripeInstance* pInstance) {
    u32 historyNum = static_cast<u32>(GetResource(pEmitter)->historyNum);
    GetDynamicHeap(pEmitter)->Free(pInstance->pHistoryBuffer, historyNum * sizeof(History));
    pInstance->pHistoryBuffer = nullptr;
    pInstance->isUsed = false;
    pSystem->m_ProcessingStripeCount--;
}

/**
 * Calculates the first color of a particle, as the particle itself would be drawn.
 * @param pOut the color
 * @param rArg the particle
 */
inline void CalculateParticleColor0(util::Vector4fType* pOut,
                                    const ParticleCalculateArgImpl& rArg) {
    util::Vector4fType random = GetAnimRandom(rArg);
    util::Vector3fType emitterColor = LoadAnimValue(rArg.pEmitter->m_EmitterAnimValue.color0);
    rArg.pEmitter->m_EmitterCalculator->CalculateParticleColor0VecFromTime(
        pOut, rArg.pEmitter->m_pEmitterRes, random, rArg.pEmitter->m_Color0, emitterColor,
        rArg.pEmitter->m_EmitterAnimValue.alpha0.x, rArg.life, rArg.time);
}

/**
 * Calculates the second color of a particle, as the particle itself would be drawn.
 * @param pOut the color
 * @param rArg the particle
 */
inline void CalculateParticleColor1(util::Vector4fType* pOut,
                                    const ParticleCalculateArgImpl& rArg) {
    util::Vector4fType random = GetAnimRandom(rArg);
    util::Vector3fType emitterColor = LoadAnimValue(rArg.pEmitter->m_EmitterAnimValue.color1);
    rArg.pEmitter->m_EmitterCalculator->CalculateParticleColor1VecFromTime(
        pOut, rArg.pEmitter->m_pEmitterRes, random, rArg.pEmitter->m_Color1, emitterColor,
        rArg.pEmitter->m_EmitterAnimValue.alpha1.x, rArg.life, rArg.time);
}

/**
 * Calculates the scale of a particle.
 * @param pOut the scale
 * @param rArg the particle
 */
inline void CalculateParticleScale(util::Vector3fType* pOut, const ParticleCalculateArgImpl& rArg) {
    util::Vector4fType random = GetAnimRandom(rArg);
    util::Vector4fType scale = GetParticleScale(rArg);
    rArg.pEmitter->m_EmitterCalculator->CalculateParticleScaleVecFromTime(
        pOut, rArg.pEmitter->m_pEmitterRes, scale, random, rArg.life, rArg.time);
}

/**
 * Calculates the rotation of a particle.
 * @param pOut the rotation
 * @param rArg the particle
 */
inline void CalculateParticleRotate(util::Vector3fType* pOut,
                                    const ParticleCalculateArgImpl& rArg) {
    util::Vector4fType rotate = GetParticleRotate(rArg);
    util::Vector4fType random = GetAnimRandom(rArg);
    rArg.pEmitter->m_EmitterCalculator->CalculateRotationMatrix(
        pOut, rArg.pEmitter->m_pEmitterRes, rotate, random, rArg.time);
}

/**
 * Copies the value of an animation key.
 * @param pOut the value
 * @param rKey the key
 */
inline void SetKeyValue(util::Float3* pOut, const ResAnimKey& rKey) {
    pOut->x = rKey.x;
    pOut->y = rKey.y;
    pOut->z = rKey.z;
}

}  // namespace

SuperStripeSystem* SuperStripeSystem::g_pStripeSystem;

/**
 * Creates the super stripe system and registers its callbacks.
 * @param pHeap the heap the stripes are allocated from
 * @param pSystem the effect system
 * @param bufferingMode the number of copies of the vertex buffers
 * @param stripeNum the number of stripes
 */
SuperStripeSystem::SuperStripeSystem(Heap* pHeap, System* pSystem, BufferingMode bufferingMode,
                                     int stripeNum)
    : m_pSystem(pSystem), m_pHeap(pHeap), m_BufferingMode(bufferingMode), m_StripeNum(stripeNum),
      m_StripeWorkSize(stripeNum * sizeof(SuperStripeInstance)), m_StripeIndex(0),
      m_ProcessingStripeCount(0), m_pStripeArray(nullptr) {
    m_pStripeArray = static_cast<SuperStripeInstance*>(m_pHeap->Alloc(m_StripeWorkSize, 0x80));
    std::memset(m_pStripeArray, 0, m_StripeNum * sizeof(SuperStripeInstance));

    CallbackSet callbackSet;
    callbackSet.emitterInitialize = InitializeStripeEmitter;
    callbackSet.emitterPreCalculate = EmitterPreCalculateCallback;
    callbackSet.emitterPostCalculate = EmitterPostCalculateCallback;
    callbackSet.emitterDraw = EmitterDrawCallback;
    callbackSet.emitterFinalize = FinalizeStripeEmitter;
    callbackSet.particleEmit = EmitStripe;
    callbackSet.particleRemove = KillStripe;
    callbackSet.particleCalculate = ParticleCalculateCallback;
    callbackSet.renderStateSet = DummyRenderStateSetCallback;
    m_pSystem->m_CallbackSet[SuperStripeCallbackId] = callbackSet;
}

/**
 * Prepares an emitter using the super stripe plugin.
 * @param rArg the emitter
 * @return whether the emitter is ready to make stripes
 */
bool SuperStripeSystem::InitializeStripeEmitter(EmitterInitializeArg& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return false;
    }

    EmitterPluginUserData* pUserData = static_cast<EmitterPluginUserData*>(
        rArg.pEmitter->GetDynamicHeap()->Alloc(sizeof(EmitterPluginUserData), 0x80));

    if (pUserData == nullptr) {
        return false;
    }

    std::memset(pUserData, 0, sizeof(EmitterPluginUserData));
    rArg.pEmitter->m_pEmitterPluginUserData = pUserData;

    const ResAnimEmitterKeyParamSet* pEmitRateAnim =
        rArg.pEmitter->m_pEmitterRes->m_EmitterAnimArray[EmitterAnimKind_EmissionRate];
    f32 maxEmitRate = rArg.pEmitter->m_pEmitterData->emitRate;

    if (pEmitRateAnim != nullptr) {
        for (int i = 0; i < pEmitRateAnim->keyNum; i++) {
            if (maxEmitRate < pEmitRateAnim->keys[i].x) {
                maxEmitRate = pEmitRateAnim->keys[i].x;
            }
        }
    }

    pUserData->maxEmitRate = static_cast<u32>(std::ceil(maxEmitRate));

    const ResAnimEmitterKeyParamSet* pLifeAnim =
        rArg.pEmitter->m_pEmitterRes->m_EmitterAnimArray[EmitterAnimKind_ParticleLife];
    f32 maxLife = static_cast<f32>(rArg.pEmitter->m_pEmitterData->particleLife);

    if (pLifeAnim != nullptr) {
        for (int i = 0; i < pLifeAnim->keyNum; i++) {
            if (maxLife < pLifeAnim->keys[i].x) {
                maxLife = pLifeAnim->keys[i].x;
            }
        }
    }

    pUserData->maxParticleLife = static_cast<s32>(maxLife);

    const ResStripeSuper* pRes = GetResource(rArg.pEmitter);
    pUserData->shaderParam.x = pRes->shaderParam0[0];
    pUserData->shaderParam.y = pRes->shaderParam0[1];
    pUserData->shaderParam.z = pRes->shaderParam1[0];
    pUserData->shaderParam.w = pRes->shaderParam1[1];

    if (!rArg.pEmitter->InitializeCustomConstantBuffer(4, sizeof(ConstantBufferObject))) {
        return false;
    }

    return g_pStripeSystem->AllocStripeSystemVertexBuffer(rArg.pEmitter);
}

/**
 * Starts a stripe for a new particle.
 * @param rArg the particle
 * @return whether a stripe was started
 */
bool SuperStripeSystem::EmitStripe(ParticleCalculateArgImpl& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return false;
    }

    SuperStripeInstance* pInstance =
        StripeSystemUtility::AllocStripe<SuperStripeSystem>(g_pStripeSystem, rArg.pEmitter);

    if (pInstance == nullptr) {
        return false;
    }

    pInstance->prevTime = 0.0f;
    pInstance->totalLength = 0.0f;
    pInstance->historyNumAtKill = 0;
    rArg.pUserData2 = pInstance;

    EmitterPluginUserData* pUserData = GetUserData(rArg.pEmitter);
    pInstance->index = pUserData->stripeIndex % pUserData->stripeNum;
    pUserData->stripeIndex =
        pUserData->stripeIndex + 1 >= pUserData->stripeNum ? 0 : pUserData->stripeIndex + 1;
    pInstance->random = GetAnimRandom(rArg);
    return true;
}

/**
 * Hands the stripe of a dying particle over to the delayed stripes.
 * @param rArg the particle
 * @return false
 */
bool SuperStripeSystem::KillStripe(ParticleCalculateArgImpl& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return false;
    }

    SuperStripeInstance* pInstance = static_cast<SuperStripeInstance*>(rArg.pUserData2);

    if (pInstance == nullptr) {
        return false;
    }

    pInstance->historyNumAtKill = pInstance->historyNum;
    UpdateStripeColor(rArg, pInstance);

    EmitterPluginUserData* pUserData = GetUserData(rArg.pEmitter);

    if (pUserData->pDelayedStripeHead == nullptr) {
        pUserData->pDelayedStripeHead = pInstance;
    } else {
        SuperStripeInstance* pTail = pUserData->pDelayedStripeHead;

        while (pTail->pNext != nullptr) {
            pTail = pTail->pNext;
        }

        pTail->pNext = pInstance;
    }

    return false;
}

/**
 * Updates the stripe of a particle.
 * @param rArg the particle
 */
void SuperStripeSystem::ParticleCalculateCallback(ParticleCalculateArgImpl& rArg) {
    SuperStripeInstance* pInstance = static_cast<SuperStripeInstance*>(rArg.pUserData2);

    if (pInstance == nullptr) {
        OutputWarning("SuperStripe instance is null\n");
        return;
    }

    if (!pInstance->isUsed) {
        OutputWarning("SuperStripe instance is not used\n");
        return;
    }

    EmitterPluginUserData* pUserData = GetUserData(rArg.pEmitter);

    if (pUserData == nullptr) {
        OutputWarning("EmitterPluginUserData is empty\n");
        return;
    }

    if (!pUserData->isBufferAllocated) {
        OutputWarning("Vertex buffer for stripe has not allocated\n");
        return;
    }

    g_pStripeSystem->CalculateStripe(rArg, pInstance);
}

/**
 * Moves to the next copy of the vertex buffer when the buffers were swapped.
 * @param rArg the emitter
 */
void SuperStripeSystem::EmitterPreCalculateCallback(EmitterPreCalculateArg& rArg) {
    if (!rArg.isBufferSwapped || rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return;
    }

    EmitterPluginUserData* pUserData = GetUserData(rArg.pEmitter);
    int bufferNum = rArg.pEmitter->m_EmitterSet->m_System->m_IsTripleBuffer ? 3 : 2;
    pUserData->bufferSide = static_cast<BufferSide>((pUserData->bufferSide + 1) % bufferNum);
}

/**
 * Updates the stripes of the dead particles of an emitter.
 * @param rArg the emitter
 */
void SuperStripeSystem::EmitterPostCalculateCallback(EmitterPostCalculateArg& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return;
    }

    g_pStripeSystem->CalculateDelayedStripe(rArg.pEmitter);
}

/**
 * Draws the stripes of an emitter.
 * @param rArg the emitter
 * @return whether the emitter was drawn
 */
bool SuperStripeSystem::EmitterDrawCallback(EmitterDrawArg& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return false;
    }

    StripeSystemUtility::DrawParticleStripeEmitter<SuperStripeSystem>(
        rArg.pCommandBuffer, g_pStripeSystem->m_pSystem, rArg.pEmitter, rArg.shaderType,
        rArg.pUserParam, rArg.pDrawParameterArg);
    return true;
}

/**
 * Frees the stripes and the plugin state of an emitter.
 * @param rArg the emitter
 */
void SuperStripeSystem::FinalizeStripeEmitter(EmitterFinalizeArg& rArg) {
    if (rArg.pEmitter->m_pEmitterData->calcType != 0) {
        return;
    }

    EmitterPluginUserData* pUserData = GetUserData(rArg.pEmitter);

    if (pUserData == nullptr) {
        return;
    }

    pUserData->isBufferAllocated = false;
    Emitter* pEmitter = rArg.pEmitter;
    SuperStripeInstance* pInstance = pUserData->pDelayedStripeHead;

    if (pInstance != nullptr) {
        SuperStripeSystem* pSystem = g_pStripeSystem;

        do {
            FreeStripe(pSystem, pEmitter, pInstance);
            pInstance = pInstance->pNext;
        } while (pInstance != nullptr);

        pUserData->pDelayedStripeHead = nullptr;
    }

    rArg.pEmitter->GetDynamicHeap()->Free(pUserData);
    rArg.pEmitter->m_pEmitterPluginUserData = nullptr;
}

/**
 * Frees the stripes.
 */
SuperStripeSystem::~SuperStripeSystem() {
    m_pHeap->Free(m_pStripeArray);
    m_pStripeArray = nullptr;
}

/**
 * Creates the super stripe system.
 * @param pHeap the heap the system is allocated from
 * @param pSystem the effect system
 * @param bufferingMode the number of copies of the vertex buffers
 * @param stripeNum the number of stripes
 */
void SuperStripeSystem::InitializeSystem(Heap* pHeap, System* pSystem,
                                         BufferingMode bufferingMode, int stripeNum) {
    if (g_pStripeSystem != nullptr) {
        OutputError("SuperStripeSystem is already initialized.\n");
    }

    void* pBuffer = pHeap->Alloc(sizeof(SuperStripeSystem), 0x80);

    if (pBuffer == nullptr) {
        OutputWarning("[SuperStripe] Memory Allocate Error!! : %d\n",
                      static_cast<int>(sizeof(SuperStripeSystem)));
        return;
    }

    g_pStripeSystem = new (pBuffer) SuperStripeSystem(pHeap, pSystem, bufferingMode, stripeNum);
}

/**
 * Destroys the super stripe system.
 * @param pHeap the heap the system was allocated from
 */
void SuperStripeSystem::FinalizeSystem(Heap* pHeap) {
    g_pStripeSystem->~SuperStripeSystem();
    pHeap->Free(g_pStripeSystem);
    g_pStripeSystem = nullptr;
}

/**
 * Allocates the vertex buffer of the stripes of an emitter.
 * @param pEmitter the emitter
 * @return whether the vertex buffer is allocated
 */
bool SuperStripeSystem::AllocStripeSystemVertexBuffer(Emitter* pEmitter) {
    const ResStripeSuper* pRes = GetResource(pEmitter);
    int historyNum = static_cast<int>(pRes->historyNum);
    EmitterPluginUserData* pUserData = GetUserData(pEmitter);
    int delayStripeNum = StripeSystemUtility::CalculateDelayStripeCount(pEmitter, historyNum);
    pUserData->stripeNum = pEmitter->m_MaxParticleNum + delayStripeNum;
    pUserData->vertexNumPerStripe = (pRes->divideNum * (historyNum - 1) + historyNum) * 2;
    int vertexNum = pUserData->vertexNumPerStripe * pUserData->stripeNum;
    pUserData->vertexNum = vertexNum * m_BufferingMode;
    pUserData->isBufferAllocated = false;

    if (vertexNum == 0) {
        return false;
    }

    if (pEmitter->InitializeCustomAttribute(m_BufferingMode, vertexNum * sizeof(VertexAttribute))) {
        pUserData->isBufferAllocated = true;
    }

    return pUserData->isBufferAllocated;
}

/**
 * Calculates the colors of a stripe.
 * @param rArg the particle of the stripe
 * @param pInstance the stripe
 */
void SuperStripeSystem::UpdateStripeColor(ParticleCalculateArgImpl& rArg,
                                          SuperStripeInstance* pInstance) {
    util::Vector4fType random = GetAnimRandom(rArg);
    rArg.pEmitter->m_EmitterCalculator->CalculateParticleColor0RawValue(
        &pInstance->color0, rArg.pEmitter->m_pEmitterRes, random, rArg.life, rArg.time);
    rArg.pEmitter->m_EmitterCalculator->CalculateParticleColor1RawValue(
        &pInstance->color1, rArg.pEmitter->m_pEmitterRes, random, rArg.life, rArg.time);

    f32 colorScale = rArg.pEmitter->m_pEmitterRes->m_pEmitterStaticUbo->colorScale;
    util::Vector4fType color0 = pInstance->color0;
    util::Vector4fType color1 = pInstance->color1;
    color0._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color0._v, 0), color0._v, 0);
    color0._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color0._v, 1), color0._v, 1);
    color0._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color0._v, 2), color0._v, 2);
    color1._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color1._v, 0), color1._v, 0);
    color1._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color1._v, 1), color1._v, 1);
    color1._v = vsetq_lane_f32(colorScale * vgetq_lane_f32(color1._v, 2), color1._v, 2);
    pInstance->color0 = color0;
    pInstance->color1 = color1;
}

/**
 * Updates a stripe and builds its polygon.
 * @param rArg the particle of the stripe
 * @param pInstance the stripe
 */
void SuperStripeSystem::CalculateStripe(ParticleCalculateArgImpl& rArg,
                                        SuperStripeInstance* pInstance) {
    Emitter* pEmitter = rArg.pEmitter;
    EmitterPluginUserData* pUserData = GetUserData(pEmitter);
    ResStripeSuper* pRes = GetResource(pEmitter);
    VertexAttribute* pVertex =
        static_cast<VertexAttribute*>(pEmitter->m_Attribute.Map(pUserData->bufferSide));

    pInstance->time = rArg.time;
    pInstance->life = rArg.life;
    pInstance->vertexNum = 0;
    UpdateStripeColor(rArg, pInstance);

    bool isHistoryUpdated = std::floor(pInstance->time) - std::floor(pInstance->prevTime) > 0.0f ||
                            pInstance->time == 0.0f;
    pInstance->prevTime = rArg.time;
    UpdateHistory(rArg, pInstance, pRes, isHistoryUpdated);

    VertexAttribute* pStripeVertex =
        &pVertex[pUserData->vertexNumPerStripe * pInstance->index];

    if (pRes->divideNum != 0) {
        MakeStripePolygonWithDivision(pStripeVertex, pInstance, pEmitter, pRes, 0.0f);
    } else {
        MakeStripePolygon(pStripeVertex, pInstance, pEmitter, pRes, 0.0f);
    }

    rArg.pEmitter->m_Attribute.Unmap();
}

/**
 * Records the current state of the particle of a stripe.
 * @param rArg the particle of the stripe
 * @param pInstance the stripe
 * @param pRes the plugin data
 * @param isHistoryUpdated whether a new record is started
 */
void SuperStripeSystem::UpdateHistory(ParticleCalculateArgImpl& rArg,
                                      SuperStripeInstance* pInstance, ResStripeSuper* pRes,
                                      bool isHistoryUpdated) {
    f32 frameRate = rArg.pEmitter->m_FrameRate;
    f32 airRegist = pRes->historyAirRegist + (1.0f - frameRate) * (1.0f - pRes->historyAirRegist);

    util::Vector4fType color0;
    CalculateParticleColor0(&color0, rArg);
    util::Vector4fType color1;
    CalculateParticleColor1(&color1, rArg);

    if (!isHistoryUpdated) {
        History* pHistory = pInstance->pHistoryHead->pNext;
        util::Vector3fType scale;
        CalculateParticleScale(&scale, rArg);
        scale._v = vmulq_f32(scale._v, rArg.pEmitter->m_EmitterSet->m_ParticleScaleForCalc._v);

        util::Vector3fType prevPos = pHistory->pos;
        util::Vector3fType pos;
        CalculateHistoryPos(&pos, rArg, pRes);
        util::Vector3fType delta;
        delta._v = vsubq_f32(pos._v, prevPos._v);
        pHistory->pos = pos;
        pInstance->pos = pHistory->pos;
        pInstance->totalLength += VectorLength(delta._v);
        pHistory->scale = vgetq_lane_f32(scale._v, 0);
        pHistory->emitterAxis._v = rArg.pEmitter->m_MatrixSrt._m.val[1];

        if (pInstance->historyNum >= 2) {
            MoveHistory(pInstance->pHistoryHead->pNext, pInstance, pRes, airRegist, frameRate);
        }

        return;
    }

    if (pRes->type == SuperStripeType_Ribbon) {
        History* pHistory = pInstance->pHistoryHead;
        History* pPrevHistory = pHistory->pNext;
        pInstance->pHistoryHead = pHistory->pPrev;

        if (pRes->historyNum - 1.0f > pInstance->historyNum) {
            pInstance->historyNum++;
        }

        util::Vector3fType pos;
        CalculateHistoryPos(&pos, rArg, pRes);
        pHistory->pos = pos;
        pInstance->pos = pHistory->pos;

        if (pInstance->historyNum >= 2) {
            MoveHistory(pHistory, pInstance, pRes, airRegist, frameRate);
        }

        CalculateTurbulence(pHistory, rArg, pInstance, pRes);
        GetEmitterAxis(&pHistory->emitterAxis, rArg.pEmitter, pRes);

        util::Vector3fType scale;
        CalculateParticleScale(&scale, rArg);
        pHistory->scale = vgetq_lane_f32(scale._v, 0) *
                          vgetq_lane_f32(rArg.pEmitter->m_EmitterSet->m_ParticleScaleForCalc._v, 0);

        if (pInstance->historyNum == 1) {
            CalculateHistoryVec(&pHistory->dir, rArg, pRes);
            util::Vector3fType acceleration;
            SetVector(&acceleration, rArg.life * pRes->historyAcceleration.x,
                      rArg.life * pRes->historyAcceleration.y,
                      rArg.life * pRes->historyAcceleration.z);
            pHistory->dir._v = vaddq_f32(pHistory->dir._v, acceleration._v);

            if (VectorDot(pHistory->dir._v, pHistory->dir._v) < DirectionEpsilon) {
                pHistory->dir._v = AxisY._v;
                pHistory->outer._v = AxisX._v;
            } else {
                util::Vector3fType dir;
                dir._v = VectorNormalize(pHistory->dir._v);
                CalculateOuterFromDir(&pHistory->outer, dir);
            }
        } else {
            pHistory->dir._v = vsubq_f32(pHistory->pos._v, pHistory->pNext->pos._v);

            float32x4_t dir;
            float32x4_t normal;
            bool isCross = true;

            if (VectorDot(pHistory->dir._v, pHistory->dir._v) < DirectionEpsilon) {
                util::Vector3fType delta;
                delta._v = vsubq_f32(pHistory->pos._v, pHistory->pNext->pos._v);

                if (VectorDot(delta._v, delta._v) < DirectionEpsilon) {
                    pHistory->outer = pHistory->pNext->outer;
                    pHistory->dir = pHistory->pNext->dir;
                    isCross = false;
                } else {
                    dir = VectorNormalize(delta._v);
                    f32 dot = VectorDot(dir, AxisY._v);

                    if (std::fabs(std::fabs(dot) - 1.0f) < DirectionEpsilon) {
                        pHistory->outer._v = VectorNormalize(vmulq_n_f32(AxisX._v, dot));
                        isCross = false;
                    } else {
                        normal = AxisY._v;
                    }
                }
            } else {
                dir = VectorNormalize(pHistory->dir._v);
                float32x4_t prevDir = VectorNormalize(pHistory->pNext->dir._v);
                float32x4_t prevOuter = VectorNormalize(pHistory->pNext->outer._v);
                normal = VectorNormalize(VectorCross(prevOuter, prevDir));
            }

            if (isCross) {
                pHistory->outer._v = VectorNormalize(VectorCross(dir, normal));
            }

            if (pInstance->historyNum == 2) {
                if (rArg.pEmitter->m_pEmitterData->rotType != 0) {
                    pHistory->dir._v = VectorNormalize(pHistory->dir._v);
                    pHistory->outer._v = VectorNormalize(pHistory->outer._v);

                    const EmitterStaticUniformBlock* pUbo =
                        rArg.pEmitter->m_pEmitterRes->m_pEmitterStaticUbo;
                    f32 angle =
                        pUbo->rotateInit.z +
                        rArg.pEmitter->m_ParticleAnimRandom[rArg.particleIndex].x *
                            pUbo->rotateInitRandom.z;
                    util::Matrix4x3fType matrix;
                    MakeAxisRotationMatrix(&matrix, pHistory->dir, angle);
                    VectorTransform(&pHistory->outer, pHistory->outer, matrix);
                }

                pHistory->pNext->outer = pHistory->outer;
                pHistory->pNext->dir = pHistory->dir;
                pHistory->pNext->outer._v = VectorNormalize(pHistory->pNext->outer._v);
                pHistory->pNext->dir._v = VectorNormalize(pHistory->pNext->dir._v);
            }
        }

        pHistory->dir._v = VectorNormalize(pHistory->dir._v);
        pHistory->outer._v = VectorNormalize(pHistory->outer._v);

        util::Vector3fType delta = pHistory->pos;

        if (pInstance->historyNum != 1) {
            delta._v = vsubq_f32(pHistory->pos._v, pPrevHistory->pos._v);
        }

        pInstance->totalLength += VectorLength(delta._v);
        return;
    }

    History* pHistory = pInstance->pHistoryHead;
    pInstance->pHistoryHead = pHistory->pPrev;
    History* pPrevHistory = pHistory->pNext;

    util::Vector3fType scale;
    CalculateParticleScale(&scale, rArg);
    scale._v = vmulq_f32(scale._v, rArg.pEmitter->m_EmitterSet->m_ParticleScaleForCalc._v);

    util::Vector3fType pos;
    CalculateHistoryPos(&pos, rArg, pRes);
    pHistory->pos = pos;
    pInstance->pos = pHistory->pos;

    util::Vector3fType delta = pos;

    if (pInstance->historyNum != 0) {
        delta._v = vsubq_f32(pos._v, pPrevHistory->pos._v);
    }

    pInstance->totalLength += VectorLength(delta._v);
    pHistory->scale = vgetq_lane_f32(scale._v, 0);
    GetEmitterAxis(&pHistory->emitterAxis, rArg.pEmitter, pRes);

    if ((pRes->type == SuperStripeType_EmitterMatrix ||
         pRes->type == SuperStripeType_EmitterUpDown) &&
        rArg.pEmitter->m_pEmitterData->rotType != 0) {
        util::Vector3fType axis;
        axis._v = vdupq_n_f32(0.0f);

        if (pInstance->historyNum > 0) {
            float32x4_t move = vsubq_f32(pHistory->pos._v, pHistory->pNext->pos._v);
            axis._v = vaddq_f32(vmulq_n_f32(move, 1.0f / rArg.pEmitter->m_FrameRate), axis._v);
        } else {
            axis = pHistory->dir;
        }

        if (VectorDot(axis._v, axis._v) < DirectionEpsilon) {
            axis._v = rArg.pEmitter->m_MatrixSrt._m.val[2];
        }

        axis._v = VectorNormalize(axis._v);

        util::Vector3fType rotate;
        CalculateParticleRotate(&rotate, rArg);
        VectorRotateArbitraryAxis(&pHistory->emitterAxis, pHistory->emitterAxis, axis,
                                  vgetq_lane_f32(rotate._v, 2));
        pHistory->emitterAxis._v = VectorNormalize(pHistory->emitterAxis._v);

        if (pInstance->historyNum == 1) {
            pHistory->pNext->emitterAxis = pHistory->emitterAxis;
        }
    }

    CalculateTurbulence(pHistory, rArg, pInstance, pRes);

    if (pRes->historyNum - 1.0f > pInstance->historyNum) {
        pInstance->historyNum++;
    }

    if (pInstance->historyNum <= 1) {
        CalculateHistoryVec(&pHistory->dir, rArg, pRes);

        if (VectorDot(pHistory->dir._v, pHistory->dir._v) > 0.0f) {
            pHistory->dir._v = VectorNormalize(pHistory->dir._v);
        } else {
            float32x4_t move = vsubq_f32(rArg.pEmitter->m_MatrixSrt._m.val[3],
                                         rArg.pEmitter->m_EmitterPrevPos._v);

            if (VectorDot(move, move) > 0.0f) {
                pHistory->dir._v = VectorNormalize(move);
            } else {
                pHistory->dir._v = DefaultDir._v;
            }
        }

        pHistory->outer = pHistory->dir;
    } else {
        pHistory->dir._v = vsubq_f32(pHistory->pos._v, pPrevHistory->pos._v);

        if (pInstance->historyNum == 2) {
            pPrevHistory->dir = pHistory->dir;
        }

        util::Vector3fType outer;
        outer._v = vdupq_n_f32(0.0f);
        const History* pDirHistory = pHistory;

        for (int i = 0; i < pInstance->historyNum; i++) {
            f32 weight = static_cast<f32>(pInstance->historyNum - i);
            outer._v = vaddq_f32(outer._v, vmulq_n_f32(pDirHistory->dir._v, weight));
            pDirHistory = pDirHistory->pNext;
        }

        if (VectorDot(outer._v, outer._v) > 0.0f) {
            outer._v = VectorNormalize(outer._v);
        }

        pHistory->outer = outer;

        if (pInstance->historyNum == 2) {
            pPrevHistory->outer = pHistory->outer;
        }
    }

    if (VectorDot(pHistory->outer._v, pHistory->outer._v) > 0.0f) {
        pHistory->outer._v = VectorNormalize(pHistory->outer._v);
    } else {
        pHistory->outer._v = AxisY._v;
    }

    if (pInstance->historyNum >= 2) {
        MoveHistory(pHistory, pInstance, pRes, airRegist, frameRate);
    }
}

/**
 * Builds the polygon of a stripe, one vertex pair for every record.
 * @param pVertex the vertices of the stripe
 * @param pInstance the stripe
 * @param pEmitter the emitter of the stripe
 * @param pRes the plugin data
 * @param texCoordOffset the offset of the uniform texture coordinates
 */
void SuperStripeSystem::MakeStripePolygon(VertexAttribute* pVertex,
                                          SuperStripeInstance* pInstance,
                                          const Emitter* pEmitter, const ResStripeSuper* pRes,
                                          f32 texCoordOffset) {
    int historyNum = pInstance->historyNum;

    if (historyNum <= 1) {
        pInstance->vertexNum = 0;
        return;
    }

    const History* pHistory = pInstance->pHistoryHead->pNext;
    f32 invTotalLength = CalculateInvTotalLength(pHistory, historyNum, pRes);
    f32 width = CalculateFadeScale(pEmitter);
    f32 offset = pInstance->time - std::floor(pInstance->time) + texCoordOffset;

    int i = 0;

    for (; i < historyNum; i++) {
        util::Vector3fType texCoord;
        texCoord._v = vdupq_n_f32(0.0f);

        switch (pRes->texCoordType) {
        case StripeTexCoordType_Uniform:
            CalculateTextureOffsetUniform(&texCoord, pRes, i, historyNum,
                                          static_cast<int>(pRes->historyNum), offset);
            break;
        case StripeTexCoordType_DistanceBased: {
            f32 length = VectorLength(vsubq_f32(pHistory->pPrev->pos._v, pHistory->pos._v));
            int prevIndex = (i - 1) * 2;
            CalculateTextureOffsetDistanceBased(
                &texCoord, pRes, i, length, pInstance->totalLength, invTotalLength,
                pVertex[prevIndex > 0 ? prevIndex : 0].texCoord);
            break;
        }
        }

        MakeDefaultVertexAttribute(pVertex, i, pHistory->pos, pHistory->dir, pHistory->outer,
                                   texCoord, width * pHistory->scale);
        pHistory = pHistory->pNext;
    }

    pInstance->vertexNum = i * 2;

    if (pRes->type == SuperStripeType_Billboard || pRes->type == SuperStripeType_Ribbon) {
        return;
    }

    pHistory = pInstance->pHistoryHead;

    for (i = 0; i < historyNum; i++) {
        pHistory = pHistory->pNext;
        StoreVector(&pVertex[i * 2].emitterAxis, pHistory->emitterAxis);
        StoreVector(&pVertex[i * 2 + 1].emitterAxis, pHistory->emitterAxis);
    }
}

/**
 * Builds the polygon of a stripe, adding interpolated vertex pairs between the records.
 * @param pVertex the vertices of the stripe
 * @param pInstance the stripe
 * @param pEmitter the emitter of the stripe
 * @param pRes the plugin data
 * @param texCoordOffset the offset of the uniform texture coordinates
 */
void SuperStripeSystem::MakeStripePolygonWithDivision(VertexAttribute* pVertex,
                                                      SuperStripeInstance* pInstance,
                                                      const Emitter* pEmitter,
                                                      const ResStripeSuper* pRes,
                                                      f32 texCoordOffset) {
    int historyNum = pInstance->historyNum;

    if (historyNum <= 2) {
        pInstance->vertexNum = 0;
        return;
    }

    const History* pHistory = pInstance->pHistoryHead->pNext;
    f32 invTotalLength = CalculateInvTotalLength(pHistory, historyNum, pRes);
    f32 width = CalculateFadeScale(pEmitter);
    int divideNum = pRes->divideNum;
    int vertexPairNum = divideNum * (historyNum - 1) + historyNum;
    f32 invDivision = 1.0f / (divideNum + 1);
    f32 offset = pInstance->time - std::floor(pInstance->time) + texCoordOffset;
    util::Vector3fType prevPos;
    prevPos._v = vdupq_n_f32(0.0f);
    int historyIndex = 0;

    int i = 0;

    for (; i < vertexPairNum; i++) {
        f32 position = invDivision * i;
        int index = static_cast<int>(std::floor(position));

        if (historyIndex < index) {
            pHistory = pHistory->pNext;
            historyIndex = index;
        }

        util::Vector3fType pos;
        util::Vector3fType dir;
        StripeSystemUtility::CalculateHermiteInterpolatedCurveVec<SuperStripeSystem>(
            &pos, &dir, pHistory, index, historyNum, position - index);

        util::Vector3fType texCoord;
        texCoord._v = vdupq_n_f32(0.0f);

        switch (pRes->texCoordType) {
        case StripeTexCoordType_Uniform: {
            int historyVertexNum = (static_cast<int>(pRes->historyNum) - 1) * divideNum +
                                   static_cast<int>(pRes->historyNum);
            CalculateTextureOffsetUniform(&texCoord, pRes, i, vertexPairNum, historyVertexNum,
                                          offset);
            break;
        }
        case StripeTexCoordType_DistanceBased: {
            f32 length = i != 0 ? VectorLength(vsubq_f32(pos._v, prevPos._v)) : 0.0f;
            int prevIndex = (i - 1) * 2;
            CalculateTextureOffsetDistanceBased(
                &texCoord, pRes, i, length, pInstance->totalLength, invTotalLength,
                pVertex[prevIndex > 0 ? prevIndex : 0].texCoord);
            prevPos = pos;
            break;
        }
        }

        MakeDefaultVertexAttribute(pVertex, i, pos, dir, pHistory->outer, texCoord,
                                   width * pHistory->scale);
    }

    pInstance->vertexNum = i * 2;

    if (pRes->type == SuperStripeType_Billboard || pRes->type == SuperStripeType_Ribbon) {
        return;
    }

    pHistory = pInstance->pHistoryHead->pNext;
    historyIndex = 0;

    for (i = 0; i < vertexPairNum; i++) {
        f32 position = invDivision * i;
        int index = static_cast<int>(std::floor(position));

        if (historyIndex < index) {
            pHistory = pHistory->pNext;
            historyIndex = index;
        }

        f32 t = position - index;
        util::Vector3fType axis;
        axis._v = vfmaq_n_f32(pHistory->emitterAxis._v,
                              vsubq_f32(pHistory->pNext->emitterAxis._v, pHistory->emitterAxis._v),
                              t);
        axis._v = VectorNormalize(axis._v);
        StoreVector(&pVertex[i * 2].emitterAxis, axis);
        StoreVector(&pVertex[i * 2 + 1].emitterAxis, axis);
    }
}

/**
 * Updates the stripes of the dead particles of an emitter until they have shrunk away.
 * @param pEmitter the emitter
 */
void SuperStripeSystem::CalculateDelayedStripe(Emitter* pEmitter) {
    EmitterPluginUserData* pUserData = GetUserData(pEmitter);
    f32 frameRate = pEmitter->m_FrameRate;
    SuperStripeInstance* pInstance = pUserData->pDelayedStripeHead;

    if (pInstance == nullptr || !pUserData->isBufferAllocated) {
        return;
    }

    const ResStripeSuper* pRes = GetResource(pEmitter);
    VertexAttribute* pVertex =
        static_cast<VertexAttribute*>(pEmitter->m_Attribute.Map(pUserData->bufferSide));
    SuperStripeInstance* pPrevInstance = nullptr;

    do {
        int historyNum = pInstance->historyNum;
        int index = pInstance->index;
        int vertexNumPerStripe = pUserData->vertexNumPerStripe;
        SuperStripeInstance* pNextInstance = pInstance->pNext;

        if (historyNum > 0) {
            f32 airRegist =
                pRes->historyAirRegist + (1.0f - frameRate) * (1.0f - pRes->historyAirRegist);
            History* pHistory = pInstance->pHistoryHead;

            for (int i = 0; i < pInstance->historyNum; i++) {
                pHistory = pHistory->pNext;
                util::Vector3fType acceleration = LoadAnimValue(pRes->historyAcceleration);
                pHistory->vec._v = vmulq_n_f32(
                    vaddq_f32(pHistory->vec._v, vmulq_n_f32(acceleration._v, frameRate)),
                    airRegist);
            }

            pHistory = pInstance->pHistoryHead;

            for (int i = 0; i < pInstance->historyNum; i++) {
                pHistory = pHistory->pNext;
                pHistory->pos._v =
                    vaddq_f32(pHistory->pos._v, vmulq_n_f32(pHistory->vec._v, frameRate));
            }

            historyNum = pInstance->historyNum;
        }

        pInstance->time += pEmitter->m_FrameRate;
        f32 prevTime = pInstance->prevTime;
        pInstance->prevTime = pInstance->time;
        int remainNum = historyNum - 1;

        if (remainNum > 0) {
            if (std::floor(pInstance->time) - std::floor(prevTime) > 0.0f) {
                pInstance->historyNum = remainNum;
            }

            f32 texCoordOffset =
                static_cast<f32>(pInstance->historyNumAtKill - pInstance->historyNum);
            VertexAttribute* pStripeVertex = &pVertex[vertexNumPerStripe * index];

            if (pRes->divideNum != 0) {
                MakeStripePolygonWithDivision(pStripeVertex, pInstance, pEmitter, pRes,
                                              texCoordOffset * (pRes->divideNum + 1));
            } else {
                MakeStripePolygon(pStripeVertex, pInstance, pEmitter, pRes, texCoordOffset);
            }

            pPrevInstance = pInstance;
        } else {
            if (pPrevInstance == nullptr) {
                pUserData->pDelayedStripeHead = pNextInstance;
            } else {
                pPrevInstance->pNext = pNextInstance;
            }

            FreeStripe(g_pStripeSystem, pEmitter, pInstance);
        }

        pInstance = pNextInstance;
    } while (pInstance != nullptr);

    pEmitter->m_Attribute.Unmap();
}

/**
 * Calculates texture coordinates spread evenly over a stripe.
 * @param pOut the texture coordinates of the three textures
 * @param pRes the plugin data
 * @param index the index of the vertex pair
 * @param vertexNum the number of vertex pairs
 * @param historyNum the number of vertex pairs of a full stripe
 * @param texCoordOffset the offset of the coordinates
 */
void SuperStripeSystem::CalculateTextureOffsetUniform(util::Vector3fType* pOut,
                                                      const ResStripeSuper* pRes, int index,
                                                      int vertexNum, int historyNum,
                                                      f32 texCoordOffset) {
    f32 stretch = static_cast<f32>(index) / static_cast<f32>(vertexNum - 1);
    f32 scroll = (static_cast<f32>(index) + texCoordOffset) / static_cast<f32>(historyNum - 1);
    util::Float3 texCoords[2];
    texCoords[0].x = stretch;
    texCoords[0].y = stretch;
    texCoords[0].z = stretch;
    texCoords[1].x = scroll > 1.0f ? 1.0f : scroll;
    texCoords[1].y = texCoords[1].x;
    texCoords[1].z = texCoords[1].x;
    SelectTexCoord(pOut, texCoords, pRes);
}

/**
 * Calculates texture coordinates following the length of a stripe.
 * @param pOut the texture coordinates of the three textures
 * @param pRes the plugin data
 * @param index the index of the vertex pair
 * @param length the length from the previous vertex pair
 * @param totalLength the length of the stripe
 * @param invTotalLength the inverse of the length of the stripe
 * @param rPrevTexCoord the texture coordinates of the previous vertex pair
 */
void SuperStripeSystem::CalculateTextureOffsetDistanceBased(util::Vector3fType* pOut,
                                                            const ResStripeSuper* pRes, int index,
                                                            f32 length, f32 totalLength,
                                                            f32 invTotalLength,
                                                            const util::Float4& rPrevTexCoord) {
    f32 stretch = length * invTotalLength;
    util::Float3 texCoords[2];
    texCoords[0].x = index == 0 ? 0.0f : stretch + rPrevTexCoord.x;
    texCoords[0].y = index == 0 ? 0.0f : stretch + rPrevTexCoord.y;
    texCoords[0].z = index == 0 ? 0.0f : stretch + rPrevTexCoord.z;

    f32 scroll = length * TexCoordPerLength;
    f32 startScroll = totalLength * TexCoordPerLength;
    texCoords[1].x = index == 0 ? startScroll : rPrevTexCoord.x - scroll;
    texCoords[1].y = index == 0 ? startScroll : rPrevTexCoord.y - scroll;
    texCoords[1].z = index == 0 ? startScroll : rPrevTexCoord.z - scroll;
    SelectTexCoord(pOut, texCoords, pRes);
}

/**
 * Writes the two vertices of one point of a stripe.
 * @param pVertex the vertices of the stripe
 * @param index the index of the vertex pair
 * @param rPos the position
 * @param rDir the direction of the stripe
 * @param rOuter the outer vector of the stripe
 * @param rTexCoord the texture coordinates
 * @param width the half width of the stripe
 */
void SuperStripeSystem::MakeDefaultVertexAttribute(VertexAttribute* pVertex, int index,
                                                   const util::Vector3fType& rPos,
                                                   const util::Vector3fType& rDir,
                                                   const util::Vector3fType& rOuter,
                                                   const util::Vector3fType& rTexCoord,
                                                   f32 width) {
    VertexAttribute* pLeft = &pVertex[index * 2];
    VertexAttribute* pRight = &pVertex[index * 2 + 1];
    StoreVector(&pLeft->pos, rPos);
    StoreVector(&pRight->pos, rPos);
    pLeft->pos.w = width;
    pRight->pos.w = -width;
    StoreVector(&pLeft->dir, rDir);
    StoreVector(&pRight->dir, rDir);
    StoreVector(&pLeft->outer, rOuter);
    StoreVector(&pRight->outer, rOuter);
    StoreVector(&pLeft->texCoord, rTexCoord);
    StoreVector(&pRight->texCoord, rTexCoord);
    pLeft->dir.w = static_cast<f32>(index);
    pRight->dir.w = static_cast<f32>(index);
}

/**
 * Gets how long a one time emitter lives on for its stripes to shrink away.
 * @param pEmitter the emitter
 * @return the number of extra frames
 */
int SuperStripeSystem::GetExtendedEndTimeForOneTimeEmitter(Emitter* pEmitter) {
    f32 historyNum = GetResource(pEmitter)->historyNum;

    if (pEmitter->m_FrameRate > 1.0f) {
        historyNum *= pEmitter->m_FrameRate;
    }

    return static_cast<int>(historyNum);
}

/**
 * Counts the stripes an emitter calculates.
 * @param pEmitter the emitter
 * @return the number of living and delayed stripes
 */
int SuperStripeSystem::GetActualSuperStripeCalclationCount(const Emitter* pEmitter) {
    const EmitterPluginUserData* pUserData = GetUserData(pEmitter);

    if (!pUserData->isBufferAllocated) {
        return 0;
    }

    int count = pEmitter->m_AliveParticleNum;

    for (const SuperStripeInstance* pInstance = pUserData->pDelayedStripeHead;
         pInstance != nullptr; pInstance = pInstance->pNext) {
        count++;
    }

    return count;
}

/**
 * Fills the constant buffer of a stripe.
 * @param pOut the constant buffer
 * @param pRes the plugin data
 * @param pUserData the plugin state of the emitter
 * @param pInstance the stripe
 * @param pEmitter the emitter
 * @param meshType the mesh drawn with the constant buffer
 */
void SuperStripeSystem::MakeConstantBufferObject(ConstantBufferObject* pOut,
                                                 const ResStripeSuper* pRes,
                                                 const EmitterPluginUserData* pUserData,
                                                 const SuperStripeInstance* pInstance,
                                                 const Emitter* pEmitter,
                                                 StripeMeshType meshType) {
    pOut->random._v = pInstance->random._v;
    pOut->shaderParam = pUserData->shaderParam;
    pOut->color0._v = pInstance->color0._v;
    pOut->color1._v = pInstance->color1._v;
    pOut->time = pInstance->time;
    pOut->vertexNum =
        static_cast<f32>((pInstance->historyNum - 1) * pRes->divideNum + pInstance->historyNum);
    pOut->meshType = static_cast<f32>(meshType);
    pOut->life = pInstance->life;
    pOut->pos.x = vgetq_lane_f32(pInstance->pos._v, 0);
    pOut->pos.y = vgetq_lane_f32(pInstance->pos._v, 1);
    pOut->pos.z = vgetq_lane_f32(pInstance->pos._v, 2);
    pOut->pos.w = 0.0f;
}

/**
 * @return the size of the memory used by the stripes
 */
size_t SuperStripeSystem::GetWorkSize() {
    if (g_pStripeSystem == nullptr) {
        return 0;
    }

    return g_pStripeSystem->m_StripeWorkSize;
}

/**
 * @return the number of stripes in use
 */
int SuperStripeSystem::GetProcessingStripeCount() {
    if (g_pStripeSystem == nullptr) {
        return 0;
    }

    return g_pStripeSystem->m_ProcessingStripeCount;
}

/**
 * Evaluates an emitter key frame animation.
 * @param pOut the animated value
 * @param pIsEnd set when the animation has reached its last key
 * @param pAnim the animation
 * @param time the frame of the emitter
 */
void CalculateEmitterKeyFrameAnimation(util::Float3* pOut, bool* pIsEnd,
                                       const ResAnimEmitterKeyParamSet* pAnim, f32 time) {
    int keyNum = pAnim->keyNum;
    const f32& rLastTime = pAnim->keys[keyNum - 1].time;

    if (keyNum == 0) {
        return;
    }

    if (keyNum == 1) {
        SetKeyValue(pOut, pAnim->keys[0]);
        return;
    }

    if (pAnim->loop) {
        time = std::fmod(time, rLastTime);
    }

    if (time < pAnim->keys[0].time) {
        SetKeyValue(pOut, pAnim->keys[0]);
        return;
    }

    if (rLastTime <= time) {
        SetKeyValue(pOut, pAnim->keys[keyNum - 1]);
        *pIsEnd = true;
        return;
    }

    for (int i = 0; i < keyNum; i++) {
        f32 startTime = pAnim->keys[i].time;
        f32 endTime = pAnim->keys[i + 1].time;

        if (startTime <= time && time < endTime) {
            f32 duration = endTime - startTime;
            f32 t = (time - startTime) / duration;
            util::Vector3fType start;
            util::VectorLoad(&start, pAnim->keys[i].GetValue());
            util::Vector3fType end;
            util::VectorLoad(&end, pAnim->keys[i + 1].GetValue());
            util::Vector3fType value;
            value._v = vaddq_f32(start._v, vmulq_n_f32(vsubq_f32(end._v, start._v), t));
            vst1_f32(pOut->v, vget_low_f32(value._v));
            vst1q_lane_f32(&pOut->v[2], value._v, 2);
            return;
        }
    }
}

}  // namespace detail
}  // namespace vfx
}  // namespace nn
