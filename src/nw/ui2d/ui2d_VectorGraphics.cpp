#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/font/font_GpuBuffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_Shader.h>
#include <nn/gfx/gfx_State.h>
#include <nn/gfx/gfx_StateInfo.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/gfx/gfx_TextureInfo.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/util/util_FormatString.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_BytePtr.h>
#include <nn/util/util_Constants.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace nn::ui2d::detail {

namespace {

/**
 * @brief Reserves space at the back of the mapped constant buffer.
 * @param pBuffer Constant buffer to allocate from.
 * @param size Byte count to reserve.
 * @return CPU pointer to the reserved block.
 */
void* AllocateFromConstantBufferBack(nn::font::GpuBuffer* pBuffer, size_t size) {
    uint64_t allocatedSize;

    if ((pBuffer->m_Flags & nn::font::GpuBuffer::Flag_AtomicAllocation) == 0) {
        allocatedSize = pBuffer->m_AllocatedSize2 += size;
    } else {
        allocatedSize =
            reinterpret_cast<std::atomic<uint64_t>*>(pBuffer->m_pAtomicAllocatedSize2)->fetch_add(
                size) +
            size;
    }

    return nn::util::BytePtr(pBuffer->GetMappedPointer(), pBuffer->m_BufferSize - allocatedSize)
        .Get();
}

/**
 * @brief Reserves space at the front of the mapped constant buffer.
 * @param pBuffer Constant buffer to allocate from.
 * @param size Byte count to reserve.
 * @return Offset of the reserved block from the start of the buffer.
 */
uint64_t AllocateFromConstantBufferFront(nn::font::GpuBuffer* pBuffer, size_t size) {
    if ((pBuffer->m_Flags & nn::font::GpuBuffer::Flag_AtomicAllocation) == 0) {
        uint64_t offset = pBuffer->m_AllocatedSize;
        pBuffer->m_AllocatedSize += size;
        return offset;
    }

    return reinterpret_cast<std::atomic<uint64_t>*>(pBuffer->m_pAtomicAllocatedSize)->fetch_add(
        size);
}

typedef nn::gfx::detail::CommandBufferImpl<nn::gfx::ApiVariationNvn8> CommandBufferImpl;
typedef nn::gfx::detail::DepthStencilViewImpl<nn::gfx::ApiVariationNvn8> DepthStencilViewImpl;
typedef nn::gfx::detail::RasterizerStateImpl<nn::gfx::ApiVariationNvn8> RasterizerStateImpl;

/**
 * @brief Gives access to the API implementation of a command buffer.
 * @param rCommandBuffer Command buffer to access.
 */
CommandBufferImpl& ToImpl(nn::gfx::CommandBuffer& rCommandBuffer) {
    return rCommandBuffer;
}

/**
 * @brief Gives access to the API implementation of a depth stencil view.
 * @param rView View to access.
 */
DepthStencilViewImpl& ToImpl(nn::gfx::DepthStencilView& rView) {
    return rView;
}

/**
 * @brief Gives access to the API implementation of a rasterizer state.
 * @param rState State to access.
 */
RasterizerStateImpl& ToImpl(nn::gfx::RasterizerState& rState) {
    return rState;
}

/**
 * @brief Multiplies one row of an affine matrix by another affine matrix.
 * @param row Row of the left-hand matrix.
 * @param rRight Right-hand matrix.
 * @return Row of the product.
 */
float32x4_t MultiplyMatrixRow(float32x4_t row, const nn::util::MatrixT4x3fType& rRight) {
    float32x4_t result = vmulq_laneq_f32(rRight._m.val[0], row, 0);
    result = vfmaq_laneq_f32(result, rRight._m.val[1], row, 1);
    result = vfmaq_laneq_f32(result, rRight._m.val[2], row, 2);
    const float32x4_t translate = vcopyq_lane_f32(vdupq_n_f32(0.0f), 3, vget_high_f32(row), 1);
    return vaddq_f32(translate, result);
}

/**
 * @brief Multiplies two affine matrices.
 * @param pOut Receives rLeft * rRight.
 */
void MultiplyMatrix(nn::util::MatrixT4x3fType* pOut, const nn::util::MatrixT4x3fType& rLeft,
                    const nn::util::MatrixT4x3fType& rRight) {
    const float32x4_t row0 = MultiplyMatrixRow(rLeft._m.val[0], rRight);
    const float32x4_t row1 = MultiplyMatrixRow(rLeft._m.val[1], rRight);
    const float32x4_t row2 = MultiplyMatrixRow(rLeft._m.val[2], rRight);
    pOut->_m.val[0] = row0;
    pOut->_m.val[1] = row1;
    pOut->_m.val[2] = row2;
}

/**
 * @brief Multiplies a projection matrix by an affine matrix.
 * @param pOut Receives rLeft * rRight.
 */
void MultiplyMatrix(nn::util::MatrixT4x4fType* pOut, const nn::util::MatrixT4x4fType& rLeft,
                    const nn::util::MatrixT4x3fType& rRight) {
    const float32x4_t row0 = MultiplyMatrixRow(rLeft._m.val[0], rRight);
    const float32x4_t row1 = MultiplyMatrixRow(rLeft._m.val[1], rRight);
    const float32x4_t row2 = MultiplyMatrixRow(rLeft._m.val[2], rRight);
    const float32x4_t row3 = MultiplyMatrixRow(rLeft._m.val[3], rRight);
    pOut->_m.val[0] = row0;
    pOut->_m.val[1] = row1;
    pOut->_m.val[2] = row2;
    pOut->_m.val[3] = row3;
}

/**
 * @brief Converts degrees to a sine table index.
 * @param degree Angle in degrees.
 */
nn::util::AngleIndex DegreeToAngleIndex(float degree) {
    return static_cast<int64_t>(
        degree * (nn::util::detail::AngleIndexHalfRound / nn::util::detail::FloatDegree180));
}

/**
 * @brief Builds an orthographic projection covering a scene.
 * @param pOut Receives the projection.
 * @param width Scene width.
 * @param height Scene height.
 * @param isFlipped Whether the vertical axis points up.
 */
void MakeSceneProjection(nn::util::MatrixT4x4fType* pOut, float width, float height,
                         bool isFlipped) {
    const float left = 0.0f;
    const float right = width;
    const float top = isFlipped ? 0.0f : height;
    const float bottom = isFlipped ? height : 0.0f;
    const float nearZ = -500.0f;
    const float farZ = 500.0f;

    const float invWidth = 1.0f / (right - left);
    const float invHeight = 1.0f / (top - bottom);
    pOut->_m.val[0] = float32x4_t{2.0f * invWidth, 0.0f, 0.0f, -(right + left) * invWidth};
    pOut->_m.val[1] = float32x4_t{0.0f, 2.0f * invHeight, 0.0f, -(top + bottom) * invHeight};
    pOut->_m.val[2] =
        float32x4_t{0.0f, 0.0f, -2.0f / (farZ - nearZ), -(farZ + nearZ) / (farZ - nearZ)};
    pOut->_m.val[3] = float32x4_t{0.0f, 0.0f, 0.0f, 1.0f};
}

/**
 * @brief Checks whether the layout is drawn with a flipped vertical axis.
 * @param rDrawInfo Drawing state.
 */
bool IsYAxisFlipped(const DrawInfo& rDrawInfo) {
    return (rDrawInfo.mFlags & 8) != 0;
}

}  // namespace

/**
 * @brief Copies a vector into a packed float pair, one component at a time.
 * @param pOut Destination pair.
 * @param rVector Source vector.
 */
void SetFloat2(nn::util::Float2* pOut, const nn::util::Vector2f& rVector) {
    pOut->x = rVector.GetX();
    pOut->y = rVector.GetY();
}

/**
 * @brief Describes one stroked segment of a generated path.
 * @param pInfo Destination segment description.
 * @param start First point of the segment.
 * @param pCurvePoints Points that follow the start point.
 * @param count Number of points at pCurvePoints.
 * @param ignore Whether stroke generation skips this segment.
 */
void SetStrokeSegmentVertexInfo(StrokeSegmentVertexInfo* pInfo, const nn::util::Vector2f& start,
                                nn::util::Float2* pCurvePoints, int count, bool ignore) {
    SetFloat2(&pInfo->start, start);
    pInfo->pCurvePoints = pCurvePoints;
    pInfo->count = count;
    pInfo->ignore = ignore;
}

/**
 * @brief Finds the bezier parameter whose x coordinate equals the given value.
 * @param pKey Key holding the easing control points.
 * @param x Normalized time inside the key.
 * @return Bezier parameter in the range [0, 1].
 */
float SolveTime(const ResBnvgAnimationKey* pKey, float x) {
    const float p0 = 0.0f;
    const float p1 = pKey->easeIn.x;
    const float p2 = pKey->easeOut.x;
    const float p3 = 1.0f;

    const float a = -p0 + 3.0f * p1 - 3.0f * p2 + p3;
    const float b = 3.0f * p0 - 6.0f * p1 + 3.0f * p2;
    const float c = 3.0f * p1 - 3.0f * p0;
    const float d = p0 - x;

    const float b2 = b / a;
    const float c2 = c / a;
    const float d2 = d / a;

    const float q = (b2 * b2 - 3.0f * c2) / 9.0f;
    const float r = (2.0f * b2 * b2 * b2 - 9.0f * b2 * c2 + 27.0f * d2) / 54.0f;
    const float q3 = q * q * q;
    const float discriminant = r * r - q3;

    if (discriminant > 0.0f) {
        const float sign = std::signbit(r) ? 1.0f : -1.0f;
        const float s = std::pow(std::fabs(r) + std::sqrt(discriminant), 1.0f / 3.0f);
        return sign * (s + q / s) + b2 / -3.0f;
    }

    const float theta = std::acos(r * (1.0f / std::sqrt(q3)));
    const float m = -2.0f * std::sqrt(q);
    const float offset = b2 / 3.0f;

    float t = m * std::cos(theta / 3.0f) - offset;

    if (t >= 0.0f && t <= 1.0f) {
        return t;
    }

    t = m * std::cos((theta + (nn::util::FloatPi + nn::util::FloatPi)) / 3.0f) - offset;

    if (t >= 0.0f && t <= 1.0f) {
        return t;
    }

    t = m * std::cos((theta + nn::util::FloatPi * 4.0f) / 3.0f) - offset;

    if (t >= 0.0f && t <= 1.0f) {
        return t;
    }

    return x;
}

/**
 * @brief Evaluates the easing curve of a key.
 * @param pKey Key holding the easing control points.
 * @param x Normalized time inside the key.
 * @return Eased progress between the start and end value.
 */
float EvaluateBezierYFromX(const ResBnvgAnimationKey* pKey, float x) {
    const float t = SolveTime(pKey, x);
    const float it = 1.0f - t;

    return 3.0f * t * it * it * pKey->easeIn.y + 3.0f * t * t * it * pKey->easeOut.y + t * t * t;
}

/**
 * @brief Checks whether a value has more than one key.
 * @param rValue Value to check.
 * @return True when the value changes over time.
 */
bool IsValueAnimated(const ResBnvgAnimatableValue& rValue) {
    return rValue.keyCount > 1;
}

/**
 * @brief Evaluates an animatable scalar.
 * @param rValue Value to evaluate.
 * @param time Frame to evaluate at.
 * @return Value at the given frame.
 */
float GetValue(const ResBnvgAnimatableValue& rValue, float time) {
    const int keyCount = rValue.keyCount;

    if (keyCount <= 1) {
        return rValue.value;
    }

    const ResBnvgAnimationKey* pKeys = rValue.pKeys;
    const ResBnvgAnimationKey* pKey = nullptr;
    float rate = 0.0f;

    if (time <= pKeys[0].frame) {
        pKey = &pKeys[0];
        rate = 0.0f;
    } else if (time >= pKeys[keyCount - 1].frame) {
        pKey = &pKeys[keyCount + (pKeys[keyCount - 1].isHold ? -1 : -2)];
        rate = 1.0f;
    } else {
        for (int i = 0; i < keyCount - 1; ++i) {
            pKey = &pKeys[i];

            if (pKeys[i].frame <= time && time < pKeys[i + 1].frame) {
                float linearRate = (time - pKeys[i].frame) / (pKeys[i + 1].frame - pKeys[i].frame);
                linearRate = std::min(std::max(linearRate, 0.0f), 1.0f);
                rate = EvaluateBezierYFromX(pKey, linearRate);
                break;
            }
        }
    }

    if (pKey == nullptr) {
        return 0.0f;
    }

    return pKey->startValue + rate * (pKey->endValue - pKey->startValue);
}

/**
 * @brief Evaluates an animatable 2D value moving along cubic bezier segments.
 * @param rValue Value to evaluate.
 * @param time Frame to evaluate at.
 * @return Value at the given frame.
 */
const nn::util::Vector2f GetValue(const ResBnvgBezierAnimatableValue2D& rValue, float time) {
    if (rValue.keyCount < 2) {
        return nn::util::Vector2f(rValue.value.x, rValue.value.y);
    }

    const ResBnvgBezierAnimationKey2D* pKey = nullptr;

    for (int i = 0; i < rValue.keyCount - 1; ++i) {
        pKey = &rValue.pKeys[i];

        if (rValue.pKeys[i + 1].frame > time && rValue.pKeys[i].frame <= time) {
            break;
        }
    }

    if (pKey == nullptr) {
        return nn::util::Vector2f(0.0f, 0.0f);
    }

    const nn::util::Vector2f p0(pKey->startValue.x, pKey->startValue.y);
    const nn::util::Vector2f p1(pKey->startValue.x + pKey->startTangent.x,
                                pKey->startValue.y + pKey->startTangent.y);
    const nn::util::Vector2f p2(pKey->endValue.x + pKey->endTangent.x,
                                pKey->endValue.y + pKey->endTangent.y);
    const nn::util::Vector2f p3(pKey->endValue.x, pKey->endValue.y);

    float t = (time - pKey->frame) / (pKey[1].frame - pKey->frame);
    t = std::min(std::max(t, 0.0f), 1.0f);
    const float it = 1.0f - t;

    nn::util::Vector2f result;
    nn::util::Vector2f term;
    nn::util::VectorMultiply(&result, p0, it * it * it);
    nn::util::VectorMultiply(&term, p1, it * (3.0f * t * it));
    nn::util::VectorAdd(&result, result, term);
    nn::util::VectorMultiply(&term, p2, it * (3.0f * t * t));
    nn::util::VectorAdd(&result, term, result);
    nn::util::VectorMultiply(&term, p3, t * t * t);
    nn::util::VectorAdd(&result, term, result);
    return result;
}

/**
 * @brief Calculates how many segments approximate a quarter circle.
 * @param radius Radius of the circle.
 * @return Number of segments for a quarter circle.
 */
int CalculateQuaterCircleDivideCount(float radius) {
    const float angle = std::acos(1.0f - 2.0f / radius);

    if (angle > 0.0f) {
        return static_cast<int>(nn::util::FloatPi / angle) + 1;
    }

    return 1;
}

/**
 * @brief Returns the midpoint of two points.
 * @param rA First point.
 * @param rB Second point.
 * @return Average of both points.
 */
const nn::util::Vector2f AcquireAverageVector(const nn::util::Vector2f& rA,
                                              const nn::util::Vector2f& rB) {
    return nn::util::Vector2f((rA.GetX() + rB.GetX()) * 0.5f, (rA.GetY() + rB.GetY()) * 0.5f);
}

/**
 * @brief Returns the squared distance from a point to a line segment.
 * @param rPoint Point to measure.
 * @param rStart Start of the segment.
 * @param rEnd End of the segment.
 * @return Squared distance.
 */
float SquaredDistanceToLine(const nn::util::Vector2f& rPoint, const nn::util::Vector2f& rStart,
                            const nn::util::Vector2f& rEnd) {
    nn::util::Vector2f line;
    nn::util::VectorSubtract(&line, rEnd, rStart);
    nn::util::Vector2f toPoint;
    nn::util::VectorSubtract(&toPoint, rPoint, rStart);

    const float dot = nn::util::VectorDot(line, toPoint);

    if (dot <= 0.0f) {
        return nn::util::VectorLengthSquared(toPoint);
    }

    const float lengthSquared = nn::util::VectorLengthSquared(line);

    if (dot > lengthSquared) {
        nn::util::Vector2f toEnd;
        nn::util::VectorSubtract(&toEnd, rEnd, rPoint);
        return nn::util::VectorLengthSquared(toEnd);
    }

    const float cross = nn::util::VectorCross(line, toPoint);
    return cross * cross / lengthSquared;
}

/**
 * @brief Returns the distance between two points.
 * @param rA First point.
 * @param rB Second point.
 * @return Euclidean distance.
 */
