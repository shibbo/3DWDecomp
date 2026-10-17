#include "Library/Math/MathUtil.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <nn/os.h>
#include <prim/seadBitUtil.h>
#include <random/seadGlobalRandom.h>

#include "Project/Math/FractalGenerator.hpp"
#include "Library/Math/MatrixUtil.hpp"

namespace al {
u32 calcHashCode(const char* pStr);
static f32 hermite(f32 y0, f32 m0, f32 y1, f32 m1, f32 t);
static void calcReverseVector(sead::Vector3f* outVec, const sead::Vector3f& normal, f32 reboundRate);
inline f32 calcAngleRadian(const sead::Vector3f& a, const sead::Vector3f& b) {
    f32 dot = a.dot(b);
    sead::Vector3f cross;
    cross.setCross(a, b);
    return sead::Mathf::atan2(cross.length(), dot);
}

inline f32 clamp(f32 value, f32 low, f32 high) {
    f32 result = high;

    if (value < low)
        result = low;
    else if (!(value > high))
        result = value;
    return result;
}

/**
 * Interpolates between `y0` and `y1` as `t` goes from 0.0 to 1.0. This interpolation is defined by
 * `m0` and `m1`, which are the rates of change of `t` at the points `y0` and `y1` respectively.
 */
static f32 hermite(f32 y0, f32 m0, f32 y1, f32 m1, f32 t) {
    f32 coef_m1 = t * (t * t - t);
    f32 coef_y1 = t * t + -2.0f * coef_m1;
    f32 coef_m0 = coef_m1 - (t * t - t);
    return y0 - coef_y1 * y0 + coef_y1 * y1 + coef_m0 * m0 + coef_m1 * m1;
}

static bool getAxisAngleFromTwoVec(sead::Vector3f* pOutAxis, f32* pOutRadian,
                                   const sead::Vector3f& rVecA, const sead::Vector3f& rVecB) {
    pOutAxis->setCross(rVecA, rVecB);

    if (isNearZero(*pOutAxis)) {
        return false;
    }

    pOutAxis->normalize();
    *pOutRadian = sead::Mathf::acos(sead::Mathf::clamp(rVecA.dot(rVecB), -1.0f, 1.0f));
    if (isNearZero(*pOutRadian)) {
        return false;
    }

    return true;
}

// Inline from sead outQuat->setAxisRadian(axis,radian) but with different store method
inline void makeQuatRotateRadian(sead::Quatf* outQuat, const sead::Vector3f& axis, f32 radian) {
    f32 halfRadian = radian * 0.5f;
    f32 cos = sead::Mathf::cos(halfRadian);
    f32 sin = sead::Mathf::sin(halfRadian);

    outQuat->set(cos, sin * axis.x, sin * axis.y, sin * axis.z);
}

static void calcReverseVector(sead::Vector3f* outVec, const sead::Vector3f& normal, f32 reboundRate) {
    *outVec -= normal.dot(*outVec) * normal * (reboundRate + 1.0f);
}

static u32 sPrimeNumbers[64] = {2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,
                                43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101,
                                103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167,
                                173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239,
                                241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311};

constexpr u32 signMaskF32 = 0x80000000;      // 1 bit shift 31
constexpr u32 exponentMaskF32 = 0x7f800000;  // 8 bit shift 23
constexpr u32 mantissaMaskF32 = 0x007fffff;  // 23 bit
constexpr u32 biasF32 = 127;
constexpr u32 specialF32 = exponentMaskF32;

constexpr u16 signMaskF16 = 0x8000;      // 1 bit shift 15
constexpr u16 exponentMaskF16 = 0x7c00;  // 5 bit shift 10
constexpr u16 mantissaMaskF16 = 0x03ff;  // 10 bit
constexpr u16 biasF16 = 15;
constexpr u16 specialF16 = exponentMaskF16;

const s32 bayerMatrix2[2][2] = {{0, 2}, {3, 1}};

f32 calcAngleDegree(const sead::Vector3f& a, const sead::Vector3f& b) {
    return sead::Mathf::rad2deg(calcAngleRadian(a, b));
}

f32 calcAngleDegree(const sead::Vector2f& a, const sead::Vector2f& b) {
    if (isNearZero(a) || isNearZero(b))
        return 0.0f;

    return sead::Mathf::rad2deg(sead::Mathf::atan2(a.cross(b), a.dot(b)));
}

bool isNearZero(const sead::Vector2f& vec, f32 tolerance) {
    return vec.squaredLength() < tolerance * tolerance;
}

bool tryCalcAngleDegree(f32* out, const sead::Vector3f& a, const sead::Vector3f& b) {
    if (isNearZero(a) || isNearZero(b))
        return false;

    *out = calcAngleDegree(a, b);
    return true;
}

bool isNearZero(const sead::Vector3f& vec, f32 tolerance) {
    return vec.squaredLength() < tolerance * tolerance;
}

f32 calcAngleOnPlaneDegree(const sead::Vector3f& a, const sead::Vector3f& b,
                           const sead::Vector3f& vertical) {
    sead::Vector3f planeA;
    verticalizeVec(&planeA, vertical, a);
    sead::Vector3f planeB;
    verticalizeVec(&planeB, vertical, b);
    f32 dot = planeA.dot(planeB);
    sead::Vector3f cross;
    cross.setCross(planeA, planeB);
    f32 angle = sead::Mathf::rad2deg(sead::Mathf::atan2(cross.length(), dot));
    return vertical.dot(cross) < 0.0f ? -angle : angle;
}

/**
 * Takes the plane perpendicular to unit vector `vertical`, projects `vec` onto it, and
 * stores the result in `out`. The effect is that `vec` and `out` will look equal
 * if looking in the direction of `vertical`.
 */
void verticalizeVec(sead::Vector3f* out, const sead::Vector3f& vertical,
                    const sead::Vector3f& vec) {
    out->setScaleAdd(-vertical.dot(vec), vertical, vec);
}

bool isNearAngleRadian(const sead::Vector3f& a, const sead::Vector3f& b, f32 tolerance) {
    if (isNearZero(a))
        return false;
    if (isNearZero(b))
        return false;

    sead::Vector3f aNorm;
    normalize(&aNorm, a);
    sead::Vector3f bNorm;
    normalize(&bNorm, b);

    return aNorm.dot(bNorm) >= sead::Mathf::cos(tolerance);
}

void normalize(sead::Vector3f* out, const sead::Vector3f& vec) {
    *out = vec;
    normalize(out);
}

bool isNearAngleDegree(const sead::Vector3f& a, const sead::Vector3f& b, f32 tolerance) {
    return isNearAngleRadian(a, b, sead::Mathf::deg2rad(tolerance));
}

// TODO: Rename parameters here and in header
bool isNearAngleRadianHV(const sead::Vector3f& rVec, const sead::Vector3f& rTarget,
                         const sead::Vector3f& rUp, f32 angleH, f32 angleV) {
    sead::Vector3f dir;

    if (!tryNormalizeOrZero(&dir, rVec)) {
        return false;
    }

    sead::Vector3f targetDir;

    if (!tryNormalizeOrZero(&targetDir, rTarget)) {
        return false;
    }

    sead::Vector3f up;

    if (!tryNormalizeOrZero(&up, rUp)) {
        return false;
    }

    sead::Vector3f dirH;
    verticalizeVec(&dirH, rUp, rVec);
    tryNormalizeOrZero(&dirH);

    if (dirH.dot(targetDir) < sead::Mathf::cos(angleH)) {
        return false;
    }

    f32 limitV = sead::Mathf::clampMax(angleV, sead::Mathf::piHalf());
    f32 dotV = sead::Mathf::abs(dir.dot(up));
    return !(dotV > sead::Mathf::abs(sead::Mathf::sin(limitV)));
}

/**
 * Copies a vector into the output and normalizes it, or sets it to zero if it is near zero.
 * @param pOut the output vector
 * @param rVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrZero(sead::Vector3f* pOut, const sead::Vector3f& rVec) {
    *pOut = rVec;
    return normalizeOrZero(pOut);
}

/**
 * Normalizes a vector, or sets it to zero if it is near zero.
 * @param pVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrZero(sead::Vector3f* pVec) {
    if (isNearZero(*pVec)) {
        pVec->set(0.0f, 0.0f, 0.0f);
        return true;
    }

    pVec->normalize();
    return false;
}

bool isNearAngleDegreeHV(const sead::Vector3f& a, const sead::Vector3f& b, const sead::Vector3f& c,
                         f32 d, f32 e) {
    return isNearAngleRadianHV(a, b, c, sead::Mathf::deg2rad(d), sead::Mathf::deg2rad(e));
}

bool isNear(f32 value, f32 target, f32 tolerance) {
    return sead::Mathf::abs(value - target) < sead::Mathf::abs(tolerance);
}

bool isNear(const sead::Vector2f& value, const sead::Vector2f& target, f32 tolerance) {
    return (value - target).length() <= tolerance;
}

bool isNear(const sead::Vector3f& value, const sead::Vector3f& target, f32 tolerance) {
    return (value - target).length() <= tolerance;
}

bool isNear(const sead::Color4f& value, const sead::Color4f& target, f32 tolerance) {
    return sead::Mathf::abs(value.r - target.r) < tolerance &&
           sead::Mathf::abs(value.g - target.g) < tolerance &&
           sead::Mathf::abs(value.b - target.b) < tolerance &&
           sead::Mathf::abs(value.a - target.a) < tolerance;
}

bool isNearZero(f32 value, f32 tolerance) {
    return sead::Mathf::abs(value) < tolerance;
}

bool isNearZero(const sead::Matrix34f& value, f32 tolerance) {
    sead::Vector3f vec;

    value.getBase(vec, 0);

    if (isNearZero(vec, tolerance))
        return true;
    value.getBase(vec, 1);

    if (isNearZero(vec, tolerance))
        return true;
    value.getBase(vec, 2);

    if (isNearZero(vec, tolerance))
        return true;

    return false;
}

bool isNearZeroOrLess(f32 value, f32 tolerance) {
    return value <= 0.0f || isNearZero(value, tolerance);
}

bool isExistNearZeroVal(const sead::Vector3f& vec, f32 tolerance) {
    return isNearZero(vec.x, tolerance) || isNearZero(vec.y, tolerance) ||
           isNearZero(vec.z, tolerance);
}

bool isNormalize(const sead::Vector3f& vec, f32 tolerance) {
    return sead::Mathf::abs(1.0f - vec.length()) <= tolerance;
}

bool isNormalize(const sead::Matrix34f& rMtx) {
    sead::Vector3f scale(1.0f, 1.0f, 1.0f);
    calcMtxScale(&scale, rMtx);

    if (!isNearZero(1.0f - scale.x) || !isNearZero(1.0f - scale.y) ||
        !isNearZero(1.0f - scale.z)) {
        return false;
    }

    sead::Vector3f side = rMtx.getBase(0);
    sead::Vector3f up = rMtx.getBase(1);
    sead::Vector3f front = rMtx.getBase(2);
    return isNearZero(side.dot(up)) && isNearZero(side.dot(front)) && isNearZero(up.dot(front));
}

bool isParallelDirection(const sead::Vector3f& a, const sead::Vector3f& b, f32 tolerance) {
    if (sead::Mathf::abs(a.y * b.z - a.z * b.y) > tolerance)
        return false;
    if (sead::Mathf::abs(a.z * b.x - a.x * b.z) > tolerance)
        return false;
    if (sead::Mathf::abs(a.x * b.y - a.y * b.x) > tolerance)
        return false;
    return true;
}

bool isReverseDirection(const sead::Vector3f& a, const sead::Vector3f& b, f32 tolerance) {
    if (a.dot(b) >= 0.0f)
        return false;

    return isParallelDirection(a, b, tolerance);
}

bool isNearDirection(const sead::Vector3f& a, const sead::Vector3f& b, f32 tolerance) {
    if (a.dot(b) < 0.0f)
        return false;

    return isParallelDirection(a, b, tolerance);
}

bool isInRange(f32 x, f32 a, f32 b) {
    if (b < a) {
        if (x < b || a < x)
            return false;
        return true;
    } else {
        if (x < a || b < x)
            return false;
        return true;
    }
}

void normalize(sead::Vector2f* vec) {
    vec->normalize();
}

void normalize(sead::Vector3f* vec) {
    vec->normalize();
}

void normalize(sead::Matrix33f* mtx) {
    sead::Vector3f up = mtx->getBase(0);
    sead::Vector3f front = mtx->getBase(1);
    sead::Vector3f side = mtx->getBase(2);

    up.normalize();
    front.normalize();
    side.normalize();

    mtx->setBase(0, up);
    mtx->setBase(1, front);
    mtx->setBase(2, side);
}

void normalize(sead::Matrix34f* mtx) {
    sead::Vector3f up = mtx->getBase(0);
    sead::Vector3f front = mtx->getBase(1);
    sead::Vector3f side = mtx->getBase(2);

    up.normalize();
    front.normalize();
    side.normalize();

    mtx->setBase(0, up);
    mtx->setBase(1, front);
    mtx->setBase(2, side);
}

void normalize(sead::Vector2f* out, const sead::Vector2f& vec) {
    *out = vec;
    normalize(out);
}

/**
 * Normalizes a vector, or sets it to zero if it is near zero.
 * @param pVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrZero(sead::Vector2f* pVec) {
    if (isNearZero(*pVec)) {
        pVec->set(0.0f, 0.0f);
        return true;
    }

    pVec->normalize();
    return false;
}

/**
 * Copies a vector into the output and normalizes it, or sets it to zero if it is near zero.
 * @param pOut the output vector
 * @param rVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrZero(sead::Vector2f* pOut, const sead::Vector2f& rVec) {
    *pOut = rVec;
    return normalizeOrZero(pOut);
}

/**
 * Normalizes a vector, or sets it to the Z axis if it is near zero.
 * @param pVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrDirZ(sead::Vector3f* pVec) {
    if (normalizeOrZero(pVec)) {
        pVec->set(sead::Vector3f::ez);
        return true;
    }

    return false;
}

/**
 * Copies a vector into the output and normalizes it, or sets it to the Z axis if it is near zero.
 * @param pOut the output vector
 * @param rVec the vector to normalize
 * @return true if the vector was near zero
 */
bool normalizeOrDirZ(sead::Vector3f* pOut, const sead::Vector3f& rVec) {
    *pOut = rVec;
    return normalizeOrDirZ(pOut);
}

bool tryNormalizeOrZero(sead::Vector2f* vec) {
    if (isNearZero(*vec)) {
        vec->set(0.0f, 0.0f);
        return false;
    }

    normalize(vec);
    return true;
}

bool tryNormalizeOrZero(sead::Vector3f* vec) {
    if (isNearZero(*vec)) {
        vec->set(0.0f, 0.0f, 0.0f);
        return false;
    }

    normalize(vec);
    return true;
}

bool tryNormalizeOrZero(sead::Vector2f* out, const sead::Vector2f& vec) {
    *out = vec;
    return tryNormalizeOrZero(out);
}

bool tryNormalizeOrZero(sead::Vector3f* out, const sead::Vector3f& vec) {
    *out = vec;
    return tryNormalizeOrZero(out);
}

bool tryNormalizeOrDirZ(sead::Vector3f* vec) {
    if (!tryNormalizeOrZero(vec)) {
        vec->set(sead::Vector3f::ez);
        return false;
    }

    return true;
}

bool tryNormalizeOrDirZ(sead::Vector3f* out, const sead::Vector3f& vec) {
    *out = vec;
    return tryNormalizeOrDirZ(out);
}

/**
 * Finds the axis with the largest absolute component, checking X against Y before Z.
 * @param rVec vector to inspect
 * @return 0, 1 or 2 for the X, Y or Z axis
 */
static s32 calcMaxAbsAxis(const sead::Vector3f& rVec) {
    f32 x = sead::Mathf::abs(rVec.x);
    f32 y = sead::Mathf::abs(rVec.y);
    f32 z = sead::Mathf::abs(rVec.z);

    bool isNotMaxX = !(x > y) || !(x > z);

    if (isNotMaxX) {
        return y > z ? 1 : 2;
    }

    return 0;
}

/**
 * Normalizes the three base axes of a matrix and rebuilds any degenerate (near zero) axes from
 * the remaining ones so that the result is a complete basis again.
 * @param pMtx matrix to normalize in place
 */
void normalizeComplement(sead::Matrix34f* pMtx) {
    sead::Vector3f axisX = pMtx->getBase(0);
    sead::Vector3f axisY = pMtx->getBase(1);
    sead::Vector3f axisZ = pMtx->getBase(2);

    bool isZeroX = isNearZero(axisX);
    bool isZeroY = isNearZero(axisY);
    bool isZeroZ = isNearZero(axisZ);
    s32 zeroNum = isZeroX + isZeroY + isZeroZ;

    if (!isZeroX) {
        axisX.normalize();
    }

    if (!isZeroY) {
        axisY.normalize();
    }

    if (!isZeroZ) {
        axisZ.normalize();
    }

    switch (zeroNum) {
    case 3:
        axisX.set(sead::Vector3f::ex);
        axisY.set(sead::Vector3f::ey);
        axisZ.set(sead::Vector3f::ez);
        break;
    case 1:
        if (isZeroX) {
            axisX.setCross(axisY, axisZ);
        } else if (isZeroY) {
            axisY.setCross(axisZ, axisX);
        } else if (isZeroZ) {
            axisZ.setCross(axisX, axisY);
        }

        break;
    case 2:
        if (!isZeroX) {
            if (calcMaxAbsAxis(axisX) != 1) {
                axisY.set(sead::Vector3f::ey);
                axisZ.setCross(axisX, axisY);
            } else {
                axisZ.set(sead::Vector3f::ez);
                axisY.setCross(axisZ, axisX);
            }
        } else if (!isZeroY) {
            if (calcMaxAbsAxis(axisY) != 2) {
                axisZ.set(sead::Vector3f::ez);
                axisX.setCross(axisY, axisZ);
            } else {
                axisX.set(sead::Vector3f::ex);
                axisZ.setCross(axisX, axisY);
            }
        } else if (!isZeroZ) {
            if (getMaxAbsElementIndex(axisZ) == 0) {
                axisY.set(sead::Vector3f::ey);
                axisX.setCross(axisY, axisZ);
            } else {
                axisX.set(sead::Vector3f::ex);
                axisY.setCross(axisZ, axisX);
            }
        }

        break;
    }

    pMtx->setBase(0, axisX);
    pMtx->setBase(1, axisY);
    pMtx->setBase(2, axisZ);
}

u32 getMaxAbsElementIndex(const sead::Vector3f& vec) {
    f32 x = sead::Mathf::abs(vec.x);
    f32 y = sead::Mathf::abs(vec.y);
    f32 z = sead::Mathf::abs(vec.z);

    return x > z && x > y ? 0 : y > z ? 1 : 2;
}

void setLength(sead::Vector3f* vec, f32 length) {
    f32 curLen = vec->length();

    if (curLen > 0.0f) {
        f32 scale = length / curLen;
        *vec *= scale;
    }
}

void setProjectionLength(sead::Vector3f* out, const sead::Vector3f& vec, f32 length) {
    f32 scale = length / sead::Mathf::abs(vec.dot(*out));
    *out *= scale;
}

bool limitLength(sead::Vector2f* out, const sead::Vector2f& vec, f32 limit) {
    f32 len = vec.length();

    if (len > limit) {
        f32 invLen = limit / len;
        out->setScale(vec, invLen);
        return true;
    } else {
        out->set(vec);
        return false;
    }
}

bool limitLength(sead::Vector3f* out, const sead::Vector3f& vec, f32 limit) {
    f32 len = vec.length();

    if (len > limit) {
        f32 invLen = limit / len;
        out->setScale(vec, invLen);
        return true;
    } else {
        out->set(vec);
        return false;
    }
}

f32 normalizeAbs(f32 x, f32 min, f32 max) {
    if (x >= 0)
        return normalize(x, min, max);
    else
        return -normalize(-x, min, max);
}

f32 normalize(f32 x, f32 min, f32 max) {
    if (sead::Mathf::abs(max - min) < 0.001f) {
        if (x < min)
            return 0.0f;
        else
            return 1.0f;
    }

    f32 clamped = sead::Mathf::clamp(x, min, max);
    return (clamped - min) / (max - min);
}

/**
 * Returns the sign of a value.
 * @param value the value
 * @return -1, 1, or the value itself if it is zero
 */
f32 sgn(f32 value) {
    if (value < 0.0f) {
        return -1.0f;
    }

    if (value > 0.0f) {
        return 1.0f;
    }

    return value;
}

/**
 * Returns the sign of a value.
 * @param value the value
 * @return -1, 1 or 0
 */
s32 sgn(s32 value) {
    if (value < 0) {
        return -1;
    }

    if (value > 0) {
        return 1;
    }

    return value;
}

void clampV3f(sead::Vector3f* out, const sead::Vector3f& min, const sead::Vector3f& max) {
    out->x = sead::Mathf::clamp(out->x, min.x, max.x);
    out->y = sead::Mathf::clamp(out->y, min.y, max.y);
    out->z = sead::Mathf::clamp(out->z, min.z, max.z);
}

void clampV2f(sead::Vector2f* out, const sead::Vector2f& min, const sead::Vector2f& max) {
    out->x = sead::Mathf::clamp(out->x, min.x, max.x);
    out->y = sead::Mathf::clamp(out->y, min.y, max.y);
}

f32 calcRate01(f32 t, f32 min, f32 max) {
    f32 range = max - min;

    if (isNearZero(range))
        return 1.0f;
    return sead::Mathf::clamp((t - min) / range, 0.0f, 1.0f);
}

f32 easeIn(f32 t) {
    return (((t * -0.5f) + 1.5f) * t) * t;
}

f32 easeOut(f32 t) {
    return (((t * -0.5f) * t) + 1.5f) * t;
}

f32 easeInOut(f32 t) {
    return (((t * -2.0f) + 3.0f) * t) * t;
}

f32 squareIn(f32 t) {
    return t * t;
}

f32 squareOut(f32 t) {
    return (2.0f - t) * t;
}

f32 easeByType(f32 t, s32 easeType) {
    switch (easeType) {
    case EaseType_EaseIn:
        return easeIn(t);
    case EaseType_EaseOut:
        return easeOut(t);
    case EaseType_EaseInOut:
        return easeInOut(t);
    case EaseType_SquareIn:
        return squareIn(t);
    case EaseType_SquareOut:
        return squareOut(t);
    default:
        return t;
    }
}

f32 hermiteRate(f32 t, f32 m0, f32 m1) {
    return hermite(0.0f, m0, 1.0f, m1, t);
}

f32 lerpValueNew(f32 a, f32 b, f32 t) {
    t = sead::Mathf::clamp(t, 0.0f, 1.0f);
    return a * (1.0f - t) + t * b;
}

f32 lerpValue(f32 t, f32 a, f32 b) {
    t = sead::Mathf::clamp(t, 0.0f, 1.0f);
    return (1.0f - t) * a + t * b;
}

f32 lerpValue(f32 a, f32 b, f32 t, f32 clampA, f32 clampB) {
    if (sead::Mathf::abs(t - b) < 0.001f)
        return a <= b ? clampA : clampB;

    f32 rate = (a - b) / (t - b);
    f32 t2 = clamp(rate, 0.0f, 1.0f);

    return clampA * (1.0f - t2) + t2 * clampB;
}

f32 lerpDegree(f32 a, f32 b, f32 t) {
    a = wrapAngle(a);
    b = wrapAngle(b);

    f32 aa = b - a > 180.0f ? a + 360.0f : a;
    f32 bb = b - a < -180.0f ? b + 360.0f : b;

    return wrapAngle(lerpValueNew(aa, bb, t));
}

f32 lerpRadian(f32 a, f32 b, f32 t) {
    a = wrapValue(a, sead::Mathf::pi2());
    b = wrapValue(b, sead::Mathf::pi2());

    f32 aa = b - a > sead::Mathf::pi() ? a + sead::Mathf::pi2() : a;
    f32 bb = b - a < -sead::Mathf::pi() ? b + sead::Mathf::pi2() : b;

    return wrapValue(lerpValueNew(aa, bb, t), sead::Mathf::pi2());
}

void lerpVec(sead::Vector2f* outVec, const sead::Vector2f& a, const sead::Vector2f& b, f32 t) {
    outVec->x = a.x + (b.x - a.x) * t;
    outVec->y = a.y + (b.y - a.y) * t;
}

void lerpVec(sead::Vector3f* outVec, const sead::Vector3f& a, const sead::Vector3f& b, f32 t) {
    outVec->x = a.x + (b.x - a.x) * t;
    outVec->y = a.y + (b.y - a.y) * t;
    outVec->z = a.z + (b.z - a.z) * t;
}

s32 converge(s32 current, s32 target, s32 step) {
    s32 result = current;

    if (current < target) {
        result += step;

        if (result > target)
            result = target;
    } else {
        result -= step;

        if (result < target)
            result = target;
    }

    return result;
}

f32 converge(f32 current, f32 target, f32 step) {
    f32 result = current;

    if (current < target) {
        result += step;

        if (result > target)
            result = target;
    } else {
        result -= step;

        if (result < target)
            result = target;
    }

    return result;
}

f32 convergeDegree(f32 current, f32 target, f32 step) {
    if ((target + 360.0f) - current < 180.0f)
        target += 360.0f;
    else if (current - (target - 360.0f) < 180.0f)
        target -= 360.0f;

    return wrapAngle(converge(current, target, step));
}

f32 convergeRadian(f32 current, f32 target, f32 step) {
    // BUG: N's mistake here. Correct comparison: (target + pi2()) - current < pi()
    if ((target + sead::Mathf::pi2()) - current < sead::Mathf::pi2())
        target += sead::Mathf::pi2();
    else if (current - (target - sead::Mathf::pi2()) < sead::Mathf::pi())
        target -= sead::Mathf::pi2();

    return wrapValue(converge(current, target, step), sead::Mathf::pi2());
}

void convergeVec(sead::Vector2f* outVec, const sead::Vector2f& current,
                 const sead::Vector2f& target, f32 step) {
    sead::Vector2f dir = target - current;

    f32 length = dir.length();

    if (length > step) {
        dir *= step / length;
    }

    outVec->setAdd(current, dir);
}

void convergeVec(sead::Vector3f* outVec, const sead::Vector3f& current,
                 const sead::Vector3f& target, f32 step) {
    sead::Vector3f dir = target - current;

    f32 length = dir.length();

    if (length > step) {
        dir *= step / length;
    }

    outVec->setAdd(current, dir);
}

f32 diffNearAngleDegree(f32 a, f32 b) {
    f32 wrappedA = wrapAngle(a);
    f32 diff = wrapAngle(b) - wrappedA;

    if (diff > 180.0f) {
        return diff - 360.0f;
    }

    if (diff < -180.0f) {
        return diff + 360.0f;
    }

    return diff;
}

/**
 * Interpolates between two values with a cosine curve.
 * @param t the interpolation rate
 * @param a the start value
 * @param b the end value
 * @return the interpolated value
 */
f32 cosInterpolation(f32 t, f32 a, f32 b) {
    f32 rate = (1.0f - sead::Mathf::cos(t * sead::Mathf::pi())) * 0.5f;
    return (1.0f - rate) * a + rate * b;
}

f32 sign(f32 x) {
    if (x < 0.0f)
        return -1.0f;
    if (x > 0.0f)
        return 1.0f;
    return x;
}

s32 sign(s32 x) {
    if (x < 0)
        return -1;
    if (x > 0)
        return 1;
    return x;
}

f32 cubeRoot(f32 x) {
    f32 onethird = 1.0f / 3.0f;

    u32 i = 0x54a0fc86 - sead::BitUtil::bitCast<u32>(x) / 3;
    f32 y = sead::BitUtil::bitCast<f32>(i);

    y = y * onethird * (4.0f - x * y * y * y);
    y = y * onethird * (4.0f - x * y * y * y);
    y = y * onethird * (4.0f - x * y * y * y);
    return x * y * y;
}

bool isSameSign(f32 a, f32 b) {
    return a * b > 0.0f;
}

u8 reverseBit8(u8 x) {
    x = ((x & 0x55) << 1) | ((x >> 1) & 0x55);  // 0101...
    x = ((x & 0x33) << 2) | ((x >> 2) & 0x33);  // 0011...
    return x << 4 | x >> 4;
}

u16 reverseBit16(u16 x) {
    x = ((x & 0x5555) << 1) | ((x >> 1) & 0x5555);  // 01010101...
    x = ((x & 0x3333) << 2) | ((x >> 2) & 0x3333);  // 00110011...
    x = ((x & 0xf0f) << 4) | ((x >> 4) & 0xf0f);    // 11110000..
    return x << 8 | x >> 8;
}

u32 reverseBit32(u32 x) {
    x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);  // 0101010101010101...
    x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);  // 0011001100110011...
    x = ((x & 0xf0f0f0f) << 4) | ((x >> 4) & 0xf0f0f0f);    // 1111000011110000...
    x = ((x & 0xff00ff) << 8) | ((x >> 8) & 0xff00ff);      // 1111111100000000...
    return x >> 16 | x << 16;
}

