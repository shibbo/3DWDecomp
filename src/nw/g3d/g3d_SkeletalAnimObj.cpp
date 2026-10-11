#include <nn/g3d/g3d_SkeletalAnimObj.h>
#include <nn/g3d/g3d_SkeletonObj.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <attributes.h>
#include <cmath>
#include <nn/util/util_Constants.h>
#include <nn/util/util_VectorApi.h>

namespace nn::util::detail {
struct SinCosSample {
    float cosValue;
    float sinValue;
    float cosDelta;
    float sinDelta;
};

extern const float SinCoefficients[5];
extern const float CosCoefficients[5];
extern const float AtanCoefficients[8];
extern const AngleIndex AngleIndexHalfRound;
extern const float FloatPiDivided2;
extern const float Float1Divided2Pi;
extern const float FloatPi;
extern const float Float2Pi;
extern const float FloatQuaternionEpsilon;
extern const SinCosSample SinCosSampleTable[256];
}  // namespace nn::util::detail

namespace nn::g3d {
namespace {
namespace math {
using util::detail::AtanCoefficients;
using util::detail::CosCoefficients;
using util::detail::Float1Divided2Pi;
using util::detail::Float2Pi;
using util::detail::FloatPi;
using util::detail::FloatPiDivided2;
using util::detail::FloatQuaternionEpsilon;
using util::detail::SinCoefficients;

/**
 * @brief Select two lanes of a vector with a byte table lookup.
 * @tparam a Lane placed in the first result lane.
 * @tparam b Lane placed in the second result lane.
 * @param v Source vector.
 * @return Selected lanes.
 */
template <int a, int b>
ALWAYS_INLINE inline float32x2_t Pick(float32x4_t v) {
    const uint8x8_t index = {a * 4, a * 4 + 1, a * 4 + 2, a * 4 + 3, b * 4, b * 4 + 1, b * 4 + 2, b * 4 + 3};
    return vreinterpret_f32_u8(vqtbl1_u8(vreinterpretq_u8_f32(v), index));
}

/**
 * @brief Select two lanes of a pair of vectors with a byte table lookup.
 * @tparam a Lane placed in the first result lane; lanes 4 to 7 select from the second vector.
 * @tparam b Lane placed in the second result lane.
 * @param v0 First source vector.
 * @param v1 Second source vector.
 * @return Selected lanes.
 */
template <int a, int b>
ALWAYS_INLINE inline float32x2_t Pick(float32x4_t v0, float32x4_t v1) {
    const uint8x8_t index = {a * 4, a * 4 + 1, a * 4 + 2, a * 4 + 3, b * 4, b * 4 + 1, b * 4 + 2, b * 4 + 3};
    uint8x16x2_t table = {{vreinterpretq_u8_f32(v0), vreinterpretq_u8_f32(v1)}};
    return vreinterpret_f32_u8(vqtbl2_u8(table, index));
}

/**
 * @brief Rearrange the lanes of a vector.
 * @param v Source vector.
 * @return Vector (v[a], v[b], v[c], v[d]).
 */
template <int a, int b, int c, int d>
ALWAYS_INLINE inline float32x4_t Swizzle(float32x4_t v) {
    return vcombine_f32(Pick<a, b>(v), Pick<c, d>(v));
}

/**
 * @brief Select lanes from a pair of vectors.
 * @param v0 First source vector, lanes 0 to 3.
 * @param v1 Second source vector, lanes 4 to 7.
 * @return Vector of the selected lanes.
 */
template <int a, int b, int c, int d>
ALWAYS_INLINE inline float32x4_t Permute(float32x4_t v0, float32x4_t v1) {
    return vcombine_f32(Pick<a, b>(v0, v1), Pick<c, d>(v0, v1));
}

/**
 * @brief Convert radians to a sample-table angle index.
 * @param radian Angle in radians.
 * @return Angle index.
 */
ALWAYS_INLINE inline util::AngleIndex RadianToAngleIndex(float radian) {
    return static_cast<int64_t>(radian * (util::detail::AngleIndexHalfRound / FloatPi));
}

/**
 * @brief Evaluate a sine with the interpolated sample table.
 * @param angleIndex Angle index.
 * @return Sine of the angle.
 */
ALWAYS_INLINE inline float SinTable(util::AngleIndex angleIndex) {
    u32 sampleTableIndex = (angleIndex >> 24) & 0xff;
    float rest = static_cast<float>(angleIndex & 0xffffff) / 0x1000000;
    const util::detail::SinCosSample* pSample = &util::detail::SinCosSampleTable[sampleTableIndex];
    return pSample->sinValue + pSample->sinDelta * rest;
}

/**
 * @brief Evaluate a cosine with the interpolated sample table.
 * @param angleIndex Angle index.
 * @return Cosine of the angle.
 */
ALWAYS_INLINE inline float CosTable(util::AngleIndex angleIndex) {
    u32 sampleTableIndex = (angleIndex >> 24) & 0xff;
    float rest = static_cast<float>(angleIndex & 0xffffff) / 0x1000000;
    const util::detail::SinCosSample* pSample = &util::detail::SinCosSampleTable[sampleTableIndex];
    return pSample->cosValue + pSample->cosDelta * rest;
}

/**
 * @brief Sum the lanes of a component-wise product.
 * @param a First vector.
 * @param b Second vector.
 * @return Dot product in both lanes.
 */
ALWAYS_INLINE inline float32x2_t Dot(float32x4_t a, float32x4_t b) {
    float32x4_t product = vmulq_f32(a, b);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    return vpadd_f32(sum, sum);
}

/**
 * @brief Calculate the cross product of the xyz components.
 * @param a First vector.
 * @param b Second vector.
 * @return a x b.
 */
ALWAYS_INLINE inline float32x4_t Cross(float32x4_t a, float32x4_t b) {
    float32x4_t result = vmulq_f32(Swizzle<1, 2, 0, 3>(a), Swizzle<2, 0, 1, 3>(b));
    return vfmsq_f32(result, Swizzle<2, 0, 1, 3>(a), Swizzle<1, 2, 0, 3>(b));
}

/**
 * @brief Multiply two quaternions.
 * @param a Left quaternion.
 * @param b Right quaternion.
 * @return Product a * b.
 */
ALWAYS_INLINE inline float32x4_t QuaternionMultiply(float32x4_t a, float32x4_t b) {
    const float32x4_t sign = {1.0f, 1.0f, 1.0f, -1.0f};
    float32x4_t result = vmulq_f32(b, Swizzle<3, 3, 3, 3>(a));
    result = vfmaq_f32(result, sign, vmulq_f32(Swizzle<0, 1, 2, 0>(a), Swizzle<3, 3, 3, 0>(b)));
    result = vfmaq_f32(result, sign, vmulq_f32(Swizzle<1, 2, 0, 1>(a), Swizzle<2, 0, 1, 1>(b)));
    return vfmsq_f32(result, Swizzle<2, 0, 1, 2>(a), Swizzle<1, 2, 0, 2>(b));
}

/**
 * @brief Invert a quaternion.
 * @param q Quaternion to invert.
 * @return Conjugate divided by the squared length, or the conjugate for a zero quaternion.
 */
ALWAYS_INLINE inline float32x4_t QuaternionInverse(float32x4_t q) {
    const float32x4_t sign = {-1.0f, -1.0f, -1.0f, 1.0f};
    float32x4_t conjugate = vmulq_f32(q, sign);
    float32x2_t lengthSq2 = Dot(q, q);
    float32x4_t lengthSq = vcombine_f32(lengthSq2, lengthSq2);
    float32x4_t inverse = vrecpeq_f32(lengthSq);
    inverse = vmulq_f32(inverse, vrecpsq_f32(inverse, lengthSq));
    inverse = vmulq_f32(inverse, vrecpsq_f32(inverse, lengthSq));
    uint32x4_t nonZero = vmvnq_u32(vceqzq_f32(lengthSq));
    return vbslq_f32(nonZero, vmulq_f32(conjugate, inverse), conjugate);
}

/**
 * @brief Normalize a quaternion with a refined reciprocal square root estimate.
 * @param q Quaternion to normalize.
 * @return Unit quaternion, or zero for a zero quaternion.
 */
ALWAYS_INLINE inline float32x4_t QuaternionNormalize(float32x4_t q) {
    float32x2_t lengthSq2 = Dot(q, q);
    float32x4_t lengthSq = vcombine_f32(lengthSq2, lengthSq2);
    float32x4_t inverse = vrsqrteq_f32(lengthSq);
    inverse = vmulq_f32(inverse, vrsqrtsq_f32(inverse, vmulq_f32(inverse, lengthSq)));
    inverse = vmulq_f32(inverse, vrsqrtsq_f32(inverse, vmulq_f32(lengthSq, inverse)));
    uint32x4_t nonZero = vmvnq_u32(vceqzq_f32(lengthSq));
    return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(vmulq_f32(inverse, q)), nonZero));
}

/**
 * @brief Normalize the xyz components of a vector with a refined reciprocal square root estimate.
 * @param value Vector to normalize.
 * @return Unit vector, or zero for a zero vector.
 */
ALWAYS_INLINE inline float32x4_t VectorNormalize(float32x4_t value) {
    float32x2_t lengthSq2 = Dot(value, value);
    float32x4_t lengthSq = vcombine_f32(lengthSq2, lengthSq2);
    float32x4_t inverse = vrsqrteq_f32(lengthSq);
    inverse = vmulq_f32(inverse, vrsqrtsq_f32(inverse, vmulq_f32(lengthSq, inverse)));
    inverse = vmulq_f32(inverse, vrsqrtsq_f32(inverse, vmulq_f32(lengthSq, inverse)));
    uint32x4_t nonZero = vmvnq_u32(vceqzq_f32(lengthSq));
    return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(vmulq_f32(value, inverse)), nonZero));
}

/**
 * @brief Reduce an angle to the range [-pi, pi].
 * @param radian Angle in radians.
 * @return Equivalent angle.
 */
ALWAYS_INLINE inline float ModTwoPi(float radian) {
    float quotient = Float1Divided2Pi * radian + (radian >= 0.0f ? 0.5f : -0.5f);
    return radian - Float2Pi * static_cast<int>(quotient);
}

/**
 * @brief Estimate the sine and cosine of an angle with polynomials.
 * @param pSin Receives the sine.
 * @param pCos Receives the cosine.
 * @param radian Angle in radians.
 */
ALWAYS_INLINE inline void SinCosEst(float* pSin, float* pCos, float radian) {
    float value = ModTwoPi(radian);
    float sign;

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
        sign = -1.0f;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
        sign = -1.0f;
    } else {
        sign = 1.0f;
    }

    float square = value * value;
    float sinPoly = SinCoefficients[1] - SinCoefficients[0] * square;
    sinPoly = sinPoly * square - SinCoefficients[2];
    sinPoly = sinPoly * square + SinCoefficients[3];
    sinPoly = sinPoly * square - SinCoefficients[4];
    *pSin = value * (sinPoly * square + 1.0f);
    float cosPoly = CosCoefficients[1] - CosCoefficients[0] * square;
    cosPoly = cosPoly * square - CosCoefficients[2];
    cosPoly = cosPoly * square + CosCoefficients[3];
    cosPoly = cosPoly * square - CosCoefficients[4];
    *pCos = sign * (cosPoly * square + 1.0f);
}

/**
 * @brief Estimate the sine of an angle with a polynomial.
 * @param radian Angle in radians.
 * @return Sine of the angle.
 */
ALWAYS_INLINE inline float SinEst(float radian) {
    float value = ModTwoPi(radian);

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
    }

    float square = value * value;
    float poly = SinCoefficients[1] - SinCoefficients[0] * square;
    poly = poly * square - SinCoefficients[2];
    poly = poly * square + SinCoefficients[3];
    poly = poly * square - SinCoefficients[4];
    return value * (poly * square + 1.0f);
}

/**
 * @brief Estimate the sines of two angles with polynomials.
 * @param radian Angles in radians.
 * @return Sines of the angles.
 */
ALWAYS_INLINE inline float32x2_t SinEst(float32x2_t radian) {
    float32x2_t quotient = vmul_n_f32(radian, Float1Divided2Pi);
    quotient = vadd_f32(quotient, vbsl_f32(vcgez_f32(quotient), vdup_n_f32(0.5f), vdup_n_f32(-0.5f)));
    float32x2_t value = vfms_n_f32(radian, vcvt_f32_s32(vcvt_s32_f32(quotient)), Float2Pi);
    value =
        vbsl_f32(vcgt_f32(value, vdup_n_f32(FloatPiDivided2)), vsub_f32(vdup_n_f32(FloatPi), value), value);
    value =
        vbsl_f32(vcgt_f32(vdup_n_f32(-FloatPiDivided2), value), vsub_f32(vdup_n_f32(-FloatPi), value), value);
    float32x2_t square = vmul_f32(value, value);
    float32x2_t poly = vfms_n_f32(vdup_n_f32(SinCoefficients[1]), square, SinCoefficients[0]);
    poly = vfma_f32(vdup_n_f32(-SinCoefficients[2]), square, poly);
    poly = vfma_f32(vdup_n_f32(SinCoefficients[3]), square, poly);
    poly = vfma_f32(vdup_n_f32(-SinCoefficients[4]), square, poly);
    poly = vfma_f32(vdup_n_f32(1.0f), square, poly);
    return vmul_f32(value, poly);
}

/**
 * @brief Estimate the sines and cosines of four angles with polynomials.
 * @param pSin Receives the sines.
 * @param pCos Receives the cosines.
 * @param radian Angles in radians.
 */