float Float2Distance(const nn::util::Float2& rA, const nn::util::Float2& rB) {
    const float dx = rA.x - rB.x;
    const float dy = rA.y - rB.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Returns the counter-clockwise angle between two unit vectors.
 * @param rA First direction.
 * @param rB Second direction.
 * @return Angle in radians in the range [0, 2pi).
 */
float Vector2Angle(const nn::util::Vector2f& rA, const nn::util::Vector2f& rB) {
    const float cross = nn::util::VectorCross(rA, rB);
    float angle = std::acos(nn::util::VectorDot(rA, rB));

    if (cross < 0.0f) {
        angle = nn::util::FloatPi + (nn::util::FloatPi - angle);
    }

    return angle;
}

/**
 * @brief Normalizes a vector, collapsing nearly zero vectors to zero.
 * @param pVector Vector to normalize in place.
 */
void NormalizeWithTruncateAroundZero(nn::util::Vector2f* pVector) {
    if (nn::util::VectorLength(*pVector) < 1.0f / 4096.0f) {
        *pVector = nn::util::Vector2f(vdup_n_f32(0.0f));
        return;
    }

    nn::util::VectorNormalize(pVector, *pVector);
}

/**
 * @brief Calculates how many points approximate a cubic bezier curve.
 * @param pControlPoints The four control points of the curve.
 * @param tolerance Maximum allowed distance between the curve and its approximation.
 * @return Number of points, rounded up to a power of two.
 */
int AcquireCubicPointCount(const nn::util::Vector2f* pControlPoints, float tolerance) {
    const float distance1 =
        SquaredDistanceToLine(pControlPoints[1], pControlPoints[0], pControlPoints[3]);
    const float distance2 =
        SquaredDistanceToLine(pControlPoints[2], pControlPoints[0], pControlPoints[3]);
    const float distance = std::sqrt(std::max(distance1, distance2));

    if (distance <= tolerance) {
        return 1;
    }

    float count = std::ceil(std::sqrt(distance / tolerance));
    ClampValue(count, 1.0f, 1024.0f);

    int value = static_cast<int>(count) - 1;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;

    uint32_t bits = ~value;
    bits = bits - ((bits >> 1) & 0x55555555);
    bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
    bits = (bits + (bits >> 4)) & 0x0F0F0F0F;
    bits = bits + (bits >> 8);
    bits = bits + (bits >> 16);
    return 1 << (32 - (bits & 0x3F));
}

/**
 * @brief Writes the points approximating a cubic bezier curve by recursive subdivision.
 * @param rP0 Start point.
 * @param rP1 First control point.
 * @param rP2 Second control point.
 * @param rP3 End point.
 * @param tolerance Squared flatness below which the curve is a line.
 * @param ppVertex Write cursor, advanced past the written points.
 * @param divideCount Remaining subdivision budget.
 * @return Number of points written.
 */
int CreateCubicPoints(const nn::util::Vector2f& rP0, const nn::util::Vector2f& rP1,
                      const nn::util::Vector2f& rP2, const nn::util::Vector2f& rP3,
                      float tolerance, nn::util::Float2** ppVertex, uint32_t divideCount) {
    if (divideCount < 2 || (SquaredDistanceToLine(rP1, rP0, rP3) < tolerance &&
                            SquaredDistanceToLine(rP2, rP0, rP3) < tolerance)) {
        (*ppVertex)->x = rP3.GetX();
        (*ppVertex)->y = rP3.GetY();
        ++*ppVertex;
        return 1;
    }

    const nn::util::Vector2f p01 = AcquireAverageVector(rP0, rP1);
    const nn::util::Vector2f p12 = AcquireAverageVector(rP1, rP2);
    const nn::util::Vector2f p23 = AcquireAverageVector(rP2, rP3);
    const nn::util::Vector2f left[3] = {p01, AcquireAverageVector(p01, p12),
                                        AcquireAverageVector(p12, p23)};
    const nn::util::Vector2f right[2] = {left[2], p23};
    const nn::util::Vector2f middle = AcquireAverageVector(left[1], left[2]);

    const uint32_t halfCount = divideCount / 2;
    const int count =
        CreateCubicPoints(rP0, left[0], left[1], middle, tolerance, ppVertex, halfCount);
    return CreateCubicPoints(middle, right[0], right[1], rP3, tolerance, ppVertex, halfCount) +
           count;
}

/**
 * @brief Builds the four bezier control points of one path segment.
 * @param pVectors Receives start point, two control points and end point.
 * @param pControlPoints Evaluated control points of the path.
 * @param index Index of the segment's first control point.
 * @param controlPointCount Number of control points of the path.
 */
void SetVectorsWithEvaluatedControlPoints(nn::util::Vector2f* pVectors,
                                          const EvaluatedControlPoint* pControlPoints, int index,
                                          int controlPointCount) {
    const EvaluatedControlPoint& rCurrent = pControlPoints[index];
    pVectors[0] = nn::util::Vector2f(rCurrent.position.x, rCurrent.position.y);
    pVectors[1] = nn::util::Vector2f(rCurrent.position.x + rCurrent.outTangent.x,
                                     rCurrent.position.y + rCurrent.outTangent.y);

    const EvaluatedControlPoint& rNext = pControlPoints[(index + 1) % controlPointCount];
    pVectors[3] = nn::util::Vector2f(rNext.position.x, rNext.position.y);
    pVectors[2] = nn::util::Vector2f(rNext.position.x + rNext.inTangent.x,
                                     rNext.position.y + rNext.inTangent.y);
}

/**
 * @brief Appends one vertex.
 * @param ppVertex Write cursor, advanced past the vertex.
 * @param pVertexCount Vertex counter to increment.
 * @param rVertex Vertex position.
 */
void WriteVertexData(nn::util::Float2** ppVertex, int* pVertexCount,
                     const nn::util::Vector2f& rVertex) {
    (*ppVertex)->x = rVertex.GetX();
    (*ppVertex)->y = rVertex.GetY();
    ++*ppVertex;
    ++*pVertexCount;
}

/**
 * @brief Appends one index.
 * @param ppIndex Write cursor, advanced past the index.
 * @param pIndexCount Index counter to increment.
 * @param index Index value.
 */
void WriteIndexData(uint32_t** ppIndex, int* pIndexCount, uint32_t index) {
    **ppIndex = index;
    ++*ppIndex;
    ++*pIndexCount;
}

/**
 * @brief Carves an aligned block from a byte cursor.
 * @param pPtr Byte cursor, advanced past the block.
 * @param pOffset Receives the block's offset from pBase.
 * @param size Size of the block.
 * @param pBase Start of the buffer the offset is relative to.
 * @param alignment Alignment of the block.
 * @return Start of the block.
 */
void* AssignMemoryAndCalculateOffset(nn::util::BytePtr* pPtr, ptrdiff_t* pOffset, size_t size,
                                     void* pBase, size_t alignment) {
    void* pMemory = pPtr->AlignUp(alignment).Get();
    *pOffset = static_cast<char*>(pMemory) - static_cast<char*>(pBase);
    pPtr->Advance(size);
    return pMemory;
}

/**
 * @brief Carves the vertex and index buffers of one shape from the constant buffer.
 * @param pInfo Receives the buffer pointers and offsets.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param fillVertexCount Number of fill vertices.
 * @param fillIndexCount Number of fill indices.
 * @param strokeVertexCount Number of stroke vertices.
 * @param strokeIndexCount Number of stroke indices.
 * @param strokeSegmentCount Number of stroke segment descriptions.
 */
void AllocateShapeMeshBufferFromConstantBuffer(ShapeMeshBufferInfo* pInfo, DrawInfo& rDrawInfo,
                                               int fillVertexCount, int fillIndexCount,
                                               int strokeVertexCount, int strokeIndexCount,
                                               int strokeSegmentCount) {
    const GraphicsResource* pResource = rDrawInfo.m_pGraphicsResource;
    const size_t vertexAlignment = pResource->m_VertexBufferAlignment;
    const size_t indexAlignment = pResource->m_IndexBufferAlignment;

    const size_t fillVertexSize = sizeof(nn::util::Float2) * fillVertexCount;
    const size_t fillIndexSize = sizeof(uint32_t) * fillIndexCount;
    const size_t strokeVertexSize = sizeof(nn::util::Float2) * strokeVertexCount;
    const size_t strokeIndexSize = sizeof(uint32_t) * strokeIndexCount;
    const size_t strokeSegmentSize =
        strokeVertexCount != 0 ? sizeof(StrokeSegmentVertexInfo) * strokeSegmentCount : 0;

    const size_t size = fillVertexSize + sizeof(uint32_t) + fillIndexSize + strokeVertexSize +
                        strokeIndexSize + strokeSegmentSize +
                        (vertexAlignment + indexAlignment) * 2;
    nn::util::BytePtr ptr(AllocateFromConstantBufferBack(rDrawInfo.m_pConstantBuffer, size));
    ptr.AlignUp(sizeof(uint32_t));

    pInfo->pFillVertex = static_cast<nn::util::Float2*>(AssignMemoryAndCalculateOffset(
        &ptr, &pInfo->fillVertexOffset, fillVertexSize,
        rDrawInfo.m_pConstantBuffer->GetMappedPointer(), vertexAlignment));
    pInfo->pFillIndex = static_cast<uint32_t*>(AssignMemoryAndCalculateOffset(
        &ptr, &pInfo->fillIndexOffset, fillIndexSize,
        rDrawInfo.m_pConstantBuffer->GetMappedPointer(), indexAlignment));
    pInfo->pStrokeVertex = static_cast<nn::util::Float2*>(AssignMemoryAndCalculateOffset(
        &ptr, &pInfo->strokeVertexOffset, strokeVertexSize,
        rDrawInfo.m_pConstantBuffer->GetMappedPointer(), vertexAlignment));
    pInfo->pStrokeIndex = static_cast<uint32_t*>(AssignMemoryAndCalculateOffset(
        &ptr, &pInfo->strokeIndexOffset, strokeIndexSize,
        rDrawInfo.m_pConstantBuffer->GetMappedPointer(), indexAlignment));

    if (strokeSegmentSize != 0) {
        pInfo->pStrokeSegment = ptr.Advance(strokeIndexSize).Get<StrokeSegmentVertexInfo>();
    }
}

/**
 * @brief Writes the vertex shader constants of a vector graphics draw.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rMatrix Projection-view matrix.
 * @return Offset of the constants in the constant buffer.
 */
ptrdiff_t SetupVertexShaderConstantBuffer(DrawInfo& rDrawInfo,
                                          const nn::util::MatrixT4x4fType& rMatrix) {
    const size_t size = nn::util::align_up(sizeof(nn::util::MatrixT4x4fType),
                                           rDrawInfo.m_pGraphicsResource->m_ConstantBufferAlignment);
    const ptrdiff_t offset = AllocateFromConstantBufferFront(rDrawInfo.m_pConstantBuffer, size);
    nn::util::BytePtr ptr(rDrawInfo.m_pConstantBuffer->GetMappedPointer(), offset);
    *ptr.Get<nn::util::MatrixT4x4fType>() = rMatrix;
    return offset;
}

/**
 * @brief Writes the pixel shader constants of a vector graphics draw.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rColor Color to draw with.
 * @return Offset of the constants in the constant buffer.
 */
ptrdiff_t SetupPixelShaderConstantBuffer(DrawInfo& rDrawInfo, const nn::util::Float4& rColor) {
    const size_t size = nn::util::align_up(sizeof(nn::util::Float4),
                                           rDrawInfo.m_pGraphicsResource->m_ConstantBufferAlignment);
    const ptrdiff_t offset = AllocateFromConstantBufferFront(rDrawInfo.m_pConstantBuffer, size);
    nn::util::Float4* pColor =
        nn::util::BytePtr(rDrawInfo.m_pConstantBuffer->GetMappedPointer(), offset)
            .Get<nn::util::Float4>();

    for (int i = 0; i < 4; ++i) {
        pColor->v[i] = rColor.v[i];
    }

    return offset;
}

/**
 * @brief Clears the stencil buffer, or lets the user callback do it.
 * @param rDrawInfo Drawing state holding the depth stencil target.
 * @param rVectorGraphicsDrawInfo Vector graphics state holding the optional callback.
 * @param rCommandBuffer Command buffer to record into.
 */
void ClearStencilBuffer(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                        nn::gfx::CommandBuffer& rCommandBuffer) {
    if (rVectorGraphicsDrawInfo.m_pClearStencilCallback != nullptr) {
        rVectorGraphicsDrawInfo.m_pClearStencilCallback(
            rDrawInfo, rCommandBuffer, rVectorGraphicsDrawInfo.m_pClearStencilCallbackUserData);
        return;
    }

    ToImpl(rCommandBuffer)
        .ClearDepthStencil(const_cast<nn::gfx::DepthStencilView*>(rDrawInfo.m_pDepthTarget), 1.0f,
                           0, nn::gfx::DepthStencilClearMode_Stencil, nullptr);
    rDrawInfo.ResetRenderTarget(rCommandBuffer);
}

/**
 * @brief Binds the vertex and pixel shader constants of a draw.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rVectorGraphicsDrawInfo Vector graphics state holding the shader slots.
 * @param rCommandBuffer Command buffer to record into.
 * @param variation Shader variation being drawn with.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param pixelShaderOffset Offset of the pixel shader constants.
 */
void ApplyConstantBuffers(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                          nn::gfx::CommandBuffer& rCommandBuffer,
                          VectorGraphicsShaderVariation variation, size_t vertexShaderOffset,
                          size_t pixelShaderOffset) {
    const int vertexSlot = rVectorGraphicsDrawInfo.m_ShaderVariations[variation].vertexConstantSlot;

    if (vertexSlot >= 0) {
        nn::gfx::GpuAddress address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
        address.Offset(vertexShaderOffset);
        rCommandBuffer.SetConstantBuffer(vertexSlot, nn::gfx::ShaderStage_Vertex, address,
                                         sizeof(nn::util::MatrixT4x4fType));
    }

    const int pixelSlot = rVectorGraphicsDrawInfo.m_ShaderVariations[variation].pixelConstantSlot;

    if (pixelSlot >= 0) {
        nn::gfx::GpuAddress address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
        address.Offset(pixelShaderOffset);
        rCommandBuffer.SetConstantBuffer(pixelSlot, nn::gfx::ShaderStage_Pixel, address,
                                         sizeof(nn::util::Float4));
    }
}

/**
 * @brief Binds a range of the constant buffer as the vertex buffer.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rCommandBuffer Command buffer to record into.
 * @param offset Offset of the vertices in the constant buffer.
 * @param vertexCount Number of vertices.
 */
void SetVertexBuffer(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer, ptrdiff_t offset,
                     int vertexCount) {
    nn::font::GpuBuffer* pBuffer = rDrawInfo.m_pConstantBuffer;
    nn::gfx::GpuAddress address;
    pBuffer->m_pBuffers[pBuffer->m_GpuAccessBufferIndex].GetGpuAddress(&address);
    address.Offset(offset);
    rCommandBuffer.SetVertexBuffer(0, address, sizeof(nn::util::Float2),
                                   sizeof(nn::util::Float2) * vertexCount);
}

/**
 * @brief Draws indexed primitives whose indices live in the constant buffer.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rCommandBuffer Command buffer to record into.
 * @param topology Primitive topology.
 * @param offset Offset of the indices in the constant buffer.
 * @param indexCount Number of indices.
 */
void DrawIndexed(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer,
                 nn::gfx::PrimitiveTopology topology, ptrdiff_t offset, int indexCount) {
    nn::font::GpuBuffer* pBuffer = rDrawInfo.m_pConstantBuffer;
    nn::gfx::GpuAddress address;
    pBuffer->m_pBuffers[pBuffer->m_GpuAccessBufferIndex].GetGpuAddress(&address);
    address.Offset(offset);
    rCommandBuffer.DrawIndexed(topology, nn::gfx::IndexFormat_Uint32, address, indexCount, 0);
}

ReservedVectorGraphicsSceneMemory::ReservedVectorGraphicsSceneMemory()
    : mMemory(nullptr), mReservedSize(0), mNext(nullptr) {}

ReservedVectorGraphicsSceneMemory::~ReservedVectorGraphicsSceneMemory() = default;

/**
 * @brief Reserves the scene arena.
 * @param size Arena size in bytes; zero leaves the current arena unchanged.
 */
void ReservedVectorGraphicsSceneMemory::Initialize(size_t size) {
    if (size) {
        mReservedSize = size;
        mMemory = static_cast<char*>(Layout::AllocateMemory(size));
        mNext = mMemory;
    }
}

/** @brief Releases the scene arena. */
void ReservedVectorGraphicsSceneMemory::Finalize() {
    if (mMemory != nullptr) {
        Layout::FreeMemory(mMemory);
        mMemory = nullptr;
        mReservedSize = 0;
        mNext = nullptr;
    }
}

/** @brief Returns the arena size in bytes. */
size_t ReservedVectorGraphicsSceneMemory::GetReservedSize() const {
    return mReservedSize;
}

/** @brief Returns the number of bytes handed out so far. */
size_t ReservedVectorGraphicsSceneMemory::GetAllocatedSize() const {
    return nn::util::BytePtr(mMemory).Distance(mNext);
}

/**
 * @brief Hands out bytes from the pre-sized scene arena.
 * @param size Number of bytes.
 * @return Start of the block.
 */
void* ReservedVectorGraphicsSceneMemory::Allocate(size_t size) {
    char* result = mNext;
    mNext += size;
    return result;
}

/**
 * @brief Hands out aligned bytes from the pre-sized scene arena.
 * @param size Number of bytes.
 * @param alignment Power-of-two byte alignment.
 * @return Start of the block.
 */
void* ReservedVectorGraphicsSceneMemory::Allocate(size_t size, size_t alignment) {
    char* result = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(mNext) + alignment - 1) &
                                           -alignment);
    mNext = result + size;
    return result;
}

/**
 * @brief Sets the visible part of the path, honoring the trim direction.
 * @param start Start of the visible part, as a fraction of the path length.
 * @param end End of the visible part, as a fraction of the path length.
 */
void VectorGraphicsShapePathProcessor::SetTrimParams(float start, float end) {
    switch (TrimPathDirection()) {
    case TrimDirection_Right:
        m_TrimStart = start;
        m_TrimEnd = end;
        break;
    case TrimDirection_Left:
        m_TrimStart = 1.0f - end;
        m_TrimEnd = 1.0f - start;
        break;
    default:
        break;
    }
}

/**
 * @brief Moves the end points of a trimmed path onto the trim positions.
 * @param pVertex Path vertices.
 * @param startIndex Segment containing the trim start.
 * @param startRate Position of the trim start inside its segment.
 * @param endIndex Segment containing the trim end.
 * @param endRate Position of the trim end inside its segment.
 */
void VectorGraphicsShapePathProcessor::AdjustTrimmedVertexPosition(nn::util::Float2* pVertex,
                                                                   int startIndex, float startRate,
                                                                   int endIndex,
                                                                   float endRate) const {
    const nn::util::Float2 startPoint = pVertex[startIndex];
    nn::util::Vector2f start;
    nn::util::VectorLoad(&start, startPoint);

    if (startIndex == endIndex) {
        nn::util::Vector2f end;
        nn::util::VectorLoad(&end, pVertex[startIndex + 1]);
        nn::util::Vector2f direction;
        nn::util::VectorSubtract(&direction, end, start);

        nn::util::Vector2f trimmedStart;
        nn::util::VectorMultiply(&trimmedStart, direction, startRate);
        nn::util::VectorAdd(&trimmedStart, trimmedStart, start);
        nn::util::Vector2f trimmedEnd;
        nn::util::VectorMultiply(&trimmedEnd, direction, endRate);
        nn::util::VectorAdd(&trimmedEnd, trimmedEnd, start);

        SetFloat2(&pVertex[startIndex], trimmedStart);
        nn::util::VectorStore(&pVertex[startIndex + 1], trimmedEnd);
        return;
    }

    nn::util::Vector2f startNext;
    nn::util::VectorLoad(&startNext, pVertex[startIndex + 1]);
    nn::util::Vector2f direction;
    nn::util::VectorSubtract(&direction, startNext, start);
    nn::util::Vector2f trimmedStart;
    nn::util::VectorMultiply(&trimmedStart, direction, startRate);
    nn::util::VectorAdd(&trimmedStart, trimmedStart, start);
    SetFloat2(&pVertex[startIndex], trimmedStart);

    nn::util::Vector2f end;
    nn::util::VectorLoad(&end, pVertex[endIndex]);
    nn::util::Vector2f endNext;
    nn::util::VectorLoad(&endNext, pVertex[endIndex + 1]);
    nn::util::VectorSubtract(&direction, endNext, end);
    nn::util::Vector2f trimmedEnd;
    nn::util::VectorMultiply(&trimmedEnd, direction, endRate);
    nn::util::VectorAdd(&trimmedEnd, end, trimmedEnd);
    nn::util::VectorStore(&pVertex[endIndex + 1], trimmedEnd);
}

/**
 * @brief Drops the stroke segments outside of the trimmed range.
 * @param pVertex Path vertices.
 * @param vertexCount Number of vertices left after trimming.
 * @param startIndex First vertex left after trimming.
 * @param pStrokeInfo Stroke segment descriptions to adjust.
 * @param strokeInfoCount Number of stroke segment descriptions.
 * @return Number of stroke segment descriptions left.
 */
int VectorGraphicsShapePathProcessor::TrimStrokeVertexInfo(nn::util::Float2* pVertex,
                                                           int vertexCount, int startIndex,
                                                           StrokeSegmentVertexInfo* pStrokeInfo,
                                                           int strokeInfoCount) const {
    int vertexIndex = 0;

    for (int i = 0; i < strokeInfoCount; ++i) {
        StrokeSegmentVertexInfo& rInfo = pStrokeInfo[i];
        const int nextVertexIndex = rInfo.count + vertexIndex;

        if (vertexIndex <= startIndex && nextVertexIndex > startIndex) {
            const int skipCount = startIndex - vertexIndex;
            rInfo.start = pVertex[startIndex];
            rInfo.pCurvePoints += skipCount;
            rInfo.count -= skipCount;
        }

        if (nextVertexIndex <= startIndex) {
            rInfo.ignore = true;
        }

        if (nextVertexIndex >= vertexCount) {
            rInfo.count += vertexCount - nextVertexIndex;
            return i + 1;
        }

        vertexIndex = nextVertexIndex;
    }

    return strokeInfoCount;
}

/**
 * @brief Cuts the path down to the trimmed range.
 * @param pVertex Path vertices.
 * @param pVertexCount Number of vertices, updated to the trimmed count.
 * @param pStrokeInfo Stroke segment descriptions to adjust.
 * @param pStrokeInfoCount Number of stroke segment descriptions, updated after trimming.
 * @return Index of the first vertex of the trimmed path.
 */
int VectorGraphicsShapePathProcessor::Trim(nn::util::Float2* pVertex, int* pVertexCount,
                                           StrokeSegmentVertexInfo* pStrokeInfo,
                                           int* pStrokeInfoCount) {
    const float trimStart = m_TrimStart;
    const float trimEnd = m_TrimEnd;
    const int segmentCount = *pVertexCount - (IsPathClosed() ? 0 : 1);

    if (m_PathLength == 0.0f) {
        for (int i = 0; i < segmentCount; ++i) {
            m_PathLength += Float2Distance(pVertex[i], pVertex[i + 1]);
        }
    }

    const float trimStartLength = trimStart * m_PathLength;
    const float trimEndLength = trimEnd * m_PathLength;

    float length = 0.0f;
    float startRate = 0.0f;
    int startIndex = -1;

    for (int i = 0; i < segmentCount; ++i) {
        const float prevLength = length;
        const float segmentLength = Float2Distance(pVertex[i], pVertex[i + 1]);
        length = prevLength + segmentLength;

        if (startIndex < 0 && length >= trimStartLength) {
            startRate = (trimStartLength - prevLength) / segmentLength;
            startIndex = i;
        }

        if (length >= trimEndLength) {
            const float endRate = (trimEndLength - (length - segmentLength)) / segmentLength;
            AdjustTrimmedVertexPosition(pVertex, startIndex, startRate, i, endRate);
            *pVertexCount = i + 1;
            *pStrokeInfoCount = TrimStrokeVertexInfo(pVertex, *pVertexCount, startIndex,
                                                     pStrokeInfo, *pStrokeInfoCount);
            break;
        }
    }

    return startIndex;
}

/**
 * @brief Writes the indices of a triangle fan.
 * @param ppIndex Write cursor, advanced past the indices.
 * @param pIndexCount Index counter.
 * @param baseIndex Center vertex of the fan.
 * @param triangleCount Number of triangles.
 */
void VectorGraphicsShapePathProcessor::GenerateFillIndex(uint32_t** ppIndex, int* pIndexCount,
                                                         int baseIndex, int triangleCount) const {
    for (int i = 0; i < triangleCount; ++i) {
        WriteIndexData(ppIndex, pIndexCount, baseIndex);
        WriteIndexData(ppIndex, pIndexCount, baseIndex + i);
        WriteIndexData(ppIndex, pIndexCount, baseIndex + i + 1);
    }
}

/**
 * @brief Returns the scene memory a free-form path needs.
 * @param pRes Path resource.
 * @return Size of the evaluated control points.
 */
size_t VectorGraphicsShapePathData::CalculateRequiredDynamicMemorySize(
    const ResBnvgShapePathData* pRes) {
    return sizeof(EvaluatedControlPoint) * pRes->controlPointCount;
}

/**
 * @brief Binds a free-form path resource.
 * @param pRes Path resource.
 * @param pReservedMemory Scene arena to allocate from.
 */