void initRandomSeedNonSync(u32 seed) {
    sead::GlobalRandomNonSync::instance()->init(seed);
}

void initRandomSeedByTickNonSync() {
    initRandomSeedNonSync(nn::os::GetSystemTick().value);
}

void initRandomSeedByStringNonSync(const char* pName) {
    initRandomSeedNonSync(calcHashCode(pName));
}

f32 getRandomNonSync() {
    u32 random = (sead::GlobalRandomNonSync::instance()->getU32() >> 9) | 0x3F800000;
    return (*reinterpret_cast<f32*>(&random)) - 1;
}

f32 getRandomNonSync(f32 factor) {
    return getRandomNonSync(0.0f, factor);
}

f32 getRandomNonSync(f32 min, f32 max) {
    return (getRandomNonSync() * (max - min)) + min;
}

f32 getRandom() {
    return sead::GlobalRandom::instance()->getU32() * 2.3283064e-10f;
}

f32 getRandom(f32 factor) {
    return getRandom(0.0f, factor);
}

f32 getRandom(f32 min, f32 max) {
    return (getRandom() * (max - min)) + min;
}

s32 getRandom(s32 factor) {
    return getRandom(0, factor);
}

s32 getRandom(s32 min, s32 max) {
    return (s32)getRandom((f32)min, (f32)max);
}