ALWAYS_INLINE inline void SinCosEst(float32x4_t* pSin, float32x4_t* pCos, float32x4_t radian) {
    float32x4_t quotient = vmulq_f32(radian, vdupq_n_f32(Float1Divided2Pi));
    quotient = vaddq_f32(quotient, vbslq_f32(vcgezq_f32(quotient), vdupq_n_f32(0.5f), vdupq_n_f32(-0.5f)));
    float32x4_t value = vfmsq_f32(radian, vcvtq_f32_s32(vcvtq_s32_f32(quotient)), vdupq_n_f32(Float2Pi));
    uint32x4_t greater = vcgtq_f32(value, vdupq_n_f32(FloatPiDivided2));
    value = vbslq_f32(greater, vsubq_f32(vdupq_n_f32(FloatPi), value), value);
    uint32x4_t less = vcgtq_f32(vdupq_n_f32(-FloatPiDivided2), value);
    value = vbslq_f32(less, vsubq_f32(vdupq_n_f32(-FloatPi), value), value);
    float32x4_t sign = vbslq_f32(vorrq_u32(less, greater), vdupq_n_f32(-1.0f), vdupq_n_f32(1.0f));
    float32x4_t square = vmulq_f32(value, value);
    float32x4_t sinPoly =
        vfmaq_f32(vdupq_n_f32(SinCoefficients[1]), vdupq_n_f32(-SinCoefficients[0]), square);
    sinPoly = vfmaq_f32(vdupq_n_f32(-SinCoefficients[2]), sinPoly, square);
    sinPoly = vfmaq_f32(vdupq_n_f32(SinCoefficients[3]), sinPoly, square);
    sinPoly = vfmaq_f32(vdupq_n_f32(-SinCoefficients[4]), sinPoly, square);
    sinPoly = vfmaq_f32(vdupq_n_f32(1.0f), sinPoly, square);
    float32x4_t cosPoly =
        vfmaq_f32(vdupq_n_f32(CosCoefficients[1]), vdupq_n_f32(-CosCoefficients[0]), square);
    cosPoly = vfmaq_f32(vdupq_n_f32(-CosCoefficients[2]), cosPoly, square);
    cosPoly = vfmaq_f32(vdupq_n_f32(CosCoefficients[3]), cosPoly, square);
    cosPoly = vfmaq_f32(vdupq_n_f32(-CosCoefficients[4]), cosPoly, square);
    cosPoly = vfmaq_f32(vdupq_n_f32(1.0f), cosPoly, square);
    *pSin = vmulq_f32(value, sinPoly);
    *pCos = vmulq_f32(sign, cosPoly);
}

/**
 * @brief Estimate the arctangent of a value in [-1, 1] with a polynomial.
 * @param x Tangent value.
 * @return Angle in radians.
 */
ALWAYS_INLINE inline float AtanEst(float x) {
    float square = x * x;
    float poly = AtanCoefficients[0] * square - AtanCoefficients[1];
    poly = poly * square + AtanCoefficients[2];
    poly = poly * square - AtanCoefficients[3];
    poly = poly * square + AtanCoefficients[4];
    poly = poly * square - AtanCoefficients[5];
    poly = poly * square + AtanCoefficients[6];
    poly = poly * square - AtanCoefficients[7];
    return x * (poly * square + 1.0f);
}

/**
 * @brief Estimate the arccosine of a value.
 * @param x Cosine value in [-1, 1].
 * @return Angle in radians.
 */
ALWAYS_INLINE inline float AcosEst(float x) {
    if (x >= 0.0f) {
        if (x > 0.70710677f) {
            return AtanEst(std::sqrt(1.0f - x * x) / x);
        }

        return FloatPiDivided2 - AtanEst(x / std::sqrt(1.0f - x * x));
    }

    if (x < -0.70710677f) {
        return FloatPi + AtanEst(-std::sqrt(1.0f - x * x) / x);
    }

    return FloatPiDivided2 + AtanEst(-x / std::sqrt(1.0f - x * x));
}

/**
 * @brief Spherically interpolate between two quaternions.
 * @param from Quaternion at t = 0.
 * @param to Quaternion at t = 1.
 * @param t Interpolation parameter.
 * @return Interpolated quaternion along the shorter arc.
 */
ALWAYS_INLINE inline float32x4_t QuaternionSlerp(float32x4_t from, float32x4_t to, float t) {
    float dot = vget_lane_f32(Dot(to, from), 0);
    float cosine = dot >= 0.0f ? dot : -dot;
    float sign = dot >= 0.0f ? 1.0f : -1.0f;
    float theta = AcosEst(cosine);
    float sinTheta = SinEst(theta);
    float32x2_t factors = {1.0f - t, t};
    float32x2_t sins = SinEst(vmul_n_f32(factors, theta));
    float threshold = 1.0f - FloatQuaternionEpsilon;
    float inverse = 1.0f / sinTheta;
    float fromScale = cosine > threshold ? 1.0f - t : inverse * vget_lane_f32(sins, 0);
    float toScale = cosine > threshold ? t * sign : inverse * (sign * vget_lane_f32(sins, 1));
    return vfmaq_n_f32(vmulq_n_f32(from, fromScale), to, toScale);
}

/**
 * @brief Build a vector from three components with a zero w component.
 * @param x First component.
 * @param y Second component.
 * @param z Third component.
 * @return Vector (x, y, z, 0).
 */
ALWAYS_INLINE inline float32x4_t MakeVector(float x, float y, float z) {
    float32x2_t low = {x, y};
    float32x2_t high = {z, 0.0f};
    return vcombine_f32(low, high);
}

/**
 * @brief Load a three-component value into a vector with a zero w component.
 * @param rValue Value to load.
 * @return Loaded vector.
 */
ALWAYS_INLINE inline float32x4_t LoadVector(const util::Float3& rValue) {
    util::Vector3fType vector;
    util::VectorLoad(&vector, rValue);
    return vector._v;
}

/**
 * @brief Store the xyz components of a vector.
 * @param pOut Receives the components.
 * @param value Vector to store.
 */
ALWAYS_INLINE inline void StoreVector(util::Float3* pOut, float32x4_t value) {
    vst1_f32(pOut->v, vget_low_f32(value));
    vst1q_lane_f32(&pOut->v[2], value, 2);
}

/**
 * @brief Convert a rotation matrix to a quaternion.
 * @param c0 First matrix column.
 * @param c1 Second matrix column.
 * @param c2 Third matrix column.
 * @return Normalized quaternion.
 */
ALWAYS_INLINE inline float32x4_t QuaternionFromMatrix(float32x4_t c0, float32x4_t c1, float32x4_t c2) {
    const float32x4_t signX = {1.0f, -1.0f, -1.0f, 1.0f};
    const float32x4_t signY = {-1.0f, 1.0f, -1.0f, 1.0f};
    const float32x4_t signZ = {-1.0f, -1.0f, 1.0f, 1.0f};
    float32x4_t trace = vmulq_laneq_f32(signX, c0, 0);
    trace = vfmaq_laneq_f32(trace, signY, c1, 1);
    trace = vfmaq_laneq_f32(trace, signZ, c2, 2);
    float32x4_t a = {vgetq_lane_f32(c0, 1), vgetq_lane_f32(c2, 0), vgetq_lane_f32(c1, 2), 0.0f};
    float32x4_t b = {vgetq_lane_f32(c1, 0), vgetq_lane_f32(c0, 2), vgetq_lane_f32(c2, 1), 0.0f};
    float32x4_t sum = vaddq_f32(a, b);
    float32x4_t diff = vsubq_f32(a, b);
    trace = vaddq_f32(trace, vdupq_n_f32(1.0f));
    float32x4_t qx = vsetq_lane_f32(vgetq_lane_f32(trace, 0), (Permute<0, 0, 1, 6>(sum, diff)), 0);
    float32x4_t qy = vsetq_lane_f32(vgetq_lane_f32(trace, 1), (Permute<0, 0, 2, 5>(sum, diff)), 1);
    float32x4_t qz = vsetq_lane_f32(vgetq_lane_f32(trace, 2), (Permute<1, 2, 0, 4>(sum, diff)), 2);
    float32x4_t qw = vsetq_lane_f32(vgetq_lane_f32(trace, 3), (Permute<6, 5, 4, 0>(sum, diff)), 3);
    float32x4_t traceX = vdupq_laneq_f32(trace, 0);
    float32x4_t traceY = vdupq_laneq_f32(trace, 1);
    uint32x4_t selectY = vcgeq_f32(traceY, traceX);
    float32x4_t maxTrace = vbslq_f32(selectY, traceY, traceX);
    float32x4_t result = vbslq_f32(selectY, qy, qx);
    float32x4_t traceZ = vdupq_laneq_f32(trace, 2);
    uint32x4_t selectZ = vcgeq_f32(traceZ, maxTrace);
    result = vbslq_f32(selectZ, qz, result);
    maxTrace = vbslq_f32(selectZ, traceZ, maxTrace);
    uint32x4_t selectW = vcgeq_f32(vdupq_laneq_f32(trace, 3), maxTrace);
    result = vbslq_f32(selectW, qw, result);
    return QuaternionNormalize(result);
}

/**
 * @brief Build the rotation columns of a matrix from a quaternion.
 * @param pColumns Receives three columns.
 * @param q Unit quaternion.
 */
ALWAYS_INLINE inline void MatrixFromQuaternion(float32x4_t* pColumns, float32x4_t q) {
    const uint32x4_t mask = {0xffffffff, 0xffffffff, 0xffffffff, 0};
    const float32x4_t one = {1.0f, 1.0f, 1.0f, 0.0f};
    float32x4_t xyz = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(q), mask));
    float32x4_t yxx = Swizzle<1, 0, 0, 3>(xyz);
    float32x4_t zzy = Swizzle<2, 2, 1, 3>(xyz);
    float32x4_t square0 = vmulq_f32(yxx, yxx);
    float32x4_t square1 = vmulq_f32(zzy, zzy);
    square0 = vaddq_f32(square0, square0);
    square1 = vaddq_f32(square1, square1);
    float32x4_t diagonal = vsubq_f32(one, vaddq_f32(square1, square0));
    float32x4_t product = vmulq_f32(Swizzle<1, 2, 0, 3>(xyz), xyz);
    product = vaddq_f32(product, product);
    float32x4_t productW = vmulq_laneq_f32((Swizzle<2, 0, 1, 3>(xyz)), q, 3);
    productW = vaddq_f32(productW, productW);
    float32x4_t sum = vaddq_f32(product, productW);
    float32x4_t diff = vsubq_f32(product, productW);
    float32x4_t restA = Permute<2, 5, 7, 7>(sum, diff);
    float32x4_t restB = Permute<0, 6, 4, 1>(sum, diff);
    pColumns[0] = Permute<0, 4, 5, 3>(diagonal, restB);
    pColumns[1] = Permute<6, 1, 7, 3>(diagonal, restB);
    pColumns[2] = Permute<4, 5, 2, 3>(diagonal, restA);
}

/**
 * @brief Build the rotation columns of a matrix from XYZ Euler angles.
 * @param pColumns Receives three columns.
 * @param euler Euler angles in radians.
 */
ALWAYS_INLINE inline void MatrixFromEuler(float32x4_t* pColumns, float32x4_t euler) {
    const float32x2_t positiveX = {1.0f, 0.0f};
    const float32x2_t negativeX = {-1.0f, 0.0f};
    const float32x4_t signA = {-1.0f, 1.0f, 0.0f, 0.0f};
    const float32x4_t signB = {1.0f, -1.0f, 0.0f, 0.0f};
    float32x4_t sin;
    float32x4_t cos;
    SinCosEst(&sin, &cos, euler);
    float sinY = vgetq_lane_f32(sin, 1);
    float cosY = vgetq_lane_f32(cos, 1);
    float32x2_t sinYSinY = {sinY, sinY};
    float32x2_t cosYZero = {cosY, 0.0f};
    float32x2_t sinYZero = {sinY, 0.0f};
    float32x2_t cosYCosY = {cosY, cosY};
    float32x2_t cosZSinZ = vzip1_f32(vget_high_f32(cos), vget_high_f32(sin));
    float32x4_t termA = vcombine_f32(vmul_f32(cosZSinZ, sinYSinY), vmul_f32(cosYZero, positiveX));
    float32x4_t termB = vcombine_f32(vzip1_f32(vget_high_f32(sin), vget_high_f32(cos)), vdup_n_f32(0.0f));
    pColumns[0] = vcombine_f32(vmul_f32(cosZSinZ, cosYCosY), vmul_f32(sinYZero, negativeX));
    pColumns[1] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(termB, cos, 0), signA), vmulq_laneq_f32(termA, sin, 0));
    pColumns[2] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(termB, sin, 0), signB), vmulq_laneq_f32(termA, cos, 0));
}

/**
 * @brief Build orthonormal rotation columns from two accumulated axes.
 * @param pColumns Receives three columns; unchanged when the axes are degenerate.
 * @param axisX Accumulated first matrix row.
 * @param pAxisY Accumulated second matrix row.
 * @return True if the axes were valid.
 */
ALWAYS_INLINE inline bool MatrixFromAxes(float32x4_t* pColumns, float32x4_t axisX,
                                         const util::Vector3fType* pAxisY) {
    float32x4_t axisY = pAxisY->_v;
    float lengthSqY = vget_lane_f32(Dot(axisY, axisY), 0);

    if (lengthSqY == 0.0f) {
        return false;
    }

    float32x4_t axisZ = Cross(axisX, axisY);
    float lengthSqZ = vget_lane_f32(Dot(axisZ, axisZ), 0);

    if (lengthSqZ == 0.0f) {
        return false;
    }

    float inverseY = 1.0f / std::sqrt(lengthSqY);
    float inverseZ = 1.0f / std::sqrt(lengthSqZ);
    axisY = vmulq_n_f32(pAxisY->_v, inverseY);
    axisZ = vmulq_n_f32(axisZ, inverseZ);
    axisX = Cross(axisY, axisZ);
    pColumns[0] = MakeVector(vgetq_lane_f32(axisX, 0), vgetq_lane_f32(axisY, 0), vgetq_lane_f32(axisZ, 0));
    pColumns[1] = MakeVector(vgetq_lane_f32(axisX, 1), vgetq_lane_f32(axisY, 1), vgetq_lane_f32(axisZ, 1));
    pColumns[2] = MakeVector(vgetq_lane_f32(axisX, 2), vgetq_lane_f32(axisY, 2), vgetq_lane_f32(axisZ, 2));
    return true;
}

