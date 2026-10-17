#pragma once

#include <arm_neon.h>
#include <nn/util/util_MathTypes.h>

namespace nn::util {

namespace neon {

/** @brief Two-lane SIMD vector backing nn::util::Vector2f. */
struct Vector2fType {
    float32x2_t _v;
};

}  // namespace neon

typedef neon::Vector2fType Vector2fType;

/** @brief Two-component float vector held in a 64-bit NEON register. */
class Vector2f : public Vector2fType {
public:
    /** @brief Leaves the components uninitialized. */
    Vector2f() {}

    /**
     * @brief Builds a vector from two components.
     * @param x Horizontal component.
     * @param y Vertical component.
     */
    Vector2f(float x, float y) {
        float32x2_t v = vdup_n_f32(x);
        _v = vset_lane_f32(y, v, 1);
    }

    /**
     * @brief Wraps a raw NEON register.
     * @param v Lanes to copy.
     */
    explicit Vector2f(float32x2_t v) { _v = v; }

    /** @brief Returns the horizontal component. */
    float GetX() const { return vget_lane_f32(_v, 0); }

    /** @brief Returns the vertical component. */
    float GetY() const { return vget_lane_f32(_v, 1); }
};

/**
 * @brief Loads a vector from a packed float pair.
 * @param pOutValue Destination vector.
 * @param rSource Source pair.
 */
inline void VectorLoad(Vector2fType* pOutValue, const Float2& rSource) {
    pOutValue->_v = vld1_f32(rSource.v);
}

/**
 * @brief Stores a vector into a packed float pair.
 * @param pOutValue Destination pair.
 * @param rSource Source vector.
 */
inline void VectorStore(Float2* pOutValue, const Vector2fType& rSource) {
    vst1_f32(pOutValue->v, rSource._v);
}

/**
 * @brief Adds two vectors.
 * @param pOutValue Receives a + b.
 */
inline void VectorAdd(Vector2fType* pOutValue, const Vector2fType& a, const Vector2fType& b) {
    pOutValue->_v = vadd_f32(a._v, b._v);
}

/**
 * @brief Subtracts two vectors.
 * @param pOutValue Receives a - b.
 */
inline void VectorSubtract(Vector2fType* pOutValue, const Vector2fType& a, const Vector2fType& b) {
    pOutValue->_v = vsub_f32(a._v, b._v);
}

/**
 * @brief Scales a vector.
 * @param pOutValue Receives v * scale.
 */
inline void VectorMultiply(Vector2fType* pOutValue, const Vector2fType& v, float scale) {
    pOutValue->_v = vmul_n_f32(v._v, scale);
}

/** @brief Returns the dot product of two vectors. */
inline float VectorDot(const Vector2fType& a, const Vector2fType& b) {
    float32x2_t product = vmul_f32(a._v, b._v);
    return vget_lane_f32(vpadd_f32(product, product), 0);
}

/** @brief Returns the z component of the cross product of two vectors. */
inline float VectorCross(const Vector2fType& a, const Vector2fType& b) {
    const float32x2_t sign = {1.0f, -1.0f};
    float32x2_t product = vmul_f32(vmul_f32(a._v, vrev64_f32(b._v)), sign);
    return vget_lane_f32(vpadd_f32(product, product), 0);
}

/** @brief Returns the squared length of a vector. */
inline float VectorLengthSquared(const Vector2fType& v) {
    return VectorDot(v, v);
}

/** @brief Returns the length of a vector. */
inline float VectorLength(const Vector2fType& v) {
    float32x2_t product = vmul_f32(v._v, v._v);
    return vget_lane_f32(vsqrt_f32(vpadd_f32(product, product)), 0);
}

/**
 * @brief Normalizes a vector with two Newton-Raphson reciprocal square root steps.
 * @param pOutValue Receives the unit vector, or zero for a zero-length input.
 * @param v Vector to normalize.
 */
inline void VectorNormalize(Vector2fType* pOutValue, const Vector2fType& v) {
    float32x2_t product = vmul_f32(v._v, v._v);
    float32x2_t lengthSquared = vpadd_f32(product, product);
    float32x2_t estimate = vrsqrte_f32(lengthSquared);
    estimate = vmul_f32(estimate, vrsqrts_f32(estimate, vmul_f32(lengthSquared, estimate)));
    estimate = vmul_f32(estimate, vrsqrts_f32(estimate, vmul_f32(lengthSquared, estimate)));
    uint32x2_t isZero = vceq_f32(lengthSquared, vdup_n_f32(0.0f));
    uint32x2_t result = vreinterpret_u32_f32(vmul_f32(v._v, estimate));
    pOutValue->_v = vreinterpret_f32_u32(vand_u32(result, vmvn_u32(isZero)));
}

}  // namespace nn::util