f32 getRandomDegree() {
    return getRandom(360.0f);
}

f32 getRandomRadian() {
    return getRandom(6.2832f);
}

void getRandomVector(sead::Vector3f* vec, f32 factor) {
    f32 x = (getRandom() * (factor + factor)) - factor;
    f32 y = (getRandom() * (factor + factor)) - factor;
    f32 z = (getRandom() * (factor + factor)) - factor;
    vec->x = x;
    vec->y = y;
    vec->z = z;
}

void getRandomOnCircle(sead::Vector2f* outPos, f32 radius) {
    f32 angle = getRandom(sead::Mathf::pi2());

    outPos->x = sead::Mathf::cos(angle) * radius;
    outPos->y = sead::Mathf::sin(angle) * radius;
}

void getRandomInCircle(sead::Vector2f* outPos, f32 maxRadius) {
    f32 angle = getRandom(sead::Mathf::pi2());
    f32 radius = sead::Mathf::sqrt(getRandom()) * maxRadius;

    outPos->x = radius * sead::Mathf::cos(angle);
    outPos->y = radius * sead::Mathf::sin(angle);
}

void getRandomInCircleMinMaxRadius(sead::Vector2f* outPos, f32 minRadius, f32 maxRadius) {
    f32 angle = getRandom(sead::Mathf::pi2());
    f32 range = sead::Mathf::square(minRadius / maxRadius);
    f32 radius = sead::Mathf::sqrt(range + getRandom() * (1.0f - range)) * maxRadius;

    outPos->x = radius * sead::Mathf::cos(angle);
    outPos->y = radius * sead::Mathf::sin(angle);
}

void getRandomInCircle(sead::Vector3f* outPos, const sead::Vector3f& pos,
                       const sead::Vector3f& front, f32 maxRadius) {
    sead::Matrix34f mtx;
    makeMtxFrontNoSupportPos(&mtx, front, pos);

    sead::Vector2f pos2D;
    getRandomInCircle(&pos2D, maxRadius);

    outPos->setMul(mtx, {pos2D.x, pos2D.y, 0.0f});
}

void getRandomOnSphere(sead::Vector3f* outPos, f32 radius) {
    f32 angle = getRandom(sead::Mathf::pi2());
    f32 zPos = 2.0f * getRandom() - 1.0f;
    f32 radiusXY = sead::Mathf::sqrt(1.0f - sead::Mathf::square(zPos)) * radius;

    outPos->x = sead::Mathf::cos(angle) * radiusXY;
    outPos->y = sead::Mathf::sin(angle) * radiusXY;
    outPos->z = zPos * radius;
}

void getRandomInSphere(sead::Vector3f* pOutPos, f32 maxRadius) {
    f32 angle = getRandom(sead::Mathf::pi2());
    f32 radius = cubeRoot(getRandom()) * maxRadius;
    f32 zPos = 2.0f * getRandom() - 1.0f;
    f32 radiusXY = sead::Mathf::sqrt(1.0f - sead::Mathf::square(zPos));

    pOutPos->x = radiusXY * sead::Mathf::cos(angle) * radius;
    pOutPos->y = radiusXY * sead::Mathf::sin(angle) * radius;
    pOutPos->z = zPos * radius;
}

bool calcRandomDirInCone(sead::Vector3f* pOutDir, const sead::Vector3f& rDir, f32 angle) {
    static const f32 sTwoPi = sead::Mathf::pi2();

    sead::Vector3f side;
    side.setCross(rDir, sead::Vector3f(0.0f, 1.0f, 0.0f));
    side.normalize();

    f32 randPhi = getRandom();
    f32 randZ = getRandom();
    f32 cosAngle = sead::Mathf::cos(angle * 0.0174533f);
    f32 phi = randPhi * sTwoPi;
    f32 z = cosAngle + (1.0f - cosAngle) * randZ;
    f32 sinTheta = std::sqrt(1.0f - z * z);

    sead::Vector3f dir = sead::Vector3f(0.0f, sinTheta * sead::Mathf::cos(phi), 0.0f) +
                         side * (sinTheta * sead::Mathf::sin(phi));
    pOutDir->set(dir + rDir * z);
    return tryNormalizeOrZero(pOutDir);
}

void getRandomInSphereMinMaxRadius(sead::Vector3f* pOutPos, f32 minRadius, f32 maxRadius) {
    f32 angle = getRandom(sead::Mathf::pi2());
    f32 range = minRadius / maxRadius * minRadius / maxRadius * minRadius / maxRadius;
    f32 radius = cubeRoot(range + getRandom() * (1.0f - range)) * maxRadius;
    f32 zPos = 2.0f * getRandom() - 1.0f;
    f32 radiusXY = sead::Mathf::sqrt(1.0f - sead::Mathf::square(zPos));

    pOutPos->x = radiusXY * sead::Mathf::cos(angle) * radius;
    pOutPos->y = radiusXY * sead::Mathf::sin(angle) * radius;
    pOutPos->z = zPos * radius;
}

void initRandomSeed(u32 seed) {
    sead::GlobalRandom::instance()->init(seed);
}

void initRandomSeedByTick() {
    initRandomSeed(nn::os::GetSystemTick().value);
}

void initRandomSeedByString(const char* name) {
    initRandomSeed(calcHashCode(name));
}

bool isHalfProbability() {
    return getRandom() < 0.5f;
}

bool isPercentProbability(f32 threshold) {
    return getRandom() * 100.0f < threshold;
}

void getRandomContext(u32* xSeed, u32* ySeed, u32* zSeed, u32* wSeed) {
    sead::GlobalRandom::instance()->getContext(xSeed, ySeed, zSeed, wSeed);
}

void setRandomContext(u32 xSeed, u32 ySeed, u32 zSeed, u32 wSeed) {
    sead::GlobalRandom::instance()->init(xSeed, ySeed, zSeed, wSeed);
}

f32 modf(f32 a, f32 b) {
    return std::fmodf(a, b);
}

s32 modi(s32 a, s32 b) {
    return a - (a / b) * b;
}

/**
 * Splits a 2D vector into its length and normalized direction.
 * @param pScalar receives the length of the vector
 * @param pDir receives the normalized direction, or zero if the vector is near zero
 * @param rVec vector to split
 * @return true if the vector is near zero
 */
bool separateScalarAndDirection(f32* pScalar, sead::Vector2f* pDir, const sead::Vector2f& rVec) {
    *pScalar = rVec.length();
    if (isNearZero(rVec)) {
        pDir->set(0.0f, 0.0f);
        return true;
    }

    normalize(pDir, rVec);
    return false;
}

bool separateScalarAndDirection(f32* pScalar, sead::Vector3f* pDir, const sead::Vector3f& rVec) {
    *pScalar = rVec.length();
    if (isNearZero(rVec)) {
        pDir->set(0.0f, 0.0f, 0.0f);
        return true;
    }

    normalize(pDir, rVec);
    return false;
}

void limitVectorSeparateHV(sead::Vector3f* pVec, const sead::Vector3f& rDir, f32 limitParallel,
                           f32 limitVertical) {
    sead::Vector3f parallel;
    sead::Vector3f vertical;
    separateVectorHV(&parallel, &vertical, rDir, *pVec);

    if (parallel.squaredLength() > limitParallel * limitParallel) {
        setLength(&parallel, limitParallel);
    }

    if (vertical.squaredLength() > limitVertical * limitVertical) {
        setLength(&vertical, limitVertical);
    }

    pVec->set(parallel + vertical);
}

void parallelizeVec(sead::Vector3f* outVec, const sead::Vector3f& dir, const sead::Vector3f& vec) {
    outVec->setScale(dir, dir.dot(vec));
}

void separateVectorHV(sead::Vector3f* outV, sead::Vector3f* outH, const sead::Vector3f& a,
                      const sead::Vector3f& b) {
    f32 dot = a.dot(b);

    outV->x = a.x * dot;
    outV->y = a.y * dot;
    outV->z = a.z * dot;

    outH->x = b.x - outV->x;
    outH->y = b.y - outV->y;
    outH->z = b.z - outV->z;
}

// computes how many `vec`s are required to go from `origin` to plane
/**
 * Adds a vector while limiting the resulting component along the added direction.
 * @param pVec vector to add to
 * @param rAdd vector to add
 * @param limit maximum component of the result along the direction of rAdd
 * @return true if anything was added
 */
bool addVectorLimit(sead::Vector3f* pVec, const sead::Vector3f& rAdd, f32 limit) {
    f32 addLength;
    sead::Vector3f dir;
    separateScalarAndDirection(&addLength, &dir, rAdd);

    if (isNearZero(dir)) {
        return false;
    }

    f32 dot = pVec->dot(dir);

    if (dot >= limit) {
        return false;
    }

    *pVec += dir * sead::Mathf::min(limit - dot, addLength);
    return true;
}

f32 calcDistanceVecToPlane(const sead::Vector3f& vec, const sead::Vector3f& planePoint,
                           const sead::Vector3f& planeNormal, const sead::Vector3f& origin) {
    f32 originToPlane = planeNormal.dot(planePoint - origin);
    f32 dirProjNormal = -vec.dot(planeNormal);
    f32 vecLength = vec.length();

    return originToPlane / dirProjNormal * vecLength;
}

f32 calcSquaredDistancePointToSegment(const sead::Vector3f& rPoint, const sead::Vector3f& rStart,
                                      const sead::Vector3f& rEnd) {
    sead::Vector3f segment = rEnd - rStart;
    sead::Vector3f toPoint = rPoint - rStart;
    f32 dot = segment.dot(toPoint);

    if (dot <= 0.0f) {
        return toPoint.squaredLength();
    }

    f32 segmentSqLen = segment.squaredLength();

    if (dot >= segmentSqLen) {
        return (rPoint - rEnd).squaredLength();
    }

    return sead::Mathf::clampMin(toPoint.squaredLength() - dot * (dot / segmentSqLen), 0.0f);
}

f32 calcDistancePointToSegment(const sead::Vector3f& rPoint, const sead::Vector3f& rStart,
                               const sead::Vector3f& rEnd) {
    return sead::Mathf::sqrt(calcSquaredDistancePointToSegment(rPoint, rStart, rEnd));
}

void roundOffVec(sead::Vector3f* outVec, const sead::Vector3f& vec) {
    outVec->x = sead::Mathf::round(vec.x);
    outVec->y = sead::Mathf::round(vec.y);
    outVec->z = sead::Mathf::round(vec.z);
}

void roundOffVec(sead::Vector3f* vec) {
    roundOffVec(vec, *vec);
}

void roundOffVec(sead::Vector2f* outVec, const sead::Vector2f& vec) {
    outVec->x = sead::Mathf::round(vec.x);
    outVec->y = sead::Mathf::round(vec.y);
}

void roundOffVec(sead::Vector2f* vec) {
    roundOffVec(vec, *vec);
}

f32 snapToGrid(f32 val, f32 gridSize, f32 offset) {
    return sead::Mathf::round((val - offset) / gridSize) * gridSize + offset;
}

void snapVecToGrid(sead::Vector3f* outVec, const sead::Vector3f& vec, f32 gridSize,
                   const sead::Vector3f& offset) {
    outVec->x = snapToGrid(vec.x, gridSize, offset.x);
    outVec->y = snapToGrid(vec.y, gridSize, offset.y);
    outVec->z = snapToGrid(vec.z, gridSize, offset.z);
}

void snapVecToDirAxisY(sead::Vector3f* pOutVec, const sead::Vector3f& rVec, s32 divNum) {
    sead::Vector3f vecH = rVec;
    verticalizeVec(&vecH, sead::Vector3f::ey, vecH);

    if (isNearZero(vecH)) {
        return;
    }

    f32 unitDegree = 360.0f / divNum;
    f32 angle = calcAngleOnPlaneDegree(sead::Vector3f::ex, vecH, sead::Vector3f::ey);
    f32 index = 0.0f;

    if (!isNearZero(angle)) {
        f32 rate = angle / unitDegree;
        index = static_cast<s32>(rate + (rate >= 0.0f ? 0.5f : -0.5f));
    }

    f32 length = rVec.length();
    f32 radian = sead::Mathf::deg2rad(unitDegree * index);
    pOutVec->set(sead::Mathf::cos(radian), 0.0f, -sead::Mathf::sin(radian));
    setLength(pOutVec, length);
}

u32 getMaxAbsElementIndex(const sead::Vector3i& vec) {
    s32 x = sead::Mathi::abs(vec.x);
    s32 y = sead::Mathi::abs(vec.y);
    s32 z = sead::Mathi::abs(vec.z);

    return x > z && x > y ? 0 : y > z ? 1 : 2;
}

f32 getMaxAbsElementValue(const sead::Vector3f& vec) {
    switch (getMaxAbsElementIndex(vec)) {
    case 0:
        return vec.x;
    case 1:
        return vec.y;
    case 2:
        return vec.z;
    }

    return vec.z;
}

s32 getMaxAbsElementValue(const sead::Vector3i& vec) {
    switch (getMaxAbsElementIndex(vec)) {
    case 0:
        return vec.x;
    case 1:
        return vec.y;
    case 2:
        return vec.z;
    }

    return vec.z;
}