/**
 * @brief Calculate the first two rows of a rotation matrix from a quaternion.
 * @param pAxisX Receives the first row.
 * @param pAxisY Receives the second row.
 * @param x Quaternion x component.
 * @param y Quaternion y component.
 * @param z Quaternion z component.
 * @param w Quaternion w component.
 */
ALWAYS_INLINE inline void AxesFromQuaternion(util::Vector3fType* pAxisX, util::Vector3fType* pAxisY, float x,
                                             float y, float z, float w) {
    float y2 = y + y;
    float w2 = w + w;
    float z2 = z + z;
    float x2 = x + x;
    float yy2 = y * y2;
    float zw2 = z * w2;
    float zz2 = z * z2;
    float yx2 = y * x2;
    float yw2 = y * w2;
    pAxisX->_v = MakeVector(1.0f - yy2 - zz2, yx2 - zw2, z * x2 + yw2);
    pAxisY->_v = MakeVector(yx2 + zw2, 1.0f - x * x2 - zz2, y2 * z - x * w2);
}
}  // namespace math
}  // namespace

namespace detail {
/**
 * @brief Rotate a bone translation by its retargeting quaternion.
 * @param pTranslation Writable three-component translation transformed in place.
 * @param rRotation Retargeting quaternion; its squared length scales the translation to the target bone.
 */
inline void RotateRetargetedTranslation(util::Float3* pTranslation, const util::Vector4fType& rRotation) {
    float rx = pTranslation->z * rRotation._v[1] + pTranslation->x * rRotation._v[3] -
               pTranslation->y * rRotation._v[2];
    float ry = pTranslation->x * rRotation._v[2] + pTranslation->y * rRotation._v[3] -
               pTranslation->z * rRotation._v[0];
    float rz = pTranslation->y * rRotation._v[0];
    rz = pTranslation->z * rRotation._v[3] + rz;
    rz = rz - pTranslation->x * rRotation._v[1];
    float rw = pTranslation->z * rRotation._v[2] +
               (pTranslation->x * rRotation._v[0] + pTranslation->y * rRotation._v[1]);
    float outX = rz * rRotation._v[1] - ry * rRotation._v[2];
    outX = rw * rRotation._v[0] + outX;
    pTranslation->x = rx * rRotation._v[3] + outX;
    float outY = rx * rRotation._v[2] - rz * rRotation._v[0];
    outY = rw * rRotation._v[1] + outY;
    pTranslation->y = ry * rRotation._v[3] + outY;
    float outZ = ry * rRotation._v[0] - rx * rRotation._v[1];
    outZ = rw * rRotation._v[2] + outZ;
    pTranslation->z = rz * rRotation._v[3] + outZ;
}

struct SkeletalAnimObjUtil {
    static bool CalculateRetargetingQuaternion(util::Vector4fType* pResult, const ResBone* pTarget,
                                               const ResBone* pSource);
    enum TargetValue { TargetValue_Rotate = 1, TargetValue_Translate = 2, TargetValue_RotateTranslate = 3 };

    template <TargetValue target>
    static void CalculateMirroring(BoneAnimResult* pResult, u32 mirroringState, const ResSkeleton* pSkeleton);

    /**
     * @brief Restore selected constant channels before mirroring or retargeting.
     * @tparam target Channels to restore: rotation, translation, or both.
     * @param pResult Writable result receiving the selected constant channels.
     * @param pAnim Animation containing the channel values and their storage indices.
     * @param pBone Bone whose mirroring state determines whether rotation is reset; unused for translation
     * alone.
     */
    template <TargetValue target>
    static void ClearAnimResultValue(BoneAnimResult* pResult, const ResBoneAnim* pAnim,
                                     const ResBone* pBone) {
        if constexpr ((target & TargetValue_Rotate) != 0) {
            if (pBone->GetMirroringState() != MirroringState_Unmirrored) {
                if ((pAnim->flags & 8) != 0) {
                    std::memcpy(&pResult->rotate, pAnim->GetBaseValue<util::Float3>(3), sizeof(util::Float3));
                } else {
                    std::memcpy(&pResult->rotate, pAnim->GetBaseValue<util::Float3>(0), sizeof(util::Float3));
                }
            }
        }

        if constexpr ((target & TargetValue_Translate) != 0) {
            std::memcpy(&pResult->translate, &pAnim->GetBaseTranslation(), sizeof(util::Float3));
        }
    }

    /** Mirroring states stored in ResBone flags, as multiples of 1 << ResBone::Shift_MirroringState. */
    enum MirroringState : u32 {
        MirroringState_State0 = 0 << ResBone::Shift_MirroringState,
        MirroringState_State1 = 1 << ResBone::Shift_MirroringState,
        MirroringState_State2 = 2 << ResBone::Shift_MirroringState,
        MirroringState_State3 = 3 << ResBone::Shift_MirroringState,
        MirroringState_Unmirrored = 4 << ResBone::Shift_MirroringState,
    };
};

namespace {
using Util = SkeletalAnimObjUtil;

/**
 * @brief Mirror the selected channels of a result across the X axis.
 * @tparam target Channels to mirror.
 * @param pResult Result whose Euler rotation and translation are mirrored in place.
 * @param state Mirroring state of the destination bone.
 */
template <Util::TargetValue target>
void CalculateMirroringX(BoneAnimResult* pResult, u32 state) {
    switch (state) {
    case Util::MirroringState_State0:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.y = -pResult->rotate.y;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_State1:
        break;
    case Util::MirroringState_State2:
        break;
    case Util::MirroringState_State3:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.x = util::FloatPi + pResult->rotate.x;
            pResult->rotate.y = -pResult->rotate.y;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_Unmirrored:
        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
            pResult->translate.y = -pResult->translate.y;
            pResult->translate.z = -pResult->translate.z;
        }

        break;
    default:
        break;
    }
}

/**
 * @brief Mirror the selected channels of a result for skeletons using XY mirroring.
 * @tparam target Channels to mirror.
 * @param pResult Result whose Euler rotation and translation are mirrored in place.
 * @param state Mirroring state of the destination bone.
 */
template <Util::TargetValue target>
void CalculateMirroringXY(BoneAnimResult* pResult, u32 state) {
    switch (state) {
    case Util::MirroringState_State0:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.y = -pResult->rotate.y;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_State1:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.x = -pResult->rotate.x;
            pResult->rotate.z = util::FloatPi - pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_State2:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.x = -pResult->rotate.x;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.y = -pResult->translate.y;
        }

        break;
    case Util::MirroringState_State3:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.y = util::FloatPi + pResult->rotate.y;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.y = -pResult->translate.y;
        }

        break;
    case Util::MirroringState_Unmirrored:
        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
            pResult->translate.y = -pResult->translate.y;
            pResult->translate.z = -pResult->translate.z;
        }

        break;
    default:
        break;
    }
}

/**
 * @brief Mirror the selected channels of a result for skeletons using XZ mirroring.
 * @tparam target Channels to mirror.
 * @param pResult Result whose Euler rotation and translation are mirrored in place.
 * @param state Mirroring state of the destination bone.
 */
template <Util::TargetValue target>
void CalculateMirroringXZ(BoneAnimResult* pResult, u32 state) {
    switch (state) {
    case Util::MirroringState_State0:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.y = -pResult->rotate.y;
            pResult->rotate.z = -pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_State1:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.x = util::FloatPi - pResult->rotate.x;
            pResult->rotate.z = util::FloatPi - pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
        }

        break;
    case Util::MirroringState_State2:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.x = -pResult->rotate.x;
            pResult->rotate.y = -pResult->rotate.y;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.z = -pResult->translate.z;
        }

        break;
    case Util::MirroringState_State3:
        if constexpr ((target & Util::TargetValue_Rotate) != 0) {
            pResult->rotate.z = util::FloatPi + pResult->rotate.z;
        }

        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.z = -pResult->translate.z;
        }

        break;
    case Util::MirroringState_Unmirrored:
        if constexpr ((target & Util::TargetValue_Translate) != 0) {
            pResult->translate.x = -pResult->translate.x;
            pResult->translate.y = -pResult->translate.y;
            pResult->translate.z = -pResult->translate.z;
        }

        break;
    default:
        break;
    }
}

using MirroringFunction = void (*)(BoneAnimResult* pResult, u32 mirroringState);

/** Mirroring functions indexed by skeleton mirroring mode and then by target channels minus one. */
const MirroringFunction s_pFuncCalculateMirroring[3][3] = {
    {CalculateMirroringX<Util::TargetValue_Rotate>, CalculateMirroringX<Util::TargetValue_Translate>,
     CalculateMirroringX<Util::TargetValue_RotateTranslate>},
    {CalculateMirroringXY<Util::TargetValue_Rotate>, CalculateMirroringXY<Util::TargetValue_Translate>,
     CalculateMirroringXY<Util::TargetValue_RotateTranslate>},
    {CalculateMirroringXZ<Util::TargetValue_Rotate>, CalculateMirroringXZ<Util::TargetValue_Translate>,
     CalculateMirroringXZ<Util::TargetValue_RotateTranslate>},
};
}  // namespace

/**
 * @brief Mirror the selected channels using the skeleton's mirroring mode.
 * @tparam target Channels to mirror.
 * @param pResult Result mirrored in place.
 * @param mirroringState Mirroring state of the destination bone.
 * @param pSkeleton Skeleton whose mirroring mode selects the mirroring axes.
 */
template <SkeletalAnimObjUtil::TargetValue target>
void SkeletalAnimObjUtil::CalculateMirroring(BoneAnimResult* pResult, u32 mirroringState,
                                             const ResSkeleton* pSkeleton) {
    int mode = pSkeleton->GetMirroringMode() >> ResSkeleton::Shift_MirroringMode;
    s_pFuncCalculateMirroring[mode][target - 1](pResult, mirroringState);
}

template void
SkeletalAnimObjUtil::CalculateMirroring<SkeletalAnimObjUtil::TargetValue_Rotate>(BoneAnimResult*, u32,
                                                                                 const ResSkeleton*);
template void
SkeletalAnimObjUtil::CalculateMirroring<SkeletalAnimObjUtil::TargetValue_Translate>(BoneAnimResult*, u32,
                                                                                    const ResSkeleton*);
template void SkeletalAnimObjUtil::CalculateMirroring<SkeletalAnimObjUtil::TargetValue_RotateTranslate>(
    BoneAnimResult*, u32, const ResSkeleton*);
template void SkeletalAnimObjUtil::ClearAnimResultValue<SkeletalAnimObjUtil::TargetValue_Rotate>(
    BoneAnimResult*, const ResBoneAnim*, const ResBone*);
template void SkeletalAnimObjUtil::ClearAnimResultValue<SkeletalAnimObjUtil::TargetValue_Translate>(
    BoneAnimResult*, const ResBoneAnim*, const ResBone*);
template void SkeletalAnimObjUtil::ClearAnimResultValue<SkeletalAnimObjUtil::TargetValue_RotateTranslate>(
    BoneAnimResult*, const ResBoneAnim*, const ResBone*);
/**
 * @brief Calculate the scaled rotation that maps a source bone's translation direction onto a target bone's.
 * @param pResult Receives a quaternion whose squared length is the source-to-target length ratio.
 * @param pTarget Bone of the bound skeleton.
 * @param pSource Matching bone of the animation's original skeleton.
 * @return False if either bone has a zero translation.
 */
bool SkeletalAnimObjUtil::CalculateRetargetingQuaternion(util::Vector4fType* pResult, const ResBone* pTarget,
                                                         const ResBone* pSource) {
    float32x4_t target = math::LoadVector(pTarget->GetTranslate());
    float lengthSqTarget = vget_lane_f32(math::Dot(target, target), 0);

    if (lengthSqTarget == 0.0f) {
        return false;
    }

    float lengthTarget = std::sqrt(lengthSqTarget);
    float32x4_t source = math::LoadVector(pSource->GetTranslate());
    float lengthSqSource = vget_lane_f32(math::Dot(source, source), 0);

    if (lengthSqSource == 0.0f) {
        return false;
    }

    float inverseTarget = 1.0f / lengthTarget;
    float32x4_t targetDir = vmulq_n_f32(target, inverseTarget);
    float lengthSource = lengthSqSource / std::sqrt(lengthSqSource);
    float scale = std::sqrt(inverseTarget * lengthSource);
    float32x4_t sourceDir = math::VectorNormalize(source);
    float cosine2 = vget_lane_f32(math::Dot(targetDir, sourceDir), 0) + 1.0f;
    cosine2 = cosine2 + cosine2;

    if (cosine2 < 1.1920929e-07f) {
        util::AngleIndex angle = math::RadianToAngleIndex(util::FloatPi * 0.5f);
        float x = vgetq_lane_f32(targetDir, 0);
        float y = vgetq_lane_f32(targetDir, 1);
        float z = vgetq_lane_f32(targetDir, 2);
        float32x4_t axis = x > y ? math::MakeVector(-z, 0.0f, x) : math::MakeVector(0.0f, 0.0f, -y);
        float sinScale = scale * math::SinTable(angle) / vget_lane_f32(math::Dot(axis, axis), 0);
        pResult->_v = (float32x4_t){sinScale * vgetq_lane_f32(axis, 0), sinScale * vgetq_lane_f32(axis, 1),
                                    vgetq_lane_f32(axis, 2) * sinScale, scale * math::CosTable(angle)};
    } else {
        float inverse = 1.0f / std::sqrt(cosine2);
        float32x4_t axis = math::Cross(targetDir, sourceDir);
        float axisScale = scale * inverse;
        pResult->_v = (float32x4_t){axisScale * vgetq_lane_f32(axis, 0), axisScale * vgetq_lane_f32(axis, 1),
                                    axisScale * vgetq_lane_f32(axis, 2), scale * 0.5f * cosine2 * inverse};
    }

    return true;
}
}  // namespace detail

