#pragma once

#include <nn/util/util_Arithmetic.h>

namespace nn::g3d::detail {
struct VectorRotationValues {
    float32x4_t sine;
    float32x4_t cosine;
};
/**
 * @brief Approximate sine and cosine for four radian angles with the resource polynomials.
 * @param angles Four angles in radians, reduced to one revolution before evaluation.
 * @return Sine and cosine vectors with corresponding input lanes.
 */
inline VectorRotationValues EvaluateRotation(float32x4_t angles) {
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
    return {vmulq_f32(angles, sine), vmulq_f32(sign, cosine)};
}

/**
 * @brief Load a packed three-component vector into zero-padded SIMD storage.
 * @param rSource Three readable floating-point components.
 * @return Vector with the source components and a zero fourth lane.
 */
inline float32x4_t LoadTransformVector(const util::Float3& rSource) {
    float32x4_t value = vdupq_n_f32(0.0f);
    value = vld1q_lane_f32(&rSource.x, value, 0);
    value = vld1q_lane_f32(&rSource.y, value, 1);
    return vld1q_lane_f32(&rSource.z, value, 2);
}
/**
 * @brief Construct a padded rotation matrix from three Euler angles.
 * @param rotation Euler angles in radians in the first three lanes; fourth lane must be zero.
 * @return Rotation matrix with a zero translation row.
 */
inline util::Matrix4x3fType MakeEulerRotation(float32x4_t rotation) {
    VectorRotationValues values = EvaluateRotation(rotation);
    float32x4_t sine = values.sine;
    float32x4_t cosine = values.cosine;
    float32x2_t sy = vdup_laneq_f32(sine, 1);
    float32x2_t cy = vdup_laneq_f32(cosine, 1);
    float32x2_t syZero = vset_lane_f32(0.0f, sy, 1);
    float32x2_t cyZero = vset_lane_f32(0.0f, cy, 1);
    float32x2_t czsz = vzip1_f32(vget_high_f32(cosine), vget_high_f32(sine));
    float32x2_t szcz = vzip1_f32(vget_high_f32(sine), vget_high_f32(cosine));
    float32x4_t shared = vcombine_f32(vmul_f32(czsz, sy), vmul_f32(cyZero, float32x2_t{1.0f, 0.0f}));
    float32x4_t axisX = vcombine_f32(vmul_f32(czsz, cy), vmul_f32(syZero, float32x2_t{-1.0f, 0.0f}));
    float32x4_t zPair = vcombine_f32(szcz, vdup_n_f32(0.0f));
    float32x4_t cosPair = vmulq_laneq_f32(zPair, cosine, 0);
    float32x4_t sinPair = vmulq_laneq_f32(zPair, sine, 0);
    float32x4_t axisY =
        vaddq_f32(vmulq_f32(cosPair, float32x4_t{-1.0f, 1.0f, 0.0f, 0.0f}), vmulq_laneq_f32(shared, sine, 0));
    float32x4_t axisZ = vaddq_f32(vmulq_f32(sinPair, float32x4_t{1.0f, -1.0f, 0.0f, 0.0f}),
                                  vmulq_laneq_f32(shared, cosine, 0));
    util::Matrix4x3fType matrix;
    matrix._m.val[0] = axisX;
    matrix._m.val[1] = axisY;
    matrix._m.val[2] = axisZ;
    matrix._m.val[3] = vdupq_n_f32(0.0f);
    return matrix;
}
/**
 * @brief Select four floating-point lanes from two vectors using packed byte tables.
 * @tparam x Source lane for the first component, in the range [0, 8).
 * @tparam y Source lane for the second component, in the range [0, 8).
 * @tparam z Source lane for the third component, in the range [0, 8).
 * @tparam w Source lane for the fourth component, in the range [0, 8).
 * @param first Source lanes zero through three.
 * @param second Source lanes four through seven.
 * @return Vector containing the selected lanes without floating-point conversion.
 */
template <int x, int y, int z, int w>
inline float32x4_t PermuteTransformLanes(float32x4_t first, float32x4_t second) {
    uint8x16x2_t table = {{vreinterpretq_u8_f32(first), vreinterpretq_u8_f32(second)}};
    uint8x8_t low = {x * 4, x * 4 + 1, x * 4 + 2, x * 4 + 3,
                    y * 4, y * 4 + 1, y * 4 + 2, y * 4 + 3};
    uint8x8_t high = {z * 4, z * 4 + 1, z * 4 + 2, z * 4 + 3,
                     w * 4, w * 4 + 1, w * 4 + 2, w * 4 + 3};
    return vreinterpretq_f32_u8(vcombine_u8(vqtbl2_u8(table, low), vqtbl2_u8(table, high)));
}
/**
 * @brief Select four lanes from one vector without numerical conversion.
 * @tparam x Source lane for the first component, in the range [0, 4).
 * @tparam y Source lane for the second component, in the range [0, 4).
 * @tparam z Source lane for the third component, in the range [0, 4).
 * @tparam w Source lane for the fourth component, in the range [0, 4).
 * @param value Vector supplying the four source lanes.
 * @return Vector containing the requested lane ordering.
 */
template <int x, int y, int z, int w>
inline float32x4_t PermuteTransformLanes(float32x4_t value) {
    uint8x8_t low = {x * 4, x * 4 + 1, x * 4 + 2, x * 4 + 3,
                    y * 4, y * 4 + 1, y * 4 + 2, y * 4 + 3};
    uint8x8_t high = {z * 4, z * 4 + 1, z * 4 + 2, z * 4 + 3,
                     w * 4, w * 4 + 1, w * 4 + 2, w * 4 + 3};
    uint8x8_t lo = vqtbl1_u8(vreinterpretq_u8_f32(value), low);
    if (x == 1 && y == 0) {
        lo = vreinterpret_u8_f32(vext_f32(vget_low_f32(value), vget_low_f32(value), 1));
    }
    return vreinterpretq_f32_u8(vcombine_u8(lo, vqtbl1_u8(vreinterpretq_u8_f32(value), high)));
}
/**
 * @brief Construct a rotation matrix from a quaternion without normalizing it.
 * @param quaternion Quaternion components in x, y, z, w order; normally unit length.
 * @return Padded rotation matrix with a zero translation row.
 */
inline util::Matrix4x3fType MakeQuaternionRotation(float32x4_t quaternion) {
    float32x4_t xyz = vreinterpretq_f32_u32(
        vandq_u32(vreinterpretq_u32_f32(quaternion), uint32x4_t{~0u, ~0u, ~0u, 0}));
    float32x4_t yxx = PermuteTransformLanes<1, 0, 0, 3>(xyz);
    float32x4_t zzy = PermuteTransformLanes<2, 2, 1, 3>(xyz);
    float32x4_t products = vmulq_f32(PermuteTransformLanes<1, 2, 0, 3>(xyz), xyz);
    float32x4_t wProducts = vmulq_laneq_f32((PermuteTransformLanes<2, 0, 1, 3>(xyz)), quaternion, 3);
    yxx = vmulq_f32(yxx, yxx);
    zzy = vmulq_f32(zzy, zzy);
    yxx = vaddq_f32(yxx, yxx);
    zzy = vaddq_f32(zzy, zzy);
    products = vaddq_f32(products, products);
    wProducts = vaddq_f32(wProducts, wProducts);
    float32x4_t diagonal = vsubq_f32(float32x4_t{1.0f, 1.0f, 1.0f, 0.0f}, vaddq_f32(zzy, yxx));
    float32x4_t sum = vaddq_f32(products, wProducts);
    float32x4_t difference = vsubq_f32(products, wProducts);
    float32x4_t yz = PermuteTransformLanes<2, 5, 7, 7>(sum, difference);
    float32x4_t xy = PermuteTransformLanes<0, 6, 4, 1>(sum, difference);
    util::Matrix4x3fType matrix;
    matrix._m.val[0] = PermuteTransformLanes<0, 4, 5, 3>(diagonal, xy);
    matrix._m.val[1] = PermuteTransformLanes<6, 1, 7, 3>(diagonal, xy);
    matrix._m.val[2] = PermuteTransformLanes<4, 5, 2, 3>(diagonal, yz);
    matrix._m.val[3] = vdupq_n_f32(0.0f);
    return matrix;
}
} // namespace nn::g3d::detail