VectorGraphicsShapePathData::VectorGraphicsShapePathData(
    const ResBnvgShapePathData* pRes, ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : VectorGraphicsShapePathProcessor(pReservedMemory), m_pRes(pRes),
      m_pEvaluatedControlPoints(nullptr), m_Tolerance(0.1f) {
    for (uint32_t i = 0; i < pRes->controlPointCount; ++i) {
        const ResBnvgControlPoint& rPoint = pRes->pControlPoints[i];

        if (IsValueAnimated(rPoint.inTangentX) || IsValueAnimated(rPoint.inTangentY) ||
            IsValueAnimated(rPoint.outTangentX) || IsValueAnimated(rPoint.outTangentY) ||
            IsValueAnimated(rPoint.positionX) || IsValueAnimated(rPoint.positionY)) {
            m_ShapeAnimated = true;
            return;
        }
    }
}

VectorGraphicsShapePathData::~VectorGraphicsShapePathData() {
    static_cast<void>(m_pEvaluatedControlPoints);
}

/** @brief Allocates the evaluated control points. */
void VectorGraphicsShapePathData::Initialize() {
    const int count = m_pRes->controlPointCount;
    m_pEvaluatedControlPoints = m_pReservedMemory->NewArray<EvaluatedControlPoint>(count);
}

/** @brief Forgets the evaluated control points. */
void VectorGraphicsShapePathData::Finalize() {
    if (m_pEvaluatedControlPoints != nullptr) {
        m_pEvaluatedControlPoints = nullptr;
    }
}

/**
 * @brief Evaluates the control point animations.
 * @param rDrawInfo Drawing state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapePathData::EvaluateParams(DrawInfo& rDrawInfo, float time) {
    for (uint32_t i = 0; i < m_pRes->controlPointCount; ++i) {
        const ResBnvgControlPoint& rPoint = m_pRes->pControlPoints[i];
        m_pEvaluatedControlPoints[i].inTangent.x = GetValue(rPoint.inTangentX, time);
        m_pEvaluatedControlPoints[i].inTangent.y = GetValue(rPoint.inTangentY, time);
        m_pEvaluatedControlPoints[i].outTangent.x = GetValue(rPoint.outTangentX, time);
        m_pEvaluatedControlPoints[i].outTangent.y = GetValue(rPoint.outTangentY, time);
        m_pEvaluatedControlPoints[i].position.x = GetValue(rPoint.positionX, time);
        m_pEvaluatedControlPoints[i].position.y = GetValue(rPoint.positionY, time);
    }

    if (m_ShapeAnimated) {
        m_PathLength = 0.0f;
    }
}

/** @brief Returns the number of control points of the path. */
uint32_t VectorGraphicsShapePathData::GetControlPointCount() const {
    return m_pRes->controlPointCount;
}

/**
 * @brief Returns how many points approximate one segment.
 * @param index Index of the segment.
 */
uint32_t VectorGraphicsShapePathData::CalculatePathDivideVertexCount(int index) const {
    nn::util::Vector2f vectors[4];
    SetVectorsWithEvaluatedControlPoints(vectors, m_pEvaluatedControlPoints, index,
                                         GetControlPointCount());
    return AcquireCubicPointCount(vectors, m_Tolerance);
}

/** @brief Returns the maximum number of vertices the path generates. */
uint32_t VectorGraphicsShapePathData::CalculateVertexCount() const {
    uint32_t count = 1;

    for (uint32_t i = 0; i < m_pRes->controlPointCount; ++i) {
        count += CalculatePathDivideVertexCount(i);
    }

    return count;
}

/**
 * @brief Generates the path vertices, the fill indices and the stroke segments.
 * @param ppVertex Vertex write cursor.
 * @param pVertexCount Vertex counter.
 * @param ppIndex Fill index write cursor.
 * @param pIndexCount Fill index counter.
 * @param pStrokeInfo Receives the stroke segment descriptions.
 * @return Number of stroke segment descriptions.
 */
int VectorGraphicsShapePathData::GenerateAndWritePathVertex(nn::util::Float2** ppVertex,
                                                            int* pVertexCount, uint32_t** ppIndex,
                                                            int* pIndexCount,
                                                            StrokeSegmentVertexInfo* pStrokeInfo) {
    const int baseVertexIndex = *pVertexCount;
    nn::util::Float2* pVertexStart = *ppVertex;
    int vertexCount = 0;
    int strokeInfoCount = 0;

    nn::util::Vector2f firstPoint;
    nn::util::VectorLoad(&firstPoint, m_pEvaluatedControlPoints[0].position);
    WriteVertexData(ppVertex, pVertexCount, firstPoint);

    for (uint32_t i = 0; i < m_pRes->controlPointCount; ++i) {
        nn::util::Vector2f vectors[4];
        SetVectorsWithEvaluatedControlPoints(vectors, m_pEvaluatedControlPoints, i,
                                             GetControlPointCount());

        if (!IsPathClosed() && i == m_pRes->controlPointCount - 1) {
            vectors[2] = vectors[3];
            vectors[1] = vectors[0];
        }

        const int divideCount = AcquireCubicPointCount(vectors, m_Tolerance);
        nn::util::Float2* pCurvePoints = *ppVertex;
        const int pointCount = CreateCubicPoints(vectors[0], vectors[1], vectors[2], vectors[3],
                                                 m_Tolerance * m_Tolerance, ppVertex, divideCount);

        if (i < m_pRes->controlPointCount - 1 ||
            (IsPathClosed() && (m_pRes->drawFlags & BnvgShapePathFlag_Stroke) != 0)) {
            SetStrokeSegmentVertexInfo(&pStrokeInfo[strokeInfoCount++], vectors[0], pCurvePoints,
                                       pointCount, false);
        }

        vertexCount += pointCount;
    }

    int startIndex = 0;

    if (IsPathTrimed()) {
        startIndex = std::max(Trim(pVertexStart, &vertexCount, pStrokeInfo, &strokeInfoCount), 0);
    }

    if ((m_pRes->drawFlags & BnvgShapePathFlag_Fill) != 0) {
        GenerateFillIndex(ppIndex, pIndexCount, baseVertexIndex + startIndex,
                          vertexCount - startIndex);
    }

    *ppVertex = pVertexStart + (vertexCount + 1);
    *pVertexCount = baseVertexIndex + vertexCount + 1;
    return strokeInfoCount;
}

/** @brief Returns the direction the trim range is measured in. */
VectorGraphicsShapePathProcessor::TrimDirection
VectorGraphicsShapePathData::TrimPathDirection() const {
    return static_cast<TrimDirection>((m_pRes->flags >> 1) & 1);
}

/**
 * @brief Returns the scene memory an ellipse path needs.
 * @param pRes Path resource.
 */
size_t VectorGraphicsShapePathEllipse::CalculateRequiredDynamicMemorySize(
    const ResBnvgShapePathEllipse* pRes) {
    return 0;
}

/**
 * @brief Binds an ellipse path resource.
 * @param pRes Path resource.
 * @param pReservedMemory Scene arena.
 */
VectorGraphicsShapePathEllipse::VectorGraphicsShapePathEllipse(
    const ResBnvgShapePathEllipse* pRes, ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : VectorGraphicsShapePathProcessor(pReservedMemory), m_pRes(pRes) {
    m_Position = nn::util::MakeFloat2(0.0f, 0.0f);
    m_Size = nn::util::MakeFloat2(1.0f, 1.0f);

    if (IsValueAnimated(pRes->sizeX) || IsValueAnimated(pRes->sizeY)) {
        m_ShapeAnimated = true;
    }
}

VectorGraphicsShapePathEllipse::~VectorGraphicsShapePathEllipse() {}

/** @brief Does nothing; an ellipse needs no scene memory. */
void VectorGraphicsShapePathEllipse::Initialize() {}

/** @brief Does nothing; an ellipse needs no scene memory. */
void VectorGraphicsShapePathEllipse::Finalize() {}

/**
 * @brief Evaluates the ellipse animations.
 * @param rDrawInfo Drawing state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapePathEllipse::EvaluateParams(DrawInfo& rDrawInfo, float time) {
    m_Size.x = GetValue(m_pRes->sizeX, time);
    m_Size.y = GetValue(m_pRes->sizeY, time);
    m_Position.x = GetValue(m_pRes->positionX, time);
    m_Position.y = GetValue(m_pRes->positionY, time);

    if (m_ShapeAnimated) {
        m_PathLength = 0.0f;
    }
}

/**
 * @brief Returns how many points approximate the ellipse.
 * @param index Unused segment index.
 */
uint32_t VectorGraphicsShapePathEllipse::CalculatePathDivideVertexCount(int index) const {
    return CalculateQuaterCircleDivideCount(std::max(m_Size.x, m_Size.y) * 0.5f) * 4;
}

/**
 * @brief Generates the ellipse vertices, the fill indices and the stroke segment.
 * @param ppVertex Vertex write cursor.
 * @param pVertexCount Vertex counter.
 * @param ppIndex Fill index write cursor.
 * @param pIndexCount Fill index counter.
 * @param pStrokeInfo Receives the stroke segment description.
 * @return Number of stroke segment descriptions.
 */
int VectorGraphicsShapePathEllipse::GenerateAndWritePathVertex(
    nn::util::Float2** ppVertex, int* pVertexCount, uint32_t** ppIndex, int* pIndexCount,
    StrokeSegmentVertexInfo* pStrokeInfo) {
    const int baseVertexIndex = *pVertexCount;
    nn::util::Float2* pVertexStart = *ppVertex;
    int strokeInfoCount = 0;

    const uint32_t divideCount = CalculatePathDivideVertexCount(0);
    nn::util::Float2* pCurvePoints = *ppVertex + 1;
    const float angleStep = (nn::util::FloatPi + nn::util::FloatPi) / divideCount;
    float angle = nn::util::FloatPi * 0.5f;
    nn::util::Vector2f firstPoint(vdup_n_f32(0.0f));

    for (uint32_t i = 0; i < divideCount + 1; ++i) {
        const nn::util::AngleIndex angleIndex = nn::util::RadianToAngleIndex(angle);
        const float cos = nn::util::CosTable(angleIndex);
        const float sin = nn::util::SinTable(angleIndex);
        const nn::util::Vector2f point(m_Position.x + m_Size.x * cos * 0.5f,
                                       m_Position.y - m_Size.y * sin * 0.5f);
        (*ppVertex)->x = point.GetX();
        (*ppVertex)->y = point.GetY();
        ++*ppVertex;
        ++*pVertexCount;

        if (i == 0) {
            firstPoint = point;
        }

        angle -= angleStep;
    }

    if ((m_pRes->drawFlags & BnvgShapePathFlag_Stroke) != 0) {
        SetStrokeSegmentVertexInfo(pStrokeInfo, firstPoint, pCurvePoints, divideCount, false);
        strokeInfoCount = 1;
    }

    int vertexCount = divideCount;
    int startIndex = 0;

    if (IsPathTrimed()) {
        startIndex = std::max(Trim(pVertexStart, &vertexCount, pStrokeInfo, &strokeInfoCount), 0);
    }

    if ((m_pRes->drawFlags & BnvgShapePathFlag_Fill) != 0) {
        GenerateFillIndex(ppIndex, pIndexCount, baseVertexIndex + startIndex,
                          vertexCount - startIndex);
    }

    *ppVertex = pVertexStart + (vertexCount + 1);
    *pVertexCount = baseVertexIndex + vertexCount + 1;
    return strokeInfoCount;
}

/** @brief Returns the direction the trim range is measured in. */
VectorGraphicsShapePathProcessor::TrimDirection
VectorGraphicsShapePathEllipse::TrimPathDirection() const {
    return static_cast<TrimDirection>((m_pRes->flags >> 1) & 1);
}

/**
 * @brief Returns the scene memory a rectangle path needs.
 * @param pRes Path resource.
 */
size_t
VectorGraphicsShapePathRect::CalculateRequiredDynamicMemorySize(const ResBnvgShapePathRect* pRes) {
    return 0;
}

/**
 * @brief Binds a rectangle path resource.
 * @param pRes Path resource.
 * @param pReservedMemory Scene arena.
 */
VectorGraphicsShapePathRect::VectorGraphicsShapePathRect(
    const ResBnvgShapePathRect* pRes, ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : VectorGraphicsShapePathProcessor(pReservedMemory), m_pRes(pRes), m_CornerRadius(0.0f) {
    m_Position = nn::util::MakeFloat2(0.0f, 0.0f);
    m_Size = nn::util::MakeFloat2(1.0f, 1.0f);

    if (IsValueAnimated(pRes->sizeX) || IsValueAnimated(pRes->sizeY) ||
        IsValueAnimated(pRes->cornerRadius)) {
        m_ShapeAnimated = true;
    }
}

VectorGraphicsShapePathRect::~VectorGraphicsShapePathRect() {}

/** @brief Does nothing; a rectangle needs no scene memory. */
void VectorGraphicsShapePathRect::Initialize() {}

/** @brief Does nothing; a rectangle needs no scene memory. */
void VectorGraphicsShapePathRect::Finalize() {}

/**
 * @brief Evaluates the rectangle animations.
 * @param rDrawInfo Drawing state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapePathRect::EvaluateParams(DrawInfo& rDrawInfo, float time) {
    m_Size.x = GetValue(m_pRes->sizeX, time);
    m_Size.y = GetValue(m_pRes->sizeY, time);
    m_Position.x = GetValue(m_pRes->positionX, time);
    m_Position.y = GetValue(m_pRes->positionY, time);
    m_CornerRadius = GetValue(m_pRes->cornerRadius, time);

    if (m_ShapeAnimated) {
        m_PathLength = 0.0f;
    }
}

/** @brief Returns one control point for a rounded rectangle and four for a sharp one. */
uint32_t VectorGraphicsShapePathRect::GetControlPointCount() const {
    return IsCurveDivideEnabled() ? 1 : 4;
}

/** @brief Checks whether the corners are rounded. */
bool VectorGraphicsShapePathRect::IsCurveDivideEnabled() const {
    return m_CornerRadius > 0.0f;
}

/**
 * @brief Returns how many points approximate one segment.
 * @param index Unused segment index.
 */
uint32_t VectorGraphicsShapePathRect::CalculatePathDivideVertexCount(int index) const {
    if (IsCurveDivideEnabled()) {
        return CalculateQuaterCircleDivideCount(m_CornerRadius) * 4 + 4;
    }

    return 1;
}

/** @brief Returns the maximum number of vertices the path generates. */
uint32_t VectorGraphicsShapePathRect::CalculateVertexCount() const {
    uint32_t count = 1;

    for (uint32_t i = 0; i < GetControlPointCount(); ++i) {
        count += CalculatePathDivideVertexCount(i);
    }

    return count;
}

/** @brief Returns the corner radius clamped to half of the rectangle size. */
float VectorGraphicsShapePathRect::CalculateRadius() const {
    const float halfWidth = m_Size.x * 0.5f;
    const float halfHeight = m_Size.y * 0.5f;
    const float radius = m_CornerRadius > halfWidth ? halfWidth : m_CornerRadius;
    return radius > halfHeight ? halfHeight : radius;
}

/**
 * @brief Returns the center of a corner's rounding circle.
 * @param corner Corner index, counter-clockwise from the bottom right.
 */
const nn::util::Float2 VectorGraphicsShapePathRect::CalculateOffset(int corner) const {
    const nn::util::Float2 signs[4] = {{{{1.0f, -1.0f}}}, {{{-1.0f, -1.0f}}}, {{{-1.0f, 1.0f}}},
                                       {{{1.0f, 1.0f}}}};
    const float halfWidth = m_Size.x * 0.5f;
    const float halfHeight = m_Size.y * 0.5f;
    const float radius = CalculateRadius();

    return nn::util::MakeFloat2(m_Position.x + signs[corner].x * (halfWidth - radius),
                                m_Position.y + signs[corner].y * (halfHeight - radius));
}

/**
 * @brief Generates the outline of a rounded rectangle.
 * @param ppVertex Vertex write cursor.
 * @param pVertexCount Vertex counter.
 */
void VectorGraphicsShapePathRect::GenerateRoundRectVertex(nn::util::Float2** ppVertex,
                                                          int* pVertexCount) const {
    const float radius = CalculateRadius();
    const uint32_t divideCount = CalculateQuaterCircleDivideCount(radius);
    nn::util::Float2* pVertexStart = *ppVertex;
    const float quarter = nn::util::FloatPi * 0.5f;
    const float angleStep = quarter / divideCount;

    for (int i = 0; i < 4; ++i) {
        const nn::util::Float2 offset = CalculateOffset(i);
        const bool isOverlapped = (i % 2 == 0) ? m_CornerRadius * 2.0f > m_Size.x :
                                                 m_CornerRadius * 2.0f > m_Size.y;
        float angle = quarter * i;

        for (uint32_t j = 0; j < divideCount + 1; ++j) {
            if (!isOverlapped || j < divideCount) {
                const nn::util::AngleIndex angleIndex = nn::util::RadianToAngleIndex(angle);
                (*ppVertex)->x = offset.x + radius * nn::util::CosTable(angleIndex);
                (*ppVertex)->y = offset.y - radius * nn::util::SinTable(angleIndex);
                ++*ppVertex;
                ++*pVertexCount;
            }

            angle += angleStep;
        }
    }

    nn::util::Vector2f firstPoint;
    nn::util::VectorLoad(&firstPoint, *pVertexStart);
    WriteVertexData(ppVertex, pVertexCount, firstPoint);
}

/**
 * @brief Generates the rectangle vertices, the fill indices and the stroke segments.
 * @param ppVertex Vertex write cursor.
 * @param pVertexCount Vertex counter.
 * @param ppIndex Fill index write cursor.
 * @param pIndexCount Fill index counter.
 * @param pStrokeInfo Receives the stroke segment descriptions.
 * @return Number of stroke segment descriptions.
 */
int VectorGraphicsShapePathRect::GenerateAndWritePathVertex(nn::util::Float2** ppVertex,
                                                            int* pVertexCount, uint32_t** ppIndex,
                                                            int* pIndexCount,
                                                            StrokeSegmentVertexInfo* pStrokeInfo) {
    const int baseVertexIndex = *pVertexCount;
    nn::util::Float2* pVertexStart = *ppVertex;
    int strokeInfoCount = 0;

    if (IsCurveDivideEnabled()) {
        GenerateRoundRectVertex(ppVertex, pVertexCount);
        const int pointCount = *pVertexCount - baseVertexIndex;

        if ((m_pRes->drawFlags & BnvgShapePathFlag_Stroke) != 0) {
            nn::util::Vector2f firstPoint;
            nn::util::VectorLoad(&firstPoint, *pVertexStart);
            SetStrokeSegmentVertexInfo(pStrokeInfo, firstPoint, pVertexStart + 1, pointCount - 1,
                                       false);
            strokeInfoCount = 1;
        }
    } else {
        nn::util::Vector2f prevPoint;

        for (int i = 0; i < 5; ++i) {
            const nn::util::Float2 offset = CalculateOffset(i % 4);
            const nn::util::Vector2f point(offset.x, offset.y);
            nn::util::Float2* pPoint = *ppVertex;
            pPoint->x = offset.x;
            (*ppVertex)->y = offset.y;
            ++*ppVertex;
            ++*pVertexCount;

            if (i > 0 && (m_pRes->drawFlags & BnvgShapePathFlag_Stroke) != 0) {
                SetStrokeSegmentVertexInfo(&pStrokeInfo[strokeInfoCount++], prevPoint, pPoint, 1,
                                           false);
            }

            prevPoint = point;
        }
    }

    int vertexCount = *pVertexCount - baseVertexIndex - 1;
    int startIndex = 0;

    if (IsPathTrimed()) {
        startIndex = std::max(Trim(pVertexStart, &vertexCount, pStrokeInfo, &strokeInfoCount), 0);
    }

    if ((m_pRes->drawFlags & BnvgShapePathFlag_Fill) != 0) {
        GenerateFillIndex(ppIndex, pIndexCount, baseVertexIndex + startIndex,
                          vertexCount - startIndex);
    }

    *ppVertex = pVertexStart + (vertexCount + 1);
    *pVertexCount = baseVertexIndex + vertexCount + 1;
    return strokeInfoCount;
}

/** @brief Returns the direction the trim range is measured in. */
VectorGraphicsShapePathProcessor::TrimDirection
VectorGraphicsShapePathRect::TrimPathDirection() const {
    return static_cast<TrimDirection>(!((m_pRes->flags >> 1) & 1));
}

/**
 * @brief Returns the scene memory a star path needs.
 * @param pRes Path resource.
 */
size_t
VectorGraphicsShapePathStar::CalculateRequiredDynamicMemorySize(const ResBnvgShapePathStar* pRes) {
    return 0;
}

/**
 * @brief Binds a star path resource.
 * @param pRes Path resource.
 * @param pReservedMemory Scene arena.
 */
VectorGraphicsShapePathStar::VectorGraphicsShapePathStar(
    const ResBnvgShapePathStar* pRes, ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : VectorGraphicsShapePathProcessor(pReservedMemory), m_pRes(pRes),
      m_pEvaluatedControlPoints(nullptr), m_ControlPointCount(0), m_Tolerance(0.1f) {
    if (IsValueAnimated(pRes->pointCount) || IsValueAnimated(pRes->outerRadius) ||
        IsValueAnimated(pRes->outerRoundness) || IsValueAnimated(pRes->innerRadius) ||
        IsValueAnimated(pRes->innerRoundness)) {
        m_ShapeAnimated = true;
    }
}

VectorGraphicsShapePathStar::~VectorGraphicsShapePathStar() {}

/** @brief Resets the position. */
void VectorGraphicsShapePathStar::Initialize() {
    m_Position = nn::util::MakeFloat2(0.0f, 0.0f);
}

/** @brief Does nothing; the control points live in the constant buffer. */
void VectorGraphicsShapePathStar::Finalize() {}

/**
 * @brief Evaluates the star animations and builds its control points.
 * @param rDrawInfo Drawing state holding the constant buffer the points are stored in.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapePathStar::EvaluateParams(DrawInfo& rDrawInfo, float time) {
    const float pointCountValue = GetValue(m_pRes->pointCount, time);
    const float outerRadius = GetValue(m_pRes->outerRadius, time);
    const float outerRoundness = GetValue(m_pRes->outerRoundness, time);
    const float innerRadius = GetValue(m_pRes->innerRadius, time);
    const float innerRoundness = GetValue(m_pRes->innerRoundness, time);
    m_Position.x = GetValue(m_pRes->positionX, time);
    m_Position.y = GetValue(m_pRes->positionY, time);
    const float rotation = GetValue(m_pRes->rotation, time);

    const int pointCount = static_cast<int>(pointCountValue);
    const bool isPolygon = m_pRes->isPolygon != 0;
    const int controlPointCount = pointCount << (isPolygon ? 0 : 1);
    void* pMemory = AllocateFromConstantBufferBack(
        rDrawInfo.m_pConstantBuffer, sizeof(EvaluatedControlPoint) * controlPointCount + 8);
    m_ControlPointCount = controlPointCount;
    m_pEvaluatedControlPoints = nn::util::BytePtr(pMemory).AlignUp(8).Get<EvaluatedControlPoint>();

    const float startAngle = nn::util::FloatPi * 0.5f - nn::util::DegreeToRadian(rotation);
    const float angleStep = nn::util::FloatPi * 2.0f / pointCount;
    const float outerTangentScale = outerRoundness * 0.275f;
    const float innerTangentScale = innerRoundness * 0.25f;

    for (int i = 0; i < pointCount; ++i) {
        const int next = i == pointCount - 1 ? 0 : i + 1;
        const float angle = startAngle - angleStep * i;
        const float nextAngle = startAngle - angleStep * next;

        const nn::util::AngleIndex angleIndex = nn::util::RadianToAngleIndex(angle);
        const float cos = nn::util::CosTable(angleIndex);
        const float sin = nn::util::SinTable(angleIndex);
        const nn::util::AngleIndex nextAngleIndex = nn::util::RadianToAngleIndex(nextAngle);
        const float nextCos = nn::util::CosTable(nextAngleIndex);
        const float nextSin = nn::util::SinTable(nextAngleIndex);

        const nn::util::Vector2f point(outerRadius * cos, -(outerRadius * sin));
        const nn::util::Vector2f nextPoint(outerRadius * nextCos, -(outerRadius * nextSin));
        nn::util::Vector2f side;
        nn::util::VectorSubtract(&side, nextPoint, point);
        const float sideLength = nn::util::VectorLength(side);

        if (isPolygon) {
            nn::util::Vector2f tangent(outerRadius * sin, outerRadius * cos);
            NormalizeWithTruncateAroundZero(&tangent);
            nn::util::Vector2f nextTangent(outerRadius * nextSin, outerRadius * nextCos);
            NormalizeWithTruncateAroundZero(&nextTangent);

            nn::util::Vector2f outTangent;
            nn::util::VectorMultiply(&outTangent, tangent, -(outerTangentScale * sideLength));
            m_pEvaluatedControlPoints[i].position.x = point.GetX();
            m_pEvaluatedControlPoints[i].position.y = point.GetY();
            nn::util::VectorStore(&m_pEvaluatedControlPoints[i].outTangent, outTangent);

            nn::util::Vector2f inTangent;
            nn::util::VectorMultiply(&inTangent, nextTangent, outerTangentScale * sideLength);
            SetFloat2(&m_pEvaluatedControlPoints[next].inTangent, inTangent);
        } else {
            const nn::util::AngleIndex innerAngleIndex =
                nn::util::RadianToAngleIndex(angle - angleStep * 0.5f);
            const float innerCos = nn::util::CosTable(innerAngleIndex);
            const float innerSin = nn::util::SinTable(innerAngleIndex);
            const nn::util::Vector2f innerPoint(innerRadius * innerCos, -(innerRadius * innerSin));
            nn::util::Vector2f toInner;
            nn::util::VectorSubtract(&toInner, innerPoint, point);
            const float innerLength = nn::util::VectorLength(toInner);

            nn::util::Vector2f tangent(outerRadius * sin, outerRadius * cos);
            NormalizeWithTruncateAroundZero(&tangent);
            nn::util::Vector2f outTangent;
            nn::util::VectorMultiply(&outTangent, tangent, -(outerTangentScale * sideLength));

            nn::util::Vector2f innerTangent(innerRadius * innerSin, innerRadius * innerCos);
            NormalizeWithTruncateAroundZero(&innerTangent);
            const float innerScale = innerTangentScale * innerLength;
            nn::util::Vector2f innerInTangent;
            nn::util::VectorMultiply(&innerInTangent, innerTangent, innerScale);

            m_pEvaluatedControlPoints[i * 2].position.x = point.GetX();
            m_pEvaluatedControlPoints[i * 2].position.y = point.GetY();
            nn::util::VectorStore(&m_pEvaluatedControlPoints[i * 2].outTangent, outTangent);

            const int innerIndex = (i * 2 + 1) % m_ControlPointCount;
            SetFloat2(&m_pEvaluatedControlPoints[innerIndex].inTangent, innerInTangent);

            nn::util::Vector2f innerOutTangent(innerRadius * innerSin, innerRadius * innerCos);
            NormalizeWithTruncateAroundZero(&innerOutTangent);
            nn::util::VectorMultiply(&innerOutTangent, innerOutTangent, -innerScale);

            nn::util::Vector2f nextTangent(outerRadius * nextSin, outerRadius * nextCos);
            NormalizeWithTruncateAroundZero(&nextTangent);
            nn::util::Vector2f nextInTangent;
            nn::util::VectorMultiply(&nextInTangent, nextTangent, outerTangentScale * sideLength);

            const int innerIndex2 = (i * 2 + 1) % m_ControlPointCount;
            m_pEvaluatedControlPoints[innerIndex2].position.x = innerPoint.GetX();
            m_pEvaluatedControlPoints[innerIndex2].position.y = innerPoint.GetY();
            nn::util::VectorStore(&m_pEvaluatedControlPoints[innerIndex2].outTangent,
                                  innerOutTangent);

            const int nextIndex = (i * 2 + 2) % m_ControlPointCount;
            SetFloat2(&m_pEvaluatedControlPoints[nextIndex].inTangent, nextInTangent);
        }
    }

    for (int i = 0; i < m_ControlPointCount; ++i) {
        m_pEvaluatedControlPoints[i].position.x += m_Position.x;
        m_pEvaluatedControlPoints[i].position.y += m_Position.y;
    }

    if (m_ShapeAnimated) {
        m_PathLength = 0.0f;
    }
}

/** @brief Returns the number of control points of the star. */
uint32_t VectorGraphicsShapePathStar::GetControlPointCount() const {
    return m_ControlPointCount;
}

/**
 * @brief Returns how many points approximate one segment.
 * @param index Index of the segment.
 */
uint32_t VectorGraphicsShapePathStar::CalculatePathDivideVertexCount(int index) const {
    nn::util::Vector2f vectors[4];
    SetVectorsWithEvaluatedControlPoints(vectors, m_pEvaluatedControlPoints, index,
                                         GetControlPointCount());
    return AcquireCubicPointCount(vectors, m_Tolerance) + 1;
}

/** @brief Returns the maximum number of vertices the path generates. */
uint32_t VectorGraphicsShapePathStar::CalculateVertexCount() const {
    uint32_t count = 1;

    for (int i = 0; i < m_ControlPointCount; ++i) {
        count += CalculatePathDivideVertexCount(i) + 1;
    }

    return count;
}

/**
 * @brief Generates the star vertices, the fill indices and the stroke segments.
 * @param ppVertex Vertex write cursor.
 * @param pVertexCount Vertex counter.
 * @param ppIndex Fill index write cursor.
 * @param pIndexCount Fill index counter.
 * @param pStrokeInfo Receives the stroke segment descriptions.
 * @return Number of stroke segment descriptions.
 */
int VectorGraphicsShapePathStar::GenerateAndWritePathVertex(nn::util::Float2** ppVertex,
                                                            int* pVertexCount, uint32_t** ppIndex,
                                                            int* pIndexCount,
                                                            StrokeSegmentVertexInfo* pStrokeInfo) {
    const int baseVertexIndex = *pVertexCount;
    nn::util::Float2* pVertexStart = *ppVertex;
    int strokeInfoCount = 0;
    int vertexCount = 0;

    nn::util::Vector2f firstPoint;
    nn::util::VectorLoad(&firstPoint, m_pEvaluatedControlPoints[0].position);
    WriteVertexData(ppVertex, pVertexCount, firstPoint);

    for (uint32_t i = 0; i < GetControlPointCount(); ++i) {
        nn::util::Vector2f vectors[4];
        SetVectorsWithEvaluatedControlPoints(vectors, m_pEvaluatedControlPoints, i,
                                             GetControlPointCount());

        const int divideCount = AcquireCubicPointCount(vectors, m_Tolerance);
        nn::util::Float2* pCurvePoints = *ppVertex;
        const int pointCount = CreateCubicPoints(vectors[0], vectors[1], vectors[2], vectors[3],
                                                 m_Tolerance * m_Tolerance, ppVertex, divideCount);

        if ((m_pRes->drawFlags & BnvgShapePathFlag_Stroke) != 0) {
            SetStrokeSegmentVertexInfo(&pStrokeInfo[strokeInfoCount++], vectors[0], pCurvePoints,
                                       pointCount, false);
        }

        vertexCount += pointCount;
    }

    int startIndex = 0;

    if (IsPathTrimed()) {
        startIndex = std::max(Trim(pVertexStart, &vertexCount, pStrokeInfo, &strokeInfoCount), 0);
    }

    if ((m_pRes->drawFlags & BnvgShapePathFlag_Fill) != 0) {
        GenerateFillIndex(ppIndex, pIndexCount, baseVertexIndex + startIndex,
                          vertexCount - startIndex);
    }

    *ppVertex = pVertexStart + (vertexCount + 1);
    *pVertexCount = baseVertexIndex + vertexCount + 1;
    m_pEvaluatedControlPoints = nullptr;
    return strokeInfoCount;
}

/** @brief Returns the direction the trim range is measured in. */
VectorGraphicsShapePathProcessor::TrimDirection
VectorGraphicsShapePathStar::TrimPathDirection() const {
    return static_cast<TrimDirection>((m_pRes->flags >> 1) & 1);
}

/** @brief Starts with an identity projection and no shader bound. */
VectorGraphicsDrawInfo::VectorGraphicsDrawInfo()
    : m_pShaderInfo(nullptr), m_pClearStencilCallback(nullptr),
      m_pClearStencilCallbackUserData(nullptr), m_ShaderVariations(), m_pMaskTextureStack(),
      m_MaskTextureStackCount(0) {
    m_ProjectionMatrix._m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
    m_ProjectionMatrix._m.val[1] = float32x4_t{0.0f, 1.0f, 0.0f, 0.0f};
    m_ProjectionMatrix._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
    m_ProjectionMatrix._m.val[3] = float32x4_t{0.0f, 0.0f, 0.0f, 1.0f};
}

VectorGraphicsDrawInfo::~VectorGraphicsDrawInfo() {}

/**
 * @brief Looks up the shader variations used for vector graphics.
 * @param pShaderInfo Shader holding the vector graphics variations.
 */
void VectorGraphicsDrawInfo::Initialize(const ShaderInfo* pShaderInfo) {
    m_pShaderInfo = pShaderInfo;

    for (uint32_t i = 0; i < VectorGraphicsShaderVariation_Max; ++i) {
        const uint32_t key = i;
        ShaderVariationInfo& rInfo = m_ShaderVariations[i];
        rInfo.variationIndex = SearchShaderVariationIndexFromTable(
            m_pShaderInfo->m_pVariationTable, VariationTableSignature, 1, &key);

        if (rInfo.variationIndex >= 0) {
            rInfo.vertexConstantSlot =
                m_pShaderInfo->GetVertexShader(rInfo.variationIndex)
                    ->GetInterfaceSlot(nn::gfx::ShaderStage_Vertex,
                                       nn::gfx::ShaderInterfaceType_ConstantBuffer,
                                       "uVgConstantBufferVS");
            rInfo.pixelConstantSlot =
                m_pShaderInfo->GetPixelShader(rInfo.variationIndex)
                    ->GetInterfaceSlot(nn::gfx::ShaderStage_Pixel,
                                       nn::gfx::ShaderInterfaceType_ConstantBuffer,
                                       "uVgConstantBufferPS");

            for (int j = 0; j < 3; ++j) {
                rInfo.textureSlots[j] =
                    m_pShaderInfo->m_pTextureSlots[m_pShaderInfo->GetTextureSlotCount() *
                                                       rInfo.variationIndex +
                                                   j];
            }
        }
    }
}

/** @brief Does nothing; the shader is owned elsewhere. */
void VectorGraphicsDrawInfo::Finalize() {}

/**
 * @brief Pushes a mask texture.
 * @param pSlot Descriptor slot of the mask texture.
 */
void VectorGraphicsDrawInfo::PushMaskTexture(nn::gfx::DescriptorSlot* pSlot) {
    m_pMaskTextureStack[m_MaskTextureStackCount] = pSlot;
    ++m_MaskTextureStackCount;
}

/** @brief Pops the top mask texture, if any. */
void VectorGraphicsDrawInfo::PopMaskTexture() {
    if (m_MaskTextureStackCount > 0) {
        --m_MaskTextureStackCount;
    }
}

/** @brief Empties the mask texture stack. */
void VectorGraphicsDrawInfo::ResetMaskTextureStack() {
    m_MaskTextureStackCount = 0;
}

/**
 * @brief Returns the scene memory the masks of a layer need.
 * @param pMaskInfoSet Masks of the layer.
 */
size_t MaskDrawer::CalculateRequiredDynamicMemorySize(const ResBnvgMaskInfoSet* pMaskInfoSet) {
    const int validMaskCount = CalculateValidMaskCount(pMaskInfoSet);

    if (validMaskCount == 0) {
        return 0;
    }

    size_t size = (sizeof(MaskData) + sizeof(ResBnvgShapePathData) +
                   sizeof(VectorGraphicsShapePathData)) *
                      validMaskCount +
                  sizeof(RenderTargetTextureInfo);

    for (int i = 0; i < pMaskInfoSet->maskCount; ++i) {
        if (pMaskInfoSet->pMaskInfos[i].isEnabled != 0) {
            size += sizeof(EvaluatedControlPoint) * pMaskInfoSet->pMaskInfos[i].controlPointCount;
        }
    }

    return size;
}

/**
 * @brief Counts the enabled masks.
 * @param pMaskInfoSet Masks of the layer.
 */
int MaskDrawer::CalculateValidMaskCount(const ResBnvgMaskInfoSet* pMaskInfoSet) {
    int count = 0;

    for (int i = 0; i < pMaskInfoSet->maskCount; ++i) {
        if (pMaskInfoSet->pMaskInfos[i].isEnabled != 0) {
            ++count;
        }
    }

    return count;
}

/**
 * @brief Starts without masks.
 * @param pReservedMemory Scene arena the masks are allocated from.
 */
MaskDrawer::MaskDrawer(ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : m_pRenderTarget(nullptr), m_pMaskInfos(nullptr), m_ValidMaskCount(0), m_pMasks(nullptr),
      m_pPathResources(nullptr), m_VertexCount(0), m_IndexCount(0), m_VertexBufferOffset(0),
      m_IndexBufferOffset(0), m_pReservedMemory(pReservedMemory) {}

MaskDrawer::~MaskDrawer() {}

/**
 * @brief Returns the constant buffer size the masks need per frame.
 * @param pDevice Device the buffer is created on.
 */
size_t MaskDrawer::GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const {
    size_t size = 0;

    for (int i = 0; i < m_ValidMaskCount; ++i) {
        size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                     sizeof(nn::util::MatrixT4x4fType));
        size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                     sizeof(nn::util::Float4));
    }

    return size;
}

/**
 * @brief Creates the mask texture and the mask paths.
 * @param pDevice Device the texture is created on.
 * @param pLayout Layout that owns the texture.
 * @param width Width of the mask texture.
 * @param height Height of the mask texture.
 * @param pMaskInfoSet Masks of the layer.
 */
void MaskDrawer::Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, int width,
                            int height, const ResBnvgMaskInfoSet* pMaskInfoSet) {
    m_pMaskInfos = pMaskInfoSet->pMaskInfos;
    const int maskCount = pMaskInfoSet->maskCount;
    m_ValidMaskCount = 0;

    if (pLayout == nullptr) {
        return;
    }

    m_ValidMaskCount = CalculateValidMaskCount(pMaskInfoSet);

    if (m_ValidMaskCount == 0) {
        return;
    }

    m_pRenderTarget = m_pReservedMemory->New<RenderTargetTextureInfo>();

    nn::gfx::TextureInfo textureInfo;
    textureInfo.SetDefault();
    textureInfo.SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
    textureInfo.SetImageFormat(nn::gfx::ImageFormat_R8_Unorm);
    textureInfo.SetGpuAccessFlags(nn::gfx::GpuAccess_Texture | nn::gfx::GpuAccess_ColorBuffer);
    textureInfo.SetWidth(width);
    textureInfo.SetHeight(height);
    textureInfo.SetMipCount(1);
    textureInfo.SetMultiSampleCount(1);

    if (!m_pRenderTarget->IsValid()) {
        m_pRenderTarget->Initialize(pDevice, pLayout, textureInfo,
                                    static_cast<RenderTargetTextureLifetime>(0));
    }

    m_pMasks = m_pReservedMemory->NewArray<MaskData>(m_ValidMaskCount);
    m_pPathResources = m_pReservedMemory->NewArray<ResBnvgShapePathData>(m_ValidMaskCount);

    int validIndex = 0;

    for (int i = 0; i < maskCount; ++i) {
        const ResBnvgMaskInfo& rMaskInfo = m_pMaskInfos[i];

        if (rMaskInfo.isEnabled == 0) {
            continue;
        }

        ResBnvgShapePathData& rPathResource = m_pPathResources[validIndex];
        rPathResource.type = BnvgShapePathType_Path;
        rPathResource.flags = 0;
        rPathResource.drawFlags = BnvgShapePathFlag_Fill;
        rPathResource.effectStartIndex = 0;
        rPathResource.controlPointCount = rMaskInfo.controlPointCount;
        rPathResource.isClosed = rMaskInfo.isClosed;
        rPathResource.pControlPoints = rMaskInfo.pControlPoints;

        VectorGraphicsShapePathData* pPath = m_pReservedMemory->New<VectorGraphicsShapePathData>(
            &rPathResource, m_pReservedMemory);
        pPath->SetTolerance(0.1f);
        pPath->Initialize();
        m_pMasks[validIndex].pPath = pPath;
        ++validIndex;
    }
}

/**
 * @brief Destroys the mask texture and the mask paths.
 * @param pDevice Device the texture was created on.
 */
void MaskDrawer::Finalize(nn::gfx::Device* pDevice) {
    for (int i = 0; i < m_ValidMaskCount; ++i) {
        m_pMasks[i].pPath->Finalize();
        m_pMasks[i].pPath->~VectorGraphicsShapePathProcessor();
    }

    m_pMasks = nullptr;
    m_pPathResources = nullptr;

    if (m_pRenderTarget != nullptr) {
        m_pRenderTarget->Finalize(pDevice);
        m_pRenderTarget = nullptr;
    }
}

/**
 * @brief Evaluates the masks and builds their meshes.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rVectorGraphicsDrawInfo Vector graphics state holding the projection.
 * @param rMatrix World matrix of the layer.
 * @param time Frame to evaluate at.
 */
void MaskDrawer::Calculate(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                           const nn::util::MatrixT4x3fType& rMatrix, float time) {
    for (int i = 0; i < m_ValidMaskCount; ++i) {
        m_pMasks[i].pPath->EvaluateParams(rDrawInfo, time);
    }

    int vertexCount = 0;
    int indexCount = 0;

    for (int i = 0; i < m_ValidMaskCount; ++i) {
        VectorGraphicsShapePathProcessor* pPath = m_pMasks[i].pPath;
        const uint32_t controlPointCount = pPath->GetControlPointCount();

        for (uint32_t j = 0; j < controlPointCount; ++j) {
            vertexCount += pPath->CalculateVertexCount();
            indexCount += pPath->CalculateVertexCount() * 3;
        }
    }

    if (vertexCount == 0) {
        return;
    }

    ShapeMeshBufferInfo bufferInfo;
    AllocateShapeMeshBufferFromConstantBuffer(&bufferInfo, rDrawInfo, vertexCount, indexCount, 0,
                                              0, 0);
    m_VertexCount = 0;
    m_IndexCount = 0;
    m_VertexBufferOffset = bufferInfo.fillVertexOffset;
    m_IndexBufferOffset = bufferInfo.fillIndexOffset;

    for (int i = 0; i < m_ValidMaskCount; ++i) {
        nn::util::MatrixT4x4fType matrix;
        MultiplyMatrix(&matrix, rVectorGraphicsDrawInfo.m_ProjectionMatrix, rMatrix);
        m_pMasks[i].vertexShaderOffset = SetupVertexShaderConstantBuffer(rDrawInfo, matrix);

        const float opacity = GetValue(m_pMaskInfos[i].opacity, time);
        m_pMasks[i].drawInfo.pixelShaderOffset = SetupPixelShaderConstantBuffer(
            rDrawInfo, nn::util::MakeFloat4(1.0f, 1.0f, 1.0f, opacity));

        const int indexStart = m_IndexCount;
        m_pMasks[i].pPath->GenerateAndWritePathVertex(&bufferInfo.pFillVertex, &m_VertexCount,
                                                      &bufferInfo.pFillIndex, &m_IndexCount,
                                                      nullptr);
        m_pMasks[i].drawInfo.indexStart = indexStart;
        m_pMasks[i].drawInfo.indexCount = m_IndexCount - indexStart;
    }
}

/**
 * @brief Draws one mask into the mask texture where the stencil is set.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param rPathDrawInfo Indices and colour of the mask.
 * @param blendStateId Blend state to draw with.
 */
void MaskDrawer::DrawMaskPolygonFill(DrawInfo& rDrawInfo,
                                     VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                     nn::gfx::CommandBuffer& rCommandBuffer,
                                     size_t vertexShaderOffset,
                                     const VectorGraphicsPathDrawInfo& rPathDrawInfo,
                                     PresetBlendStateId blendStateId) const {
    SetVertexBuffer(rDrawInfo, rCommandBuffer, m_VertexBufferOffset, m_VertexCount);
    ApplyConstantBuffers(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                         VectorGraphicsShaderVariation_NoMask, vertexShaderOffset,
                         rPathDrawInfo.pixelShaderOffset);

    GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.m_pGraphicsResource);
    ToImpl(rCommandBuffer)
        .SetDepthStencilState(&pResource->m_PresetVectorGraphicsDepthStencilState[1]);
    ToImpl(rCommandBuffer).SetBlendState(pResource->GetPresetBlendState(blendStateId));
    DrawIndexed(rDrawInfo, rCommandBuffer, nn::gfx::PrimitiveTopology_TriangleList,
                m_IndexBufferOffset + sizeof(uint32_t) * rPathDrawInfo.indexStart,
                rPathDrawInfo.indexCount);

    if (rDrawInfo.m_pDepthStencilState != nullptr) {
        ToImpl(rCommandBuffer).SetDepthStencilState(rDrawInfo.m_pDepthStencilState);
    }
}

/**
 * @brief Draws one mask into the stencil buffer.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param rPathDrawInfo Indices and colour of the mask.
 */
void MaskDrawer::DrawMaskPolygonStencil(DrawInfo& rDrawInfo,
                                        VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                        nn::gfx::CommandBuffer& rCommandBuffer,
                                        size_t vertexShaderOffset,
                                        const VectorGraphicsPathDrawInfo& rPathDrawInfo) const {
    SetVertexBuffer(rDrawInfo, rCommandBuffer, m_VertexBufferOffset, m_VertexCount);
    ApplyConstantBuffers(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                         VectorGraphicsShaderVariation_NoMask, vertexShaderOffset,
                         rPathDrawInfo.pixelShaderOffset);

    GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.m_pGraphicsResource);
    ToImpl(rCommandBuffer)
        .SetDepthStencilState(&pResource->m_PresetVectorGraphicsDepthStencilState[0]);
    ToImpl(rCommandBuffer)
        .SetBlendState(pResource->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
    DrawIndexed(rDrawInfo, rCommandBuffer, nn::gfx::PrimitiveTopology_TriangleList,
                m_IndexBufferOffset + sizeof(uint32_t) * rPathDrawInfo.indexStart,
                rPathDrawInfo.indexCount);

    if (rDrawInfo.m_pDepthStencilState != nullptr) {
        ToImpl(rCommandBuffer).SetDepthStencilState(rDrawInfo.m_pDepthStencilState);
    }
}

/**
 * @brief Draws every mask, each through its own stencil pass.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param blendStateId Blend state the masks are combined with.
 */
void MaskDrawer::DrawMaskShape(DrawInfo& rDrawInfo,
                               VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                               nn::gfx::CommandBuffer& rCommandBuffer,
                               PresetBlendStateId blendStateId) const {
    SetupShaderWithShaderCache(
        rDrawInfo, rCommandBuffer, rVectorGraphicsDrawInfo.m_pShaderInfo,
        rVectorGraphicsDrawInfo.m_ShaderVariations[VectorGraphicsShaderVariation_NoMask]
            .variationIndex);

    for (int i = 0; i < m_ValidMaskCount; ++i) {
        ClearStencilBuffer(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);
        DrawMaskPolygonStencil(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                               m_pMasks[i].vertexShaderOffset, m_pMasks[i].drawInfo);
        DrawMaskPolygonFill(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                            m_pMasks[i].vertexShaderOffset, m_pMasks[i].drawInfo, blendStateId);
    }
}

/**
 * @brief Renders the masks into the mask texture.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 */
void MaskDrawer::Draw(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                      nn::gfx::CommandBuffer& rCommandBuffer) const {
    const nn::gfx::ColorTargetView* pPrevColorTarget = rDrawInfo.m_pColorTarget;
    nn::gfx::ColorTargetView* pColorTarget = &m_pRenderTarget->mColorTarget;
    ToImpl(rCommandBuffer).ClearColor(pColorTarget, 0.0f, 0.0f, 0.0f, 0.0f, nullptr);
    rDrawInfo.m_pColorTarget = pColorTarget;
    rDrawInfo.ResetRenderTarget(rCommandBuffer);

    DrawMaskShape(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer, PresetBlendStateId_Addition);

    rCommandBuffer.FlushMemory(nn::gfx::GpuAccess_ColorBuffer);
    ToImpl(rCommandBuffer).InvalidateMemory(nn::gfx::GpuAccess_Texture);
    rDrawInfo.m_pColorTarget = pPrevColorTarget;
    rDrawInfo.ResetRenderTarget(rCommandBuffer);
}

/**
 * @brief Returns the scene memory a layer needs.
 * @param pBasicInfo Layer resource.
 * @param pMaskInfoLayer Layer holding the masks, or nullptr.
 */
size_t VectorGraphicsLayer::CalculateRequiredDynamicMemorySize(
    const ResBnvgLayerBasicInfo* pBasicInfo, const ResBnvgMaskInfoLayer* pMaskInfoLayer) {
    size_t size = 0;

    if (pMaskInfoLayer != nullptr && (pBasicInfo->flags & 2) != 0) {
        size = MaskDrawer::CalculateRequiredDynamicMemorySize(
            &pMaskInfoLayer->pMaskInfoSets[pBasicInfo->maskInfoIndex]);
    }

    return size + sizeof(nn::util::MatrixT4x3fType) + 16;
}

/**
 * @brief Binds a layer resource.
 * @param pBasicInfo Layer resource, or nullptr for the root layer.
 * @param pMaskInfoLayer Layer holding the masks, or nullptr.
 * @param pReservedMemory Scene arena.
 */
VectorGraphicsLayer::VectorGraphicsLayer(const ResBnvgLayerBasicInfo* pBasicInfo,
                                         const ResBnvgMaskInfoLayer* pMaskInfoLayer,
                                         ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : m_Opacity(1.0f), m_IsVisible(true), m_pMatrix(nullptr), m_pBasicInfo(pBasicInfo),
      m_pMaskInfoLayer(pMaskInfoLayer), m_pParent(nullptr), m_MaskDrawer(pReservedMemory),
      m_pReservedMemory(pReservedMemory) {}

VectorGraphicsLayer::~VectorGraphicsLayer() {
    static_cast<void>(m_pMatrix);
}

/**
 * @brief Returns the constant buffer size the layer needs per frame.
 * @param pDevice Device the buffer is created on.
 */
size_t VectorGraphicsLayer::GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const {
    return m_MaskDrawer.GetRequiredConstantBufferSize(pDevice);
}

/**
 * @brief Allocates the world matrix and creates the masks.
 * @param pDevice Device the mask texture is created on.
 * @param pLayout Layout that owns the mask texture.
 * @param width Width of the scene.
 * @param height Height of the scene.
 */
void VectorGraphicsLayer::Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, int width,
                                     int height) {
    nn::util::MatrixT4x3fType* pMatrix = static_cast<nn::util::MatrixT4x3fType*>(
        m_pReservedMemory->Allocate(sizeof(nn::util::MatrixT4x3fType), 16));
    *pMatrix = nn::util::MatrixT4x3fType();
    m_pMatrix = pMatrix;
    m_pMatrix->_m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
    m_pMatrix->_m.val[1] = float32x4_t{0.0f, 1.0f, 0.0f, 0.0f};
    m_pMatrix->_m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};

    if (m_pMaskInfoLayer != nullptr && (m_pBasicInfo->flags & 2) != 0) {
        m_MaskDrawer.Initialize(pDevice, pLayout, width, height,
                                &m_pMaskInfoLayer->pMaskInfoSets[m_pBasicInfo->maskInfoIndex]);
    }
}