/** @brief Rotation converter from quaternions to accumulated matrix rows. */
struct QuatToAxes {
    /**
     * @brief Accumulate a quaternion rotation as weighted matrix rows.
     * @param pBlend Accumulator receiving the rotation.
     * @param pResult Result storing a quaternion.
     * @param weight Blend weight.
     */
    ALWAYS_INLINE static void Blend(BoneAnimBlendResult* pBlend, const BoneAnimResult* pResult,
                                    float weight) {
        util::Vector3fType axisX;
        util::Vector3fType axisY;
        math::AxesFromQuaternion(&axisX, &axisY, pResult->rotate.x, pResult->rotate.y, pResult->rotate.z,
                                 pResult->rotate.w);
        pBlend->axisX._v = vaddq_f32(pBlend->axisX._v, vmulq_n_f32(axisX._v, weight));
        pBlend->axisY._v = vaddq_f32(pBlend->axisY._v, vmulq_n_f32(axisY._v, weight));
    }

    /**
     * @brief Replace an accumulated quaternion with matrix rows.
     * @param pBlend Accumulator converted in place.
     */
    static void ConvertResult(BoneAnimBlendResult* pBlend) {
        float32x4_t q = pBlend->axisX._v;
        math::AxesFromQuaternion(&pBlend->axisX, &pBlend->axisY, vgetq_lane_f32(q, 0), vgetq_lane_f32(q, 1),
                                 vgetq_lane_f32(q, 2), vgetq_lane_f32(q, 3));
    }
};

/** @brief Rotation converter from Euler angles to accumulated matrix rows. */
struct EulerToAxes {
    /**
     * @brief Accumulate an Euler rotation as weighted matrix rows.
     * @param pBlend Accumulator receiving the rotation.
     * @param pResult Result storing XYZ Euler angles.
     * @param weight Blend weight.
     */
    ALWAYS_INLINE static void Blend(BoneAnimBlendResult* pBlend, const BoneAnimResult* pResult,
                                    float weight) {
        float sx = math::SinTable(math::RadianToAngleIndex(pResult->rotate.x));
        float cx = math::CosTable(math::RadianToAngleIndex(pResult->rotate.x));
        float sy = math::SinTable(math::RadianToAngleIndex(pResult->rotate.y));
        float cy = math::CosTable(math::RadianToAngleIndex(pResult->rotate.y));
        float sz = math::SinTable(math::RadianToAngleIndex(pResult->rotate.z));
        float cz = math::CosTable(math::RadianToAngleIndex(pResult->rotate.z));
        float sxsy = sx * sy;
        float cxcz = cx * cz;
        util::Vector3fType axisX;
        util::Vector3fType axisY;
        axisX._v = math::MakeVector(cy * cz, sxsy * cz - sz * cx, sx * sz + sy * cxcz);
        axisY._v = math::MakeVector(sz * cy, sz * sxsy + cxcz, sy * (sz * cx) - sx * cz);
        pBlend->axisX._v = vaddq_f32(pBlend->axisX._v, vmulq_n_f32(axisX._v, weight));
        pBlend->axisY._v = vaddq_f32(pBlend->axisY._v, vmulq_n_f32(axisY._v, weight));
    }
};

/**
 * @brief Accumulate a quaternion by interpolating toward it by its share of the total weight.
 * @param pBlend Accumulator storing a quaternion.
 * @param q Quaternion to accumulate.
 * @param weight Blend weight.
 */
ALWAYS_INLINE inline void BlendQuaternion(BoneAnimBlendResult* pBlend, float32x4_t q, float weight) {
    if (pBlend->weight == 0.0f) {
        pBlend->axisX._v = q;
    } else {
        float totalWeight = pBlend->weight + weight;
        pBlend->axisX._v = math::QuaternionSlerp(pBlend->axisX._v, q, weight / totalWeight);
    }
}

/** @brief Rotation converter that reads quaternions unchanged. */
struct QuatToQuat {
    /**
     * @brief Accumulate a quaternion rotation.
     * @param pBlend Accumulator storing a quaternion.
     * @param pResult Result storing a quaternion.
     * @param weight Blend weight.
     */
    ALWAYS_INLINE static void Blend(BoneAnimBlendResult* pBlend, const BoneAnimResult* pResult,
                                    float weight) {
        BlendQuaternion(pBlend, vld1q_f32(pResult->rotate.v), weight);
    }

    /**
     * @brief Read the accumulated quaternion of a blend result.
     * @param pOut Quaternion receiving the rotation.
     * @param pBlend Blend result storing a quaternion.
     */
    static void Convert(util::Vector4fType* pOut, const BoneAnimBlendResult* pBlend) {
        pOut->_v = pBlend->axisX._v;
    }

    /**
     * @brief Read the quaternion of an animation result.
     * @param pOut Quaternion receiving the rotation.
     * @param pResult Result storing a quaternion.
     */
    static void Convert(util::Vector4fType* pOut, const BoneAnimResult* pResult) {
        pOut->_v = vld1q_f32(pResult->rotate.v);
    }
};

/** @brief Rotation converter from XYZ Euler angles to quaternions. */
struct EulerToQuat {
    /**
     * @brief Convert XYZ Euler angles to a quaternion.
     * @param pOut Receives the quaternion.
     * @param pResult Result storing XYZ Euler angles.
     * @return Always true.
     */
    NOINLINE static bool Convert(util::Vector4fType* pOut, const BoneAnimResult* pResult) {
        float sx;
        float cx;
        float sy;
        float cy;
        float sz;
        float cz;
        math::SinCosEst(&sx, &cx, pResult->rotate.x * 0.5f);
        math::SinCosEst(&sy, &cy, pResult->rotate.y * 0.5f);
        math::SinCosEst(&sz, &cz, pResult->rotate.z * 0.5f);
        float sxcy = sx * cy;
        float cxsy = cx * sy;
        float sxsy = sx * sy;
        float cxcy = cx * cy;
        pOut->_v = (float32x4_t){sxcy * cz - cxsy * sz, cxsy * cz + sxcy * sz, cxcy * sz - sxsy * cz,
                                 cxcy * cz + sxsy * sz};
        return true;
    }

    /**
     * @brief Accumulate an Euler rotation as a quaternion.
     * @param pBlend Accumulator storing a quaternion.
     * @param pResult Result storing XYZ Euler angles.
     * @param weight Blend weight.
     */
    ALWAYS_INLINE static void Blend(BoneAnimBlendResult* pBlend, const BoneAnimResult* pResult,
                                    float weight) {
        float32x4_t columns[3];
        math::MatrixFromEuler(columns,
                              math::LoadVector(*reinterpret_cast<const util::Float3*>(pResult->rotate.v)));
        BlendQuaternion(pBlend, math::QuaternionFromMatrix(columns[0], columns[1], columns[2]), weight);
    }
};

/** @brief Rotation converter from accumulated matrix rows to quaternions. */
struct AxesToQuat {
    /**
     * @brief Convert accumulated matrix rows to a quaternion.
     * @param pOut Receives the quaternion.
     * @param pBlend Accumulator storing the first two matrix rows.
     * @return True if the rows were valid.
     */
    NOINLINE static bool Convert(util::Vector4fType* pOut, const BoneAnimBlendResult* pBlend) {
        util::Matrix4x3fType mtx;
        bool isValid = math::MatrixFromAxes(mtx._m.val, pBlend->axisX._v, &pBlend->axisY);
        pOut->_v = math::QuaternionFromMatrix(mtx._m.val[0], mtx._m.val[1], mtx._m.val[2]);
        return isValid;
    }

    /**
     * @brief Replace accumulated matrix rows with a quaternion.
     * @param pBlend Accumulator converted in place.
     */
    static void ConvertResult(BoneAnimBlendResult* pBlend) {
        Convert(reinterpret_cast<util::Vector4fType*>(&pBlend->axisX), pBlend);
    }
};

struct SkeletalAnimObj::Impl {
    using ClearFunction = void (SkeletalAnimObj::*)(const ResSkeleton*);
    static const ClearFunction s_pFuncClearImpl[4];
    using CalculateFunction = void (SkeletalAnimObj::*)();
    static const CalculateFunction s_pFuncCalculateImpl[4];
    using ApplyFunction = void (SkeletalAnimObj::*)(SkeletonObj*) const;
    static const ApplyFunction s_pFuncApplyToImpl[2];
};
/**
 * @brief Restore ordinary animation results from their bound bones without mirroring or retargeting.
 * @param pSkeleton Skeleton supplying defaults for bound bone animations; must not be null.
 */
template <>
void SkeletalAnimObj::ClearImpl<false, false>(const ResSkeleton* pSkeleton) {
    int count = mBindTable.mAnimCount;
    auto* pResults = static_cast<BoneAnimResult*>(mResult);

    for (int i = 0; i < count; ++i) {
        unsigned target = mBindTable.mEntries[i] & 0x7fff;

        if (target != 0x7fff) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            const ResBone* pBone = pSkeleton->GetBone(target);
            pAnim->Initialize(&pResults[i], pBone);
        }
    }
}
/**
 * @brief Restore bound bone defaults and retarget eligible constant translations.
 * @param pSkeleton Non-null bound skeleton supplying default bone transforms.
 */
template <>
void SkeletalAnimObj::ClearImpl<false, true>(const ResSkeleton* pSkeleton) {
    int count = mBindTable.mAnimCount;
    auto* pResult = static_cast<BoneAnimResult*>(mResult);

    for (int i = 0; i < count; ++i, ++pResult) {
        unsigned target = mBindTable.mEntries[i] & 0x7fff;

        if (target != 0x7fff) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            const ResBone* pBone = pSkeleton->GetBone(target);
            pAnim->Initialize(pResult, pBone);

            if ((pAnim->flags & 0xe020) == 0x20) {
                detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[i]);
            }
        }
    }
}
/**
 * @brief Mirror the constant rotation and translation channels of a mirrored bone's result.
 * @param pResult Result of the mirrored bone, already holding the animation's constant values.
 * @param pAnim Animation whose flags tell which channels are constant.
 * @param pBone Mirrored bone supplying the mirroring state.
 */
inline void SkeletalAnimObj::MirrorConstantResult(BoneAnimResult* pResult, const ResBoneAnim* pAnim,
                                                  const ResBone* pBone) const {
    using Util = detail::SkeletalAnimObjUtil;

    u32 flags = pAnim->flags;

    if ((flags & 0x1e10) == 0x10) {
        if ((flags & 0xe020) == 0x20) {
            Util::CalculateMirroring<Util::TargetValue_RotateTranslate>(pResult, pBone->GetMirroringState(),
                                                                        m_pBoundSkeleton);
        } else {
            Util::CalculateMirroring<Util::TargetValue_Rotate>(pResult, pBone->GetMirroringState(),
                                                               m_pBoundSkeleton);
        }
    } else if ((flags & 0xe020) == 0x20) {
        Util::CalculateMirroring<Util::TargetValue_Translate>(pResult, pBone->GetMirroringState(),
                                                              m_pBoundSkeleton);
    }
}

/**
 * @brief Restore bound bone defaults, writing mirrored bones into their counterparts' results.
 * @param pSkeleton Non-null bound skeleton supplying default bone transforms.
 */
template <>
void SkeletalAnimObj::ClearImpl<true, false>(const ResSkeleton* pSkeleton) {
    int count = mBindTable.mAnimCount;
    auto* pResults = static_cast<BoneAnimResult*>(mResult);
    auto* pResult = pResults;

    for (int i = 0; i < count; ++i, ++pResult) {
        unsigned target = mBindTable.mEntries[i] & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        const ResBoneAnim* pAnim = &m_pBoneAnims[i];
        const ResBone* pBones = pSkeleton->ToData().pBoneArray.Get();
        ptrdiff_t mirrorIndex = GetMirroringBoneIndex(m_pBoundSkeleton, target);

        if (mirrorIndex < 0) {
            pAnim->Initialize(pResult, &pBones[target]);
            continue;
        }

        BoneAnimResult* pMirrorResult = &pResults[GetAnimIndex(mirrorIndex)];
        const ResBone* pBone = &pBones[mirrorIndex];
        pAnim->Initialize(pMirrorResult, pBone);
        MirrorConstantResult(pMirrorResult, pAnim, pBone);
    }
}

/**
 * @brief Restore bound bone defaults with mirroring and retargeting of constant channels.
 * @param pSkeleton Non-null bound skeleton supplying default bone transforms.
 */
template <>
void SkeletalAnimObj::ClearImpl<true, true>(const ResSkeleton* pSkeleton) {
    int count = mBindTable.mAnimCount;
    auto* pResults = static_cast<BoneAnimResult*>(mResult);

    for (int i = 0; i < count; ++i) {
        unsigned target = mBindTable.mEntries[i] & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        const ResBoneAnim* pAnim = &m_pBoneAnims[i];
        const ResBone* pBones = pSkeleton->ToData().pBoneArray.Get();
        ptrdiff_t mirrorIndex = GetMirroringBoneIndex(m_pBoundSkeleton, target);
        size_t animIndex = i;
        size_t boneIndex = target;

        if (mirrorIndex >= 0) {
            animIndex = GetAnimIndex(mirrorIndex);
            boneIndex = mirrorIndex;
        }

        BoneAnimResult* pResult = &pResults[animIndex];
        pAnim->Initialize(pResult, &pBones[boneIndex]);

        if ((pAnim->flags & 0xe020) == 0x20) {
            detail::RotateRetargetedTranslation(&pResults[animIndex].translate, m_pRetargeting[i]);
        }

        if (mirrorIndex >= 0) {
            MirrorConstantResult(pResult, pAnim, &pBones[boneIndex]);
        }
    }
}