u32 getMinAbsElementIndex(const sead::Vector3f& vec) {
    f32 x = sead::Mathf::abs(vec.x);
    f32 y = sead::Mathf::abs(vec.y);
    f32 z = sead::Mathf::abs(vec.z);

    return x < z && x < y ? 0 : y < z ? 1 : 2;
}

u32 getMinAbsElementIndex(const sead::Vector3i& vec) {
    s32 x = sead::Mathi::abs(vec.x);
    s32 y = sead::Mathi::abs(vec.y);
    s32 z = sead::Mathi::abs(vec.z);

    return x < z && x < y ? 0 : y < z ? 1 : 2;
}

f32 getMinAbsElementValue(const sead::Vector3f& vec) {
    switch (getMinAbsElementIndex(vec)) {
    case 0:
        return vec.x;
    case 1:
        return vec.y;
    case 2:
        return vec.z;
    }

    return vec.z;
}

s32 getMinAbsElementValue(const sead::Vector3i& vec) {
    switch (getMinAbsElementIndex(vec)) {
    case 0:
        return vec.x;
    case 1:
        return vec.y;
    case 2:
        return vec.z;
    }

    return vec.z;
}

Axis calcNearVecFromAxis2(sead::Vector3f* pOutVec, const sead::Vector3f& rVec,
                          const sead::Vector3f& rAxisA, const sead::Vector3f& rAxisB) {
    f32 dotA = rVec.dot(rAxisA);
    f32 dotB = rVec.dot(rAxisB);

    if (sead::Mathf::abs(dotA) > sead::Mathf::abs(dotB)) {
        if (dotA > 0.0f) {
            pOutVec->set(rAxisA);
            return Axis::X;
        }

        pOutVec->set(-rAxisA);
        return Axis::InvertX;
    }

    if (dotB > 0.0f) {
        pOutVec->set(rAxisB);
        return Axis::Y;
    }

    pOutVec->set(-rAxisB);
    return Axis::InvertY;
}

Axis calcNearVecFromAxis3(sead::Vector3f* pOutVec, const sead::Vector3f& rVec,
                          const sead::Vector3f& rAxisA, const sead::Vector3f& rAxisB,
                          const sead::Vector3f& rAxisC) {
    f32 dotA = rVec.dot(rAxisA);
    f32 dotB = rVec.dot(rAxisB);
    f32 dotC = rVec.dot(rAxisC);
    f32 absA = sead::Mathf::abs(dotA);
    f32 absB = sead::Mathf::abs(dotB);
    f32 absC = sead::Mathf::abs(dotC);

    if (absA > absB) {
        if (absA > absC) {
            if (dotA > 0.0f) {
                if (pOutVec) {
                    pOutVec->set(rAxisA);
                }

                return Axis::X;
            }

            if (pOutVec) {
                pOutVec->set(-rAxisA);
            }

            return Axis::InvertX;
        } else {
            if (dotC > 0.0f) {
                if (pOutVec) {
                    pOutVec->set(rAxisC);
                }

                return Axis::Z;
            }

            if (pOutVec) {
                pOutVec->set(-rAxisC);
            }

            return Axis::InvertZ;
        }
    } else {
        if (absB > absC) {
            if (dotB > 0.0f) {
                if (pOutVec) {
                    pOutVec->set(rAxisB);
                }

                return Axis::Y;
            }

            if (pOutVec) {
                pOutVec->set(-rAxisB);
            }

            return Axis::InvertY;
        } else {
            if (dotC > 0.0f) {
                if (pOutVec) {
                    pOutVec->set(rAxisC);
                }

                return Axis::Z;
            }

            if (pOutVec) {
                pOutVec->set(-rAxisC);
            }

            return Axis::InvertZ;
        }
    }
}

void calcDirVerticalAny(sead::Vector3f* pOutVec, const sead::Vector3f& rVec) {
    sead::Vector3f axis = sead::Vector3f::zero;
    f32* element;

    switch (getMinAbsElementIndex(rVec)) {
    case 0:
        element = &axis.x;
        break;
    case 1:
        element = &axis.y;
        break;
    case 2:
        element = &axis.z;
        break;
    default:
        element = &axis.z;
        break;
    }

    *element = 1.0f;
    verticalizeVec(pOutVec, rVec, axis);
    tryNormalizeOrZero(pOutVec);
}

Axis calcNearVecFromAxis3(sead::Vector3f* outVec, const sead::Vector3f& vec,
                          const sead::Quatf& quat) {
    sead::Vector3f side, up, front;
    calcQuatLocalAxisAll(quat, &side, &up, &front);
    return calcNearVecFromAxis3(outVec, vec, side, up, front);
}

void calcQuatLocalAxisAll(const sead::Quatf& quat, sead::Vector3f* outSide, sead::Vector3f* outUp,
                          sead::Vector3f* outFront) {
    sead::Matrix33f mtx;
    mtx.fromQuat(quat);

    outSide->set(mtx.getBase(0));
    outUp->set(mtx.getBase(1));
    outFront->set(mtx.getBase(2));
}

void addRandomVector(sead::Vector3f* pOutVec, const sead::Vector3f& rVec, f32 range) {
    f32 x = getRandom(-range, range);
    f32 y = getRandom(-range, range);
    f32 z = getRandom(-range, range);
    pOutVec->set(rVec + sead::Vector3f(x, y, z));
}

void turnRandomVector(sead::Vector3f* pOutVec, const sead::Vector3f& rVec, f32 range) {
    f32 length = rVec.length();
    addRandomVector(pOutVec, rVec, range);

    if (isNearZero(*pOutVec)) {
        pOutVec->set(rVec);
        return;
    }

    setLength(pOutVec, length);
}

void makeQuatFromTwoAxis(sead::Quatf* outQuat, const sead::Vector3f& vectorA,
                         const sead::Vector3f& vectorB, s32 axisA, s32 axisB) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFromTwoAxis(&mtx, vectorA, vectorB, axisA, axisB);
    mtx.toQuat(*outQuat);
}

void makeQuatFrontUp(sead::Quatf* outQuat, const sead::Vector3f& front, const sead::Vector3f& up) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFrontUp(&mtx, front, up);
    mtx.toQuat(*outQuat);
}

void makeQuatFrontSide(sead::Quatf* outQuat, const sead::Vector3f& front,
                       const sead::Vector3f& side) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFrontSide(&mtx, front, side);
    mtx.toQuat(*outQuat);
}

void makeQuatFrontNoSupport(sead::Quatf* outQuat, const sead::Vector3f& front) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFrontNoSupport(&mtx, front);
    mtx.toQuat(*outQuat);
}

void makeQuatUpFront(sead::Quatf* outQuat, const sead::Vector3f& up, const sead::Vector3f& front) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxUpFront(&mtx, up, front);
    mtx.toQuat(*outQuat);
}

void makeQuatUpNoSupport(sead::Quatf* outQuat, const sead::Vector3f& up) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxUpNoSupport(&mtx, up);
    mtx.toQuat(*outQuat);
}

void makeQuatSideUp(sead::Quatf* outQuat, const sead::Vector3f& side, const sead::Vector3f& up) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFromTwoAxis(&mtx, side, up, 0, 1);
    mtx.toQuat(*outQuat);
}

void makeQuatSideFront(sead::Quatf* outQuat, const sead::Vector3f& side,
                       const sead::Vector3f& front) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxFromTwoAxis(&mtx, side, front, 0, 2);
    mtx.toQuat(*outQuat);
}

void makeQuatSideNoSupport(sead::Quatf* outQuat, const sead::Vector3f& side) {
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    makeMtxSideNoSupport(&mtx, side);
    mtx.toQuat(*outQuat);
}

void makeQuatFromToQuat(sead::Quatf* outQuat, const sead::Quatf& quatA, const sead::Quatf& quatB) {
    sead::Quatf quat;
    quat.setInverse(quatA);

    outQuat->setMul(quatB, quat);
}