/**
 * @brief Destroys the masks and the child layers.
 * @param pDevice Device the resources were created on.
 */
void VectorGraphicsLayer::Finalize(nn::gfx::Device* pDevice) {
    m_MaskDrawer.Finalize(pDevice);
    m_pMatrix = nullptr;

    nn::util::IntrusiveListNode* pNode = m_Children.GetNext();

    while (pNode != &m_Children) {
        VectorGraphicsLayer* pChild = FromLink(pNode);
        nn::util::IntrusiveListNode* pNext = pNode->GetNext();
        pChild->Finalize(pDevice);
        pChild->~VectorGraphicsLayer();
        pNode = pNext;
    }
}

/**
 * @brief Evaluates the layer transform and opacity, then the child layers.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsLayer::Calculate(DrawInfo& rDrawInfo,
                                    VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo, float time) {
    if (m_pBasicInfo != nullptr) {
        if (m_pBasicInfo->inFrame > time || m_pBasicInfo->outFrame <= time) {
            m_IsVisible = false;
            return;
        }

        m_IsVisible = true;

        nn::util::MatrixT4x3fType matrix;
        CalcTransform(&matrix, m_pBasicInfo->transform, time);
        const float opacity = GetValue(m_pBasicInfo->opacity, time);

        if (m_pParent != nullptr) {
            MultiplyMatrix(m_pMatrix, *m_pParent->m_pMatrix, matrix);
        } else {
            *m_pMatrix = matrix;
        }

        m_Opacity = opacity;
    }

    if (m_MaskDrawer.GetValidMaskCount() > 0) {
        m_MaskDrawer.Calculate(rDrawInfo, rVectorGraphicsDrawInfo, *m_pMatrix, time);
    }

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        FromLink(pNode)->Calculate(rDrawInfo, rVectorGraphicsDrawInfo, time);
    }
}

/**
 * @brief Evaluates a transform into a world matrix.
 * @param pMatrix Receives the matrix.
 * @param rTransform Transform resource.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsLayer::CalcTransform(nn::util::MatrixT4x3fType* pMatrix,
                                        const ResBnvgTransform& rTransform, float time) const {
    const nn::util::Vector2f anchor = GetValue(rTransform.anchor, time);
    nn::util::MatrixT4x3fType anchorMatrix;
    const float32x4_t axisX = {1.0f, 0.0f, 0.0f, 0.0f};
    const float32x4_t axisY = {0.0f, 1.0f, 0.0f, 0.0f};
    anchorMatrix._m.val[0] = vsetq_lane_f32(-anchor.GetX(), axisX, 3);
    anchorMatrix._m.val[1] = vsetq_lane_f32(-anchor.GetY(), axisY, 3);
    anchorMatrix._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};

    const float scaleX = GetValue(rTransform.scaleX, time);
    const float scaleY = GetValue(rTransform.scaleY, time);
    const float rotation = GetValue(rTransform.rotation, time);
    const nn::util::Vector2f position = GetValue(rTransform.position, time);

    const nn::util::AngleIndex angleIndex = DegreeToAngleIndex(rotation);
    const float cos = nn::util::CosTable(angleIndex);
    const float sin = nn::util::SinTable(angleIndex);

    nn::util::MatrixT4x3fType srtMatrix;
    srtMatrix._m.val[0] = float32x4_t{scaleX * cos, -(scaleY * sin), 0.0f, position.GetX()};
    srtMatrix._m.val[1] = float32x4_t{scaleX * sin, scaleY * cos, 0.0f, position.GetY()};
    srtMatrix._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
    MultiplyMatrix(pMatrix, srtMatrix, anchorMatrix);
}

/**
 * @brief Draws the masks, the layer contents and the child layers.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsLayer::Draw(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                               nn::gfx::CommandBuffer& rCommandBuffer) {
    if (m_MaskDrawer.GetValidMaskCount() > 0) {
        m_MaskDrawer.Draw(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);
        rVectorGraphicsDrawInfo.PushMaskTexture(&m_MaskDrawer.m_pRenderTarget->mDescriptor);
    }

    DrawLayerImpl(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        FromLink(pNode)->Draw(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);
    }

    if (m_MaskDrawer.GetValidMaskCount() > 0) {
        rVectorGraphicsDrawInfo.PopMaskTexture();
    }
}

/**
 * @brief Checks whether an effect produces geometry.
 * @param type Effect type.
 */