const SkeletalAnimObj::Impl::ClearFunction SkeletalAnimObj::Impl::s_pFuncClearImpl[4] = {
    &SkeletalAnimObj::ClearImpl<false, false>, &SkeletalAnimObj::ClearImpl<false, true>,
    &SkeletalAnimObj::ClearImpl<true, false>, &SkeletalAnimObj::ClearImpl<true, true>};

/** @brief Evaluate ordinary bone channels without mirroring or retargeting. */
template <>
void SkeletalAnimObj::CalculateImpl<false, false>() {
    float frame = GetFrameCtrl().GetFrame();
    auto* pResults = static_cast<BoneAnimResult*>(mResult);

    if (mContext.IsCacheValid()) {
        int count = mBindTable.mAnimCount;
        unsigned cacheIndex = 0;

        for (int i = 0; i < count; ++i, ++pResults) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            unsigned nextCacheIndex = cacheIndex + pAnim->curveCount;

            if ((mBindTable.mEntries[i] & 0x40000000) == 0) {
                pAnim->Evaluate(pResults, frame, &mContext.mCache[cacheIndex]);
            }

            cacheIndex = nextCacheIndex;
        }
    } else {
        int count = mBindTable.mAnimCount;

        for (int i = 0; i < count; ++i) {
            if ((mBindTable.mEntries[i] & 0x40000000) == 0) {
                m_pBoneAnims[i].Evaluate(&pResults[i], frame);
            }
        }
    }
}
/** @brief Evaluate bone channels and retarget animated translations. */
template <>
void SkeletalAnimObj::CalculateImpl<false, true>() {
    using Util = detail::SkeletalAnimObjUtil;

    float frame = GetFrameCtrl().GetFrame();
    auto* pResult = static_cast<BoneAnimResult*>(mResult);

    if (mContext.IsCacheValid()) {
        int count = mBindTable.mAnimCount;
        unsigned cacheIndex = 0;

        for (int i = 0; i < count; ++i, ++pResult) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            unsigned nextCacheIndex = cacheIndex + pAnim->curveCount;

            if ((mBindTable.mEntries[i] & 0x40000000) == 0) {
                if ((pAnim->flags & 0xe000) != 0) {
                    Util::ClearAnimResultValue<Util::TargetValue_Translate>(pResult, pAnim, nullptr);
                    pAnim->Evaluate(pResult, frame, &mContext.mCache[cacheIndex]);
                    detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[i]);
                } else {
                    pAnim->Evaluate(pResult, frame, &mContext.mCache[cacheIndex]);
                }
            }

            cacheIndex = nextCacheIndex;
        }
    } else {
        int count = mBindTable.mAnimCount;

        for (int i = 0; i < count; ++i, ++pResult) {
            if ((mBindTable.mEntries[i] & 0x40000000) == 0) {
                const ResBoneAnim* pAnim = &m_pBoneAnims[i];

                if ((pAnim->flags & 0xe000) != 0) {
                    Util::ClearAnimResultValue<Util::TargetValue_Translate>(pResult, pAnim, nullptr);
                    pAnim->Evaluate(pResult, frame);
                    detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[i]);
                } else {
                    pAnim->Evaluate(pResult, frame);
                }
            }
        }
    }
}

/**
 * @brief Evaluate one bone animation, mirroring and retargeting its result as requested.
 * @tparam retargeted Whether animated translations are retargeted.
 * @param pResults Result array indexed by animation.
 * @param animIndex Index of the evaluated animation.
 * @param rEvaluate Evaluates the animation into a result.
 */
template <bool retargeted, class Evaluator>
ALWAYS_INLINE inline void SkeletalAnimObj::CalculateMirroredBone(BoneAnimResult* pResults, int animIndex,
                                                                 const Evaluator& rEvaluate) {
    using Util = detail::SkeletalAnimObjUtil;

    const ResBoneAnim* pAnim = &m_pBoneAnims[animIndex];
    unsigned target = mBindTable.mEntries[animIndex] & 0x7fff;
    int mirrorIndex = -1;
    unsigned resultIndex = animIndex;

    if (target != 0x7fff) {
        mirrorIndex = GetMirroringBoneIndex(m_pBoundSkeleton, target);

        if (mirrorIndex >= 0) {
            resultIndex = GetAnimIndex(mirrorIndex);
        }
    }

    if ((mBindTable.mEntries[resultIndex] & 0x40000000) != 0) {
        return;
    }

    BoneAnimResult* pResult = &pResults[resultIndex];

    if (mirrorIndex < 0) {
        if (retargeted && (pAnim->flags & 0xe000) != 0) {
            Util::ClearAnimResultValue<Util::TargetValue_Translate>(pResult, pAnim, nullptr);
            rEvaluate(pResult);
            detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[animIndex]);
        } else {
            rEvaluate(pResult);
        }

        return;
    }

    u32 flags = pAnim->flags;

    if ((flags & 0xe000) != 0 && (flags & 0x1e00) != 0) {
        Util::ClearAnimResultValue<Util::TargetValue_RotateTranslate>(pResult, pAnim,
                                                                      m_pBoundSkeleton->GetBone(mirrorIndex));
        rEvaluate(pResult);

        if (retargeted) {
            detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[animIndex]);
        }

        Util::CalculateMirroring<Util::TargetValue_RotateTranslate>(
            pResult, m_pBoundSkeleton->GetBone(mirrorIndex)->GetMirroringState(), m_pBoundSkeleton);
    } else if ((flags & 0x1e00) != 0) {
        Util::ClearAnimResultValue<Util::TargetValue_Rotate>(pResult, pAnim,
                                                             m_pBoundSkeleton->GetBone(mirrorIndex));
        rEvaluate(pResult);
        Util::CalculateMirroring<Util::TargetValue_Rotate>(
            pResult, m_pBoundSkeleton->GetBone(mirrorIndex)->GetMirroringState(), m_pBoundSkeleton);
    } else if ((flags & 0xe000) != 0) {
        Util::ClearAnimResultValue<Util::TargetValue_Translate>(pResult, pAnim,
                                                                m_pBoundSkeleton->GetBone(mirrorIndex));
        rEvaluate(pResult);

        if (retargeted) {
            detail::RotateRetargetedTranslation(&pResult->translate, m_pRetargeting[animIndex]);
        }

        Util::CalculateMirroring<Util::TargetValue_Translate>(
            pResult, m_pBoundSkeleton->GetBone(mirrorIndex)->GetMirroringState(), m_pBoundSkeleton);
    } else {
        rEvaluate(pResult);
    }
}

/**
 * @brief Evaluate bone channels with mirroring and optional translation retargeting.
 * @tparam mirrored Always true for this implementation.
 * @tparam retargeted Whether animated translations are retargeted.
 */
template <bool mirrored, bool retargeted>
void SkeletalAnimObj::CalculateImpl() {
    float frame = GetFrameCtrl().GetFrame();
    auto* pResults = static_cast<BoneAnimResult*>(mResult);

    if (mContext.IsCacheValid()) {
        int count = mBindTable.mAnimCount;
        unsigned cacheIndex = 0;

        for (int i = 0; i < count; ++i) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            unsigned nextCacheIndex = cacheIndex + pAnim->curveCount;
            CalculateMirroredBone<retargeted>(pResults, i, [&](BoneAnimResult* pResult) {
                pAnim->Evaluate(pResult, frame, &mContext.mCache[cacheIndex]);
            });
            cacheIndex = nextCacheIndex;
        }
    } else {
        int count = mBindTable.mAnimCount;

        for (int i = 0; i < count; ++i) {
            const ResBoneAnim* pAnim = &m_pBoneAnims[i];
            CalculateMirroredBone<retargeted>(
                pResults, i, [&](BoneAnimResult* pResult) { pAnim->Evaluate(pResult, frame); });
        }
    }
}

template void SkeletalAnimObj::CalculateImpl<true, false>();
template void SkeletalAnimObj::CalculateImpl<true, true>();

const SkeletalAnimObj::Impl::CalculateFunction SkeletalAnimObj::Impl::s_pFuncCalculateImpl[4] = {
    &SkeletalAnimObj::CalculateImpl<false, false>, &SkeletalAnimObj::CalculateImpl<false, true>,
    &SkeletalAnimObj::CalculateImpl<true, false>, &SkeletalAnimObj::CalculateImpl<true, true>};
/** @brief Converter writing quaternion results into local matrices. */
struct QuatToMtx {
    /**
     * @brief Write the rotation of a result into a local matrix.
     * @param pLocal Local matrix receiving the rotation columns.
     * @param pResult Result storing a quaternion.
     */
    static void Convert(LocalMtx* pLocal, const BoneAnimResult* pResult) {
        float32x4_t columns[3];
        math::MatrixFromQuaternion(columns, vld1q_f32(pResult->rotate.v));
        pLocal->mtx._m.val[0] = columns[0];
        pLocal->mtx._m.val[1] = columns[1];
        pLocal->mtx._m.val[2] = columns[2];
    }

    /**
     * @brief Write an accumulated quaternion into a matrix.
     * @param pMtx Matrix receiving the rotation columns.
     * @param pBlend Accumulator storing a quaternion.
     */
    static void Convert(util::Matrix4x3fType* pMtx, const BoneAnimBlendResult* pBlend) {
        float32x4_t columns[3];
        math::MatrixFromQuaternion(columns, pBlend->axisX._v);
        pMtx->_m.val[0] = columns[0];
        pMtx->_m.val[1] = columns[1];
        pMtx->_m.val[2] = columns[2];
    }
};

/** @brief Converter writing Euler angle results into local matrices. */
struct EulerToMtx {
    /**
     * @brief Write the rotation of a result into a local matrix.
     * @param pLocal Local matrix receiving the rotation columns.
     * @param pResult Result storing XYZ Euler angles.
     */
    static void Convert(LocalMtx* pLocal, const BoneAnimResult* pResult) {
        float32x4_t columns[3];
        math::MatrixFromEuler(columns,
                              math::LoadVector(*reinterpret_cast<const util::Float3*>(pResult->rotate.v)));
        pLocal->mtx._m.val[0] = columns[0];
        pLocal->mtx._m.val[1] = columns[1];
        pLocal->mtx._m.val[2] = columns[2];
    }
};

/** @brief Converter writing accumulated matrix rows into local matrices. */
struct AxesToMtx {
    /**
     * @brief Write orthonormalized accumulated rows into a matrix.
     * @param pMtx Matrix receiving the rotation; unchanged for degenerate rows.
     * @param pBlend Accumulator storing the first two matrix rows.
     */
    static void Convert(util::Matrix4x3fType* pMtx, const BoneAnimBlendResult* pBlend) {
        float32x4_t columns[3];

        if (math::MatrixFromAxes(columns, pBlend->axisX._v, &pBlend->axisY)) {
            pMtx->_m.val[0] = columns[0];
            pMtx->_m.val[1] = columns[1];
            pMtx->_m.val[2] = columns[2];
        }
    }
};

/**
 * @brief Write the scale, translation and transform flags of a result into a local matrix.
 * @param pLocal Local matrix receiving the transform.
 * @param pResult Animation result.
 */
inline void SetLocalTransform(LocalMtx* pLocal, const BoneAnimResult* pResult) {
    constexpr u32 mask = ResBone::Mask_Transform;
    float32x4_t scale = math::LoadVector(pResult->scale);
    float32x4_t translate = math::LoadVector(pResult->translate);
    pLocal->flag = (pLocal->flag & ~mask) | (pResult->flags & mask);
    pLocal->scale._v = scale;
    pLocal->mtx._m.val[3] = translate;
}

/**
 * @brief Apply calculated bone transforms to a skeleton.
 * @tparam Converter Conversion from the animation's rotations to matrices.
 * @param pSkeleton Skeleton receiving local transforms for applicable bound bones.
 */
template <class Converter>
void SkeletalAnimObj::ApplyToImpl(SkeletonObj* pSkeleton) const {
    int count = mBindTable.mAnimCount;
    LocalMtx* pLocalMtxArray = pSkeleton->GetLocalMtxArray();
    const auto* pResult = static_cast<const BoneAnimResult*>(mResult);

    for (int i = 0; i < count; ++i, ++pResult) {
        u32 entry = mBindTable.mEntries[i];

        if ((entry & 0x80000000) != 0) {
            continue;
        }

        int target = entry & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        LocalMtx* pLocal = &pLocalMtxArray[target];
        SetLocalTransform(pLocal, pResult);
        Converter::Convert(pLocal, pResult);
    }
}

template void SkeletalAnimObj::ApplyToImpl<QuatToMtx>(SkeletonObj*) const;
template void SkeletalAnimObj::ApplyToImpl<EulerToMtx>(SkeletonObj*) const;

const SkeletalAnimObj::Impl::ApplyFunction SkeletalAnimObj::Impl::s_pFuncApplyToImpl[2] = {
    &SkeletalAnimObj::ApplyToImpl<QuatToMtx>, &SkeletalAnimObj::ApplyToImpl<EulerToMtx>};