void makeQuatRotationRate(sead::Quatf* outQuat, const sead::Vector3f& vecA,
                          const sead::Vector3f& vecB, f32 rate) {
    sead::Vector3f axis;
    f32 radian = 0.0f;

    if (!getAxisAngleFromTwoVec(&axis, &radian, vecA, vecB)) {
        outQuat->set(1.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    makeQuatRotateRadian(outQuat, axis, radian * rate);
}

bool makeQuatRotationLimit(sead::Quatf* outQuat, const sead::Vector3f& vecA,
                           const sead::Vector3f& vecB, f32 limit) {
    sead::Vector3f axis;
    f32 radian = 0.0f;

    if (!getAxisAngleFromTwoVec(&axis, &radian, vecA, vecB)) {
        outQuat->set(1.0f, 0.0f, 0.0f, 0.0f);
        return false;
    }

    bool isReached = radian < limit;
    f32 rate = sead::Mathf::clamp(limit / radian, 0.0f, 1.0f);
    makeQuatRotateRadian(outQuat, axis, radian * rate);

    return isReached;
}

void slerpQuat(sead::Quatf* outQuat, const sead::Quatf& quatA, const sead::Quatf& quatB, f32 rate) {
    outQuat->slerpTo(quatA, quatB, rate);
}

void calcQuatSide(sead::Vector3f* outVec, const sead::Quatf& quat) {
    outVec->set(1.0f - 2.0f * (quat.y * quat.y) - 2.0f * (quat.z * quat.z),
                2.0f * (quat.y * quat.x) + 2.0f * (quat.z * quat.w),
                2.0f * (quat.z * quat.x) - 2.0f * (quat.y * quat.w));
}

void calcQuatUp(sead::Vector3f* outVec, const sead::Quatf& quat) {
    outVec->set(2.0f * (quat.x * quat.y) - 2.0f * (quat.w * quat.z), calcQuatUpY(quat),
                2.0f * (quat.y * quat.z) + 2.0f * (quat.x * quat.w));
}

void calcQuatFront(sead::Vector3f* outVec, const sead::Quatf& quat) {
    outVec->set(2.0f * (quat.x * quat.z) + 2.0f * (quat.w * quat.y), calcQuatFrontY(quat),
                1.0f - 2.0f * (quat.x * quat.x) - 2.0f * (quat.y * quat.y));
}

f32 calcQuatUpY(const sead::Quatf& quat) {
    return 1.0f - 2.0f * (quat.x * quat.x) - 2.0f * (quat.z * quat.z);
}

f32 calcQuatFrontY(const sead::Quatf& quat) {
    return 2.0f * (quat.y * quat.z) - 2.0f * (quat.w * quat.x);
}

void calcQuatLocalAxis(sead::Vector3f* pOutVec, const sead::Quatf& rQuat, s32 axis) {
    switch (axis) {
    case 0:
        calcQuatSide(pOutVec, rQuat);
        return;
    case 1:
        calcQuatUp(pOutVec, rQuat);
        return;
    case 2:
        calcQuatFront(pOutVec, rQuat);
        return;
    }
}

/**
 * Calculates a signed local axis of a quaternion.
 * @param pOutVec receives the axis
 * @param rQuat quaternion to read the axis from
 * @param axis 1, 2 or 3 for side, up or front; negative values flip the axis
 */
void calcQuatLocalSignAxis(sead::Vector3f* pOutVec, const sead::Quatf& rQuat, s32 axis) {
    switch (sead::Mathi::abs(axis)) {
    case 1:
        calcQuatSide(pOutVec, rQuat);
        break;
    case 2:
        calcQuatUp(pOutVec, rQuat);
        break;
    case 3:
        calcQuatFront(pOutVec, rQuat);
        break;
    default:
        return;
    }

    if (axis <= 0) {
        pOutVec->negate();
    }
}

void makeQuatRotateDegree(sead::Quatf* outQuat, const sead::Vector3f& axis, f32 angle) {
    makeQuatRotateRadian(outQuat, axis, sead::Mathf::deg2rad(angle));
}

void calcQuatRotateDegree(sead::Vector3f* outVec, const sead::Quatf& quat) {
    calcQuatRotateRadian(outVec, quat);
    // TODO: potentially add `sead` function to convert Vec3 between deg/rad?
    outVec->set(*outVec * (180.0f / sead::Mathf::pi()));
}

void calcQuatRotateRadian(sead::Vector3f* outVec, const sead::Quatf& quat) {
    quat.calcRPY(*outVec);
}

/**
 * Converts a quaternion into a rotation axis and an angle in the range [-180, 180].
 * @param outAxis receives the normalized rotation axis
 * @param outDegree receives the rotation angle in degrees
 * @param quat quaternion to convert
 */
void calcQuatRotateAxisAndDegree(sead::Vector3f* outAxis, f32* outDegree, const sead::Quatf& quat) {
    outAxis->set(quat.x, quat.y, quat.z);
    f32 len = outAxis->length();
    f32 quatW = quat.w;

    if (isNearZero(len))
        outAxis->set(sead::Vector3f::zero);
    else
        *outAxis *= 1.0f / len;

    f32 radian = sead::Mathf::atan2(len, quatW);
    f32 degree = wrapAngle(sead::Mathf::rad2deg(2.0f * radian));

    if (degree > 180.0f) {
        degree -= 360.0f;
    } else if (degree < -180.0f) {
        degree += 360.0f;
    }

    *outDegree = degree;
}

void calcQuatRotateAxisAndDegree(sead::Vector3f* outAxis, f32* outDegree, const sead::Quatf& quatA,
                                 const sead::Quatf& quatB) {
    sead::Quatf invA;
    invA.setInverse(quatA);

    calcQuatRotateAxisAndDegree(outAxis, outDegree, quatB * invA);
}

void rotateQuatRadian(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& axis,
                      f32 radian) {
    sead::Quatf rotation;
    rotation.setAxisRadian(axis, radian);
    outQuat->setMul(rotation, quat);
    outQuat->normalize();
}

void makeQuatXDegree(sead::Quatf* outQuat, f32 angle) {
    f32 angleRad = sead::Mathf::deg2rad(angle * 0.5f);
    f32 cos = sead::Mathf::cos(angleRad);
    f32 sin = sead::Mathf::sin(angleRad);
    outQuat->w = cos;
    outQuat->x = sin;
    outQuat->y = 0.0f;
    outQuat->z = 0.0f;
}

void makeQuatYDegree(sead::Quatf* outQuat, f32 angle) {
    f32 angleRad = sead::Mathf::deg2rad(angle * 0.5f);
    f32 cos = sead::Mathf::cos(angleRad);
    f32 sin = sead::Mathf::sin(angleRad);
    outQuat->w = cos;
    outQuat->x = 0.0f;
    outQuat->y = sin;
    outQuat->z = 0.0f;
}

void makeQuatZDegree(sead::Quatf* outQuat, f32 angle) {
    f32 angleRad = sead::Mathf::deg2rad(angle * 0.5f);
    f32 cos = sead::Mathf::cos(angleRad);
    f32 sin = sead::Mathf::sin(angleRad);
    outQuat->w = cos;
    outQuat->x = 0.0f;
    outQuat->y = 0.0f;
    outQuat->z = sin;
}

void rotateQuatXDirDegree(sead::Quatf* outQuat, const sead::Quatf& quat, f32 angle) {
    sead::Quatf rotation;
    makeQuatXDegree(&rotation, angle);
    outQuat->setMul(quat, rotation);
    outQuat->normalize();
}

void rotateQuatYDirDegree(sead::Quatf* outQuat, const sead::Quatf& quat, f32 angle) {
    sead::Quatf rotation;
    makeQuatYDegree(&rotation, angle);
    outQuat->setMul(quat, rotation);
    outQuat->normalize();
}

void rotateQuatZDirDegree(sead::Quatf* outQuat, const sead::Quatf& quat, f32 angle) {
    sead::Quatf rotation;
    makeQuatZDegree(&rotation, angle);
    outQuat->setMul(quat, rotation);
    outQuat->normalize();
}

void rotateQuatLocalDirDegree(sead::Quatf* outQuat, const sead::Quatf& quat, s32 axis, f32 angle) {
    sead::Vector3f vec;

    switch (axis) {
    case 0:
        vec.setRotated(quat, sead::Vector3f::ex);
        break;
    case 1:
        vec.setRotated(quat, sead::Vector3f::ey);
        break;
    case 2:
        vec.setRotated(quat, sead::Vector3f::ez);
        break;
    default:
        return;
    }

    rotateQuatRadian(outQuat, quat, vec, sead::Mathf::deg2rad(angle));
}

/**
 * Rotates a quaternion by a moment vector whose length is the angle in radians.
 * @param outQuat receives the rotated quaternion
 * @param quat quaternion to rotate
 * @param vec rotation axis scaled by the angle in radians
 */
void rotateQuatMoment(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& vec) {
    f32 radian;
    sead::Vector3f axis;
    separateScalarAndDirection(&radian, &axis, vec);

    // rotateQuatRadian(...)
    sead::Quatf rotation;
    rotation.setAxisRadian(axis, radian);

    outQuat->setMul(rotation, quat);
    outQuat->normalize();
}

/**
 * Rotates a quaternion by a moment vector whose length is the angle in degrees.
 * @param outQuat receives the rotated quaternion
 * @param quat quaternion to rotate
 * @param vec rotation axis scaled by the angle in degrees
 */
void rotateQuatMomentDegree(sead::Quatf* outQuat, const sead::Quatf& quat,
                            const sead::Vector3f& vec) {
    f32 degree;
    sead::Vector3f axis;
    separateScalarAndDirection(&degree, &axis, vec);

    // rotateQuatDegree(...)
    sead::Quatf rotation;
    rotation.setAxisAngle(axis, degree);

    outQuat->setMul(rotation, quat);
    outQuat->normalize();
}

void rotateQuatRollBall(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& vecA,
                        const sead::Vector3f& vecB, f32 scale) {
    sead::Vector3f vecNorm;
    calcMomentRollBall(&vecNorm, vecA, vecB, scale);
    rotateQuatMoment(outQuat, quat, vecNorm);
}

/**
 * Calculates the rotation moment of a ball rolling with a velocity on a surface.
 * @param outVec receives the moment, or zero if the up vector is near zero
 * @param vecA velocity of the ball
 * @param vecB up direction of the surface
 * @param scale radius of the ball
 */
void calcMomentRollBall(sead::Vector3f* outVec, const sead::Vector3f& vecA,
                        const sead::Vector3f& vecB, f32 scale) {
    sead::Vector3f moment = vecB;

    if (tryNormalizeOrZero(&moment)) {
        moment.setCross(moment, vecA);
        moment = (1.0f / scale) * moment;
    }

    outVec->set(moment.x, moment.y, moment.z);
}

bool turnQuat(sead::Quatf* pOutQuat, const sead::Quatf& rQuat, const sead::Vector3f& rAxis,
              const sead::Vector3f& rDir, f32 radian) {
    sead::Vector3f from;
    sead::Vector3f target = rDir;

    if (rAxis.dot(rDir) >= 0.0f || !isParallelDirection(rAxis, rDir, 0.01f)) {
        from.set(rAxis);
    } else {
        turnRandomVector(&from, rAxis, 0.001f);
    }

    tryNormalizeOrZero(&from);
    tryNormalizeOrZero(&target);
    sead::Quatf rotate;
    makeQuatRotationLimit(&rotate, from, target, radian);
    pOutQuat->setMul(rotate, rQuat);
    pOutQuat->normalize();
    return from.dot(target) > 0.995f;
}

bool turnQuatXDirRadian(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& dir,
                        f32 radian) {
    sead::Vector3f axis;
    axis.setRotated(quat, sead::Vector3f::ex);
    return turnQuat(outQuat, quat, axis, dir, radian);
}

bool turnQuatYDirRadian(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& dir,
                        f32 radian) {
    sead::Vector3f axis;
    axis.setRotated(quat, sead::Vector3f::ey);
    return turnQuat(outQuat, quat, axis, dir, radian);
}

bool turnQuatZDirRadian(sead::Quatf* outQuat, const sead::Quatf& quat, const sead::Vector3f& dir,
                        f32 radian) {
    sead::Vector3f axis;
    axis.setRotated(quat, sead::Vector3f::ez);
    return turnQuat(outQuat, quat, axis, dir, radian);
}

// TODO: rename parameters
bool turnQuatZDirToTargetWithAxis(sead::Quatf* pQuat, const sead::Vector3f& rTarget,
                                  const sead::Vector3f& rAxis, f32 maxRadian) {
    sead::Vector3f front;
    calcQuatFront(&front, *pQuat);
    sead::Vector3f cross;
    cross.setCross(front, rTarget);

    if (isNearZero(cross)) {
        return true;
    }

    cross.normalize();
    sead::Vector3f targetH;
    verticalizeVec(&targetH, rAxis, rTarget);

    if (isNearZero(targetH)) {
        return false;
    }

    sead::Vector3f frontH;
    verticalizeVec(&frontH, rAxis, front);
    targetH.normalize();

    if (isNearZero(frontH)) {
        return false;
    }

    frontH.normalize();
    f32 angle = sead::Mathf::acos(sead::Mathf::clamp(targetH.dot(frontH), -1.0f, 1.0f));

    if (cross.dot(rAxis) <= 0.0f) {
        angle = sead::Mathf::pi2() - angle;
    }

    f32 turnAngle = angle > maxRadian ? maxRadian : angle;
    rotateQuatRadian(pQuat, *pQuat, rAxis, turnAngle);
    return isNearZero(turnAngle - angle);
}

void turnQuatXDirRate(sead::Quatf* pOutQuat, const sead::Quatf& rQuat, const sead::Vector3f& rDir,
                      f32 rate) {
    sead::Vector3f axis;
    axis.setRotated(rQuat, sead::Vector3f::ex);

    if (!(axis.dot(rDir) >= 0.0f) && isParallelDirection(axis, rDir, 0.01f)) {
        turnRandomVector(&axis, axis, 0.001f);
    }

    tryNormalizeOrZero(&axis);
    sead::Vector3f dir;
    tryNormalizeOrZero(&dir, rDir);
    sead::Quatf rotate;
    makeQuatRotationRate(&rotate, axis, dir, rate);
    pOutQuat->setMul(rotate, rQuat);
    pOutQuat->normalize();
}

void turnQuatYDirRate(sead::Quatf* pOutQuat, const sead::Quatf& rQuat, const sead::Vector3f& rDir,
                      f32 rate) {
    sead::Vector3f axis;
    axis.setRotated(rQuat, sead::Vector3f::ey);

    if (!(axis.dot(rDir) >= 0.0f) && isParallelDirection(axis, rDir, 0.01f)) {
        turnRandomVector(&axis, axis, 0.001f);
    }

    tryNormalizeOrZero(&axis);
    sead::Vector3f dir;
    tryNormalizeOrZero(&dir, rDir);
    sead::Quatf rotate;
    makeQuatRotationRate(&rotate, axis, dir, rate);
    pOutQuat->setMul(rotate, rQuat);
    pOutQuat->normalize();
}

void turnQuatZDirRate(sead::Quatf* pOutQuat, const sead::Quatf& rQuat, const sead::Vector3f& rDir,
                      f32 rate) {
    sead::Vector3f axis;
    axis.setRotated(rQuat, sead::Vector3f::ez);

    if (!(axis.dot(rDir) >= 0.0f) && isParallelDirection(axis, rDir, 0.01f)) {
        turnRandomVector(&axis, axis, 0.001f);
    }

    tryNormalizeOrZero(&axis);
    sead::Vector3f dir;
    tryNormalizeOrZero(&dir, rDir);
    sead::Quatf rotate;
    makeQuatRotationRate(&rotate, axis, dir, rate);
    pOutQuat->setMul(rotate, rQuat);
    pOutQuat->normalize();
}

bool turnQuatFrontToDirDegreeH(sead::Quatf* pQuat, const sead::Vector3f& rDir, f32 degree) {
    sead::Vector3f dirH = rDir;
    dirH.y = 0.0f;

    if (!tryNormalizeOrZero(&dirH)) {
        return true;
    }

    sead::Vector3f front;
    front.setRotated(*pQuat, sead::Vector3f::ez);

    if (!(front.dot(dirH) >= 0.0f) && isParallelDirection(front, dirH, 0.01f)) {
        sead::Vector3f side;
        side.setRotated(*pQuat, sead::Vector3f::ex);
        dirH += side * 0.01f;
    }

    bool result = turnQuat(pQuat, *pQuat, front, dirH, sead::Mathf::deg2rad(degree));
    turnQuatYDirRate(pQuat, *pQuat, sead::Vector3f(0.0f, 1.0f, 0.0f), 0.2f);
    return result;
}

void rotateQuatAndTransDegree(sead::Quatf* pOutQuat, sead::Vector3f* pOutTrans,
                              const sead::Quatf& rQuat, const sead::Vector3f& rTrans,
                              const sead::Vector3f& rAxis, const sead::Vector3f& rCenter,
                              f32 degree) {
    sead::Vector3f diff = rTrans - rCenter;
    sead::Quatf rotate = sead::Quatf::unit;

    if (!isNearZero(rAxis)) {
        makeQuatRotateDegree(&rotate, rAxis, degree);
    }

    if (pOutQuat) {
        pOutQuat->setMul(rotate, rQuat);
        pOutQuat->normalize();
    }

    if (pOutTrans) {
        pOutTrans->setRotated(rotate, diff);
        *pOutTrans += rCenter;
    }
}

bool turnVecToVecDegree(sead::Vector3f* pOutVec, const sead::Vector3f& rVec,
                        const sead::Vector3f& rTarget, f32 degree) {
    sead::Quatf quat;
    bool result = makeQuatRotationLimit(&quat, rVec, rTarget, sead::Mathf::deg2rad(degree));
    pOutVec->setRotated(quat, rVec);
    normalize(pOutVec);
    return result;
}

bool turnVecToVecCos(sead::Vector3f* pOutVec, const sead::Vector3f& rFrom, const sead::Vector3f& rTo,
                     f32 cosLimit, const sead::Vector3f& rAxis, f32 rate) {
    if (isNearZero(rFrom) || isNearZero(rTo)) {
        return false;
    }

    if (rFrom.dot(rTo) > cosLimit) {
        pOutVec->set(rTo);
        normalize(pOutVec);
        return true;
    }

    f32 sinLimit = sead::Mathf::sqrt(1.0f - cosLimit * cosLimit);
    sead::Vector3f vertical;
    verticalizeVec(&vertical, rFrom, rTo);

    if (isNearZero(vertical)) {
        sead::Vector3f side;
        side.setCross(rFrom, rAxis);
        normalize(&side);
        pOutVec->setScaleAdd(rate, side, rFrom);
    } else {
        normalize(&vertical);
        pOutVec->setScale(rFrom, cosLimit);
        *pOutVec += vertical * sinLimit;
    }

    normalize(pOutVec);
    return false;
}

/**
 * Turns a vector towards another one on the plane given by a normal.
 * @param pOutVec receives the turned vector
 * @param rFrom vector to turn
 * @param rTo vector to turn towards
 * @param rPlaneNormal normal of the plane to turn on
 * @param cosLimit cosine of the maximum turn angle
 * @return true if the target direction was reached
 */
bool turnVecToVecCosOnPlane(sead::Vector3f* pOutVec, const sead::Vector3f& rFrom,
                            const sead::Vector3f& rTo, const sead::Vector3f& rPlaneNormal,
                            f32 cosLimit) {
    sead::Vector3f to;
    verticalizeVec(&to, rPlaneNormal, rTo);
    tryNormalizeOrZero(&to);
    sead::Vector3f from;
    verticalizeVec(&from, rPlaneNormal, rFrom);
    tryNormalizeOrZero(&from);

    if (isNearZero(from)) {
        from.set(-to);
    }

    if (isNearZero(to)) {
        return false;
    }

    if (cosLimit > -1.0f) {
        return turnVecToVecCos(pOutVec, from, to, cosLimit, rPlaneNormal, 0.02f);
    }

    pOutVec->set(to);
    return true;
}

bool turnVecToVecCosOnPlane(sead::Vector3f* outVec, const sead::Vector3f& vecA,
                            const sead::Vector3f& vecB, f32 value) {
    return turnVecToVecCosOnPlane(outVec, *outVec, vecA, vecB, value);
}

void rotateVectorDegree(sead::Vector3f* pOutVec, const sead::Vector3f& rVec,
                        const sead::Vector3f& rAxis, f32 degree) {
    sead::Quatf quat;
    makeQuatRotateDegree(&quat, rAxis, degree);
    pOutVec->setRotated(quat, rVec);
}

void rotateVectorDegreeX(sead::Vector3f* pVec, f32 degree) {
    rotateVectorDegree(pVec, sead::Vector3f(1.0f, 0.0f, 0.0f), degree);
}

void rotateVectorDegreeY(sead::Vector3f* pVec, f32 degree) {
    rotateVectorDegree(pVec, sead::Vector3f(0.0f, 1.0f, 0.0f), degree);
}

void rotateVectorDegreeZ(sead::Vector3f* pVec, f32 degree) {
    rotateVectorDegree(pVec, sead::Vector3f(0.0f, 0.0f, 1.0f), degree);
}

void rotateVectorQuat(sead::Vector3f* pVec, const sead::Quatf& rQuat) {
    sead::Quatf vecQuat(0.0f, pVec->x, pVec->y, pVec->z);
    sead::Quatf conjugate(rQuat.w, -rQuat.x, -rQuat.y, -rQuat.z);
    sead::Quatf rotated = rQuat * vecQuat * conjugate;
    pVec->set(rotated.x, rotated.y, rotated.z);
}

void createBoundingBox(const sead::Vector3f* pPoints, u32 count, sead::Vector3f* pMin,
                       sead::Vector3f* pMax) {
    *pMin = pPoints[0];
    *pMax = pPoints[0];
    for (u32 i = 1; i < count; i++) {
        updateBoundingBox(pPoints[i], pMin, pMax);
    }
}

void updateBoundingBox(sead::Vector3f value, sead::Vector3f* min, sead::Vector3f* max) {
    if (value.x < min->x)
        min->x = value.x;
    else if (max->x < value.x)
        max->x = value.x;

    if (value.y < min->y)
        min->y = value.y;
    else if (max->y < value.y)
        max->y = value.y;

    if (value.z < min->z)
        min->z = value.z;
    else if (max->z < value.z)
        max->z = value.z;
}

f32 calcDistanceToFarthestBoundingBoxVertex(const sead::Vector3f& rPos, const sead::Vector3f& rMin,
                                            const sead::Vector3f& rMax) {
    f32 x = sead::Mathf::max(sead::Mathf::abs(rMin.x - rPos.x), sead::Mathf::abs(rMax.x - rPos.x));
    f32 y = sead::Mathf::max(sead::Mathf::abs(rMin.y - rPos.y), sead::Mathf::abs(rMax.y - rPos.y));
    f32 z = sead::Mathf::max(sead::Mathf::abs(rMin.z - rPos.z), sead::Mathf::abs(rMax.z - rPos.z));
    return sead::Vector3f(x, y, z).length();
}

/**
 * Calculates the smallest sphere enclosing two spheres.
 * @param pOutCenter receives the center of the enclosing sphere
 * @param pOutRadius receives the radius of the enclosing sphere
 * @param rCenterA center of the first sphere
 * @param radiusA radius of the first sphere
 * @param rCenterB center of the second sphere
 * @param radiusB radius of the second sphere
 */
void calcSphereMargeSpheres(sead::Vector3f* pOutCenter, f32* pOutRadius,
                            const sead::Vector3f& rCenterA, f32 radiusA,
                            const sead::Vector3f& rCenterB, f32 radiusB) {
    sead::Vector3f diff = rCenterB - rCenterA;
    f32 squaredDistance = diff.squaredLength();
    f32 radiusDiff = radiusB - radiusA;

    if (radiusDiff * radiusDiff >= squaredDistance) {
        if (radiusA >= radiusB) {
            pOutCenter->set(rCenterA);
            *pOutRadius = radiusA;
        } else {
            pOutCenter->set(rCenterB);
            *pOutRadius = radiusB;
        }

        return;
    }

    f32 distance = diff.length();
    *pOutRadius = (radiusA + radiusB + distance) * 0.5f;
    pOutCenter->set(rCenterA);

    if (!isNearZero(distance)) {
        pOutCenter->setScaleAdd((*pOutRadius - radiusA) / distance, diff, *pOutCenter);
    }
}

bool calcCrossLinePoint(sead::Vector2f* crossPoint, const sead::Vector2f& pointA,
                        const sead::Vector2f& dirA, const sead::Vector2f& pointB,
                        const sead::Vector2f& dirB) {
    f32 det = dirB.y * dirA.x - dirA.y * dirB.x;

    if (isNearZero(det))
        return false;

    f32 distance = (dirA.x * (pointA.y - pointB.y) + dirA.y * (pointB.x - pointA.x)) / det;

    crossPoint->x = dirB.x * distance + pointB.x;
    crossPoint->y = dirB.y * distance + pointB.y;
    return true;
}

f32 calcSquaredDistanceHitSegmentToSegment(const sead::Vector3f& rStartA,
                                           const sead::Vector3f& rEndA,
                                           const sead::Vector3f& rStartB,
                                           const sead::Vector3f& rEndB, sead::Vector3f* pHitPosA,
                                           sead::Vector3f* pHitPosB) {
    sead::Vector3f dirA = rEndA - rStartA;
    sead::Vector3f dirB = rEndB - rStartB;
    sead::Vector3f diff = rStartA - rStartB;
    f32 sqLengthA = dirA.dot(dirA);
    f32 sqLengthB = dirB.dot(dirB);
    bool isZeroA = isNearZero(sqLengthA);
    bool isZeroB = isNearZero(sqLengthB);

    if (isZeroA && isZeroB) {
        if (pHitPosA) {
            pHitPosA->set(rStartA);
        }

        if (pHitPosB) {
            pHitPosB->set(rStartA);
        }

        return diff.dot(diff);
    }

    f32 dotB = dirB.dot(diff);
    f32 rateA;
    f32 rateB;

    if (isZeroA) {
        rateA = 0.0f;
        rateB = sead::Mathf::clamp(dotB / sqLengthB, 0.0f, 1.0f);
    } else if (isZeroB) {
        rateB = 0.0f;
        rateA = sead::Mathf::clamp(-dirA.dot(diff) / sqLengthA, 0.0f, 1.0f);
    } else {
        f32 dotAB = dirA.dot(dirB);
        f32 dotA = dirA.dot(diff);
        f32 denom = sqLengthA * sqLengthB - dotAB * dotAB;

        if (!isNearZero(denom)) {
            rateA = sead::Mathf::clamp((dotAB * dotB - dotA * sqLengthB) / denom, 0.0f, 1.0f);
        } else {
            rateA = 0.0f;
        }

        f32 rateNumB = dotB + dotAB * rateA;

        if (rateNumB < 0.0f) {
            rateB = 0.0f;
            rateA = sead::Mathf::clamp(-dotA / sqLengthA, 0.0f, 1.0f);
        } else if (rateNumB > sqLengthB) {
            rateB = 1.0f;
            rateA = sead::Mathf::clamp((dotAB - dotA) / sqLengthA, 0.0f, 1.0f);
        } else {
            rateB = rateNumB / sqLengthB;
        }
    }

    sead::Vector3f hitPosA = rStartA + dirA * rateA;
    sead::Vector3f hitPosB = rStartB + dirB * rateB;

    if (pHitPosA) {
        pHitPosA->set(hitPosA);
    }

    if (pHitPosB) {
        pHitPosB->set(hitPosB);
    }

    return (hitPosA - hitPosB).squaredLength();
}

bool checkHitSemilinePlane(sead::Vector3f* pHitPos, const sead::Vector3f& rStart,
                           const sead::Vector3f& rDir, const sead::Vector3f& rPlanePos,
                           const sead::Vector3f& rPlaneNormal) {
    f32 dot = rDir.dot(rPlaneNormal);

    if (dot > 0.0f) {
        return false;
    }

    if (pHitPos) {
        f32 rate = rPlaneNormal.dot(rPlanePos - rStart) / dot;
        pHitPos->set(rStart);
        *pHitPos += rDir * rate;
    }

    return true;
}

bool checkHitSegmentPlane(sead::Vector3f* pHitPos, const sead::Vector3f& rStart,
                          const sead::Vector3f& rSegment, const sead::Vector3f& rPlanePos,
                          const sead::Vector3f& rPlaneNormal, bool isCheckBothSide) {
    f32 dot = rSegment.dot(rPlaneNormal);

    if (dot >= 0.0f && !isCheckBothSide) {
        return false;
    }

    if (isNearZero(dot, 0.0001f)) {
        return false;
    }

    f32 rate = rPlaneNormal.dot(rPlanePos - rStart) / dot;

    if (rate < 0.0f || rate > 1.0f) {
        return false;
    }

    if (pHitPos) {
        pHitPos->set(rStart);
        *pHitPos += rSegment * rate;
    }

    return true;
}

bool checkHitSegmentSphere(const sead::Vector3f& rCenter, const sead::Vector3f& rStart,
                           const sead::Vector3f& rEnd, f32 radius, sead::Vector3f* pHitNormal,
                           sead::Vector3f* pHitPos) {
    sead::Vector3f toCenter = rCenter - rStart;
    sead::Vector3f segment = rEnd - rStart;
    f32 dot = toCenter.dot(segment);
    f32 radiusSq = radius * radius;
    sead::Vector3f normal;

    if (dot < 0.0f) {
        if ((rStart - rCenter).squaredLength() < radiusSq) {
            tryNormalizeOrZero(&normal, toCenter);
        } else {
            return false;
        }
    } else {
        f32 segmentSqLength = segment.squaredLength();

        if (segmentSqLength < dot) {
            if ((rEnd - rCenter).squaredLength() < radiusSq) {
                tryNormalizeOrZero(&normal, rCenter - rEnd);
            } else {
                return false;
            }
        } else if (isNearZero(segmentSqLength)) {
            if (toCenter.squaredLength() <= radiusSq) {
                tryNormalizeOrZero(&normal, toCenter);
            } else {
                return false;
            }
        } else {
            sead::Vector3f diff = segment * (dot / segmentSqLength) - toCenter;

            if (diff.squaredLength() <= radiusSq) {
                tryNormalizeOrZero(&normal, -diff);
            } else {
                return false;
            }
        }
    }

    if (pHitNormal) {
        pHitNormal->set(normal);
    }

    if (pHitPos) {
        pHitPos->setScaleAdd(-radius, normal, rCenter);
    }

    return true;
}

bool checkHitSegmentSphereNearDepth(const sead::Vector3f& rCenter, const sead::Vector3f& rStart,
                                    const sead::Vector3f& rEnd, f32 radius,
                                    sead::Vector3f* pHitPos, sead::Vector3f* pHitNormal) {
    sead::Vector3f toStart = rStart - rCenter;
    sead::Vector3f dir = rEnd - rStart;
    dir.normalize();
    f32 dot = toStart.dot(dir);

    if (dot > 0.0f) {
        return false;
    }

    f32 distance = toStart.squaredLength() - radius * radius;
    f32 discriminant = dot * dot - distance;

    if (discriminant < 0.0f) {
        return false;
    }

    f32 depth = -dot - sead::Mathf::sqrt(discriminant);
    f32 segmentLength = (rStart - rEnd).length();

    if (sead::Mathf::abs(depth) > segmentLength) {
        return false;
    }

    pHitPos->setScaleAdd(depth, dir, rStart);
    pHitNormal->setSub(*pHitPos, rCenter);
    tryNormalizeOrZero(pHitNormal);
    return true;
}

bool checkHitHalfLineSphere(const sead::Vector3f& center, const sead::Vector3f& rayStart,
                            const sead::Vector3f& rayDir, f32 radius) {
    sead::Vector3f diff = center - rayStart;
    f32 dot = diff.dot(rayDir);

    if (dot < 0.0f) {
        // NOTE: Some sort of is isNearDirection but reversed
        // BUG: returns `true` if the sphere is too far "behind" the ray
        return !(radius < -dot || radius * radius < (rayStart - center).squaredLength());
    }

    return isNearZero(rayDir * dot - diff, radius);
}

static bool tryCalcHitPosX(sead::Vector3f* pHitPos, const sead::Vector3f& rStart,
                           const sead::Vector3f& rSegment, const sead::BoundBox3f& rBox, f32 rate) {
    if (rate < 0.0f || rate > 1.0f) {
        return false;
    }

    f32 y = rStart.y + rSegment.y * rate;

    if (!(rBox.getMin().y <= y && y <= rBox.getMax().y)) {
        return false;
    }

    f32 z = rStart.z + rSegment.z * rate;

    if (!(rBox.getMin().z <= z && z <= rBox.getMax().z)) {
        return false;
    }

    if (pHitPos) {
        pHitPos->set(rStart.x + rSegment.x * rate, y, z);
    }

    return true;
}

static bool tryCalcHitPosY(sead::Vector3f* pHitPos, const sead::Vector3f& rStart,
                           const sead::Vector3f& rSegment, const sead::BoundBox3f& rBox, f32 rate) {
    if (rate < 0.0f || rate > 1.0f) {
        return false;
    }

    f32 x = rStart.x + rate * rSegment.x;

    if (!(rBox.getMin().x <= x && x <= rBox.getMax().x)) {
        return false;
    }

    f32 z = rStart.z + rate * rSegment.z;

    if (!(rBox.getMin().z <= z && z <= rBox.getMax().z)) {
        return false;
    }

    if (pHitPos) {
        pHitPos->set(x, rStart.y + rate * rSegment.y, z);
    }

    return true;
}

static bool tryCalcHitPosZ(sead::Vector3f* pHitPos, const sead::Vector3f& rStart,
                           const sead::Vector3f& rSegment, const sead::BoundBox3f& rBox, f32 rate) {
    if (rate < 0.0f || rate > 1.0f) {
        return false;
    }

    f32 x = rStart.x + rate * rSegment.x;

    if (!(rBox.getMin().x <= x && x <= rBox.getMax().x)) {
        return false;
    }

    f32 y = rStart.y + rate * rSegment.y;

    if (!(rBox.getMin().y <= y && y <= rBox.getMax().y)) {
        return false;
    }

    if (pHitPos) {
        pHitPos->set(x, y, rStart.z + rate * rSegment.z);
    }

    return true;
}

bool checkHitSegmentBox(const sead::Vector3f& rStart, const sead::Vector3f& rSegment,
                        const sead::BoundBox3f& rBox, sead::Vector3f* pHitPos) {
    const sead::Vector3f& min = rBox.getMin();
    const sead::Vector3f& max = rBox.getMax();
    bool isStartOutside = !rBox.isInside(rStart);

    if (rBox.isInside(rStart + rSegment) && !isStartOutside) {
        return true;
    }

    f32 rate;

    if (rSegment.x > 0.0f) {
        rate = (min.x - rStart.x) / rSegment.x;

        if (tryCalcHitPosX(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    } else if (rSegment.x < 0.0f) {
        rate = (max.x - rStart.x) / rSegment.x;

        if (tryCalcHitPosX(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    }

    if (rSegment.y > 0.0f) {
        rate = (min.y - rStart.y) / rSegment.y;

        if (tryCalcHitPosY(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    } else if (rSegment.y < 0.0f) {
        rate = (max.y - rStart.y) / rSegment.y;

        if (tryCalcHitPosY(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    }

    if (rSegment.z > 0.0f) {
        rate = (min.z - rStart.z) / rSegment.z;

        if (tryCalcHitPosZ(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    } else if (rSegment.z < 0.0f) {
        rate = (max.z - rStart.z) / rSegment.z;

        if (tryCalcHitPosZ(pHitPos, rStart, rSegment, rBox, rate)) {
            return true;
        }
    }

    return false;
}

bool checkHitPointCone(const sead::Vector3f& rPoint, const sead::Vector3f& rApex,
                       const sead::Vector3f& rDir, f32 height, f32 angleDegree) {
    sead::Vector3f toPoint = rPoint;
    toPoint -= rApex;
    f32 depth = toPoint.dot(rDir);

    if (depth < 0.0f || depth > height) {
        return false;
    }

    sead::Vector3f projected = rDir * depth;
    f32 radius = depth * sead::Mathf::tan(sead::Mathf::deg2rad(angleDegree));

    if (radius < (toPoint - projected).length()) {
        return false;
    }

    return true;
}

/**
 * Result of the shared sphere/cone test used by checkHitSphereCone and checkHitSphereSpotLight.
 */
enum class SphereConeHitResult : s32 {
    Hit = 0,
    NoHit = 1,
    CheckBase = 2,
};

/**
 * Tests a sphere against the side of an infinite cone whose apex is at the origin.
 * @param rDir normalized direction of the cone axis
 * @param rApexToSphere vector from the cone apex to the sphere center
 * @param radius radius of the sphere
 * @param height height of the cone
 * @param angleDegree half opening angle of the cone in degrees
 * @return whether the sphere hits, misses, or the base of the cone has to be checked
 */
static SphereConeHitResult checkHitSphereConeSide(const sead::Vector3f& rDir,
                                                  const sead::Vector3f& rApexToSphere, f32 radius,
                                                  f32 height, f32 angleDegree) {
    f32 distance;
    sead::Vector3f dirToSphere;

    if (separateScalarAndDirection(&distance, &dirToSphere, rApexToSphere)) {
        return SphereConeHitResult::NoHit;
    }

    if (isReverseDirection(rDir, dirToSphere)) {
        return distance < radius ? SphereConeHitResult::Hit : SphereConeHitResult::NoHit;
    }

    if (isNearDirection(rDir, dirToSphere)) {
        return distance < radius + height ? SphereConeHitResult::Hit :
                                            SphereConeHitResult::NoHit;
    }

    sead::Vector3f axis;
    axis.setCross(rDir, dirToSphere);
    tryNormalizeOrZero(&axis);

    sead::Vector3f sideDir = rDir;
    rotateVectorDegree(&sideDir, sideDir, axis, angleDegree);
    tryNormalizeOrZero(&sideDir);
    f32 sideDepth = sideDir.dot(rApexToSphere);

    if (sideDepth <= 0.0f) {
        return distance < radius ? SphereConeHitResult::Hit : SphereConeHitResult::NoHit;
    }

    sead::Vector3f cross;
    cross.setCross(sideDir, dirToSphere);

    if (axis.dot(cross) > 0.0f) {
        f32 sideLength = height / sead::Mathf::cos(sead::Mathf::deg2rad(angleDegree));
        sead::Vector3f nearestPos = sideDir * sead::Mathf::clamp(sideDepth, 0.0f, sideLength);
        return (nearestPos - rApexToSphere).squaredLength() <= radius * radius ?
                   SphereConeHitResult::Hit :
                   SphereConeHitResult::NoHit;
    }

    return SphereConeHitResult::CheckBase;
}

/**
 * Checks whether a sphere intersects a cone.
 * @param rSpherePos center of the sphere
 * @param sphereRadius radius of the sphere
 * @param rConePos position of the cone apex
 * @param rConeDir normalized direction of the cone axis
 * @param coneHeight height of the cone
 * @param coneAngle half opening angle of the cone in degrees
 * @return true if the sphere and the cone intersect
 */
bool checkHitSphereCone(const sead::Vector3f& rSpherePos, f32 sphereRadius,
                        const sead::Vector3f& rConePos, const sead::Vector3f& rConeDir,
                        f32 coneHeight, f32 coneAngle) {
    sead::Vector3f apexToSphere = rSpherePos;
    apexToSphere -= rConePos;
    SphereConeHitResult result = checkHitSphereConeSide(rConeDir, apexToSphere, sphereRadius,
                                                        coneHeight, coneAngle);

    if (result != SphereConeHitResult::CheckBase) {
        return result == SphereConeHitResult::Hit;
    }

    sead::Vector3f basePos = rConeDir * coneHeight + rConePos;
    f32 depth = rConeDir.dot(rSpherePos - basePos);

    if (depth <= 0.0f) {
        return true;
    }

    if (depth > sphereRadius) {
        return false;
    }

    f32 cutRadius = sead::Mathf::sqrt(sphereRadius * sphereRadius - depth * depth);
    sead::Vector3f baseToCut = apexToSphere - rConeDir * depth - rConeDir * coneHeight;
    f32 baseRadius = sead::Mathf::tan(sead::Mathf::deg2rad(coneAngle)) * coneHeight;
    return baseToCut.length() <= cutRadius + baseRadius;
}

/**
 * Checks whether a sphere intersects a spot light shaped volume (a cone capped by a sphere).
 * @param rSpherePos center of the sphere
 * @param sphereRadius radius of the sphere
 * @param rLightPos position of the light
 * @param rLightDir normalized direction of the light
 * @param lightLength reach of the light
 * @param lightAngle half opening angle of the light in degrees
 * @return true if the sphere and the light volume intersect
 */
bool checkHitSphereSpotLight(const sead::Vector3f& rSpherePos, f32 sphereRadius,
                             const sead::Vector3f& rLightPos, const sead::Vector3f& rLightDir,
                             f32 lightLength, f32 lightAngle) {
    sead::Vector3f lightToSphere = rSpherePos;
    lightToSphere -= rLightPos;
    SphereConeHitResult result = checkHitSphereConeSide(rLightDir, lightToSphere, sphereRadius,
                                                        lightLength, lightAngle);

    if (result != SphereConeHitResult::CheckBase) {
        return result == SphereConeHitResult::Hit;
    }

    return lightToSphere.length() <= sphereRadius + lightLength;
}

bool isNearCollideSphereAabb(const sead::Vector3f& center, f32 radius,
                             const sead::BoundBox3f& boundBox) {
    const sead::Vector3f& min = boundBox.getMin();
    const sead::Vector3f& max = boundBox.getMax();

    if (center.x < min.x - radius || max.x + radius < center.x)
        return false;
    if (center.y < min.y - radius || max.y + radius < center.y)
        return false;
    if (center.z < min.z - radius || max.z + radius < center.z)
        return false;
    return true;
}

void calcBoxFacePoint(sead::Vector3f facePoints[4], const sead::BoundBox3f& boundBox, s32 axis) {
    const sead::Vector3f& min = boundBox.getMin();
    const sead::Vector3f& max = boundBox.getMax();

    switch (static_cast<Axis>(axis)) {
    case Axis::X:
        facePoints[0].set(max.x, max.y, max.z);
        facePoints[1].set(max.x, max.y, min.z);
        facePoints[2].set(max.x, min.y, min.z);
        facePoints[3].set(max.x, min.y, max.z);
        return;
    case Axis::Y:
        facePoints[0].set(max.x, max.y, max.z);
        facePoints[1].set(max.x, max.y, min.z);
        facePoints[2].set(min.x, max.y, min.z);
        facePoints[3].set(min.x, max.y, max.z);
        return;
    case Axis::Z:
        facePoints[0].set(max.x, max.y, max.z);
        facePoints[1].set(max.x, min.y, max.z);
        facePoints[2].set(min.x, min.y, max.z);
        facePoints[3].set(min.x, max.y, max.z);
        return;
    case Axis::InvertX:
        facePoints[0].set(min.x, max.y, max.z);
        facePoints[1].set(min.x, max.y, min.z);
        facePoints[2].set(min.x, min.y, min.z);
        facePoints[3].set(min.x, min.y, max.z);
        return;
    case Axis::InvertY:
        facePoints[0].set(max.x, min.y, max.z);
        facePoints[1].set(max.x, min.y, min.z);
        facePoints[2].set(min.x, min.y, min.z);
        facePoints[3].set(min.x, min.y, max.z);
        return;
    case Axis::InvertZ:
        facePoints[0].set(max.x, max.y, min.z);
        facePoints[1].set(max.x, min.y, min.z);
        facePoints[2].set(min.x, min.y, min.z);
        facePoints[3].set(min.x, max.y, min.z);
        return;
    default:
        return;
    }
}

/**
 * Calculates the four corners of a box face transformed by a matrix.
 * @param facePoints receives the four corners
 * @param rBox box to read the face from
 * @param axis face to read
 * @param rMtx matrix to transform the corners with
 */
void calcBoxFacePoint(sead::Vector3f facePoints[4], const sead::BoundBox3f& rBox, s32 axis,
                      const sead::Matrix34f& rMtx) {
    sead::Vector3f localPoints[4];
    calcBoxFacePoint(localPoints, rBox, axis);

    for (s32 i = 0; i < 4; i++) {
        facePoints[i].setMul(rMtx, localPoints[i]);
    }
}

void calcBoxFacePoint(sead::Vector3f facePoints[4], const sead::BoundBox3f& rBox, s32 axis,
                      const sead::Quatf& rQuat, const sead::Vector3f& rTrans) {
    sead::Matrix34f mtx;
    mtx.makeQT(rQuat, rTrans);
    calcBoxFacePoint(facePoints, rBox, axis, mtx);
}

void calcFittingBoxPoseEqualAxisAll(sead::Quatf* pOutQuat, const sead::Quatf& rQuatA,
                                    const sead::Quatf& rQuatB) {
    sead::Vector3f sideB;
    sead::Vector3f upB;
    sead::Vector3f frontB;
    calcQuatLocalAxisAll(rQuatB, &sideB, &upB, &frontB);

    sead::Vector3f up;
    sead::Vector3f front;
    calcQuatUp(&up, rQuatA);
    calcQuatFront(&front, rQuatA);

    switch (calcNearVecFromAxis3(&up, up, sideB, upB, frontB)) {
    case Axis::X:
    case Axis::InvertX:
        calcNearVecFromAxis2(&front, front, upB, frontB);
        break;
    case Axis::Y:
    case Axis::InvertY:
        calcNearVecFromAxis2(&front, front, sideB, frontB);
        break;
    default:
        calcNearVecFromAxis2(&front, front, sideB, upB);
        break;
    }

    makeQuatFrontUp(pOutQuat, front, up);
}

void calcFittingBoxPoseEqualAxisNone(sead::Quatf* pOutQuat, const sead::Quatf& rQuatA,
                                     const sead::Quatf& rQuatB) {
    sead::Vector3f sideB;
    sead::Vector3f upB;
    sead::Vector3f frontB;
    calcQuatLocalAxisAll(rQuatB, &sideB, &upB, &frontB);

    sead::Vector3f up;
    sead::Vector3f front;
    calcQuatUp(&up, rQuatA);
    calcQuatFront(&front, rQuatA);

    up = upB.dot(up) >= 0.0f ? upB : -upB;
    front = frontB.dot(front) >= 0.0f ? frontB : -frontB;
    makeQuatFrontUp(pOutQuat, front, up);
}

void calcFittingBoxPoseEqualAxisTwo(sead::Quatf* pOutQuat, const sead::Quatf& rQuatA,
                                    const sead::Quatf& rQuatB, s32 axis) {
    sead::Vector3f axisB;
    sead::Vector3f axisA;
    calcQuatLocalAxis(&axisB, rQuatB, axis);
    calcQuatLocalAxis(&axisA, rQuatA, axis);
    if (axisA.dot(axisB) >= 0.0f) {
        axisA = axisB;
    } else {
        axisA = -axisB;
    }

    s32 nextAxis = (axis + 1) % 3;
    s32 lastAxis = (axis + 2) % 3;
    sead::Vector3f nextAxisB;
    sead::Vector3f lastAxisB;
    sead::Vector3f nextAxisA;
    calcQuatLocalAxis(&nextAxisB, rQuatB, nextAxis);
    calcQuatLocalAxis(&lastAxisB, rQuatB, lastAxis);
    calcQuatLocalAxis(&nextAxisA, rQuatA, nextAxis);
    calcNearVecFromAxis2(&nextAxisA, nextAxisA, nextAxisB, lastAxisB);
    makeQuatFromTwoAxis(pOutQuat, axisA, nextAxisA, axis, nextAxis);
}

void calcFittingBoxPose(sead::Quatf* pOutQuat, const sead::BoundBox3f& rBox,
                        const sead::Quatf& rQuatA, const sead::Quatf& rQuatB) {
    sead::Vector3f size = rBox.getMax() - rBox.getMin();

    if (isNearZero(size.x - size.y)) {
        if (isNearZero(size.y - size.z)) {
            calcFittingBoxPoseEqualAxisAll(pOutQuat, rQuatA, rQuatB);
            return;
        }

        calcFittingBoxPoseEqualAxisTwo(pOutQuat, rQuatA, rQuatB, 2);
        return;
    }

    if (isNearZero(size.y - size.z)) {
        calcFittingBoxPoseEqualAxisTwo(pOutQuat, rQuatA, rQuatB, 0);
        return;
    }

    if (isNearZero(size.z - size.x)) {
        calcFittingBoxPoseEqualAxisTwo(pOutQuat, rQuatA, rQuatB, 1);
        return;
    }

    calcFittingBoxPoseEqualAxisNone(pOutQuat, rQuatA, rQuatB);
}

/**
 * Calculates the foot of the perpendicular from a point to a line segment, clamped to the segment.
 * @param pOut receives the foot of the perpendicular
 * @param rPos point to project
 * @param rLineStart start of the segment
 * @param rLineEnd end of the segment
 * @return Clamped fraction along the line segment.
 */
f32 calcPerpendicFootToLineInside(sead::Vector3f* pOut, const sead::Vector3f& rPos,
                                   const sead::Vector3f& rLineStart,
                                   const sead::Vector3f& rLineEnd) {
    sead::Vector3f dir = rLineEnd;
    dir -= rLineStart;
    f32 rate = (dir.dot(rPos) - rLineStart.dot(dir)) / dir.squaredLength();
    rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);
    pOut->e = rLineStart.e;
    pOut->setScaleAdd(rate, dir, *pOut);
    return rate;
}

bool calcReflectionVector(sead::Vector3f* vec, const sead::Vector3f& normal, f32 reboundRate,
                          f32 minSink) {
    f32 dot = normal.dot(*vec);

    if (dot < -minSink) {
        calcReverseVector(vec, normal, reboundRate);
        return true;
    }

    if (dot < 0.0f)
        *vec -= dot * normal;
    return false;
}

void calcParabolicFunctionParam(f32* pGravity, f32* pInitialVelY, f32 maxHeight,
                                f32 verticalDistance) {
    f32 discriminant = (maxHeight - verticalDistance) * maxHeight;

    if (discriminant <= 0.0f) {
        *pGravity = -maxHeight;
        *pInitialVelY = maxHeight + verticalDistance;
        return;
    }

    if (isNearZero(verticalDistance, 0.0001f)) {
        *pGravity = maxHeight * -4.0f;
        *pInitialVelY = maxHeight * 4.0f;
        return;
    }

    f32 root = sead::Mathf::sqrt(discriminant);
    f32 time = (root + maxHeight) / verticalDistance;

    if (!(time >= 0.0f && time <= 1.0f)) {
        f32 otherTime = (maxHeight - root) / verticalDistance;
        time = (otherTime >= 0.0f && otherTime <= 1.0f) ? otherTime : 1.0f;
    }

    *pGravity = -maxHeight / (time * time);
    *pInitialVelY = time * -2.0f * *pGravity;
}

/**
 * Interpolates between two values with a decaying vibration.
 * @param rate interpolation rate in [0, 1]
 * @param start value at rate 0
 * @param end value at rate 1
 * @param amplitude strength of the vibration
 * @param frequency frequency of the vibration
 * @return the interpolated value
 */
f32 calcConvergeVibrationValue(f32 rate, f32 start, f32 end, f32 amplitude, f32 frequency) {
    f32 rateSq = rate * rate;
    f32 invRate = 1.0f - rate;
    f32 invRateSq = invRate * invRate;
    f32 invRateQuad = invRateSq * invRateSq;
    f32 vibration = invRate * amplitude *
                    sead::Mathf::sin((rateSq * rateSq * frequency + rate) * sead::Mathf::pi());
    f32 value = (1.0f - invRateQuad) + vibration;
    return (1.0f - value) * start + value * end;
}

bool calcSphericalPolarCoordPY(sead::Vector2f* pOutCoord, const sead::Vector3f& rDir,
                               const sead::Vector3f& rFront, const sead::Vector3f& rUp) {
    if (isNearZero(rDir)) {
        return false;
    }

    if (isParallelDirection(rUp, rDir, 0.01f)) {
        pOutCoord->x = rDir.dot(rUp) > 0.0f ? sead::Mathf::piHalf() : -sead::Mathf::piHalf();
        pOutCoord->y = 0.0f;
        return false;
    }

    sead::Vector3f side;
    side.setCross(rUp, rFront);
    sead::Vector3f local(rDir.dot(rUp), rDir.dot(side), rDir.dot(rFront));
    f32 length = local.length();
    f32 horizontalLength = sead::Vector2f(local.z, local.y).length();
    pOutCoord->x = sead::Mathf::asin(sead::Mathf::clamp(local.x / length, -1.0f, 1.0f));
    pOutCoord->y = sign(local.y) *
                   sead::Mathf::acos(sead::Mathf::clamp(local.z / horizontalLength, -1.0f, 1.0f));
    return true;
}

const char* axisIndexToString(s32 axisIndex) {
    switch (axisIndex) {
    case 0:
        return "X";
    case 1:
        return "Y";
    case 2:
        return "Z";
    default:
        return "UnKnown";
    }
}

void visitCellsOverlapped(const sead::Vector3f& rStart, const sead::Vector3f& rEnd, f32 cellSize,
                          const VisitCellCallBack& rCallBack) {
    s32 stepX = rStart.x < rEnd.x ? 1 : (rStart.x > rEnd.x ? -1 : 0);
    s32 stepY = rStart.y < rEnd.y ? 1 : (rStart.y > rEnd.y ? -1 : 0);
    s32 stepZ = rStart.z < rEnd.z ? 1 : (rStart.z > rEnd.z ? -1 : 0);

    f32 cellStartX = rStart.x / cellSize;
    f32 lengthX = sead::Mathf::abs(rEnd.x - rStart.x);
    f32 maxX = sead::Mathf::maxNumber();

    if (!(lengthX < 1e-6f)) {
        f32 cellPos = std::floor(cellStartX) * cellSize;
        f32 dist = rStart.x > rEnd.x ? rStart.x - cellPos : cellPos + cellSize - rStart.x;
        maxX = dist / lengthX;
    }

    f32 cellStartY = rStart.y / cellSize;
    f32 lengthY = sead::Mathf::abs(rEnd.y - rStart.y);
    f32 maxY = sead::Mathf::maxNumber();

    if (!(lengthY < 1e-6f)) {
        f32 cellPos = std::floor(cellStartY) * cellSize;
        f32 dist = rStart.y > rEnd.y ? rStart.y - cellPos : cellPos + cellSize - rStart.y;
        maxY = dist / lengthY;
    }

    f32 cellStartZ = rStart.z / cellSize;
    f32 lengthZ = sead::Mathf::abs(rEnd.z - rStart.z);
    f32 maxZ = sead::Mathf::maxNumber();

    if (!(lengthZ < 1e-6f)) {
        f32 cellPos = std::floor(cellStartZ) * cellSize;
        f32 dist = rStart.z > rEnd.z ? rStart.z - cellPos : cellPos + cellSize - rStart.z;
        maxZ = dist / lengthZ;
    }

    f32 deltaX = 0.0f;
    f32 deltaY = 0.0f;
    f32 deltaZ = 0.0f;

    if (!(lengthX < 1e-6f)) {
        deltaX = cellSize / lengthX;
    }

    if (!(lengthY < 1e-6f)) {
        deltaY = cellSize / lengthY;
    }

    if (!(lengthZ < 1e-6f)) {
        deltaZ = cellSize / lengthZ;
    }

    s32 x = static_cast<s32>(std::floor(cellStartX));
    s32 y = static_cast<s32>(std::floor(cellStartY));
    s32 z = static_cast<s32>(std::floor(cellStartZ));
    s32 endX = static_cast<s32>(std::floor(rEnd.x / cellSize));
    s32 endY = static_cast<s32>(std::floor(rEnd.y / cellSize));
    s32 endZ = static_cast<s32>(std::floor(rEnd.z / cellSize));

    while (true) {
        rCallBack.visit(x, y, z);

        if (maxX <= maxY && maxX <= maxZ) {
            if (x == endX) {
                return;
            }

            maxX += deltaX;
            x += stepX;
        } else if (maxY <= maxX && maxY <= maxZ) {
            if (y == endY) {
                return;
            }

            maxY += deltaY;
            y += stepY;
        } else {
            if (z == endZ) {
                return;
            }

            maxZ += deltaZ;
            z += stepZ;
        }
    }
}

f32 calcMultValueToDestination(u32 step, f32 start, f32 end) {
    return powf(10.0f, log10f(end / start) / step);
}

f32 getHaltonSequence(u32 primeIndex, u32 index) {
    u32 base = sPrimeNumbers[primeIndex];
    f32 invBase = 1.0f / (f32)base;

    f32 result = 0.0f;
    f32 f = invBase;

    do {
        u32 digit = index % base;
        u32 fraction = index - digit;

        result += f * digit;
        index = fraction * invBase;
        f *= invBase;
    } while (index != 0);

    return result;
}

f32 calcFractal(f32 x, f32 y, u32 permutations, f32 amplitude, f32 scale, f32 nextOrderAmplitude,
                bool useSmoothPerlingNoise) {
    FractalGenerator generator(permutations, amplitude, scale, nextOrderAmplitude);
    return generator.calcFractal(x, y, useSmoothPerlingNoise);
}

f32 calcMultiFractal(f32 x, f32 y, f32 baseAmplitude, u32 permutations, f32 amplitude, f32 scale,
                     f32 nextOrderAmplitude, bool useSmoothPerlingNoise) {
    FractalGenerator generator(permutations, amplitude, scale, nextOrderAmplitude);
    return generator.calcMultiFractal(x, y, baseAmplitude, useSmoothPerlingNoise);
}

f32 calcNormalDistribution(f32 x, f32 mu, f32 sigma) {
    return (1.0f / sead::Mathf::sqrt(sigma * sead::Mathf::pi2())) *
           sead::Mathf::exp(sead::Mathf::pow(x - mu, 2) * -0.5f / sigma);
}

void calcVecViewInput(sead::Vector3f* pOutVec, const sead::Vector2f& rInput,
                      const sead::Vector3f& rUp, const sead::Matrix34f* pViewMtx) {
    sead::Vector3f camSide(pViewMtx->m[0][0], pViewMtx->m[0][1], pViewMtx->m[0][2]);
    sead::Vector3f camUp(pViewMtx->m[1][0], pViewMtx->m[1][1], pViewMtx->m[1][2]);
    sead::Vector3f camFront(-pViewMtx->m[2][0], -pViewMtx->m[2][1], -pViewMtx->m[2][2]);
    f32 dotUp = rUp.dot(camUp);
    f32 dotFront = rUp.dot(camFront);
    sead::Vector3f dir;

    if (sead::Mathf::abs(dotFront) > sead::Mathf::abs(dotUp)) {
        dir = dotFront < 0.0f ? -camUp : camUp;
    } else {
        dir = dotUp < 0.0f ? camFront : -camFront;
    }

    sead::Vector3f side;
    side.setCross(rUp, dir);
    tryNormalizeOrZero(&side);
    sead::Vector3f front;
    front.setCross(rUp, camSide);
    tryNormalizeOrZero(&front);
    pOutVec->set(side * rInput.x + front * rInput.y);
}

bool calcDirViewInput(sead::Vector3f* pOutVec, const sead::Vector2f& rInput,
                      const sead::Vector3f& rUp, const sead::Matrix34f* pViewMtx) {
    if (isNearZero(rInput)) {
        pOutVec->set(sead::Vector3f::zero);
        return false;
    }

    calcVecViewInput(pOutVec, rInput, rUp, pViewMtx);
    tryNormalizeOrZero(pOutVec);
    return true;
}

/**
 * Builds a Bayer dithering matrix.
 * @param outMtx receives (1 << size) * (1 << size) threshold values
 * @param size log2 of the matrix width
 */
void makeBayerMatrix(s32* outMtx, s32 size) {
    for (s32 y = 0; y < 1 << size; ++y) {
        for (s32 x = 0; x < 1 << size; x++) {
            s32 value = 0;
            s32 shift = 0;

            for (s32 k = size; k != 0 && size > 0; k--) {
                s32 bitY = (y % (1 << k)) / (1 << (k - 1));
                s32 bitX = (x % (1 << k)) / (1 << (k - 1));

                value += bayerMatrix2[bitY][bitX] << shift;
                shift += 2;
            }

            outMtx[x + (y << size)] = value;
        }
    }
}

bool calcDirH(sead::Vector3f* outVec, const sead::Vector3f& vecA, const sead::Vector3f& vecB) {
    return calcDirOnPlane(outVec, vecA, vecB, sead::Vector3f::ey);
}

bool calcDirOnPlane(sead::Vector3f* outVec, const sead::Vector3f& vecA, const sead::Vector3f& vecB,
                    const sead::Vector3f& plane) {
    outVec->setSub(vecB, vecA);
    outVec->setScaleAdd(-plane.dot(*outVec), plane, *outVec);
    return !tryNormalizeOrZero(outVec);
}

void calcDirFromLongitudeLatitude(sead::Vector3f* outVec, f32 longitude, f32 latitude) {
    outVec->y = -sead::Mathf::sin(sead::Mathf::deg2rad(latitude));
    f32 cosLatitude = -sead::Mathf::cos(sead::Mathf::deg2rad(latitude));
    outVec->x = sead::Mathf::sin(sead::Mathf::deg2rad(longitude)) * cosLatitude;
    outVec->z = sead::Mathf::cos(sead::Mathf::deg2rad(longitude)) * cosLatitude;
}

void calcLongitudeLatitudeFromDir(f32* longitude, f32* latitude, const sead::Vector3f& dir) {
    sead::Vector3f dirNormalized = dir;
    dirNormalized.normalize();

    if (isNearZero(dirNormalized))
        return;
    *latitude = sead::Mathf::asin(sead::Mathf::clamp(-dirNormalized.y, -1.0f, 1.0f));

    sead::Vector2f newVec = {-dirNormalized.z, -dirNormalized.x};
    newVec.normalize();

    if (isNearZero(newVec))
        return;

    *longitude = sead::Mathf::atan2(newVec.y, newVec.x);
}

}  // namespace al

namespace Intersect {

bool calcX(sead::Vector3f* outVec, f32 value, const sead::Vector3f& vectorA,
           const sead::Vector3f& vectorB, const sead::Vector3f& min, const sead::Vector3f& max) {
    f32 x = (value - vectorA.x) / vectorB.x;

    if ((x < 0.0f || x != 1.0f) && (x < 0.0f || 1.0f <= x))
        return false;

    f32 y = vectorA.y + x * vectorB.y;

    if (!(min.y <= y && y <= max.y))
        return false;

    f32 z = vectorA.z + x * vectorB.z;

    if (!(min.z <= z && z <= max.z))
        return false;

    x = vectorA.x + vectorB.x * x;

    if (outVec) {
        outVec->x = x;
        outVec->y = y;
        outVec->z = z;
    }

    return true;
}

bool calcY(sead::Vector3f* outVec, f32 value, const sead::Vector3f& vectorA,
           const sead::Vector3f& vectorB, const sead::Vector3f& min, const sead::Vector3f& max) {
    f32 y = (value - vectorA.y) / vectorB.y;

    if ((y < 0.0f || y != 1.0f) && (y < 0.0f || 1.0f <= y))
        return false;

    f32 x = vectorA.x + y * vectorB.x;

    if (!(min.x <= x && x <= max.x))
        return false;

    f32 z = vectorA.z + y * vectorB.z;

    if (!(min.z <= z && z <= max.z))
        return false;
    y = vectorA.y + vectorB.y * y;

    if (outVec) {
        outVec->x = x;
        outVec->y = y;
        outVec->z = z;
    }

    return true;
}

bool calcZ(sead::Vector3f* outVec, f32 value, const sead::Vector3f& vectorA,
           const sead::Vector3f& vectorB, const sead::Vector3f& min, const sead::Vector3f& max) {
    f32 z = (value - vectorA.z) / vectorB.z;

    if ((z < 0.0f || z != 1.0f) && (z < 0.0f || 1.0f <= z))
        return false;

    f32 x = vectorA.x + z * vectorB.x;

    if (!(min.x <= x && x <= max.x))
        return false;
    f32 y = vectorA.y + z * vectorB.y;

    if (!(min.y <= y && y <= max.y))
        return false;

    z = vectorA.z + vectorB.z * z;

    if (outVec) {
        outVec->x = x;
        outVec->y = y;
        outVec->z = z;
    }

    return true;
}

}  // namespace Intersect