bool VectorGraphicsShapeLayer::IsDrawableEffect(BnvgShapeEffectType type) {
    return type == BnvgShapeEffectType_Fill || type == BnvgShapeEffectType_Stroke;
}

/**
 * @brief Counts the draw entries a shape can produce.
 * @param rShapeInfo Shape resource.
 */
int VectorGraphicsShapeLayer::CalculateDrawablePathCount(const ResBnvgShapeInfo& rShapeInfo) {
    int count = 0;

    for (int i = 0; i < rShapeInfo.pathCount; ++i) {
        for (int j = rShapeInfo.ppPaths[i]->effectStartIndex; j < rShapeInfo.effectCount; ++j) {
            if (IsDrawableEffect(static_cast<BnvgShapeEffectType>(rShapeInfo.ppEffects[j]->type))) {
                ++count;
            }
        }
    }

    return count;
}

/**
 * @brief Returns the scene memory a shape layer needs.
 * @param pShapeLayer Layer resource.
 * @param pMaskInfoLayer Layer holding the masks, or nullptr.
 */
size_t VectorGraphicsShapeLayer::CalculateRequiredDynamicMemorySize(
    const ResBnvgShapeLayer* pShapeLayer, const ResBnvgMaskInfoLayer* pMaskInfoLayer) {
    size_t size = VectorGraphicsLayer::CalculateRequiredDynamicMemorySize(pShapeLayer,
                                                                          pMaskInfoLayer);

    if (pShapeLayer->groupCount != 0) {
        size += sizeof(GroupInfo) * pShapeLayer->groupCount + 16;
    }

    size += sizeof(ShapeDrawInfo) * pShapeLayer->shapeCount;

    for (uint32_t i = 0; i < pShapeLayer->shapeCount; ++i) {
        const ResBnvgShapeInfo& rShapeInfo = pShapeLayer->pShapes[i];
        size += sizeof(VectorGraphicsShapePathProcessor*) * rShapeInfo.pathCount;

        for (int j = 0; j < rShapeInfo.pathCount; ++j) {
            const ResBnvgShapePath* pPath = rShapeInfo.ppPaths[j];

            switch (pPath->type) {
            case BnvgShapePathType_Path:
                size += VectorGraphicsShapePathData::CalculateRequiredDynamicMemorySize(
                            static_cast<const ResBnvgShapePathData*>(pPath)) +
                        sizeof(VectorGraphicsShapePathData);
                break;
            case BnvgShapePathType_Ellipse:
                size += sizeof(VectorGraphicsShapePathEllipse);
                break;
            case BnvgShapePathType_Rect:
                size += sizeof(VectorGraphicsShapePathRect);
                break;
            case BnvgShapePathType_Star:
                size += sizeof(VectorGraphicsShapePathStar);
                break;
            default:
                break;
            }
        }

        const int drawablePathCount = CalculateDrawablePathCount(rShapeInfo);

        if (drawablePathCount != 0) {
            size += sizeof(VectorGraphicsPathDrawInfo) * drawablePathCount;
        }

        size += sizeof(StrokeInfo) * rShapeInfo.effectCount;

        if (rShapeInfo.effectCount != 0) {
            size += sizeof(size_t) * rShapeInfo.effectCount;
        }
    }

    return size;
}

/**
 * @brief Binds a shape layer resource.
 * @param pShapeLayer Layer resource.
 * @param pMaskInfoLayer Layer holding the masks, or nullptr.
 * @param pReservedMemory Scene arena.
 */
VectorGraphicsShapeLayer::VectorGraphicsShapeLayer(
    const ResBnvgShapeLayer* pShapeLayer, const ResBnvgMaskInfoLayer* pMaskInfoLayer,
    ReservedVectorGraphicsSceneMemory* pReservedMemory)
    : VectorGraphicsLayer(pShapeLayer, pMaskInfoLayer, pReservedMemory),
      m_pShapes(pShapeLayer->pShapes), m_pShapeDrawInfos(nullptr), m_pGroups(nullptr),
      m_pGroupInfos(nullptr), m_ShapeCount(pShapeLayer->shapeCount), m_GroupCount(0),
      m_Tolerance(0.1f), m_FillVertexCount(0), m_FillIndexCount(0), m_FillVertexOffset(0),
      m_FillIndexOffset(0), m_StrokeVertexCount(0), m_StrokeIndexCount(0),
      m_StrokeVertexOffset(0), m_StrokeIndexOffset(0) {
    const int groupCount = pShapeLayer->groupCount;

    if (groupCount != 0) {
        m_GroupCount = groupCount;
        m_pGroups = pShapeLayer->pGroups;
        m_pGroupInfos = pReservedMemory->NewArray<GroupInfo>(groupCount, 16);
    }
}

VectorGraphicsShapeLayer::~VectorGraphicsShapeLayer() {}

/**
 * @brief Allocates the world matrix, creates the masks and the shape drawing state.
 * @param pDevice Device the mask texture is created on.
 * @param pLayout Layout that owns the mask texture.
 * @param width Width of the scene.
 * @param height Height of the scene.
 */
void VectorGraphicsShapeLayer::Initialize(nn::gfx::Device* pDevice, const Layout* pLayout,
                                          int width, int height) {
    VectorGraphicsLayer::Initialize(pDevice, pLayout, width, height);
    InitializeShapeDrawInfo();
}

/** @brief Creates the path processors and the drawing state of every shape. */
void VectorGraphicsShapeLayer::InitializeShapeDrawInfo() {
    m_pShapeDrawInfos = m_pReservedMemory->NewArray<ShapeDrawInfo>(m_ShapeCount);

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        const ResBnvgShapeInfo& rShapeInfo = m_pShapes[i];

        m_pShapeDrawInfos[i].ppPathProcessors =
            m_pReservedMemory->NewArray<VectorGraphicsShapePathProcessor*>(rShapeInfo.pathCount);

        for (uint32_t j = 0; j < rShapeInfo.pathCount; ++j) {
            m_pShapeDrawInfos[i].ppPathProcessors[j] =
                CreateAndInitializePathProcessor(rShapeInfo.ppPaths[j]);
        }

        const int drawablePathCount = CalculateDrawablePathCount(rShapeInfo);
        VectorGraphicsPathDrawInfo* pPathDrawInfos = nullptr;

        if (drawablePathCount != 0) {
            pPathDrawInfos =
                m_pReservedMemory->NewArray<VectorGraphicsPathDrawInfo>(drawablePathCount);
        }

        m_pShapeDrawInfos[i].pPathDrawInfos = pPathDrawInfos;
        m_pShapeDrawInfos[i].pathDrawInfoCountMax = drawablePathCount;

        size_t* pPixelShaderOffsets = nullptr;

        if (rShapeInfo.effectCount != 0) {
            pPixelShaderOffsets = m_pReservedMemory->NewArray<size_t>(rShapeInfo.effectCount);
        }

        m_pShapeDrawInfos[i].pPixelShaderOffsets = pPixelShaderOffsets;

        m_pShapeDrawInfos[i].pStrokeInfos =
            m_pReservedMemory->NewArray<StrokeInfo>(rShapeInfo.effectCount);

        for (uint32_t j = 0; j < rShapeInfo.effectCount; ++j) {
            const ResBnvgShapeEffect* pEffect = m_pShapes[i].ppEffects[j];

            if (pEffect->type == BnvgShapeEffectType_Stroke) {
                m_pShapeDrawInfos[i].pStrokeInfos[j].pStrokeEffect = pEffect;
            } else {
                m_pShapeDrawInfos[i].pStrokeInfos[j].pStrokeEffect = nullptr;
            }
        }
    }
}

/**
 * @brief Creates the processor that generates the vertices of a path.
 * @param pPath Path resource.
 * @return Processor, or nullptr for an unknown path type.
 */
VectorGraphicsShapePathProcessor*
VectorGraphicsShapeLayer::CreateAndInitializePathProcessor(const ResBnvgShapePath* pPath) {
    VectorGraphicsShapePathProcessor* pProcessor = nullptr;

    switch (pPath->type) {
    case BnvgShapePathType_Path: {
        VectorGraphicsShapePathData* pData = m_pReservedMemory->New<VectorGraphicsShapePathData>(
            static_cast<const ResBnvgShapePathData*>(pPath), m_pReservedMemory);
        pData->SetTolerance(m_Tolerance);
        pProcessor = pData;
        break;
    }
    case BnvgShapePathType_Ellipse: {
        pProcessor = m_pReservedMemory->New<VectorGraphicsShapePathEllipse>(
            static_cast<const ResBnvgShapePathEllipse*>(pPath), m_pReservedMemory);
        break;
    }
    case BnvgShapePathType_Rect: {
        pProcessor = m_pReservedMemory->New<VectorGraphicsShapePathRect>(
            static_cast<const ResBnvgShapePathRect*>(pPath), m_pReservedMemory);
        break;
    }
    case BnvgShapePathType_Star: {
        VectorGraphicsShapePathStar* pStar = m_pReservedMemory->New<VectorGraphicsShapePathStar>(
            static_cast<const ResBnvgShapePathStar*>(pPath), m_pReservedMemory);
        pStar->SetTolerance(m_Tolerance);
        pProcessor = pStar;
        break;
    }
    default:
        return nullptr;
    }

    if (pProcessor != nullptr) {
        pProcessor->Initialize();
    }

    return pProcessor;
}

/**
 * @brief Destroys the path processors, the masks and the child layers.
 * @param pDevice Device the resources were created on.
 */
void VectorGraphicsShapeLayer::Finalize(nn::gfx::Device* pDevice) {
    if (m_pShapeDrawInfos != nullptr) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
            ShapeDrawInfo& rInfo = m_pShapeDrawInfos[i];

            if (rInfo.pStrokeInfos != nullptr) {
                rInfo.pStrokeInfos = nullptr;
            }

            if (rInfo.pPixelShaderOffsets != nullptr) {
                rInfo.pPixelShaderOffsets = nullptr;
            }

            if (rInfo.pPathDrawInfos != nullptr) {
                rInfo.pPathDrawInfos = nullptr;
            }

            for (int j = 0; j < m_pShapes[i].pathCount; ++j) {
                VectorGraphicsShapePathProcessor* pProcessor = rInfo.ppPathProcessors[j];

                if (pProcessor != nullptr) {
                    pProcessor->Finalize();
                    rInfo.ppPathProcessors[j]->~VectorGraphicsShapePathProcessor();
                }
            }
        }

        m_pShapeDrawInfos = nullptr;
    }

    VectorGraphicsLayer::Finalize(pDevice);
}

/**
 * @brief Returns the constant buffer size the layer needs per frame.
 * @param pDevice Device the buffer is created on.
 */
size_t VectorGraphicsShapeLayer::GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const {
    size_t size = VectorGraphicsLayer::GetRequiredConstantBufferSize(pDevice);

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                     sizeof(nn::util::MatrixT4x4fType));

        for (int j = 0; j < m_pShapes[i].effectCount; ++j) {
            if (IsDrawableEffect(
                    static_cast<BnvgShapeEffectType>(m_pShapes[i].ppEffects[j]->type))) {
                size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                             sizeof(nn::util::Float4));
            }
        }
    }

    return size;
}

/**
 * @brief Writes the constants of one shape: its matrix and the colour of each effect.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rVectorGraphicsDrawInfo Vector graphics state holding the projection.
 * @param shapeIndex Index of the shape.
 * @param rShapeInfo Shape resource.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::SetupConstantBuffer(DrawInfo& rDrawInfo,
                                                   VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                                   int shapeIndex,
                                                   const ResBnvgShapeInfo& rShapeInfo,
                                                   float time) {
    float opacity = m_Opacity;
    nn::util::MatrixT4x3fType shapeMatrix;

    if (m_GroupCount != 0) {
        const GroupInfo& rGroupInfo = m_pGroupInfos[m_pShapes[shapeIndex].groupIndex];
        shapeMatrix = rGroupInfo.matrix;
        opacity *= rGroupInfo.opacity;
    } else {
        CalcTransform(&shapeMatrix, rShapeInfo.transform, time);
    }

    nn::util::MatrixT4x3fType worldMatrix;
    MultiplyMatrix(&worldMatrix, *m_pMatrix, shapeMatrix);
    nn::util::MatrixT4x4fType matrix;
    MultiplyMatrix(&matrix, rVectorGraphicsDrawInfo.m_ProjectionMatrix, worldMatrix);
    m_pShapeDrawInfos[shapeIndex].vertexShaderOffset =
        SetupVertexShaderConstantBuffer(rDrawInfo, matrix);

    for (int i = 0; i < rShapeInfo.effectCount; ++i) {
        const ResBnvgShapeEffect* pEffect = rShapeInfo.ppEffects[i];
        size_t offset;

        switch (pEffect->type) {
        case BnvgShapeEffectType_Fill: {
            const ResBnvgShapeFillEffect* pFill =
                static_cast<const ResBnvgShapeFillEffect*>(pEffect);
            const float r = GetValue(pFill->colorR, time);
            const float g = GetValue(pFill->colorG, time);
            const float b = GetValue(pFill->colorB, time);
            const float a = GetValue(pFill->colorA, time);
            offset = SetupPixelShaderConstantBuffer(rDrawInfo,
                                                    nn::util::MakeFloat4(r, g, b, opacity * a));
            break;
        }
        case BnvgShapeEffectType_Stroke: {
            const ResBnvgShapeStrokeEffect* pStroke =
                static_cast<const ResBnvgShapeStrokeEffect*>(pEffect);
            const float r = GetValue(pStroke->colorR, time);
            const float g = GetValue(pStroke->colorG, time);
            const float b = GetValue(pStroke->colorB, time);
            const float a = GetValue(pStroke->colorA, time);
            offset = SetupPixelShaderConstantBuffer(rDrawInfo,
                                                    nn::util::MakeFloat4(r, g, b, opacity * a));
            break;
        }
        default:
            offset = 0;
            break;
        }

        m_pShapeDrawInfos[shapeIndex].pPixelShaderOffsets[i] = offset;
    }
}

/**
 * @brief Checks whether a shape draws anything.
 * @param rShapeInfo Shape resource.
 */