/** @brief Calculate workspace blocks for bone results, bindings, curve caches and retargeting rotations. */
void SkeletalAnimObj::InitializeArgument::CalculateMemorySize() {
    int bindings = boneCount < boneAnimCount ? boneAnimCount : boneCount;

    for (int i = 0; i < 4; ++i) {
        blocks[i].Initialize(0);
    }

    blocks[0].size = boneAnimCount * sizeof(BoneAnimResult);
    blocks[1].size = bindings * sizeof(u32);
    int curves = curveCount;
    blocks[2].size =
        isContextAvailable && isContextEnabled ? (curves * sizeof(AnimFrameCache) + 7) & ~size_t(7) : 0;
    blocks[3].size = isRetargetingEnabled ? boneAnimCount * sizeof(util::Vector4fType) : 0;
    blocks[3].alignment = 16;
    blocks[3].pointer = nullptr;
    memorySize = 0;
    memoryAlignment = 8;

    for (int i = 0; i < 4; ++i) {
        detail::AppendWorkspaceBlock(blocks[i], memorySize, memoryAlignment, blocks[i].alignment);
    }
}
/**
 * @brief Attach skeletal animation storage to a previously calculated workspace layout.
 * @param rArg Initialization capacities and calculated block offsets.
 * @param pBuffer Caller-owned storage aligned to the calculated workspace alignment.
 * @param bufferSize Available bytes in pBuffer, at least the calculated workspace size.
 * @return True if the layout was calculated and fits in the supplied buffer.
 */
bool SkeletalAnimObj::Initialize(const InitializeArgument& rArg, void* pBuffer, size_t bufferSize) {
    if (rArg.memoryAlignment == 0) {
        return false;
    }

    if (rArg.memorySize > bufferSize) {
        return false;
    }

    int bindings = rArg.boneCount < rArg.boneAnimCount ? rArg.boneAnimCount : rArg.boneCount;
    int curves = rArg.curveCount;
    mWorkMemory = pBuffer;
    mResource = nullptr;
    mBindTable.Initialize(rArg.blocks[1].GetPointer<u32>(pBuffer), bindings);
    mContext.Initialize(rArg.blocks[2].GetPointer<AnimFrameCache>(pBuffer), curves);
    mResult = rArg.blocks[0].GetPointer(pBuffer);
    m_BoneAnimCapacity = rArg.boneAnimCount;
    m_pRetargeting = rArg.blocks[3].GetPointer<util::Vector4fType>(pBuffer);
    return true;
}
/**
 * @brief Select a skeletal animation and reset its playback, bindings and curve caches.
 * @param pRes Non-null animation resource whose counts fit the initialized capacities.
 */
void SkeletalAnimObj::SetResource(const ResSkeletalAnim* pRes) {
    mResource = pRes;
    m_pBoneAnims = pRes->boneAnims;
    mBindTable.mFlags &= ~1;
    bool loop = pRes->IsLooped();
    int frames = pRes->GetFrameCount();
    ResetFrameCtrl(frames, loop);
    mBindTable.mAnimCount = pRes->GetBoneAnimCount();
    mContext.SetCurveCount(pRes->GetCurveCount());
}
/**
 * @brief Resolve animation bone names against a skeleton and reset the evaluation cache.
 * @param pSkeleton Non-null skeleton whose bone count fits the binding-table capacity.
 * @return Combined success and failure flags for all animation bone bindings.
 */
BindResult SkeletalAnimObj::BindImpl(const ResSkeleton* pSkeleton) {
    mBindTable.ClearAll(pSkeleton->GetBoneCount());
    BindResult result;
    int count = mBindTable.mAnimCount;

    for (int i = 0; i < count; ++i) {
        const util::ResDic* pDic = pSkeleton->ToData().pBoneDic.Get();
        int target = pDic != nullptr ? pDic->FindIndex(m_pBoneAnims[i].name.Get()->GetData()) : -1;

        if (target >= 0) {
            mBindTable.mEntries[i] &= 0x3fff8000;
            mBindTable.mEntries[i] |= target & 0x7fff;
            mBindTable.mEntries[target] &= 0xc0007fff;
            mBindTable.mEntries[target] |= (i & 0x7fff) << 15;
            result.Merge(BindResult(BindResult::Flag_Success));
        } else {
            result.Merge(BindResult(BindResult::Flag_Failure));
        }
    }

    mBindTable.mFlags |= 1;
    mContext.Reset();
    return result;
}
/**
 * @brief Reset results using the active mirroring and retargeting policy.
 * @param pSkeleton Skeleton supplying the default transforms for bound bones.
 */
void SkeletalAnimObj::ClearResult(const ResSkeleton* pSkeleton) {
    (this->*Impl::s_pFuncClearImpl[(m_Flags >> 5) & 3])(pSkeleton);
}
/**
 * @brief Bind directly to a skeleton with mirroring and retargeting disabled.
 * @param pSkeleton Non-null skeleton supplying target bones and default transforms.
 * @return Combined success and failure flags from name-based binding.
 */
BindResult SkeletalAnimObj::Bind(const ResSkeleton* pSkeleton) {
    m_pBoundSkeleton = pSkeleton;
    m_Flags &= ~0x60;
    BindResult result;
    result.Merge(BindImpl(pSkeleton));
    ClearResult(pSkeleton);
    return result;
}
/**
 * @brief Bind to the resource of a skeleton object.
 * @param pSkeleton Initialized skeleton object supplying the target resource.
 * @return Combined success and failure flags from name-based binding.
 */
BindResult SkeletalAnimObj::Bind(const SkeletonObj* pSkeleton) {
    return Bind(pSkeleton->GetRes());
}
/**
 * @brief Bind to the skeleton belonging to a model resource.
 * @param pModel Non-null model resource containing the target skeleton.
 * @return Combined success and failure flags from name-based binding.
 */
BindResult SkeletalAnimObj::Bind(const ResModel* pModel) {
    return Bind(pModel->GetSkeleton());
}
/**
 * @brief Bind to the skeleton resource of an initialized model object.
 * @param pModel Initialized model object containing the target skeleton object.
 * @return Combined success and failure flags from name-based binding.
 */
BindResult SkeletalAnimObj::Bind(const ModelObj* pModel) {
    return Bind(pModel->GetSkeleton()->GetRes());
}
/**
 * @brief Bind bones by name and configure optional mirroring and retargeting.
 * @param rArg Non-null target and source skeletons and the desired transformation policies.
 * @return Retargeting results when enabled, otherwise the name-based binding results.
 */
BindResult SkeletalAnimObj::Bind(const BindArgument& rArg) {
    const ResSkeleton* pTarget = rArg.pTargetSkeleton;
    const ResSkeleton* pSource = rArg.pSourceSkeleton;
    m_pBoundSkeleton = pTarget;
    BindResult result;
    result.Merge(BindImpl(pTarget));
    m_Flags = !rArg.isMirroringEnabled ? m_Flags & ~0x40 : m_Flags | 0x40;

    if (pTarget != pSource && rArg.isRetargetingEnabled) {
        m_Flags |= 0x20;
        result = BindResult();
        result.Merge(InitRetargeting(pTarget, pSource));
    } else {
        m_Flags &= ~0x20;
    }

    ClearResult(pTarget);
    return result;
}
/**
 * @brief Bind precomputed bone indices and configure optional mirroring and retargeting.
 * @param rArg Skeletons and policies; the target must match the resource's precomputed indices.
 * @return Success combined with any failures encountered during retargeting.
 */
BindResult SkeletalAnimObj::BindFast(const BindArgument& rArg) {
    const ResSkeleton* pTarget = rArg.pTargetSkeleton;
    const ResSkeleton* pSource = rArg.pSourceSkeleton;
    m_pBoundSkeleton = pTarget;
    BindFastImpl(pTarget);
    BindResult result(BindResult::Flag_Success);
    m_Flags = !rArg.isMirroringEnabled ? m_Flags & ~0x40 : m_Flags | 0x40;

    if (pTarget != pSource && rArg.isRetargetingEnabled) {
        m_Flags |= 0x20;
        result.Merge(InitRetargeting(pTarget, pSource));
    } else {
        m_Flags &= ~0x20;
    }

    ClearResult(pTarget);
    return result;
}
/**
 * @brief Install precomputed bone bindings and reset cached channel evaluation.
 * @param pTarget Skeleton whose bone indices match the resource's binding array.
 */
inline void SkeletalAnimObj::BindFastImpl(const ResSkeleton* pTarget) {
    mBindTable.ClearAll(pTarget->GetBoneCount());
    mBindTable.BindAll(mResource->bindIndices);
    mBindTable.mFlags |= 1;
    mContext.Reset();
}
/**
 * @brief Initialize rotation corrections for bound bones using matching source bone names.
 * @param pTarget Non-null bound skeleton providing destination bone orientations.
 * @param pSource Non-null skeleton providing the original orientations by bone name.
 * @return Success and failure flags for bones with valid target and source bindings.
 */
BindResult SkeletalAnimObj::InitRetargeting(const ResSkeleton* pTarget, const ResSkeleton* pSource) {
    BindResult result;
    int count = mBindTable.mAnimCount;

    for (int i = 0; i < count; ++i) {
        util::Vector4fType* pRotation = &m_pRetargeting[i];
        pRotation->_v = (float32x4_t){0.0f, 0.0f, 0.0f, 1.0f};
        const util::ResDic* pDic = pSource->ToData().pBoneDic.Get();

        if (pDic != nullptr) {
            const char* pName = m_pBoneAnims[i].name.Get()->GetData();
            unsigned target = mBindTable.mEntries[i] & 0x7fff;
            int source = pDic->FindIndex(pName);

            if (target != 0x7fff && source >= 0) {
                result.Merge(BindResult(BindResult::Flag_Success));
                detail::SkeletalAnimObjUtil::CalculateRetargetingQuaternion(
                    pRotation, pTarget->GetBone(target), pSource->GetBone(source));
                continue;
            }
        }

        result.Merge(BindResult(BindResult::Flag_Failure));
    }

    return result;
}
/**
 * @brief Bind animation bones and retarget their rotations when the two skeletons differ.
 * @param pTarget Non-null destination skeleton with sufficient binding capacity.
 * @param pSource Non-null skeleton defining the animation's original bone orientations.
 * @return Retargeting results, or ordinary binding results when both skeletons are identical.
 */
BindResult SkeletalAnimObj::Bind(const ResSkeleton* pTarget, const ResSkeleton* pSource) {
    BindResult result;

    if (pTarget != pSource) {
        m_pBoundSkeleton = pTarget;
        m_Flags |= 0x20;
        BindImpl(pTarget);
        result.Merge(InitRetargeting(pTarget, pSource));
        ClearResult(pTarget);
    } else {
        result.Merge(Bind(pTarget));
    }

    return result;
}
/**
 * @brief Bind and retarget using the resources of two skeleton objects.
 * @param pTarget Initialized destination skeleton object.
 * @param pSource Initialized skeleton defining the original bone orientations.
 * @return Combined binding or retargeting result flags.
 */
BindResult SkeletalAnimObj::Bind(const SkeletonObj* pTarget, const SkeletonObj* pSource) {
    return Bind(pTarget->GetRes(), pSource->GetRes());
}
/**
 * @brief Bind and retarget using two model resources.
 * @param pTarget Model resource containing the destination skeleton.
 * @param pSource Model resource containing the original animation skeleton.
 * @return Combined binding or retargeting result flags.
 */
BindResult SkeletalAnimObj::Bind(const ResModel* pTarget, const ResModel* pSource) {
    return Bind(pTarget->GetSkeleton(), pSource->GetSkeleton());
}
/**
 * @brief Bind and retarget using the skeletons of two model objects.
 * @param pTarget Initialized destination model object.
 * @param pSource Initialized model defining the original animation skeleton.
 * @return Combined binding or retargeting result flags.
 */
BindResult SkeletalAnimObj::Bind(const ModelObj* pTarget, const ModelObj* pSource) {
    return Bind(pTarget->GetSkeleton()->GetRes(), pSource->GetSkeleton()->GetRes());
}
/**
 * @brief Use precomputed bindings and retarget rotations when source and target differ.
 * @param pTarget Non-null skeleton matching the resource's precomputed bone indices.
 * @param pSource Non-null skeleton defining original bone orientations.
 * @return Retargeting result flags, or success when the skeletons are identical.
 */
BindResult SkeletalAnimObj::BindFast(const ResSkeleton* pTarget, const ResSkeleton* pSource) {
    if (pTarget == pSource) {
        BindFast(pTarget);
        return BindResult(BindResult::Flag_Success);
    }

    m_Flags |= 0x20;
    m_pBoundSkeleton = pTarget;
    BindFastImpl(pTarget);
    BindResult result;
    result.Merge(InitRetargeting(pTarget, pSource));
    ClearResult(pTarget);
    return result;
}
/**
 * @brief Bind model skeletons using precomputed indices and optional retargeting.
 * @param pTarget Model containing a skeleton matching the precomputed binding indices.
 * @param pSource Model containing the original animation skeleton.
 * @return Combined retargeting result flags, or success for identical skeletons.
 */
BindResult SkeletalAnimObj::BindFast(const ResModel* pTarget, const ResModel* pSource) {
    BindResult result;
    result.Merge(BindFast(pTarget->GetSkeleton(), pSource->GetSkeleton()));
    return result;
}
/**
 * @brief Bind using the bone indices precomputed in the animation resource.
 * @param pSkeleton Skeleton matching the animation's precomputed binding indices.
 */
void SkeletalAnimObj::BindFast(const ResSkeleton* pSkeleton) {
    m_Flags &= ~0x60;
    mBindTable.ClearAll(pSkeleton->GetBoneCount());
    mBindTable.BindAll(mResource->bindIndices);
    m_pBoundSkeleton = pSkeleton;
    mBindTable.mFlags |= 1;
    mContext.Reset();
    ClearResult(pSkeleton);
}
/**
 * @brief Bind a model resource using the animation's precomputed bone indices.
 * @param pModel Model resource whose skeleton matches the precomputed binding indices.
 */