bool VectorGraphicsShapeLayer::IsShapeNeedRendering(const ResBnvgShapeInfo& rShapeInfo) const {
    if (rShapeInfo.pathCount == 0) {
        return false;
    }

    if (rShapeInfo.effectCount == 0) {
        return false;
    }

    return m_Opacity > 0.0f;
}

/**
 * @brief Checks whether a path draws anything.
 * @param rPath Path resource.
 */
bool VectorGraphicsShapeLayer::IsPathNeedRendering(const ResBnvgShapePath& rPath) const {
    if (rPath.drawFlags == 0) {
        return false;
    }

    return (rPath.flags & 1) == 0;
}

/**
 * @brief Adds the stroke vertices and indices of one path segment to the counters.
 * @param pVertexCount Vertex counter.
 * @param pIndexCount Index counter.
 * @param shapeIndex Index of the shape.
 * @param effectStart First effect applied to the path.
 * @param pointCount Number of points of the segment.
 */
void VectorGraphicsShapeLayer::CalculateShapeStrokeEffectSegmentVertexInfoCount(
    int* pVertexCount, int* pIndexCount, int shapeIndex, int effectStart, int pointCount) const {
    for (uint32_t i = effectStart; i < m_pShapes[shapeIndex].effectCount; ++i) {
        const StrokeInfo& rStrokeInfo = m_pShapeDrawInfos[shapeIndex].pStrokeInfos[i];
        const ResBnvgShapeStrokeEffect* pStroke =
            static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

        if (pStroke == nullptr) {
            continue;
        }

        *pVertexCount += pointCount * 2;
        *pIndexCount += pointCount * 2;

        switch (pStroke->lineJoin) {
        case BnvgLineJoinType_Miter:
            *pVertexCount += 8;
            *pIndexCount += 14;
            break;
        case BnvgLineJoinType_Round:
            *pVertexCount += rStrokeInfo.roundDivideCount + 2;
            *pIndexCount += (rStrokeInfo.roundDivideCount + 2) * 2 + 4;
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Adds the line cap vertices and indices of one path to the counters.
 * @param pVertexCount Vertex counter.
 * @param pIndexCount Index counter.
 * @param shapeIndex Index of the shape.
 * @param effectStart First effect applied to the path.
 */
void VectorGraphicsShapeLayer::CalculateShapeStrokeEffectLineCapVertexInfoCount(
    int* pVertexCount, int* pIndexCount, int shapeIndex, int effectStart) const {
    for (uint32_t i = effectStart; i < m_pShapes[shapeIndex].effectCount; ++i) {
        const StrokeInfo& rStrokeInfo = m_pShapeDrawInfos[shapeIndex].pStrokeInfos[i];
        const ResBnvgShapeStrokeEffect* pStroke =
            static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

        if (pStroke == nullptr) {
            continue;
        }

        switch (pStroke->lineCap) {
        case BnvgLineCapType_Round: {
            const int count = rStrokeInfo.roundDivideCount * 2 + 4;
            *pVertexCount += count;
            *pIndexCount += count * 2;
            break;
        }
        case BnvgLineCapType_Square:
            *pVertexCount += 4;
            *pIndexCount += 4;
            break;
        default:
            break;
        }
    }
}

/**
 * @brief Calculates the largest mesh the layer can generate.
 * @param pFillVertexCount Fill vertex counter.
 * @param pFillIndexCount Fill index counter.
 * @param pStrokeVertexCount Stroke vertex counter.
 * @param pStrokeIndexCount Stroke index counter.
 */
void VectorGraphicsShapeLayer::CalculateMaxGenerateVertexInfo(int* pFillVertexCount,
                                                              int* pFillIndexCount,
                                                              int* pStrokeVertexCount,
                                                              int* pStrokeIndexCount) const {
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        const ResBnvgShapeInfo& rShapeInfo = m_pShapes[i];

        if (!IsShapeNeedRendering(rShapeInfo)) {
            continue;
        }

        for (uint32_t j = 0; j < rShapeInfo.pathCount; ++j) {
            const ResBnvgShapePath* pPath = rShapeInfo.ppPaths[j];

            if (!IsPathNeedRendering(*pPath)) {
                continue;
            }

            VectorGraphicsShapePathProcessor* pProcessor =
                m_pShapeDrawInfos[i].ppPathProcessors[j];
            const uint32_t controlPointCount = pProcessor->GetControlPointCount();

            for (uint32_t k = 0; k < controlPointCount; ++k) {
                const int divideCount = pProcessor->CalculatePathDivideVertexCount(k);
                CalculateShapeStrokeEffectSegmentVertexInfoCount(
                    pStrokeVertexCount, pStrokeIndexCount, i, pPath->effectStartIndex,
                    divideCount + 1);
            }

            if (j == 0) {
                CalculateShapeStrokeEffectLineCapVertexInfoCount(
                    pStrokeVertexCount, pStrokeIndexCount, i, pPath->effectStartIndex);
            }

            const int vertexCount = pProcessor->CalculateVertexCount();
            *pFillVertexCount += vertexCount;

            for (uint32_t k = pPath->effectStartIndex; k < rShapeInfo.effectCount; ++k) {
                const ResBnvgShapeEffect* pEffect = rShapeInfo.ppEffects[k];

                switch (pEffect->type) {
                case BnvgShapeEffectType_Fill:
                    *pFillIndexCount += vertexCount * 3;
                    break;
                case BnvgShapeEffectType_Stroke:
                    if (controlPointCount > 1 &&
                        static_cast<const ResBnvgShapeStrokeEffect*>(pEffect)->lineJoin != 0) {
                        *pStrokeIndexCount += 2;
                    }

                    break;
                default:
                    break;
                }
            }
        }
    }
}

/** @brief Returns the largest control point count of the drawn paths. */
uint32_t VectorGraphicsShapeLayer::FindMaxControlPointCount() const {
    uint32_t maxCount = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        const ResBnvgShapeInfo& rShapeInfo = m_pShapes[i];

        for (uint32_t j = 0; j < rShapeInfo.pathCount; ++j) {
            if (!IsPathNeedRendering(*rShapeInfo.ppPaths[j])) {
                continue;
            }

            maxCount = std::max(
                maxCount, m_pShapeDrawInfos[i].ppPathProcessors[j]->GetControlPointCount());
        }
    }

    return maxCount;
}

/** @brief Resets the fill and stroke counters. */
void VectorGraphicsShapeLayer::ResetVertexInfo() {
    m_FillVertexCount = 0;
    m_FillIndexCount = 0;
    m_StrokeVertexCount = 0;
    m_StrokeIndexCount = 0;
}

/**
 * @brief Fills one draw entry.
 * @param pPathDrawInfo Entry to fill.
 * @param type Effect drawn by the entry.
 * @param indexStart First index.
 * @param indexCount Number of indices.
 * @param pixelShaderOffset Offset of the colour constants.
 */
void VectorGraphicsShapeLayer::SetPathDrawInfo(VectorGraphicsPathDrawInfo* pPathDrawInfo,
                                               BnvgShapeEffectType type, int indexStart,
                                               int indexCount, size_t pixelShaderOffset) const {
    pPathDrawInfo->type = type;
    pPathDrawInfo->indexStart = indexStart;
    pPathDrawInfo->indexCount = indexCount;
    pPathDrawInfo->pixelShaderOffset = pixelShaderOffset;
}

/**
 * @brief Generates a miter join between two stroke segments.
 * @param ppVertex Stroke vertex write cursor.
 * @param ppIndex Stroke index write cursor.
 * @param rStrokeInfo Stroke parameters.
 * @param time Frame the miter limit is evaluated at.
 * @param rJoinInfo Edges of the two segments.
 * @param isLastJoin Whether the join closes the path.
 * @param baseIndex First vertex of the path.
 */
void VectorGraphicsShapeLayer::MakeStrokeMiterJoinPolygon(nn::util::Float2** ppVertex,
                                                          uint32_t** ppIndex,
                                                          StrokeInfo& rStrokeInfo, float time,
                                                          const StrokeJoinInfo& rJoinInfo,
                                                          bool isLastJoin, int baseIndex) {
    const float cross = nn::util::VectorCross(rJoinInfo.prevTangent, rJoinInfo.nextTangent);

    if (cross == 0.0f) {
        return;
    }

    nn::util::Vector2f outerDelta;
    nn::util::VectorSubtract(&outerDelta, rJoinInfo.nextOuter, rJoinInfo.prevOuter);
    nn::util::Vector2f miterPoint;
    nn::util::VectorMultiply(&miterPoint, rJoinInfo.prevTangent,
                             nn::util::VectorCross(outerDelta, rJoinInfo.nextTangent) / cross);
    nn::util::VectorAdd(&miterPoint, rJoinInfo.prevOuter, miterPoint);

    nn::util::Vector2f miterOffset;
    nn::util::VectorSubtract(&miterOffset, miterPoint, rJoinInfo.center);
    const ResBnvgShapeStrokeEffect* pStroke =
        static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);
    const float miterLimit = GetValue(pStroke->miterLimit, time);

    if (nn::util::VectorLength(miterOffset) < miterLimit * rStrokeInfo.halfWidth) {
        nn::util::Vector2f innerDelta;
        nn::util::VectorSubtract(&innerDelta, rJoinInfo.nextInner, rJoinInfo.prevInner);
        nn::util::Vector2f innerPoint;
        nn::util::VectorMultiply(&innerPoint, rJoinInfo.prevTangent,
                                 nn::util::VectorCross(innerDelta, rJoinInfo.nextTangent) / cross);
        nn::util::VectorAdd(&innerPoint, rJoinInfo.prevInner, innerPoint);

        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount - 1);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.prevOuter);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.center);

        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, miterPoint);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.nextOuter);

        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount - 1);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.nextInner);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, innerPoint);

        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.center);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, rJoinInfo.prevInner);

        if (!isLastJoin) {
            WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount - 1);
            WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        }
    } else if (isLastJoin) {
        WriteIndexData(ppIndex, &m_StrokeIndexCount, baseIndex);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, baseIndex + 1);
    }
}

/**
 * @brief Generates a triangle fan approximating a circular arc.
 * @param ppVertex Stroke vertex write cursor.
 * @param ppIndex Stroke index write cursor.
 * @param rCenter Center of the arc.
 * @param rStartDirection Direction the arc starts at.
 * @param sweepAngle Angle the arc covers.
 * @param radius Radius of the arc.
 * @param divideCount Number of arc segments.
 */
void VectorGraphicsShapeLayer::MakeStrokeCirclePolygon(nn::util::Float2** ppVertex,
                                                       uint32_t** ppIndex,
                                                       const nn::util::Vector2f& rCenter,
                                                       const nn::util::Vector2f& rStartDirection,
                                                       float sweepAngle, float radius,
                                                       int divideCount) {
    const int centerIndex = m_StrokeVertexCount;
    WriteVertexData(ppVertex, &m_StrokeVertexCount, rCenter);

    float angle = Vector2Angle(rStartDirection, nn::util::Vector2f(1.0f, 0.0f));
    const float angleStep = sweepAngle / divideCount;

    for (int i = 0; i <= divideCount; ++i) {
        const nn::util::AngleIndex angleIndex = nn::util::RadianToAngleIndex(angle);
        const float cos = nn::util::CosTable(angleIndex);
        const float sin = nn::util::SinTable(angleIndex);

        WriteIndexData(ppIndex, &m_StrokeIndexCount, centerIndex);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        (*ppVertex)->x = rCenter.GetX() + cos * radius;
        (*ppVertex)->y = rCenter.GetY() - sin * radius;
        ++*ppVertex;
        ++m_StrokeVertexCount;

        angle += angleStep;
    }
}

/**
 * @brief Generates a round join between two stroke segments.
 * @param ppVertex Stroke vertex write cursor.
 * @param ppIndex Stroke index write cursor.
 * @param rStrokeInfo Stroke parameters.
 * @param rCenter Point the segments meet at.
 * @param rNormal Normal of the previous segment.
 * @param isLastJoin Whether the join closes the path.
 */
void VectorGraphicsShapeLayer::MakeStrokeRoundJoinPolygon(nn::util::Float2** ppVertex,
                                                          uint32_t** ppIndex,
                                                          StrokeInfo& rStrokeInfo,
                                                          const nn::util::Vector2f& rCenter,
                                                          const nn::util::Vector2f& rNormal,
                                                          bool isLastJoin) {
    WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount - 1);
    WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
    MakeStrokeCirclePolygon(ppVertex, ppIndex, rCenter, rNormal, nn::util::FloatPi,
                            rStrokeInfo.halfWidth, rStrokeInfo.roundDivideCount);

    if (isLastJoin) {
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount - 1);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
    }
}

/**
 * @brief Generates the cap at one end of an open stroke.
 * @param ppVertex Stroke vertex write cursor.
 * @param ppIndex Stroke index write cursor.
 * @param rStrokeInfo Stroke parameters.
 * @param rPoint End point of the path.
 * @param rNormal Normal at the end point.
 * @param rTangent Tangent at the end point.
 * @param isStart Whether the cap is at the start of the path.
 */
void VectorGraphicsShapeLayer::MakeStrokeLineCapPolygon(nn::util::Float2** ppVertex,
                                                        uint32_t** ppIndex,
                                                        StrokeInfo& rStrokeInfo,
                                                        const nn::util::Vector2f& rPoint,
                                                        const nn::util::Vector2f& rNormal,
                                                        const nn::util::Vector2f& rTangent,
                                                        bool isStart) {
    const ResBnvgShapeStrokeEffect* pStroke =
        static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

    switch (pStroke->lineCap) {
    case BnvgLineCapType_Round: {
        nn::util::Vector2f negativeNormal;
        nn::util::VectorSubtract(&negativeNormal, nn::util::Vector2f(vdup_n_f32(0.0f)), rNormal);
        const nn::util::Vector2f normal = isStart ? negativeNormal : rNormal;
        MakeStrokeCirclePolygon(ppVertex, ppIndex, rPoint, normal, nn::util::FloatPi,
                                rStrokeInfo.halfWidth, rStrokeInfo.roundDivideCount);
        break;
    }
    case BnvgLineCapType_Square: {
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);

        nn::util::Vector2f negativeTangent;
        nn::util::VectorSubtract(&negativeTangent, nn::util::Vector2f(vdup_n_f32(0.0f)),
                                 rTangent);
        const nn::util::Vector2f tangent = isStart ? negativeTangent : rTangent;
        nn::util::Vector2f center;
        nn::util::VectorMultiply(&center, tangent, rStrokeInfo.halfWidth);
        nn::util::VectorAdd(&center, rPoint, center);
        nn::util::Vector2f offset;
        nn::util::VectorMultiply(&offset, rNormal, rStrokeInfo.halfWidth);

        nn::util::Vector2f outer;
        nn::util::VectorAdd(&outer, offset, center);
        nn::util::Vector2f inner;
        nn::util::VectorSubtract(&inner, center, offset);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, outer);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, inner);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief Calculates the normal and the tangent of a path at a point.
 * @param pNormal Receives the unit normal.
 * @param pTangent Receives the unit tangent.
 * @param pPath Path being stroked.
 * @param rStart Point on the path.
 * @param rEnd Next point on the path.
 */
void VectorGraphicsShapeLayer::CalculateStrokeNormalAndTangent(
    nn::util::Vector2f* pNormal, nn::util::Vector2f* pTangent,
    const VectorGraphicsShapePathProcessor* pPath, const nn::util::Vector2f& rStart,
    const nn::util::Vector2f& rEnd) {
    const nn::util::Float2 position = pPath->GetPosition();

    if (pPath->GetType() == BnvgShapePathType_Ellipse) {
        *pNormal = nn::util::Vector2f(rStart.GetX() - position.x, rStart.GetY() - position.y);
        NormalizeWithTruncateAroundZero(pNormal);
        *pTangent = nn::util::Vector2f(-pNormal->GetY(), pNormal->GetX());
    } else {
        nn::util::VectorSubtract(pTangent, rEnd, rStart);
        NormalizeWithTruncateAroundZero(pTangent);
    }

    *pNormal = nn::util::Vector2f(-pTangent->GetY(), pTangent->GetX());
}

/**
 * @brief Generates the stroke of a path: segments, joins and caps.
 * @param pPath Path being stroked.
 * @param ppVertex Stroke vertex write cursor.
 * @param ppIndex Stroke index write cursor.
 * @param rStrokeInfo Stroke parameters.
 * @param time Frame to evaluate at.
 * @param pSegments Segments of the path.
 * @param segmentCount Number of segments.
 * @param isClosed Whether the path is closed.
 */
void VectorGraphicsShapeLayer::MakeStrokePolygon(const VectorGraphicsShapePathProcessor* pPath,
                                                 nn::util::Float2** ppVertex, uint32_t** ppIndex,
                                                 StrokeInfo& rStrokeInfo, float time,
                                                 const StrokeSegmentVertexInfo* pSegments,
                                                 int segmentCount, bool isClosed) {
    if (rStrokeInfo.halfWidth <= 0.0f || rStrokeInfo.pStrokeEffect == nullptr) {
        return;
    }

    const int baseIndex = m_StrokeVertexCount;

    for (int i = 0; i < segmentCount; ++i) {
        const int next = i + 1 == segmentCount ? 0 : i + 1;
        const StrokeSegmentVertexInfo& rSegment = pSegments[i];

        if (rSegment.ignore) {
            continue;
        }

        const StrokeSegmentVertexInfo& rNextSegment = pSegments[next];
        nn::util::Vector2f start;
        nn::util::VectorLoad(&start, rSegment.start);
        nn::util::Vector2f firstPoint;
        nn::util::VectorLoad(&firstPoint, rSegment.pCurvePoints[0]);
        nn::util::Vector2f normal;
        nn::util::Vector2f tangent;
        CalculateStrokeNormalAndTangent(&normal, &tangent, pPath, start, firstPoint);

        if (i == 0 && !isClosed) {
            MakeStrokeLineCapPolygon(ppVertex, ppIndex, rStrokeInfo, start, normal, tangent,
                                     true);
        }

        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
        WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);

        nn::util::Vector2f offset;
        nn::util::VectorMultiply(&offset, normal, rStrokeInfo.halfWidth);
        nn::util::Vector2f outer;
        nn::util::VectorAdd(&outer, offset, start);
        nn::util::Vector2f inner;
        nn::util::VectorSubtract(&inner, start, offset);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, outer);
        WriteVertexData(ppVertex, &m_StrokeVertexCount, inner);

        for (uint32_t j = 0; j < rSegment.count; ++j) {
            WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount);
            WriteIndexData(ppIndex, &m_StrokeIndexCount, m_StrokeVertexCount + 1);

            nn::util::Vector2f point;
            nn::util::VectorLoad(&point, rSegment.pCurvePoints[j]);

            if (j == rSegment.count - 1) {
                if (rSegment.count >= 2) {
                    nn::util::Vector2f prevPoint;
                    nn::util::VectorLoad(&prevPoint, rSegment.pCurvePoints[j - 1]);
                    CalculateStrokeNormalAndTangent(&normal, &tangent, pPath, prevPoint, point);
                }
            } else {
                nn::util::Vector2f nextPoint;
                nn::util::VectorLoad(&nextPoint, rSegment.pCurvePoints[j + 1]);
                CalculateStrokeNormalAndTangent(&normal, &tangent, pPath, point, nextPoint);
            }

            nn::util::VectorMultiply(&offset, normal, rStrokeInfo.halfWidth);
            nn::util::VectorAdd(&outer, offset, point);
            nn::util::VectorSubtract(&inner, point, offset);
            WriteVertexData(ppVertex, &m_StrokeVertexCount, outer);
            WriteVertexData(ppVertex, &m_StrokeVertexCount, inner);
        }

        StrokeJoinInfo joinInfo;
        nn::util::VectorLoad(&joinInfo.center, rNextSegment.start);
        joinInfo.prevOuter = outer;
        joinInfo.prevInner = inner;
        joinInfo.prevTangent = tangent;

        nn::util::Vector2f nextStart;
        nn::util::VectorLoad(&nextStart, rNextSegment.start);
        nn::util::Vector2f nextFirstPoint;
        nn::util::VectorLoad(&nextFirstPoint, rNextSegment.pCurvePoints[0]);
        nn::util::Vector2f nextNormal;
        nn::util::Vector2f nextTangent;
        CalculateStrokeNormalAndTangent(&nextNormal, &nextTangent, pPath, nextStart,
                                        nextFirstPoint);
        nn::util::VectorMultiply(&offset, nextNormal, rStrokeInfo.halfWidth);
        nn::util::VectorAdd(&joinInfo.nextOuter, offset, nextStart);
        nn::util::VectorSubtract(&joinInfo.nextInner, nextStart, offset);
        joinInfo.nextTangent = nextTangent;

        if (i < segmentCount - 1 || isClosed) {
            const ResBnvgShapeStrokeEffect* pStroke =
                static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

            if (pStroke != nullptr) {
                switch (pStroke->lineJoin) {
                case BnvgLineJoinType_Miter:
                    MakeStrokeMiterJoinPolygon(ppVertex, ppIndex, rStrokeInfo, time, joinInfo,
                                               i == segmentCount - 1, baseIndex);
                    break;
                case BnvgLineJoinType_Round: {
                    const nn::util::Vector2f joinNormal(-tangent.GetY(), tangent.GetX());
                    MakeStrokeRoundJoinPolygon(ppVertex, ppIndex, rStrokeInfo, joinInfo.center,
                                               joinNormal, i == segmentCount - 1);
                    break;
                }
                default:
                    break;
                }
            }
        }

        if (i == segmentCount - 1 && !isClosed) {
            nn::util::Vector2f capNormal;
            nn::util::VectorSubtract(&capNormal, outer, inner);
            NormalizeWithTruncateAroundZero(&capNormal);
            nn::util::Vector2f endPoint;
            nn::util::VectorLoad(&endPoint, rSegment.pCurvePoints[rSegment.count - 1]);
            MakeStrokeLineCapPolygon(ppVertex, ppIndex, rStrokeInfo, endPoint, capNormal, tangent,
                                     false);
        }
    }

    const ResBnvgShapeStrokeEffect* pStroke =
        static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

    if (pStroke != nullptr && pStroke->lineJoin == BnvgLineJoinType_Bevel) {
        *(*ppIndex)++ = baseIndex;
        *(*ppIndex)++ = baseIndex + 1;
        m_StrokeIndexCount += 2;
    }
}

/**
 * @brief Evaluates the transform and opacity of every shape group.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::CalculateGroupInfo(float time) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_GroupCount); ++i) {
        const ResBnvgShapeGroup& rGroup = m_pGroups[i];
        float opacity = GetValue(rGroup.opacity, time);

        if (rGroup.parentIndex >= 0) {
            nn::util::MatrixT4x3fType matrix;
            CalcTransform(&matrix, rGroup.transform, time);
            const GroupInfo& rParent = m_pGroupInfos[rGroup.parentIndex];
            MultiplyMatrix(&m_pGroupInfos[i].matrix, rParent.matrix, matrix);
            opacity *= rParent.opacity;
        } else {
            CalcTransform(&m_pGroupInfos[i].matrix, rGroup.transform, time);
        }

        m_pGroupInfos[i].opacity = opacity;
    }
}

/**
 * @brief Combines the trim effects applied to a path.
 * @param pTrimStart Receives the start of the visible part.
 * @param pTrimEnd Receives the end of the visible part.
 * @param rShapeInfo Shape resource.
 * @param rPath Path resource.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::CalculateTrimEffect(float* pTrimStart, float* pTrimEnd,
                                                   const ResBnvgShapeInfo& rShapeInfo,
                                                   const ResBnvgShapePath& rPath,
                                                   float time) const {
    float trimStart = 0.0f;
    float trimEnd = 1.0f;

    for (uint32_t i = rPath.effectStartIndex; i < rShapeInfo.effectCount; ++i) {
        const ResBnvgShapeEffect* pEffect = rShapeInfo.ppEffects[i];

        if ((pEffect->flags & 1) != 0 || pEffect->type != BnvgShapeEffectType_Trim ||
            (rPath.drawFlags & 4) == 0) {
            continue;
        }

        const ResBnvgShapeTrimEffect* pTrim = static_cast<const ResBnvgShapeTrimEffect*>(pEffect);
        const float range = trimEnd - trimStart;
        const float start = trimStart + range * GetValue(pTrim->start, time);
        const float end = trimStart + range * GetValue(pTrim->end, time);

        if (start > end) {
            trimEnd = start;
            trimStart = end;
        } else {
            trimEnd = end;
            trimStart = start;
        }

        if (trimEnd - trimStart < 1.0f) {
            float offset = GetValue(pTrim->offset, time);

            if (offset > 1.0f) {
                offset = offset - static_cast<int>(offset);
            }

            if (offset < 0.0f) {
                offset += 1.0f;
            }

            trimStart = fminf(trimStart + offset, 1.0f);
            trimEnd = fminf(trimEnd + offset, 1.0f);
        }
    }

    *pTrimStart = trimStart;
    *pTrimEnd = trimEnd;
}

/**
 * @brief Evaluates the path and stroke animations of every shape.
 * @param rDrawInfo Drawing state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::EvaluateParams(DrawInfo& rDrawInfo, float time) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        const ResBnvgShapeInfo& rShapeInfo = m_pShapes[i];
        ShapeDrawInfo& rInfo = m_pShapeDrawInfos[i];

        for (uint32_t j = 0; j < rShapeInfo.pathCount; ++j) {
            if (rInfo.ppPathProcessors[j] != nullptr) {
                rInfo.ppPathProcessors[j]->EvaluateParams(rDrawInfo, time);
            }
        }

        for (uint32_t j = 0; j < rShapeInfo.effectCount; ++j) {
            StrokeInfo& rStrokeInfo = rInfo.pStrokeInfos[j];
            const ResBnvgShapeStrokeEffect* pStroke =
                static_cast<const ResBnvgShapeStrokeEffect*>(rStrokeInfo.pStrokeEffect);

            if (pStroke != nullptr) {
                rStrokeInfo.halfWidth = GetValue(pStroke->width, time) * 0.5f;
                rStrokeInfo.roundDivideCount =
                    CalculateQuaterCircleDivideCount(rStrokeInfo.halfWidth);
            }
        }
    }
}

/**
 * @brief Generates the meshes and the draw entries of every shape.
 * @param rDrawInfo Drawing state holding the constant buffer.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::CalculateShapeDrawInfo(DrawInfo& rDrawInfo,
                                                      VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                                      float time) {
    int fillVertexCount = 0;
    int fillIndexCount = 0;
    int strokeVertexCount = 0;
    int strokeIndexCount = 0;
    const uint32_t maxControlPointCount = FindMaxControlPointCount();
    CalculateMaxGenerateVertexInfo(&fillVertexCount, &fillIndexCount, &strokeVertexCount,
                                   &strokeIndexCount);

    if (fillVertexCount == 0 && strokeVertexCount == 0) {
        return;
    }

    ShapeMeshBufferInfo bufferInfo;
    AllocateShapeMeshBufferFromConstantBuffer(&bufferInfo, rDrawInfo, fillVertexCount,
                                              fillIndexCount, strokeVertexCount,
                                              strokeIndexCount, maxControlPointCount);
    m_FillVertexCount = 0;
    m_FillIndexCount = 0;
    m_FillVertexOffset = bufferInfo.fillVertexOffset;
    m_FillIndexOffset = bufferInfo.fillIndexOffset;
    m_StrokeVertexCount = 0;
    m_StrokeIndexCount = 0;
    m_StrokeVertexOffset = bufferInfo.strokeVertexOffset;
    m_StrokeIndexOffset = bufferInfo.strokeIndexOffset;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        const ResBnvgShapeInfo& rShapeInfo = m_pShapes[i];

        if (!IsShapeNeedRendering(rShapeInfo)) {
            continue;
        }

        SetupConstantBuffer(rDrawInfo, rVectorGraphicsDrawInfo, i, rShapeInfo, time);
        int pathDrawInfoCount = 0;

        for (uint32_t j = 0; j < rShapeInfo.pathCount; ++j) {
            const ResBnvgShapePath* pPath = rShapeInfo.ppPaths[j];

            if (!IsPathNeedRendering(*pPath)) {
                continue;
            }

            VectorGraphicsShapePathProcessor* pProcessor =
                m_pShapeDrawInfos[i].ppPathProcessors[j];
            const int fillIndexStart = m_FillIndexCount;
            float trimEnd = 1.0f;
            float trimStart = 0.0f;
            CalculateTrimEffect(&trimStart, &trimEnd, rShapeInfo, *pPath, time);

            if (trimEnd - trimStart < 0.001f) {
                continue;
            }

            pProcessor->SetTrimParams(trimStart, trimEnd);
            const int segmentCount = pProcessor->GenerateAndWritePathVertex(
                &bufferInfo.pFillVertex, &m_FillVertexCount, &bufferInfo.pFillIndex,
                &m_FillIndexCount, bufferInfo.pStrokeSegment);

            for (uint32_t k = pPath->effectStartIndex; k < rShapeInfo.effectCount; ++k) {
                const ResBnvgShapeEffect* pEffect = rShapeInfo.ppEffects[k];

                if ((pEffect->flags & 1) != 0) {
                    continue;
                }

                switch (pEffect->type) {
                case BnvgShapeEffectType_Fill:
                    if ((pPath->drawFlags & BnvgShapePathFlag_Fill) != 0) {
                        SetPathDrawInfo(
                            &m_pShapeDrawInfos[i].pPathDrawInfos[pathDrawInfoCount++],
                            BnvgShapeEffectType_Fill, fillIndexStart,
                            m_FillIndexCount - fillIndexStart,
                            m_pShapeDrawInfos[i].pPixelShaderOffsets[k]);
                    }

                    break;
                case BnvgShapeEffectType_Stroke:
                    if ((pPath->drawFlags & BnvgShapePathFlag_Stroke) != 0) {
                        const int strokeIndexStart = m_StrokeIndexCount;
                        const bool isClosed =
                            pProcessor->IsPathClosed() && !pProcessor->IsPathTrimed();
                        MakeStrokePolygon(pProcessor, &bufferInfo.pStrokeVertex,
                                          &bufferInfo.pStrokeIndex,
                                          m_pShapeDrawInfos[i].pStrokeInfos[k], time,
                                          bufferInfo.pStrokeSegment, segmentCount, isClosed);
                        SetPathDrawInfo(
                            &m_pShapeDrawInfos[i].pPathDrawInfos[pathDrawInfoCount++],
                            static_cast<BnvgShapeEffectType>(pEffect->type), strokeIndexStart,
                            m_StrokeIndexCount - strokeIndexStart,
                            m_pShapeDrawInfos[i].pPixelShaderOffsets[k]);
                    }

                    break;
                default:
                    break;
                }
            }
        }

        m_pShapeDrawInfos[i].pathDrawInfoCount = pathDrawInfoCount;
    }
}

/**
 * @brief Evaluates the layer, its groups, its shapes and builds the meshes.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsShapeLayer::Calculate(DrawInfo& rDrawInfo,
                                         VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                         float time) {
    VectorGraphicsLayer::Calculate(rDrawInfo, rVectorGraphicsDrawInfo, time);

    if (m_IsVisible) {
        CalculateGroupInfo(time);
        EvaluateParams(rDrawInfo, time);
        CalculateShapeDrawInfo(rDrawInfo, rVectorGraphicsDrawInfo, time);
    }
}

/**
 * @brief Draws one fill where the stencil buffer is set.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param rPathDrawInfo Draw entry.
 */
void VectorGraphicsShapeLayer::DrawFillPolygon(DrawInfo& rDrawInfo,
                                               VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                               nn::gfx::CommandBuffer& rCommandBuffer,
                                               size_t vertexShaderOffset,
                                               const VectorGraphicsPathDrawInfo& rPathDrawInfo) {
    const int variation = std::max(rVectorGraphicsDrawInfo.m_MaskTextureStackCount, 0);
    SetVertexBuffer(rDrawInfo, rCommandBuffer, m_FillVertexOffset, m_FillVertexCount);
    ApplyConstantBuffers(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                         static_cast<VectorGraphicsShaderVariation>(variation), vertexShaderOffset,
                         rPathDrawInfo.pixelShaderOffset);

    GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.m_pGraphicsResource);
    ToImpl(rCommandBuffer)
        .SetDepthStencilState(&pResource->m_PresetVectorGraphicsDepthStencilState[1]);
    ToImpl(rCommandBuffer).SetBlendState(pResource->GetPresetBlendState(PresetBlendStateId_Default));
    DrawIndexed(rDrawInfo, rCommandBuffer, nn::gfx::PrimitiveTopology_TriangleList,
                m_FillIndexOffset + sizeof(uint32_t) * rPathDrawInfo.indexStart,
                rPathDrawInfo.indexCount);

    if (rDrawInfo.m_pDepthStencilState != nullptr) {
        ToImpl(rCommandBuffer).SetDepthStencilState(rDrawInfo.m_pDepthStencilState);
    }
}

/**
 * @brief Draws one fill into the stencil buffer.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param rPathDrawInfo Draw entry.
 */
void VectorGraphicsShapeLayer::DrawFillPolygonStencil(
    DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
    nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
    const VectorGraphicsPathDrawInfo& rPathDrawInfo) {
    const int variation = std::max(rVectorGraphicsDrawInfo.m_MaskTextureStackCount, 0);
    SetVertexBuffer(rDrawInfo, rCommandBuffer, m_FillVertexOffset, m_FillVertexCount);
    ApplyConstantBuffers(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                         static_cast<VectorGraphicsShaderVariation>(variation), vertexShaderOffset,
                         rPathDrawInfo.pixelShaderOffset);

    GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.m_pGraphicsResource);
    ToImpl(rCommandBuffer)
        .SetDepthStencilState(&pResource->m_PresetVectorGraphicsDepthStencilState[0]);
    ToImpl(rCommandBuffer)
        .SetBlendState(pResource->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
    DrawIndexed(rDrawInfo, rCommandBuffer, nn::gfx::PrimitiveTopology_TriangleList,
                m_FillIndexOffset + sizeof(uint32_t) * rPathDrawInfo.indexStart,
                rPathDrawInfo.indexCount);

    if (rDrawInfo.m_pDepthStencilState != nullptr) {
        ToImpl(rCommandBuffer).SetDepthStencilState(rDrawInfo.m_pDepthStencilState);
    }
}

/**
 * @brief Draws one stroke.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constants.
 * @param rPathDrawInfo Draw entry.
 */
void VectorGraphicsShapeLayer::DrawStrokePolygon(DrawInfo& rDrawInfo,
                                                 VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                                 nn::gfx::CommandBuffer& rCommandBuffer,
                                                 size_t vertexShaderOffset,
                                                 const VectorGraphicsPathDrawInfo& rPathDrawInfo) {
    if (m_StrokeVertexCount <= 0 || m_StrokeIndexCount <= 0) {
        return;
    }

    const int variation = std::max(rVectorGraphicsDrawInfo.m_MaskTextureStackCount, 0);
    GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.m_pGraphicsResource);
    ToImpl(rCommandBuffer)
        .SetDepthStencilState(&pResource->m_PresetVectorGraphicsDepthStencilState[2]);
    SetVertexBuffer(rDrawInfo, rCommandBuffer, m_StrokeVertexOffset, m_StrokeVertexCount);
    ApplyConstantBuffers(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                         static_cast<VectorGraphicsShaderVariation>(variation), vertexShaderOffset,
                         rPathDrawInfo.pixelShaderOffset);
    ToImpl(rCommandBuffer).SetBlendState(pResource->GetPresetBlendState(PresetBlendStateId_Default));
    DrawIndexed(rDrawInfo, rCommandBuffer, nn::gfx::PrimitiveTopology_TriangleStrip,
                m_StrokeIndexOffset + sizeof(uint32_t) * rPathDrawInfo.indexStart,
                rPathDrawInfo.indexCount);
}

/**
 * @brief Draws every shape of the layer.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state holding the mask textures.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsShapeLayer::DrawVectorGraphicsTexture(
    DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
    nn::gfx::CommandBuffer& rCommandBuffer) {
    const int variation = std::max(rVectorGraphicsDrawInfo.m_MaskTextureStackCount, 0);
    const VectorGraphicsDrawInfo::ShaderVariationInfo& rVariation =
        rVectorGraphicsDrawInfo.m_ShaderVariations[variation];
    SetupShaderWithShaderCache(rDrawInfo, rCommandBuffer, rVectorGraphicsDrawInfo.m_pShaderInfo,
                               rVariation.variationIndex);

    const nn::gfx::DescriptorSlot sampler =
        rDrawInfo.m_pGraphicsResource->GetSamplerDescriptorSlot(
            PresetSamplerId_ClampToEdgeU_ClampToEdgeV_MinPoint_MagPoint_MipPoint);

    for (int i = 0; i < rVectorGraphicsDrawInfo.m_MaskTextureStackCount; ++i) {
        rCommandBuffer.SetTextureAndSampler(rVariation.textureSlots[i], nn::gfx::ShaderStage_Pixel,
                                            *rVectorGraphicsDrawInfo.m_pMaskTextureStack[i],
                                            sampler);
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ShapeCount); ++i) {
        if (!IsShapeNeedRendering(m_pShapes[i])) {
            continue;
        }

        for (uint32_t j = 0; j < static_cast<uint32_t>(m_pShapeDrawInfos[i].pathDrawInfoCount);
             ++j) {
            const VectorGraphicsPathDrawInfo& rPathDrawInfo =
                m_pShapeDrawInfos[i]
                    .pPathDrawInfos[m_pShapeDrawInfos[i].pathDrawInfoCount - 1 - j];
            ClearStencilBuffer(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);

            switch (rPathDrawInfo.type) {
            case BnvgShapeEffectType_Fill:
                for (uint32_t k = 0;
                     k < static_cast<uint32_t>(m_pShapeDrawInfos[i].pathDrawInfoCount); ++k) {
                    if (rPathDrawInfo.type == BnvgShapeEffectType_Fill &&
                        m_pShapeDrawInfos[i].pPathDrawInfos[k].pixelShaderOffset ==
                            rPathDrawInfo.pixelShaderOffset) {
                        DrawFillPolygonStencil(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                                               m_pShapeDrawInfos[i].vertexShaderOffset,
                                               m_pShapeDrawInfos[i].pPathDrawInfos[k]);
                    }
                }

                DrawFillPolygon(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                                m_pShapeDrawInfos[i].vertexShaderOffset, rPathDrawInfo);
                break;
            case BnvgShapeEffectType_Stroke:
                DrawStrokePolygon(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer,
                                  m_pShapeDrawInfos[i].vertexShaderOffset, rPathDrawInfo);
                break;
            default:
                break;
            }
        }
    }
}

/**
 * @brief Draws the shapes when the layer is visible and generated geometry.
 * @param rDrawInfo Drawing state.
 * @param rVectorGraphicsDrawInfo Vector graphics state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsShapeLayer::DrawLayerImpl(DrawInfo& rDrawInfo,
                                             VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                             nn::gfx::CommandBuffer& rCommandBuffer) {
    if (m_IsVisible && (m_FillVertexCount > 0 || m_StrokeVertexCount > 0)) {
        DrawVectorGraphicsTexture(rDrawInfo, rVectorGraphicsDrawInfo, rCommandBuffer);
    }
}

/** @brief Starts without a file. */
VectorGraphicsScene::VectorGraphicsScene()
    : m_pFileHeader(nullptr), m_pRootLayer(nullptr), m_Width(1), m_Height(1) {}

/**
 * @brief Builds a new instance of an already built scene.
 * @param rSource Scene to copy.
 * @param pDevice Device the resources are created on.
 */
VectorGraphicsScene::VectorGraphicsScene(const VectorGraphicsScene& rSource,
                                         nn::gfx::Device* pDevice)
    : VectorGraphicsScene(rSource, pDevice, nullptr) {}

/**
 * @brief Builds a new instance of an already built scene.
 * @param rSource Scene to copy.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the mask textures.
 */
VectorGraphicsScene::VectorGraphicsScene(const VectorGraphicsScene& rSource,
                                         nn::gfx::Device* pDevice, const Layout* pLayout)
    : m_pFileHeader(nullptr), m_pRootLayer(nullptr), m_Width(1), m_Height(1) {
    m_pFileHeader = rSource.m_pFileHeader;
    m_Width = rSource.m_Width;
    m_Height = rSource.m_Height;

    const nn::util::BinaryBlockHeader* pBlock = m_pFileHeader->GetFirstBlock();
    m_ReservedMemory.Initialize(rSource.m_ReservedMemory.GetReservedSize());

    for (; pBlock != nullptr; pBlock = pBlock->GetNextBlock()) {
        if (pBlock->signature._packed == BnvgCompositionDataBlock::Signature) {
            m_pRootLayer = m_ReservedMemory.New<VectorGraphicsLayer>(
                static_cast<const ResBnvgLayerBasicInfo*>(nullptr),
                static_cast<const ResBnvgMaskInfoLayer*>(nullptr), &m_ReservedMemory);
            m_pRootLayer->Initialize(pDevice, pLayout, m_Width, m_Height);
            BuildComposition(pDevice, pLayout, m_pRootLayer,
                             static_cast<const BnvgCompositionDataBlock*>(pBlock), m_Width,
                             m_Height);
            break;
        }
    }

    m_DrawInfo = rSource.m_DrawInfo;
}

/**
 * @brief Creates the layers of one composition.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the mask textures.
 * @param pParent Layer the root layers of the composition are attached to.
 * @param pBlock Composition block.
 * @param width Width of the scene.
 * @param height Height of the scene.
 * @return Constant buffer size the created layers need per frame.
 */
size_t VectorGraphicsScene::BuildComposition(nn::gfx::Device* pDevice, const Layout* pLayout,
                                             VectorGraphicsLayer* pParent,
                                             const BnvgCompositionDataBlock* pBlock, int width,
                                             int height) {
    VectorGraphicsLayer* pLayers[pBlock->layerCount];
    size_t constantBufferSize = 0;
    int layerCount = 0;
    const ResBnvgMaskInfoLayer* pMaskInfoLayer = nullptr;

    for (uint32_t i = 0; i < pBlock->layerCount; ++i) {
        const ResBnvgLayerBasicInfo* pLayerInfo = pBlock->ppLayers[i];
        VectorGraphicsLayer* pLayer = nullptr;

        switch (pLayerInfo->type) {
        case BnvgLayerType_Shape: {
            pLayer = m_ReservedMemory.New<VectorGraphicsShapeLayer>(
                static_cast<const ResBnvgShapeLayer*>(pLayerInfo), pMaskInfoLayer,
                &m_ReservedMemory);
            break;
        }
        case BnvgLayerType_Mask:
            pMaskInfoLayer = static_cast<const ResBnvgMaskInfoLayer*>(pLayerInfo);
            continue;
        case BnvgLayerType_Composition: {
            pLayer = m_ReservedMemory.New<VectorGraphicsLayer>(pLayerInfo, pMaskInfoLayer,
                                                                &m_ReservedMemory);

            uint32_t compositionIndex = 0;

            for (const nn::util::BinaryBlockHeader* pChildBlock = m_pFileHeader->GetFirstBlock();
                 pChildBlock != nullptr; pChildBlock = pChildBlock->GetNextBlock()) {
                if (pChildBlock->signature._packed == BnvgCompositionDataBlock::Signature) {
                    if (compositionIndex == pLayerInfo->compositionIndex) {
                        constantBufferSize += BuildComposition(
                            pDevice, pLayout, pLayer,
                            static_cast<const BnvgCompositionDataBlock*>(pChildBlock), width,
                            height);
                        break;
                    }

                    ++compositionIndex;
                }
            }

            break;
        }
        case BnvgLayerType_Solid:
        case BnvgLayerType_Image:
        case BnvgLayerType_Null:
        case BnvgLayerType_Text: {
            pLayer = m_ReservedMemory.New<VectorGraphicsLayer>(pLayerInfo, pMaskInfoLayer,
                                                                &m_ReservedMemory);
            break;
        }
        default:
            continue;
        }

        if (pLayer != nullptr) {
            pLayer->Initialize(pDevice, pLayout, width, height);
            constantBufferSize += pLayer->GetRequiredConstantBufferSize(pDevice);
            pLayers[layerCount++] = pLayer;
        }
    }

    for (int i = 0; i < layerCount; ++i) {
        VectorGraphicsLayer* pLayer = pLayers[i];
        const int parentLayerId = pLayer->m_pBasicInfo->parentLayerId;

        if (parentLayerId < 0) {
            pParent->AppendChild(pLayer);
            continue;
        }

        for (uint32_t j = 0; j < pBlock->layerCount; ++j) {
            if (pLayers[j]->m_pBasicInfo->layerId == parentLayerId) {
                pLayers[j]->AppendChild(pLayer);
                break;
            }
        }
    }

    return constantBufferSize;
}