void SkeletalAnimObj::BindFast(const ResModel* pModel) {
    BindFast(pModel->GetSkeleton());
}
/** @brief Restore resource-provided default results without using a bound skeleton. */
void SkeletalAnimObj::ClearResult() {
    int count = mBindTable.mAnimCount;
    auto* pResult = static_cast<BoneAnimResult*>(mResult);

    for (int i = 0; i < count; ++i, ++pResult) {
        m_pBoneAnims[i].Initialize(pResult, nullptr);
    }
}
/** @brief Evaluate the current playback frame only when it differs from the last calculated frame. */
void SkeletalAnimObj::Calculate() {
    if (mContext.mLastFrame != GetFrameCtrl().GetFrame()) {
        (this->*Impl::s_pFuncCalculateImpl[(m_Flags >> 5) & 3])();
        mContext.mLastFrame = GetFrameCtrl().GetFrame();
    }
}
/**
 * @brief Apply calculated bone transforms using the animation's rotation representation.
 * @param pSkeleton Initialized skeleton receiving transforms for applicable bound bones.
 */
void SkeletalAnimObj::ApplyTo(SkeletonObj* pSkeleton) const {
    (this->*Impl::s_pFuncApplyToImpl[(mResource->flags >> 12) & 7])(pSkeleton);
}
/**
 * @brief Apply calculated bone transforms to a model's skeleton.
 * @param pModel Initialized model object containing the bound target skeleton.
 */
void SkeletalAnimObj::ApplyTo(ModelObj* pModel) const {
    ApplyTo(pModel->GetSkeleton());
}
/**
 * @brief Set a binding policy for a bone and every bone in its contiguous descendant branch.
 * @param pSkeleton Skeleton defining the branch extent; must match the binding table.
 * @param boneIndex First bone index, within the skeleton's bone array.
 * @param flag Calculation and application policy for each bound bone in the branch.
 */
void SkeletalAnimObj::SetBindFlag(const ResSkeleton* pSkeleton, int boneIndex, BindFlag flag) {
    u32 flags = static_cast<u32>(flag) << 30;
    int endIndex = pSkeleton->GetBranchEndIndex(boneIndex);
    ptrdiff_t index = boneIndex;
    do {
        mBindTable.SetFlagsForTarget(index, flags);
        ++index;
    } while (index < endIndex);
}
/**
 * @brief Accumulate the rotation, scale and translation of one bone result.
 * @tparam Converter Conversion from the result's rotation to the accumulator's storage.
 * @param pResults Accumulators indexed by bone.
 * @param pResult Bone result to accumulate.
 * @param boneIndex Bone index within the accumulators.
 * @param weight Blend weight.
 */
template <class Converter>
void SkeletalAnimBlender::BlendResultImpl(BoneAnimBlendResult* pResults, const BoneAnimResult* pResult,
                                          int boneIndex, float weight) {
    Converter::Blend(&pResults[boneIndex], pResult, weight);
    BoneAnimBlendResult* pBlend = &pResults[boneIndex];

    if ((m_Flags & 1) == 0) {
        pBlend->scale._v = vaddq_f32(pBlend->scale._v, vmulq_n_f32(math::LoadVector(pResult->scale), weight));
        pBlend->translate._v =
            vaddq_f32(pBlend->translate._v, vmulq_n_f32(math::LoadVector(pResult->translate), weight));
        pBlend->flags |= ~pResult->flags;
    } else {
        float32x4_t scale = math::MakeVector(powf(pResult->scale.x, weight), powf(pResult->scale.y, weight),
                                             powf(pResult->scale.z, weight));

        if (fabsf(pBlend->weight) < 0.001f) {
            pBlend->scale._v = math::MakeVector(1.0f, 1.0f, 1.0f);
        }

        pBlend->scale._v = vmulq_f32(scale, pBlend->scale._v);
        pBlend->translate._v =
            vaddq_f32(pBlend->translate._v, vmulq_n_f32(math::LoadVector(pResult->translate), weight));
        pBlend->flags |= ~pResult->flags;
    }

    pBlend->weight += weight;
}

/**
 * @brief Convert every active accumulator to another rotation representation.
 * @tparam Converter Conversion applied in place.
 */
template <class Converter>
void SkeletalAnimBlender::ConvertResultRotate() {
    BoneAnimBlendResult* pResult = mResult;

    for (size_t i = 0; i < mMaxBoneCount; ++i, ++pResult) {
        if (!(fabsf(pResult->weight) < 0.001f)) {
            Converter::ConvertResult(pResult);
        }
    }
}

/**
 * @brief Calculate the transform difference between two bone results.
 * @tparam AnimConverter Reads the animation's rotation as a quaternion.
 * @tparam BaseConverter Reads the reference rotation as a quaternion.
 * @param pOut Receives the difference, with its rotation stored as a quaternion.
 * @param rAnim Animation result.
 * @param rBase Reference result.
 */
template <class AnimConverter, class BaseConverter>
void SkeletalAnimBlender::CalculateBoneAnimDiffImpl(BoneAnimResult* pOut, const BoneAnimResult& rAnim,
                                                    const BoneAnimResult& rBase) {
    float32x4_t inverseScale = {1.0f / rBase.scale.x, 1.0f / rBase.scale.y, 1.0f / rBase.scale.z, 0.0f};
    math::StoreVector(&pOut->scale, vmulq_f32(inverseScale, math::LoadVector(rAnim.scale)));
    util::Vector4fType animRotate;
    util::Vector4fType baseRotate;
    AnimConverter::Convert(&animRotate, &rAnim);
    BaseConverter::Convert(&baseRotate, &rBase);
    vst1q_f32(pOut->rotate.v,
              math::QuaternionMultiply(math::QuaternionInverse(baseRotate._v), animRotate._v));
    pOut->flags = rAnim.flags & ~ResBone::Mask_Rot;
    math::StoreVector(&pOut->translate,
                      vsubq_f32(math::LoadVector(rAnim.translate), math::LoadVector(rBase.translate)));
}

/**
 * @brief Apply accumulated transforms to a skeleton.
 * @tparam Converter Conversion from the accumulator's rotation storage to matrices.
 * @tparam mode Whether scale and translation accumulators are normalized by their weight.
 * @param pSkeleton Skeleton whose bones correspond to the accumulators.
 */
template <class Converter, SkeletalAnimBlender::BlendMode mode>
void SkeletalAnimBlender::ApplyToImpl(SkeletonObj* pSkeleton) const {
    BoneAnimBlendResult* pBlendResults = mResult;
    LocalMtx* pLocalMtxArray = pSkeleton->GetLocalMtxArray();

    for (size_t i = 0; i < mMaxBoneCount; ++i) {
        BoneAnimBlendResult* pBlend = &pBlendResults[i];
        LocalMtx* pLocal = &pLocalMtxArray[i];
        float weight = pBlend->weight;

        if (fabsf(weight) < 0.001f) {
            continue;
        }

        if (mode == BlendMode_Interpolate && !(fabsf(weight - 1.0f) < 0.001f)) {
            float inverse = 1.0f / weight;
            pBlend->scale._v = vmulq_n_f32(pBlend->scale._v, inverse);
            pBlend->translate._v = vmulq_n_f32(pBlend->translate._v, inverse);
        }

        constexpr u32 mask = ResBone::Mask_Transform;
        pLocal->flag = (~pBlend->flags & mask) | (pLocal->flag & ~mask);
        pLocal->scale = pBlend->scale;
        pLocal->mtx._m.val[3] = pBlend->translate._v;
        Converter::Convert(&pLocal->mtx, pBlend);
    }
}

/**
 * @brief Express a difference rotation relative to the accumulated rotation.
 * @tparam Converter Reads the accumulated rotation as a quaternion.
 * @param pOut Result receiving the combined rotation.
 * @param rBlend Accumulated blend result.
 * @param rResult Difference result supplying the relative rotation.
 */
template <class Converter>
void SkeletalAnimBlender::ConvertValidResult(BoneAnimResult* pOut, const BoneAnimBlendResult& rBlend,
                                             const BoneAnimResult& rResult) {
    util::Vector4fType rotate = {{0.0f, 0.0f, 0.0f, 1.0f}};
    Converter::Convert(&rotate, &rBlend);
    vst1q_f32(pOut->rotate.v, math::QuaternionMultiply(rotate._v, vld1q_f32(rResult.rotate.v)));
}

struct SkeletalAnimBlender::Impl {
    using BlendFunction = void (SkeletalAnimBlender::*)(SkeletalAnimObj*, float);
    using BlendResultFunction = void (SkeletalAnimBlender::*)(BoneAnimBlendResult*, const BoneAnimResult*,
                                                              int, float);
    using ConvertValidResultFunction = void (SkeletalAnimBlender::*)(BoneAnimResult*,
                                                                     const BoneAnimBlendResult&,
                                                                     const BoneAnimResult&);
    using ApplyFunction = void (SkeletalAnimBlender::*)(SkeletonObj*) const;
    using ConvertFunction = void (SkeletalAnimBlender::*)();
    using CalculateBoneAnimDiffFunction = void (*)(BoneAnimResult*, const BoneAnimResult&,
                                                   const BoneAnimResult&);
    static const BlendFunction s_pFuncBlend[8];
    static const BlendResultFunction s_pFuncBlendResult[4];
    static const ConvertValidResultFunction s_pFuncConvertValidResult[2];
    static const ApplyFunction s_pFuncApplyTo[4];
    static const ConvertFunction s_pFuncConvertResultRotate[2];
    static const CalculateBoneAnimDiffFunction s_pFuncCalculateBoneAnimDiff[4];
};

/**
 * @brief Copy a bone's bind pose into an animation result.
 * @param pResult Result receiving the bone flags and transform.
 * @param pBone Bone supplying the bind pose.
 */
inline void InitializeResultFromBone(BoneAnimResult* pResult, const ResBone* pBone) {
    pResult->flags = pBone->ToData().flag;
    pResult->rotate = pBone->GetRotateQuat();
    pResult->scale = pBone->GetScale();
    pResult->translate = pBone->GetTranslate();
}

/**
 * @brief Blend every bound bone of an evaluated animation into the accumulated results.
 * @tparam Converter Conversion from the animation's rotations to the blender's storage.
 * @tparam useCallback Whether the blend callback adjusts each bone's weight.
 * @param pAnimObj Calculated animation with valid bindings.
 * @param weight Blend contribution of the animation.
 */
template <class Converter, bool useCallback>
void SkeletalAnimBlender::BlendImpl(SkeletalAnimObj* pAnimObj, float weight) {
    int count = pAnimObj->GetBoneAnimCount();
    BoneAnimResult* pResult = pAnimObj->GetResultArray();
    BoneAnimBlendResult* pBlendResults = mResult;

    for (int i = 0; i < count; ++i, ++pResult) {
        u32 entry = pAnimObj->GetBindEntry(i);

        if ((entry & 0x80000000) != 0) {
            continue;
        }

        int target = entry & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        if constexpr (useCallback) {
            ICalculateBlendCallback::CallbackArg arg;
            arg.pAnimObj = pAnimObj;
            arg.boneIndex = target;
            arg.weight = weight;
            m_pCallback->Exec(&arg);
            BlendResultImpl<Converter>(pBlendResults, pResult, target, arg.weight);
        } else {
            BlendResultImpl<Converter>(pBlendResults, pResult, target, weight);
        }
    }
}

const SkeletalAnimBlender::Impl::BlendFunction SkeletalAnimBlender::Impl::s_pFuncBlend[8] = {
    &SkeletalAnimBlender::BlendImpl<QuatToAxes, false>, &SkeletalAnimBlender::BlendImpl<EulerToAxes, false>,
    &SkeletalAnimBlender::BlendImpl<QuatToQuat, false>, &SkeletalAnimBlender::BlendImpl<EulerToQuat, false>,
    &SkeletalAnimBlender::BlendImpl<QuatToAxes, true>,  &SkeletalAnimBlender::BlendImpl<EulerToAxes, true>,
    &SkeletalAnimBlender::BlendImpl<QuatToQuat, true>,  &SkeletalAnimBlender::BlendImpl<EulerToQuat, true>};
const SkeletalAnimBlender::Impl::BlendResultFunction SkeletalAnimBlender::Impl::s_pFuncBlendResult[4] = {
    &SkeletalAnimBlender::BlendResultImpl<QuatToAxes>, &SkeletalAnimBlender::BlendResultImpl<EulerToAxes>,
    &SkeletalAnimBlender::BlendResultImpl<QuatToQuat>, &SkeletalAnimBlender::BlendResultImpl<EulerToQuat>};
const SkeletalAnimBlender::Impl::ConvertValidResultFunction
    SkeletalAnimBlender::Impl::s_pFuncConvertValidResult[2] = {
        &SkeletalAnimBlender::ConvertValidResult<AxesToQuat>,
        &SkeletalAnimBlender::ConvertValidResult<QuatToQuat>};
const SkeletalAnimBlender::Impl::ApplyFunction SkeletalAnimBlender::Impl::s_pFuncApplyTo[4] = {
    &SkeletalAnimBlender::ApplyToImpl<AxesToMtx, BlendMode_Interpolate>,
    &SkeletalAnimBlender::ApplyToImpl<AxesToMtx, BlendMode_Additive>,
    &SkeletalAnimBlender::ApplyToImpl<QuatToMtx, BlendMode_Interpolate>,
    &SkeletalAnimBlender::ApplyToImpl<QuatToMtx, BlendMode_Additive>};
const SkeletalAnimBlender::Impl::ConvertFunction SkeletalAnimBlender::Impl::s_pFuncConvertResultRotate[2] = {
    &SkeletalAnimBlender::ConvertResultRotate<AxesToQuat>,
    &SkeletalAnimBlender::ConvertResultRotate<QuatToAxes>};
const SkeletalAnimBlender::Impl::CalculateBoneAnimDiffFunction
    SkeletalAnimBlender::Impl::s_pFuncCalculateBoneAnimDiff[4] = {
        &SkeletalAnimBlender::CalculateBoneAnimDiffImpl<QuatToQuat, QuatToQuat>,
        &SkeletalAnimBlender::CalculateBoneAnimDiffImpl<QuatToQuat, EulerToQuat>,
        &SkeletalAnimBlender::CalculateBoneAnimDiffImpl<EulerToQuat, QuatToQuat>,
        &SkeletalAnimBlender::CalculateBoneAnimDiffImpl<EulerToQuat, EulerToQuat>};

/** @brief Calculate workspace storage for the maximum number of blended bone results. */
void SkeletalAnimBlender::InitializeArgument::CalculateMemorySize() {
    blocks[0].Initialize(boneCount * sizeof(BoneAnimBlendResult));
    memorySize = 0;
    memoryAlignment = 8;
    blocks[0].AppendTo(memorySize, memoryAlignment);
}
/**
 * @brief Attach a blender to its calculated caller-owned workspace.
 * @param rArg Bone capacity and previously calculated storage layout.
 * @param pBuffer Workspace aligned to the calculated requirement; may be null for an empty layout.
 * @param bufferSize Available workspace bytes, at least the calculated requirement.
 * @return True if the layout was calculated and fits in the supplied storage.
 */
bool SkeletalAnimBlender::Initialize(const InitializeArgument& rArg, void* pBuffer, size_t bufferSize) {
    if (rArg.memoryAlignment == 0) {
        return false;
    }

    if (rArg.memorySize > bufferSize) {
        return false;
    }

    m_pWorkMemory = pBuffer;
    mResult = rArg.blocks[0].GetPointer<BoneAnimBlendResult>(pBuffer);
    u16 count = rArg.boneCount;
    mBoneCount = count;
    mMaxBoneCount = count;
    return true;
}
/** @brief Clear every result slot and mark the accumulated blend as empty. */
void SkeletalAnimBlender::ClearResult() {
    m_Flags |= 4;
    size_t bytes = mMaxBoneCount * sizeof(BoneAnimBlendResult);
    std::memset(mResult, 0, bytes);
}
/**
 * @brief Evaluate an animation and accumulate its weighted transforms.
 * @param pAnimObj Initialized animation with valid bone bindings.
 * @param weight Blend contribution; magnitudes below 0.001 are ignored.
 */
void SkeletalAnimBlender::Blend(SkeletalAnimObj* pAnimObj, float weight) {
    if (fabsf(weight) < 0.001f) {
        return;
    }

    m_Flags &= ~4;
    pAnimObj->Calculate();
    u32 rotateMode = pAnimObj->GetRotateMode();
    u32 index = (m_Flags & 2) | rotateMode;

    if (m_pCallback != nullptr) {
        index |= 4;
    }

    (this->*Impl::s_pFuncBlend[index])(pAnimObj, weight);
}

/**
 * @brief Evaluate two animations and accumulate their weighted transform differences.
 * @param pAnimObj Initialized source animation with valid bone bindings.
 * @param pBaseAnimObj Initialized reference animation bound to the same target skeleton.
 * @param weight Blend contribution; magnitudes below 0.001 are ignored.
 */
void SkeletalAnimBlender::Blend(SkeletalAnimObj* pAnimObj, SkeletalAnimObj* pBaseAnimObj, float weight) {
    if (fabsf(weight) < 0.001f) {
        return;
    }

    m_Flags &= ~4;
    pAnimObj->Calculate();
    pBaseAnimObj->Calculate();
    BlendDiffAnim(pAnimObj, pBaseAnimObj, weight);
}
/**
 * @brief Accumulate the difference between two animations, relative to the current accumulation.
 * @param pAnimObj Calculated animation with valid bone bindings.
 * @param pBaseAnimObj Calculated reference animation bound to the same skeleton.
 * @param weight Blend contribution of the difference.
 */
void SkeletalAnimBlender::BlendDiffAnim(SkeletalAnimObj* pAnimObj, SkeletalAnimObj* pBaseAnimObj,
                                        float weight) {
    int count = pAnimObj->GetBoneAnimCount();
    const BoneAnimResult* pBaseResults = pBaseAnimObj->GetResultArray();
    BoneAnimBlendResult* pBlendResults = mResult;
    float rest = 1.0f - weight;
    u32 animRotateMode = pAnimObj->GetRotateMode() << 1;
    Impl::ConvertValidResultFunction convert = Impl::s_pFuncConvertValidResult[(m_Flags >> 1) & 1];
    const BoneAnimResult* pAnimResult = pAnimObj->GetResultArray();

    for (int i = 0; i < count; ++i, ++pAnimResult) {
        u32 entry = pAnimObj->GetBindEntry(i);

        if ((entry & 0x80000000) != 0) {
            continue;
        }

        int target = entry & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        u32 baseIndex = (pBaseAnimObj->GetBindEntry(target) >> 15) & 0x7fff;

        if (baseIndex == 0x7fff) {
            continue;
        }

        if ((pBaseAnimObj->GetBindEntry(baseIndex) >> 30) != 0) {
            continue;
        }

        const BoneAnimResult* pBaseResult = &pBaseResults[baseIndex];
        BoneAnimBlendResult* pBlend = &pBlendResults[target];
        u32 baseRotateMode = pBaseAnimObj->GetRotateMode();

        if (fabsf(pBlend->weight) < 0.001f) {
            BoneAnimResult bindPose;
            InitializeResultFromBone(&bindPose, pAnimObj->GetBoundSkeleton()->GetBone(target));
            BlendResult(bindPose, target, 1.0f);
            pBlend->weight = 0.0f;
        }

        BoneAnimResult diff;
        Impl::s_pFuncCalculateBoneAnimDiff[baseRotateMode | animRotateMode](&diff, *pAnimResult,
                                                                            *pBaseResult);
        (this->*convert)(&diff, *pBlend, diff);

        if ((m_Flags & 2) == 0) {
            pBlend->axisX._v = vmulq_n_f32(pBlend->axisX._v, rest);
            pBlend->axisY._v = vmulq_n_f32(pBlend->axisY._v, rest);
            BlendResult(diff, target, weight);
        } else {
            float prevWeight = pBlend->weight;
            pBlend->weight = rest;
            BlendResult(diff, target, weight);
            pBlend->weight = prevWeight + weight;
        }
    }
}

/**
 * @brief Accumulate a single bone result.
 * @param rResult Result whose flags select the rotation representation.
 * @param boneIndex Bone index within the result array.
 * @param weight Blend contribution; magnitudes below 0.001 are ignored.
 */
void SkeletalAnimBlender::BlendResult(const BoneAnimResult& rResult, int boneIndex, float weight) {
    if (fabsf(weight) < 0.001f) {
        return;
    }

    m_Flags &= ~4;
    u32 index = (m_Flags & 2) | ((rResult.flags >> 12) & 7);

    if (m_pCallback != nullptr) {
        ICalculateBlendCallback::CallbackArg arg;
        arg.pAnimObj = nullptr;
        arg.boneIndex = boneIndex;
        arg.weight = weight;
        m_pCallback->Exec(&arg);
        weight = arg.weight;
    }

    (this->*Impl::s_pFuncBlendResult[index])(mResult, &rResult, boneIndex, weight);
}

/**
 * @brief Accumulate a bone difference relative to the current accumulation.
 * @param rDiff Difference result; ignored when it is marked invalid.
 * @param boneIndex Bone index within the result array.
 * @param weight Blend contribution of the difference.
 */
void SkeletalAnimBlender::BlendResult(const BoneAnimDiffResult& rDiff, int boneIndex, float weight) {
    if ((rDiff.result.flags & 0x10) != 0) {
        return;
    }

    BoneAnimResult result = rDiff.result;
    BoneAnimBlendResult* pBlendResults = mResult;
    (this->*Impl::s_pFuncConvertValidResult[(m_Flags >> 1) & 1])(&result, pBlendResults[boneIndex], result);
    BoneAnimBlendResult* pBlend = &pBlendResults[boneIndex];

    if ((m_Flags & 2) == 0) {
        float rest = 1.0f - weight;
        pBlend->axisX._v = vmulq_n_f32(pBlend->axisX._v, rest);
        pBlend->axisY._v = vmulq_n_f32(pBlend->axisY._v, rest);
        BlendResult(result, boneIndex, weight);
    } else {
        float prevWeight = pBlend->weight;
        pBlend->weight = 1.0f - weight;
        BlendResult(result, boneIndex, weight);
        pBlend->weight = prevWeight + weight;
    }
}

/**
 * @brief Apply accumulated transforms and mark the blend as consumed.
 * @param pSkeleton Initialized target skeleton whose bones correspond to the result array.
 */
void SkeletalAnimBlender::ApplyTo(SkeletonObj* pSkeleton) const {
    (this->*Impl::s_pFuncApplyTo[(m_Flags & 3) | ((m_Flags >> 2) & 1)])(pSkeleton);
    m_Flags |= 4;
}
/**
 * @brief Begin blending, converting stored rotations when the representation changes.
 * @param flags Bit zero selects additive blending and bit one selects quaternion rotation storage.
 */
void SkeletalAnimBlender::BeginBlend(u32 flags) {
    size_t mode = m_Flags & 2;

    if (static_cast<u32>(mode) != (flags & 2)) {
        (this->*Impl::s_pFuncConvertResultRotate[mode / 2])();
    }

    m_Flags = (flags & 3) | 8;
}
/** @brief Normalize non-additive scale and translation accumulators and finish blending. */
void SkeletalAnimBlender::EndBlend() {
    BoneAnimBlendResult* pResult = mResult;

    for (size_t i = 0; i < mMaxBoneCount; ++i, ++pResult) {
        float weight = pResult->weight;

        if (!(fabsf(weight) < 0.001f)) {
            if ((m_Flags & 1) == 0 && !(fabsf(weight - 1.0f) < 0.001f)) {
                float inverse = 1.0f / weight;
                pResult->scale._v = vmulq_n_f32(pResult->scale._v, inverse);
                pResult->translate._v = vmulq_n_f32(pResult->translate._v, inverse);
            }

            pResult->weight = 1.0f;
        }
    }

    m_Flags = (m_Flags | 4) ^ 8;
}
/**
 * @brief Calculate per-bone differences between two animations.
 * @param pResults Difference results indexed by bone; unbound bones are marked invalid.
 * @param count Number of entries in pResults.
 * @param pAnimObj Animation with valid bone bindings.
 * @param pBaseAnimObj Reference animation bound to the same skeleton.
 */
void SkeletalAnimBlender::CalculateBoneAnimDiff(BoneAnimDiffResult* pResults, int count,
                                                SkeletalAnimObj* pAnimObj, SkeletalAnimObj* pBaseAnimObj) {
    for (int i = 0; i < count; ++i) {
        pResults[i].result.flags |= 0x10;
    }

    pAnimObj->Calculate();
    pBaseAnimObj->Calculate();
    int animCount = pAnimObj->GetBoneAnimCount();
    const BoneAnimResult* pBaseResults = pBaseAnimObj->GetResultArray();
    const BoneAnimResult* pAnimResult = pAnimObj->GetResultArray();
    u32 animRotateMode = pAnimObj->GetRotateMode() << 1;

    for (int i = 0; i < animCount; ++i, ++pAnimResult) {
        u32 entry = pAnimObj->GetBindEntry(i);

        if ((entry & 0x80000000) != 0) {
            continue;
        }

        int target = entry & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        u32 baseIndex = (pBaseAnimObj->GetBindEntry(target) >> 15) & 0x7fff;

        if (baseIndex == 0x7fff) {
            continue;
        }

        if ((pBaseAnimObj->GetBindEntry(baseIndex) >> 30) != 0) {
            continue;
        }

        BoneAnimDiffResult* pDiff = &pResults[target];
        const BoneAnimResult* pBaseResult = &pBaseResults[baseIndex];
        u32 index = pBaseAnimObj->GetRotateMode() | animRotateMode;
        pDiff->result.flags &= ~0x10;
        BoneAnimResult diff;
        Impl::s_pFuncCalculateBoneAnimDiff[index](&diff, *pAnimResult, *pBaseResult);
        pDiff->result = diff;
    }
}

/**
 * @brief Calculate per-bone differences between an animation and its skeleton's bind pose.
 * @param pResults Difference results indexed by bone; unbound bones are marked invalid.
 * @param count Number of entries in pResults.
 * @param pAnimObj Animation with valid bone bindings.
 */
void SkeletalAnimBlender::CalculateBoneAnimDiff(BoneAnimDiffResult* pResults, int count,
                                                SkeletalAnimObj* pAnimObj) {
    for (int i = 0; i < count; ++i) {
        pResults[i].result.flags |= 0x10;
    }

    pAnimObj->Calculate();
    int animCount = pAnimObj->GetBoneAnimCount();
    const BoneAnimResult* pAnimResult = pAnimObj->GetResultArray();
    u32 index = (pAnimObj->GetRotateMode() << 1) |
                (pAnimObj->GetBoundSkeleton()->GetRotateMode() >> ResSkeleton::Shift_Rot);

    for (int i = 0; i < animCount; ++i, ++pAnimResult) {
        u32 entry = pAnimObj->GetBindEntry(i);

        if ((entry & 0x80000000) != 0) {
            continue;
        }

        int target = entry & 0x7fff;

        if (target == 0x7fff) {
            continue;
        }

        BoneAnimDiffResult* pDiff = &pResults[target];
        pDiff->result.flags &= ~0x10;
        BoneAnimResult bindPose;
        InitializeResultFromBone(&bindPose, pAnimObj->GetBoundSkeleton()->GetBone(target));
        BoneAnimResult diff;
        Impl::s_pFuncCalculateBoneAnimDiff[index](&diff, *pAnimResult, bindPose);
        pDiff->result = diff;
    }
}
}  // namespace nn::g3d