VectorGraphicsScene::~VectorGraphicsScene() {}

/**
 * @brief Builds the scene with the size stored in the file.
 * @param pResult Receives the constant buffer size the scene needs.
 * @param pDevice Device the resources are created on.
 * @param pResourceAccessor Accessor the shader is loaded from.
 * @param pFileHeader File to build.
 * @return True.
 */
bool VectorGraphicsScene::Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                ResourceAccessor* pResourceAccessor,
                                const BnvgFileHeader* pFileHeader) {
    Build(pResult, pDevice, nullptr, pResourceAccessor, pFileHeader, 0, 0);
    return true;
}

/**
 * @brief Builds the scene with the size stored in the file.
 * @param pResult Receives the constant buffer size the scene needs.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the mask textures.
 * @param pResourceAccessor Accessor the shader is loaded from.
 * @param pFileHeader File to build.
 * @return True.
 */
bool VectorGraphicsScene::Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                const Layout* pLayout, ResourceAccessor* pResourceAccessor,
                                const BnvgFileHeader* pFileHeader) {
    Build(pResult, pDevice, pLayout, pResourceAccessor, pFileHeader, 0, 0);
    return true;
}

/**
 * @brief Builds the scene.
 * @param pResult Receives the constant buffer size the scene needs.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the mask textures.
 * @param pResourceAccessor Accessor the shader is loaded from.
 * @param pFileHeader File to build.
 * @param width Width of the scene, or zero to use the file's.
 * @param height Height of the scene, or zero to use the file's.
 * @return True.
 */
bool VectorGraphicsScene::Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                const Layout* pLayout, ResourceAccessor* pResourceAccessor,
                                const BnvgFileHeader* pFileHeader, int width, int height) {
    m_ReservedMemory.Initialize(CaclulateRequiredMemorySize(pFileHeader));
    m_pFileHeader = pFileHeader;
    m_Width = width > 0 ? width : static_cast<int>(pFileHeader->width);
    m_Height = height > 0 ? height : static_cast<int>(pFileHeader->height);

    for (const nn::util::BinaryBlockHeader* pBlock = pFileHeader->GetFirstBlock();
         pBlock != nullptr; pBlock = pBlock->GetNextBlock()) {
        if (pBlock->signature._packed == BnvgCompositionDataBlock::Signature) {
            m_pRootLayer = m_ReservedMemory.New<VectorGraphicsLayer>(
                static_cast<const ResBnvgLayerBasicInfo*>(nullptr),
                static_cast<const ResBnvgMaskInfoLayer*>(nullptr), &m_ReservedMemory);
            m_pRootLayer->Initialize(pDevice, pLayout, m_Width, m_Height);
            const size_t constantBufferSize =
                BuildComposition(pDevice, pLayout, m_pRootLayer,
                                 static_cast<const BnvgCompositionDataBlock*>(pBlock), m_Width,
                                 m_Height);

            if (pResult != nullptr) {
                pResult->requiredUi2dConstantBufferSize += constantBufferSize;
            }

            break;
        }
    }

    const uint32_t key = 0;
    m_DrawInfo.Initialize(pResourceAccessor->AcquireArchiveShader(
        pDevice, VectorGraphicsDrawInfo::VariationTableSignature, 1, &key));
    return true;
}

/**
 * @brief Returns the scene memory a file needs.
 * @param pFileHeader File to measure.
 */
size_t VectorGraphicsScene::CaclulateRequiredMemorySize(const BnvgFileHeader* pFileHeader) {
    for (const nn::util::BinaryBlockHeader* pBlock = pFileHeader->GetFirstBlock();
         pBlock != nullptr; pBlock = pBlock->GetNextBlock()) {
        if (pBlock->signature._packed == BnvgCompositionDataBlock::Signature) {
            return CaclulateRequiredMemorySize(
                       pFileHeader, static_cast<const BnvgCompositionDataBlock*>(pBlock)) +
                   sizeof(VectorGraphicsLayer) + sizeof(nn::util::MatrixT4x3fType) + 16;
        }
    }

    return 0;
}

/**
 * @brief Returns the scene memory one composition needs.
 * @param pFileHeader File holding the composition.
 * @param pBlock Composition block.
 */
size_t VectorGraphicsScene::CaclulateRequiredMemorySize(const BnvgFileHeader* pFileHeader,
                                                        const BnvgCompositionDataBlock* pBlock) {
    size_t size = 0;
    const ResBnvgMaskInfoLayer* pMaskInfoLayer = nullptr;

    for (uint32_t i = 0; i < pBlock->layerCount; ++i) {
        const ResBnvgLayerBasicInfo* pLayerInfo = pBlock->ppLayers[i];

        switch (pLayerInfo->type) {
        case BnvgLayerType_Shape:
            size += VectorGraphicsShapeLayer::CalculateRequiredDynamicMemorySize(
                        static_cast<const ResBnvgShapeLayer*>(pLayerInfo), pMaskInfoLayer) +
                    sizeof(VectorGraphicsShapeLayer);
            break;
        case BnvgLayerType_Mask:
            pMaskInfoLayer = static_cast<const ResBnvgMaskInfoLayer*>(pLayerInfo);
            break;
        case BnvgLayerType_Composition: {
            size += sizeof(VectorGraphicsLayer) +
                    VectorGraphicsLayer::CalculateRequiredDynamicMemorySize(pLayerInfo,
                                                                            pMaskInfoLayer);
            uint32_t compositionIndex = 0;

            for (const nn::util::BinaryBlockHeader* pChildBlock = pFileHeader->GetFirstBlock();
                 pChildBlock != nullptr; pChildBlock = pChildBlock->GetNextBlock()) {
                if (pChildBlock->signature._packed == BnvgCompositionDataBlock::Signature) {
                    if (compositionIndex == pLayerInfo->compositionIndex) {
                        size += CaclulateRequiredMemorySize(
                            pFileHeader, static_cast<const BnvgCompositionDataBlock*>(pChildBlock));
                        break;
                    }

                    ++compositionIndex;
                }
            }

            break;
        }
        case BnvgLayerType_Solid:
        case BnvgLayerType_Image:
        case BnvgLayerType_Null:
        case BnvgLayerType_Text:
            size += sizeof(VectorGraphicsLayer) +
                    VectorGraphicsLayer::CalculateRequiredDynamicMemorySize(pLayerInfo,
                                                                            pMaskInfoLayer);
            break;
        default:
            break;
        }
    }

    return size;
}

/** @brief Destroys the layers and releases the scene memory. */
void VectorGraphicsScene::Finalize() {
    if (m_pRootLayer != nullptr) {
        m_pRootLayer->Finalize(nullptr);
        m_pRootLayer->~VectorGraphicsLayer();
        m_pRootLayer = nullptr;
    }

    if (m_pFileHeader != nullptr) {
        m_pFileHeader = nullptr;
    }

    m_ReservedMemory.Finalize();
}

/**
 * @brief Destroys the layers and releases the scene memory.
 * @param pDevice Device the resources were created on.
 */
void VectorGraphicsScene::Finalize(nn::gfx::Device* pDevice) {
    if (m_pRootLayer != nullptr) {
        m_pRootLayer->Finalize(pDevice);
        m_pRootLayer->~VectorGraphicsLayer();
        m_pRootLayer = nullptr;
    }

    if (m_pFileHeader != nullptr) {
        m_pFileHeader = nullptr;
    }

    m_ReservedMemory.Finalize();
}

/**
 * @brief Evaluates the scene.
 * @param rDrawInfo Drawing state.
 * @param time Frame to evaluate at.
 */
void VectorGraphicsScene::Calculate(DrawInfo& rDrawInfo, float time) {
    if (m_pRootLayer == nullptr) {
        return;
    }

    MakeSceneProjection(&m_DrawInfo.m_ProjectionMatrix, m_pFileHeader->width,
                        m_pFileHeader->height, IsYAxisFlipped(rDrawInfo));
    m_pRootLayer->Calculate(rDrawInfo, m_DrawInfo, time);
}

/**
 * @brief Draws the scene into the current render target.
 * @param rDrawInfo Drawing state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsScene::Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer) {
    if (m_pRootLayer == nullptr) {
        return;
    }

    m_DrawInfo.ResetMaskTextureStack();
    m_pRootLayer->Draw(rDrawInfo, m_DrawInfo, rCommandBuffer);
}

/**
 * @brief Builds the name a layout uses to refer to a vector graphics texture.
 * @param pName Receives the name.
 * @param nameLength Size of the name buffer.
 * @param pFileName Name of the bnvg file.
 * @param pResource Texture resource.
 */
void VectorGraphicsTexture::MakeRefTextureName(char* pName, int nameLength, const char* pFileName,
                                               const ResVectorGraphicsTexture* pResource) {
    nn::util::SNPrintf(pName, nameLength, "%s_%d", pFileName, pResource->index);
}

/**
 * @brief Creates an empty texture.
 * @param pName Name of the texture.
 */
VectorGraphicsTexture::VectorGraphicsTexture(const char* pName)
    : DynamicRenderingTexture(pName), m_pDepthStencilTexture(nullptr),
      m_pDepthStencilTextureView(nullptr), m_pDepthStencilTextureSlot(nullptr),
      m_pMultiSampleRenderTarget(nullptr), m_pRasterizerState(nullptr) {
    m_Time = 0.0f;
    m_MultiSampleCount = 0;
    InitializeParams();
}

/** @brief Resets the clear colour and the update flags. */
void VectorGraphicsTexture::InitializeParams() {
    m_Color = nn::util::MakeFloat4(0.0f, 0.0f, 0.0f, 0.0f);
    m_IsTimeUpdated = 1;
    m_IsUpdateRequested = 0;
}

/**
 * @brief Creates a new instance of a texture used by another layout.
 * @param rSource Texture to copy.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the texture.
 */
VectorGraphicsTexture::VectorGraphicsTexture(const VectorGraphicsTexture& rSource,
                                             nn::gfx::Device* pDevice, const Layout* pLayout)
    : DynamicRenderingTexture(rSource.m_pName), m_pDepthStencilTexture(nullptr),
      m_pDepthStencilTextureView(nullptr), m_pDepthStencilTextureSlot(nullptr),
      m_pMultiSampleRenderTarget(nullptr), m_pRasterizerState(nullptr) {
    m_Time = 0.0f;
    m_MultiSampleCount = 0;
    InitializeParams();
    new (&m_Scene) VectorGraphicsScene(rSource.m_Scene, pDevice, pLayout);
    m_MultiSampleCount = rSource.m_MultiSampleCount;
    InitializeResources(pDevice, pLayout);
}

/**
 * @brief Creates the render targets and the depth stencil buffer.
 * @param pDevice Device the resources are created on.
 * @param pLayout Layout that owns the render targets.
 */
void VectorGraphicsTexture::InitializeResources(nn::gfx::Device* pDevice, const Layout* pLayout) {
    const int width = static_cast<int>(m_Scene.m_pFileHeader->width);
    const int height = static_cast<int>(m_Scene.m_pFileHeader->height);

    if (m_MultiSampleCount > 0) {
        m_pMultiSampleRenderTarget = Layout::NewObj<RenderTargetTextureInfo>();

        nn::gfx::TextureInfo textureInfo;
        textureInfo.SetDefault();
        textureInfo.SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
        textureInfo.SetImageFormat(nn::gfx::ImageFormat_R8_G8_B8_A8_Unorm);
        textureInfo.SetGpuAccessFlags(nn::gfx::GpuAccess_Texture |
                                      nn::gfx::GpuAccess_ColorBuffer);
        textureInfo.SetWidth(width);
        textureInfo.SetHeight(height);
        textureInfo.SetMultiSampleCount(m_MultiSampleCount);
        textureInfo.SetMipCount(1);

        if (!m_pMultiSampleRenderTarget->IsValid()) {
            m_pMultiSampleRenderTarget->Initialize(pDevice, pLayout, textureInfo,
                                                   static_cast<RenderTargetTextureLifetime>(0));
        }

        nn::gfx::RasterizerStateInfo rasterizerInfo;
        rasterizerInfo.SetDefault();
        rasterizerInfo.SetCullMode(nn::gfx::CullMode_None);
        rasterizerInfo.SetMultisampleEnabled(true);
        rasterizerInfo.SetDepthClipEnabled(false);
        rasterizerInfo.SetScissorEnabled(true);
        rasterizerInfo.EditMultisampleStateInfo().SetDefault();
        rasterizerInfo.EditMultisampleStateInfo().SetSampleCount(m_MultiSampleCount);

        m_pRasterizerState = Layout::NewObj<nn::gfx::RasterizerState>();
        ToImpl(*m_pRasterizerState).Initialize(pDevice, rasterizerInfo);
    }

    nn::gfx::TextureInfo textureInfo;
    textureInfo.SetDefault();
    textureInfo.SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
    textureInfo.SetImageFormat(nn::gfx::ImageFormat_R8_G8_B8_A8_Unorm);
    textureInfo.SetGpuAccessFlags(nn::gfx::GpuAccess_Texture | nn::gfx::GpuAccess_ColorBuffer);
    textureInfo.SetWidth(width);
    textureInfo.SetHeight(height);
    textureInfo.SetMipCount(1);

    RenderTargetTextureInfo* pRenderTarget = GetRenderTarget();

    if (!pRenderTarget->IsValid()) {
        pRenderTarget->Initialize(pDevice, pLayout, textureInfo,
                                  static_cast<RenderTargetTextureLifetime>(0));
    }

    nn::gfx::TextureInfo depthInfo;
    depthInfo.SetDefault();
    depthInfo.SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
    depthInfo.SetImageFormat(nn::gfx::ImageFormat_D24_Unorm_S8_Uint);
    depthInfo.SetGpuAccessFlags(nn::gfx::GpuAccess_DepthStencil);
    depthInfo.SetWidth(width);
    depthInfo.SetHeight(height);
    depthInfo.SetDepth(1);
    depthInfo.SetMipCount(1);
    depthInfo.SetMultiSampleCount(m_MultiSampleCount);
    Layout::g_pCreateRenderTargetTextureResourceCallback(
        &m_pDepthStencilTexture, &m_pDepthStencilTextureView, &m_pDepthStencilTextureSlot,
        nullptr, depthInfo, Layout::g_pRenderTargetTextureCallbackUserData,
        static_cast<RenderTargetTextureLifetime>(0));

    nn::gfx::DepthStencilViewInfo viewInfo;
    viewInfo.SetDefault();
    viewInfo.SetImageDimension(m_MultiSampleCount > 0 ? nn::gfx::ImageDimension_2dMultisample :
                                                        nn::gfx::ImageDimension_2d);
    viewInfo.SetTexturePtr(m_pDepthStencilTexture);
    new (&m_DepthStencilView) DepthStencilViewImpl();
    ToImpl(m_DepthStencilView).Initialize(pDevice, viewInfo);
}

VectorGraphicsTexture::~VectorGraphicsTexture() {}

/**
 * @brief Builds the scene of the texture and creates its resources.
 * @param pResult Receives the constant buffer size the scene needs.
 * @param pDevice Device the resources are created on.
 * @param pResourceAccessor Accessor the shader is loaded from.
 * @param pLayout Layout that owns the texture.
 * @param pResource Texture resource.
 * @param pFileName Name of the bnvg file.
 * @param pFileHeader The bnvg file.
 * @return True.
 */
bool VectorGraphicsTexture::Initialize(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                       ResourceAccessor* pResourceAccessor, const Layout* pLayout,
                                       const ResVectorGraphicsTexture* pResource,
                                       const char* pFileName, const BnvgFileHeader* pFileHeader) {
    char name[71];
    MakeRefTextureName(name, sizeof(name), pFileName, pResource);
    m_MultiSampleCount = ConvertMultiSampleTypeToMultiSampleCount(pResource->multiSampleType);
    m_Scene.Build(pResult, pDevice, pLayout, pResourceAccessor, pFileHeader, 0, 0);
    InitializeResources(pDevice, pLayout);
    return true;
}

/**
 * @brief Converts a multisample type to a sample count.
 * @param multiSampleType Multisample type of the resource.
 * @return Sample count, or zero without multisampling.
 */
int VectorGraphicsTexture::ConvertMultiSampleTypeToMultiSampleCount(int multiSampleType) const {
    switch (multiSampleType) {
    case 1:
        return 2;
    case 2:
        return 4;
    case 3:
        return 8;
    default:
        return 0;
    }
}

/**
 * @brief Destroys the resources and the scene.
 * @param pDevice Device the resources were created on.
 */
void VectorGraphicsTexture::Finalize(nn::gfx::Device* pDevice) {
    FinalizeResources(pDevice);
    m_Scene.Finalize(pDevice);
    DynamicRenderingTexture::Finalize(pDevice);
}

/**
 * @brief Destroys the render targets and the depth stencil buffer.
 * @param pDevice Device the resources were created on.
 */
void VectorGraphicsTexture::FinalizeResources(nn::gfx::Device* pDevice) {
    ToImpl(m_DepthStencilView).Finalize(pDevice);
    Layout::g_pDestroyRenderTargetTextureResourceCallback(
        m_pDepthStencilTexture, m_pDepthStencilTextureView, m_pDepthStencilTextureSlot, nullptr,
        Layout::g_pRenderTargetTextureCallbackUserData,
        static_cast<RenderTargetTextureLifetime>(0));
    m_pDepthStencilTexture = nullptr;
    m_pDepthStencilTextureView = nullptr;
    m_pDepthStencilTextureSlot = nullptr;

    if (m_pRasterizerState != nullptr) {
        ToImpl(*m_pRasterizerState).Finalize(pDevice);
        Layout::DeleteObj(m_pRasterizerState);
        m_pRasterizerState = nullptr;
    }

    if (m_pMultiSampleRenderTarget != nullptr) {
        m_pMultiSampleRenderTarget->Finalize(pDevice);
        Layout::DeleteObj(m_pMultiSampleRenderTarget);
        m_pMultiSampleRenderTarget = nullptr;
    }
}

/**
 * @brief Evaluates the scene when it has to be redrawn.
 * @param rDrawInfo Drawing state.
 */
void VectorGraphicsTexture::Calculate(DrawInfo& rDrawInfo) {
    if (m_IsTimeUpdated && m_IsUpdateRequested) {
        m_Scene.Calculate(rDrawInfo, m_Time);
    }
}

/**
 * @brief Draws the scene into the texture when it has to be redrawn.
 * @param pDevice Device the resources were created on.
 * @param rDrawInfo Drawing state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsTexture::Draw(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                 nn::gfx::CommandBuffer& rCommandBuffer) {
    if (!m_IsUpdateRequested) {
        return;
    }

    m_IsUpdateRequested = 0;

    if (!m_IsTimeUpdated) {
        return;
    }

    m_IsTimeUpdated = 0;

    const nn::gfx::ColorTargetView* pColorTarget = rDrawInfo.m_pColorTarget;
    const nn::gfx::ViewportStateInfo viewport = rDrawInfo.mViewport;
    const nn::gfx::ScissorStateInfo scissor = rDrawInfo.mScissor;
    const nn::gfx::DepthStencilView* pDepthTarget = rDrawInfo.m_pDepthTarget;
    const nn::gfx::RasterizerState* pRasterizerState = rDrawInfo.m_pRasterizerState;

    BeginRendering(rDrawInfo, rCommandBuffer);
    m_Scene.Draw(rDrawInfo, rCommandBuffer);

    rDrawInfo.m_pColorTarget = pColorTarget;
    rDrawInfo.m_pDepthTarget = pDepthTarget;
    rDrawInfo.mViewport = viewport;
    rDrawInfo.mScissor = scissor;
    rDrawInfo.m_pRasterizerState = pRasterizerState;
    EndRendering(rDrawInfo, rCommandBuffer);
}

/**
 * @brief Makes the texture the current render target.
 * @param rDrawInfo Drawing state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsTexture::BeginRendering(DrawInfo& rDrawInfo,
                                           nn::gfx::CommandBuffer& rCommandBuffer) {
    RenderTargetTextureInfo* pRenderTarget =
        m_pMultiSampleRenderTarget != nullptr ? m_pMultiSampleRenderTarget : GetRenderTarget();

    nn::gfx::ViewportStateInfo viewport;
    const TextureSize viewportSize = pRenderTarget->GetSize();
    viewport.SetDefault();
    viewport.SetWidth(viewportSize.width);
    viewport.SetHeight(viewportSize.height);

    nn::gfx::ScissorStateInfo scissor;
    const TextureSize scissorSize = pRenderTarget->GetSize();
    scissor.SetDefault();
    scissor.SetWidth(scissorSize.width);
    scissor.SetHeight(scissorSize.height);

    nn::gfx::ColorTargetView* pColorTarget = &pRenderTarget->mColorTarget;
    ToImpl(rCommandBuffer)
        .ClearColor(pColorTarget, m_Color.x, m_Color.y, m_Color.z, m_Color.w, nullptr);

    rDrawInfo.m_pDepthTarget = &m_DepthStencilView;
    rDrawInfo.m_pColorTarget = pColorTarget;
    rDrawInfo.mViewport = viewport;
    rDrawInfo.mScissor = scissor;

    if (m_pRasterizerState != nullptr) {
        rDrawInfo.m_pRasterizerState = m_pRasterizerState;
    }

    rDrawInfo.ResetRenderTarget(rCommandBuffer);
}

/**
 * @brief Restores the drawing state and resolves the multisampled image.
 * @param rDrawInfo Drawing state.
 * @param rCommandBuffer Command buffer to record into.
 */
void VectorGraphicsTexture::EndRendering(DrawInfo& rDrawInfo,
                                         nn::gfx::CommandBuffer& rCommandBuffer) {
    rDrawInfo.ResetCurrentShader();
    rDrawInfo.mVertexBufferDirty = true;
    rDrawInfo.ResetRenderTarget(rCommandBuffer);

    if (m_pMultiSampleRenderTarget != nullptr) {
        ToImpl(rCommandBuffer)
            .Resolve(GetRenderTarget()->mTexture, 0, 0,
                     &m_pMultiSampleRenderTarget->mColorTarget, nullptr);
    }
}

/**
 * @brief Sets the frame the scene is drawn at.
 * @param time Frame.
 */
void VectorGraphicsTexture::SetTime(float time) {
    if (m_Time != time) {
        m_Time = time;
        m_IsTimeUpdated = 1;
    }
}

}  // namespace nn::ui2d::detail
