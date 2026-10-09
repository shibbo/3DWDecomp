#include <nn/ui2d/ui2d_Alignment.h>

#include <algorithm>
#include <cfloat>

#include <nn/font/font_TagProcessorBase.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_Picture.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/ui2d/ui2d_Window.h>
#include <nn/util/util_MatrixApi.h>
#include <nn/util/util_VectorApi.h>

namespace nn::ui2d {
namespace {
using nn::util::IntrusiveListNode;
using nn::util::MatrixT4x3fType;
using nn::util::Vector3fType;

/** @brief Placement data of one child pane along the alignment axis. */
struct AlignmentPaneInfo {
    AlignmentPaneInfo() : size(0.0f), margin(0.0f), offset(0.0f), pPane(nullptr) {}

    /** @brief Extent of the pane along the axis. */
    float size;
    /** @brief Spacing inserted in front of the pane. */
    float margin;
    /** @brief Distance from the pane origin to the center of its visible area. */
    float offset;
    /** @brief The pane itself. */
    Pane* pPane;
};

/**
 * @param pPane Pane to check.
 * @return Whether the origin of the pane is at its center on both axes.
 */
inline bool IsCenterOrigin(const Pane* pPane) {
    // The low four bits hold the horizontal and the vertical base position.
    return (pPane->mOriginFlags & 0xf) == 0;
}

/**
 * @param rSize Size of the rectangle.
 * @param pPane Pane whose origin settings are used.
 * @return Position of the top-left vertex of a rectangle placed at the origin of the pane.
 */
inline nn::util::Float2 GetVertexPos(const Size& rSize, const Pane* pPane) {
    nn::util::Float2 pos;

    switch (pPane->GetBasePositionX()) {
    case 1:
        pos.x = 0.0f;
        break;
    case 2:
        pos.x = -rSize.width;
        break;
    default:
        pos.x = rSize.width * -0.5f;
        break;
    }

    switch (pPane->GetBasePositionY()) {
    case 1:
        pos.y = 0.0f;
        break;
    case 2:
        pos.y = rSize.height;
        break;
    default:
        pos.y = rSize.height * 0.5f;
        break;
    }

    return pos;
}

/**
 * @brief Inverts an affine matrix.
 * @param pOut Receives the inverse; it is zero when rMtx is singular.
 * @param rMtx Matrix to invert.
 */
inline void MatrixInverse(MatrixT4x3fType* pOut, const MatrixT4x3fType& rMtx) {
    const float32x4_t row0 = rMtx._m.val[0];
    const float32x4_t row1 = rMtx._m.val[1];
    const float32x4_t row2 = rMtx._m.val[2];
    const float m00 = vgetq_lane_f32(row0, 0);
    const float m01 = vgetq_lane_f32(row0, 1);
    const float m02 = vgetq_lane_f32(row0, 2);
    const float m03 = vgetq_lane_f32(row0, 3);
    const float m10 = vgetq_lane_f32(row1, 0);
    const float m11 = vgetq_lane_f32(row1, 1);
    const float m12 = vgetq_lane_f32(row1, 2);
    const float m13 = vgetq_lane_f32(row1, 3);
    const float m20 = vgetq_lane_f32(row2, 0);
    const float m21 = vgetq_lane_f32(row2, 1);
    const float m22 = vgetq_lane_f32(row2, 2);
    const float m23 = vgetq_lane_f32(row2, 3);

    const float det = m00 * m11 * m22 + m01 * m12 * m20 + m02 * m10 * m21 - m11 * m20 * m02 -
                      m01 * m10 * m22 - m00 * m21 * m12;
    float invDet;

    if (det == 0.0f) {
        invDet = 0.0f;
    } else {
        invDet = 1.0f / det;
    }

    const float i00 = (m11 * m22 - m12 * m21) * invDet;
    const float i01 = (m02 * m21 - m01 * m22) * invDet;
    const float i02 = (m01 * m12 - m02 * m11) * invDet;
    const float i10 = (m12 * m20 - m10 * m22) * invDet;
    const float i11 = (m00 * m22 - m02 * m20) * invDet;
    const float i12 = (m02 * m10 - m00 * m12) * invDet;
    const float i20 = (m10 * m21 - m11 * m20) * invDet;
    const float i21 = (m01 * m20 - m00 * m21) * invDet;
    const float i22 = (m00 * m11 - m01 * m10) * invDet;
    const float i03 = -(m03 * i00) - i01 * m13 - i02 * m23;
    const float i13 = -(m03 * i10) - i11 * m13 - i12 * m23;
    const float i23 = -(m03 * i20) - i21 * m13 - i22 * m23;

    pOut->_m.val[0] = float32x4_t{i00, i01, i02, i03};
    pOut->_m.val[1] = float32x4_t{i10, i11, i12, i13};
    pOut->_m.val[2] = float32x4_t{i20, i21, i22, i23};
}

/**
 * @param row Row of a matrix.
 * @return A vector holding only the translation (fourth) element of row.
 */
inline float32x4_t GetTranslationVector(float32x4_t row) {
    // Broadcast first, then keep a single lane.
    const float32x4_t translation = vdupq_n_f32(vgetq_lane_f32(row, 3));
    return vsetq_lane_f32(vgetq_lane_f32(translation, 1), vdupq_n_f32(0.0f), 3);
}

/**
 * @brief Multiplies a row of a 4x3 matrix by another 4x3 matrix.
 * @param row Row of the left matrix.
 * @param right0 First row of the right matrix.
 * @param right1 Second row of the right matrix.
 * @param right2 Third row of the right matrix.
 * @return Row of the product.
 */
inline float32x4_t MultiplyRow(float32x4_t row, float32x4_t right0, float32x4_t right1,
                               float32x4_t right2) {
    float32x4_t result = vmulq_laneq_f32(right0, row, 0);
    result = vfmaq_laneq_f32(result, right1, row, 1);
    result = vfmaq_laneq_f32(result, right2, row, 2);
    return vaddq_f32(GetTranslationVector(row), result);
}

/**
 * @brief Multiplies two 4x3 matrices.
 * @param pOut Receives rLeft * rRight.
 * @param rLeft Left matrix.
 * @param rRight Right matrix.
 */
inline void MatrixMultiply(MatrixT4x3fType* pOut, const MatrixT4x3fType& rLeft,
                           const MatrixT4x3fType& rRight) {
    const float32x4_t left0 = rLeft._m.val[0];
    const float32x4_t left1 = rLeft._m.val[1];
    const float32x4_t left2 = rLeft._m.val[2];
    const float32x4_t right0 = rRight._m.val[0];
    const float32x4_t right1 = rRight._m.val[1];
    const float32x4_t right2 = rRight._m.val[2];
    pOut->_m.val[0] = MultiplyRow(left0, right0, right1, right2);
    pOut->_m.val[1] = MultiplyRow(left1, right0, right1, right2);
    pOut->_m.val[2] = MultiplyRow(left2, right0, right1, right2);
}

/**
 * @brief Builds a translation matrix.
 * @param pOut Receives the matrix.
 * @param rTranslation Translation of the matrix.
 */
inline void MatrixTranslation(MatrixT4x3fType* pOut, const Vector3fType& rTranslation) {
    const float32x4_t axisX = {1.0f, 0.0f, 0.0f, 0.0f};
    const float32x4_t axisY = {0.0f, 1.0f, 0.0f, 0.0f};
    const float32x4_t axisZ = {0.0f, 0.0f, 1.0f, 0.0f};
    pOut->_m.val[0] = vsetq_lane_f32(nn::util::VectorGetX(rTranslation), axisX, 3);
    pOut->_m.val[1] = vsetq_lane_f32(nn::util::VectorGetY(rTranslation), axisY, 3);
    pOut->_m.val[2] = vsetq_lane_f32(nn::util::VectorGetZ(rTranslation), axisZ, 3);
}

/**
 * @brief Removes the translation of an affine matrix.
 * @param pMtx Matrix to modify.
 */
inline void MatrixClearTranslation(MatrixT4x3fType* pMtx) {
    pMtx->_m.val[0] = vsetq_lane_f32(0.0f, pMtx->_m.val[0], 3);
    pMtx->_m.val[1] = vsetq_lane_f32(0.0f, pMtx->_m.val[1], 3);
    pMtx->_m.val[2] = vsetq_lane_f32(0.0f, pMtx->_m.val[2], 3);
}

/**
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param z Z coordinate.
 * @return A vector made of the coordinates.
 */
inline Vector3fType MakeVector3(float x, float y, float z) {
    Vector3fType vector;
    vector._v = float32x4_t{x, y, z, 0.0f};
    return vector;
}

/**
 * @brief Transforms a point by an affine matrix.
 * @param rPoint Point to transform.
 * @param rMtx Matrix to apply.
 * @return The transformed point.
 */
inline Vector3fType TransformPoint(const Vector3fType& rPoint, const MatrixT4x3fType& rMtx) {
    float32x4x4_t matrix;
    matrix.val[0] = rMtx._m.val[0];
    matrix.val[1] = rMtx._m.val[1];
    matrix.val[2] = rMtx._m.val[2];
    matrix.val[3] = vdupq_n_f32(0.0f);
    const float32x4x4_t transposed = nn::util::detail::Matrix4x4fTranspose(matrix);

    float32x4_t value = vmulq_laneq_f32(transposed.val[0], rPoint._v, 0);
    value = vfmaq_laneq_f32(value, transposed.val[1], rPoint._v, 1);
    value = vfmaq_laneq_f32(value, transposed.val[2], rPoint._v, 2);

    Vector3fType result;
    result._v = vaddq_f32(transposed.val[3], value);
    return result;
}

/**
 * @brief Transforms a point by an affine matrix.
 * @param x X coordinate of the point.
 * @param y Y coordinate of the point.
 * @param z Z coordinate of the point.
 * @param rMtx Matrix to apply.
 * @return The transformed point.
 */
inline Vector3fType TransformPoint(float x, float y, float z, const MatrixT4x3fType& rMtx) {
    float32x4x4_t matrix;
    matrix.val[0] = rMtx._m.val[0];
    matrix.val[1] = rMtx._m.val[1];
    matrix.val[2] = rMtx._m.val[2];
    matrix.val[3] = vdupq_n_f32(0.0f);
    const float32x4x4_t transposed = nn::util::detail::Matrix4x4fTranspose(matrix);

    float32x4_t value = vmulq_n_f32(transposed.val[0], x);
    value = vfmaq_n_f32(value, transposed.val[1], y);
    value = vfmaq_n_f32(value, transposed.val[2], z);

    Vector3fType result;
    result._v = vaddq_f32(transposed.val[3], value);
    return result;
}

/** @brief Bounding rectangle that grows to contain points. */
struct BoundingRect {
    BoundingRect() : left(FLT_MAX), top(-FLT_MAX), right(-FLT_MAX), bottom(FLT_MAX) {}

    /**
     * @brief Grows the rectangle so that it contains a point.
     * @param rPoint Point to include.
     */
    void Expand(const Vector3fType& rPoint) {
        const float x = nn::util::VectorGetX(rPoint);
        const float y = nn::util::VectorGetY(rPoint);
        right = x < right ? right : x;
        top = y < top ? top : y;
        left = left < x ? left : x;
        bottom = bottom < y ? bottom : y;
    }

    /**
     * @brief Copies the rectangle.
     * @param pRect Receives the rectangle.
     */
    void Get(nn::font::Rectangle* pRect) const {
        pRect->left = left;
        pRect->top = top;
        pRect->right = right;
        pRect->bottom = bottom;
    }

    float left;
    float top;
    float right;
    float bottom;
};

/**
 * @brief Calculates the matrix of a pane relative to its parent from the global matrices.
 * @param pPane Pane to calculate the matrix of.
 * @return The local matrix of the pane, or its global matrix when it has no parent.
 */
MatrixT4x3fType CalculateLocalMatrix(const Pane* pPane) {
    const Pane* pParent = pPane->GetParent();

    if (pParent == nullptr) {
        return pPane->GetGlobalMatrix();
    }

    MatrixT4x3fType invParentMtx;
    MatrixInverse(&invParentMtx, pParent->GetGlobalMatrix());

    MatrixT4x3fType mtx;
    MatrixMultiply(&mtx, invParentMtx, pPane->GetGlobalMatrix());
    return mtx;
}

/**
 * @brief Calculates the bounding rectangle of a rectangle drawn by a pane.
 * @param pRect Receives the bounding rectangle in the space of the parent of the pane.
 * @param pPane Pane that draws the rectangle.
 * @param pParentMtx Additional matrix applied after the local matrix of the pane, or nullptr.
 * @param size Size of the rectangle.
 * @param basePos Top-left vertex of the rectangle relative to the pane origin.
 */
void CalculatePaneRect(nn::font::Rectangle* pRect, const Pane* pPane,
                       const MatrixT4x3fType* pParentMtx, Size size, nn::util::Float2 basePos) {
    const float halfWidth = size.width * 0.5f;
    const float halfHeight = size.height * 0.5f;

    MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);

    if (pParentMtx != nullptr) {
        MatrixMultiply(&mtx, *pParentMtx, mtx);
    }

    const Vector3fType center = MakeVector3(halfWidth + basePos.x, basePos.y - halfHeight, 0.0f);
    MatrixT4x3fType translation;
    MatrixTranslation(&translation, center);
    MatrixMultiply(&mtx, mtx, translation);

    const Vector3fType topLeft = TransformPoint(MakeVector3(-halfWidth, halfHeight, 0.0f), mtx);
    const Vector3fType topRight = TransformPoint(MakeVector3(halfWidth, halfHeight, 0.0f), mtx);
    const Vector3fType bottomRight =
        TransformPoint(MakeVector3(halfWidth, -halfHeight, 0.0f), mtx);
    const Vector3fType bottomLeft =
        TransformPoint(MakeVector3(-halfWidth, -halfHeight, 0.0f), mtx);

    BoundingRect bound;
    bound.Expand(topLeft);
    bound.Expand(topRight);
    bound.Expand(bottomRight);
    bound.Expand(bottomLeft);

    bound.Get(pRect);
}

/**
 * @brief Calculates the bounding rectangle of the text of a text box.
 * @param pRect Receives the bounding rectangle in the space of the parent of the pane.
 * @param pPane Text box pane.
 * @param size Size of the pane.
 * @param basePos Top-left vertex of the pane relative to its origin.
 * @param textSize Size of the drawn text.
 */
void CalculateTextRect(nn::font::Rectangle* pRect, Pane* pPane, Size size,
                       nn::util::Float2 basePos, Size textSize) {
    MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);

    const float halfWidth = size.width * 0.5f;
    const float halfHeight = size.height * 0.5f;
    const float halfTextWidth = textSize.width * 0.5f;
    const float halfTextHeight = textSize.height * 0.5f;

    const Vector3fType center = MakeVector3(halfWidth + basePos.x, basePos.y - halfHeight, 0.0f);
    MatrixT4x3fType translation;
    MatrixTranslation(&translation, center);
    MatrixMultiply(&mtx, mtx, translation);

    Vector3fType corners[4];
    corners[0] = MakeVector3(-halfTextWidth, halfTextHeight, 0.0f);
    corners[1] = MakeVector3(halfTextWidth, halfTextHeight, 0.0f);
    corners[2] = MakeVector3(halfTextWidth, -halfTextHeight, 0.0f);
    corners[3] = MakeVector3(-halfTextWidth, -halfTextHeight, 0.0f);

    const TextBox* pTextBox = DynamicCast<const TextBox*>(pPane);

    if (pTextBox != nullptr) {
        switch (pTextBox->GetTextPositionH()) {
        case 2:
            for (int i = 0; i < 4; i++) {
                const float offsetX = halfWidth - halfTextWidth;
                corners[i] = MakeVector3(offsetX + nn::util::VectorGetX(corners[i]),
                                         nn::util::VectorGetY(corners[i]), 0.0f);
            }

            break;
        case 1:
            for (int i = 0; i < 4; i++) {
                const float offsetX = halfWidth - halfTextWidth;
                corners[i] = MakeVector3(nn::util::VectorGetX(corners[i]) - offsetX,
                                         nn::util::VectorGetY(corners[i]), 0.0f);
            }

            break;
        }

        if (!pTextBox->IsTextFlag12()) {
            switch (pTextBox->GetTextPositionV()) {
            case 2:
                for (int i = 0; i < 4; i++) {
                    const float offsetY = halfHeight - halfTextHeight;
                    corners[i] = MakeVector3(nn::util::VectorGetX(corners[i]),
                                             nn::util::VectorGetY(corners[i]) - offsetY, 0.0f);
                }

                break;
            case 1:
                for (int i = 0; i < 4; i++) {
                    const float offsetY = halfHeight - halfTextHeight;
                    corners[i] = MakeVector3(nn::util::VectorGetX(corners[i]),
                                             offsetY + nn::util::VectorGetY(corners[i]), 0.0f);
                }

                break;
            }
        }
    }

    Vector3fType transformedCorners[4];

    for (int i = 0; i < 4; i++) {
        transformedCorners[i] = TransformPoint(corners[i], mtx);
    }

    BoundingRect bound;

    for (int i = 0; i < 4; i++) {
        bound.Expand(transformedCorners[i]);
    }

    bound.Get(pRect);
}

/**
 * @param pTextBox Text box to measure.
 * @return Size of the text drawn by the text box.
 */
inline Size GetTextDrawSize(const TextBox* pTextBox) {
    Size size;
    size.width = pTextBox->GetTextDrawRect().GetWidth();
    const nn::font::Rectangle rect = pTextBox->GetTextDrawRect();
    size.height = rect.top - rect.bottom;
    return size;
}

/**
 * @brief Extends a horizontal range by a pane and its descendants.
 * @param pPane Pane to measure.
 * @param pLeft Left edge of the range, updated in place.
 * @param pRight Right edge of the range, updated in place.
 * @param pMtx Matrix from the parent of the pane to the space of the range.
 */
void CalculateHorizontalBound(Pane* pPane, float* pLeft, float* pRight,
                              const MatrixT4x3fType* pMtx) {
    if (!pPane->IsVisible()) {
        return;
    }

    // Ignored panes do not count themselves, but their children still do.
    if (!pPane->IsAlignmentIgnore()) {
        float left = FLT_MAX;
        float right = -FLT_MAX;

        if (const TextBox* pTextBox = DynamicCast<const TextBox*>(pPane)) {
            nn::font::Rectangle paneRect;
            CalculatePaneRect(&paneRect, pPane, pMtx, pPane->GetSize(),
                              GetVertexPos(pPane->GetSize(), pPane));

            const Size textSize = GetTextDrawSize(pTextBox);
            nn::font::Rectangle textRect;
            CalculatePaneRect(&textRect, pPane, pMtx, textSize, GetVertexPos(textSize, pPane));

            const float textWidth = textRect.GetWidth();

            switch (pTextBox->GetTextPositionH()) {
            case 2:
                right = paneRect.right;
                left = right - textWidth;
                break;
            case 1:
                left = paneRect.left;
                right = textWidth + left;
                break;
            default:
                left = textRect.left;
                right = textRect.right;
                break;
            }
        } else {
            const Picture* pPicture = DynamicCast<const Picture*>(pPane);
            const Window* pWindow = DynamicCast<const Window*>(pPane);

            if (pPicture != nullptr || pWindow != nullptr) {
                nn::font::Rectangle rect;
                CalculatePaneRect(&rect, pPane, pMtx, pPane->GetSize(),
                                  GetVertexPos(pPane->GetSize(), pPane));
                left = rect.left;
                right = rect.right;
            }
        }

        *pLeft = std::min(*pLeft, left);
        *pRight = std::max(*pRight, right);
    }

    for (IntrusiveListNode* pNode = pPane->m_Children.GetNext(); pNode != &pPane->m_Children;
         pNode = pNode->GetNext()) {
        CalculateHorizontalBound(Pane::FromLink(pNode), pLeft, pRight, pMtx);
    }
}

/**
 * @brief Extends a vertical range by a pane and its descendants.
 * @param pPane Pane to measure.
 * @param pTop Top edge of the range, updated in place.
 * @param pBottom Bottom edge of the range, updated in place.
 * @param pMtx Matrix from the parent of the pane to the space of the range.
 */
void CalculateVerticalBound(Pane* pPane, float* pTop, float* pBottom,
                            const MatrixT4x3fType* pMtx) {
    if (!pPane->IsVisible()) {
        return;
    }

    // Ignored panes do not count themselves, but their children still do.
    if (!pPane->IsAlignmentIgnore()) {
        float bottom = FLT_MAX;
        float top = -FLT_MAX;

        if (const TextBox* pTextBox = DynamicCast<const TextBox*>(pPane)) {
            nn::font::Rectangle paneRect;
            CalculatePaneRect(&paneRect, pPane, pMtx, pPane->GetSize(),
                              GetVertexPos(pPane->GetSize(), pPane));

            const Size textSize = GetTextDrawSize(pTextBox);
            nn::font::Rectangle textRect;
            CalculatePaneRect(&textRect, pPane, pMtx, textSize, GetVertexPos(textSize, pPane));

            const float textHeight = textRect.top - textRect.bottom;

            if (pTextBox->IsTextFlag12()) {
                bottom = textRect.bottom;
                top = textRect.top;
            } else {
                switch (pTextBox->GetTextPositionV()) {
                case 1:
                    top = paneRect.top;
                    bottom = top - textHeight;
                    break;
                case 2:
                    bottom = paneRect.bottom;
                    top = textHeight + bottom;
                    break;
                default:
                    bottom = textRect.bottom;
                    top = textRect.top;
                    break;
                }
            }
        } else {
            const Picture* pPicture = DynamicCast<const Picture*>(pPane);
            const Window* pWindow = DynamicCast<const Window*>(pPane);

            if (pPicture != nullptr || pWindow != nullptr) {
                nn::font::Rectangle rect;
                CalculatePaneRect(&rect, pPane, pMtx, pPane->GetSize(),
                                  GetVertexPos(pPane->GetSize(), pPane));
                top = rect.top;
                bottom = rect.bottom;
            }
        }

        *pTop = std::max(*pTop, top);
        *pBottom = std::min(*pBottom, bottom);
    }

    for (IntrusiveListNode* pNode = pPane->m_Children.GetNext(); pNode != &pPane->m_Children;
         pNode = pNode->GetNext()) {
        CalculateVerticalBound(Pane::FromLink(pNode), pTop, pBottom, pMtx);
    }
}

/**
 * @brief Sets the size and the offset of a pane from the horizontal bounds of its children.
 * @param pInfo Receives the size and the offset.
 * @param pPane Pane whose children are measured.
 * @return Whether the pane is visible and its children have a size.
 */
inline bool CalculateHorizontalChildrenSize(AlignmentPaneInfo* pInfo, Pane* pPane) {
    const bool isVisible = pPane->IsVisible();
    float left = FLT_MAX;
    float right = -FLT_MAX;
    const MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);

    for (IntrusiveListNode* pNode = pPane->m_Children.GetNext(); pNode != &pPane->m_Children;
         pNode = pNode->GetNext()) {
        CalculateHorizontalBound(Pane::FromLink(pNode), &left, &right, &mtx);
    }

    const bool isEmpty = left > right;
    float size = 0.0f;

    if (!isEmpty) {
        const float translateX = vgetq_lane_f32(mtx._m.val[0], 3);
        right -= translateX;
        left -= translateX;
        size = right - left;
        pInfo->offset = (right + left) * 0.5f;
    }

    pInfo->size = size;
    return isVisible && size != 0.0f;
}

/**
 * @brief Sets the size and the offset of a pane from the vertical bounds of its children.
 * @param pInfo Receives the size and the offset.
 * @param pPane Pane whose children are measured.
 * @return Whether the pane is visible and its children have a size.
 */
inline bool CalculateVerticalChildrenSize(AlignmentPaneInfo* pInfo, Pane* pPane) {
    const bool isVisible = pPane->IsVisible();
    float top = -FLT_MAX;
    float bottom = FLT_MAX;
    const MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);

    for (IntrusiveListNode* pNode = pPane->m_Children.GetNext(); pNode != &pPane->m_Children;
         pNode = pNode->GetNext()) {
        CalculateVerticalBound(Pane::FromLink(pNode), &top, &bottom, &mtx);
    }

    const bool isEmpty = top < bottom;
    float size = 0.0f;

    if (!isEmpty) {
        const float translateY = vgetq_lane_f32(mtx._m.val[1], 3);
        top -= translateY;
        bottom -= translateY;
        size = top - bottom;
        pInfo->offset = (top + bottom) * 0.5f;
    }

    pInfo->size = size;
    return isVisible && size != 0.0f;
}

/**
 * @brief Sets the size and the offset of a pane from its horizontal bounds.
 * @param pInfo Receives the size and the offset.
 * @param pPane Pane to measure.
 */
inline void CalculateHorizontalPaneSize(AlignmentPaneInfo* pInfo, Pane* pPane) {
    nn::font::Rectangle rect;
    CalculatePaneRect(&rect, pPane, nullptr, pPane->GetSize(),
                      GetVertexPos(pPane->GetSize(), pPane));
    pInfo->size = rect.GetWidth();

    if (!IsCenterOrigin(pPane)) {
        const nn::util::Float2 basePos = GetVertexPos(pPane->GetSize(), pPane);
        const float centerX = pPane->GetSizeX() * 0.5f + basePos.x;
        const float centerY = basePos.y - pPane->GetSizeY() * 0.5f;
        MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);
        MatrixClearTranslation(&mtx);
        const Vector3fType center = TransformPoint(centerX, centerY, 0.0f, mtx);
        pInfo->offset += nn::util::VectorGetX(center);
    }
}

/**
 * @brief Measures a child of a horizontal alignment.
 * @param pPane Child pane.
 * @param pInfo Receives the placement data of the pane.
 * @return Whether the pane takes part in the alignment.
 */
bool GetHorizontalAlignmentInfo(Pane* pPane, AlignmentPaneInfo* pInfo) {
    if (pPane->IsAlignmentIgnore()) {
        return false;
    }

    const Alignment* pAlignment = DynamicCast<const Alignment*>(pPane->GetParent());
    pInfo->pPane = pPane;
    pInfo->size = pPane->GetSizeX();
    pInfo->margin = 0.0f;
    pInfo->offset = 0.0f;
    // Whether the pane is visible and takes up space.
    bool hasContent = pPane->IsVisible();

    if (Parts* pParts = DynamicCast<Parts*>(pPane)) {
        hasContent = CalculateHorizontalChildrenSize(pInfo, pParts);
    } else if (pPane->IsAlignmentNullPane() && pPane->m_Children.IsLinked()) {
        hasContent = CalculateHorizontalChildrenSize(pInfo, pPane);
    } else if (const TextBox* pTextBox = DynamicCast<const TextBox*>(pPane)) {
        const Size textSize = GetTextDrawSize(pTextBox);
        nn::font::Rectangle rect;
        CalculateTextRect(&rect, pPane, pPane->GetSize(), GetVertexPos(pPane->GetSize(), pPane),
                          textSize);
        pInfo->size = rect.GetWidth();
        pInfo->offset = rect.GetWidth() * 0.5f + (rect.left - pPane->GetPositionX());
    } else {
        CalculateHorizontalPaneSize(pInfo, pPane);
    }

    if (!hasContent) {
        pInfo->margin = 0.0f;
        return false;
    }

    if (pPane->IsAlignmentMarginEnabled()) {
        pInfo->margin = pInfo->pPane->GetAlignmentMargin();
        return true;
    }

    pInfo->margin = pAlignment->GetDefaultMargin();
    return true;
}

/**
 * @brief Sets the size and the offset of a pane from its vertical bounds.
 * @param pInfo Receives the size and the offset.
 * @param pPane Pane to measure.
 */
inline void CalculateVerticalPaneSize(AlignmentPaneInfo* pInfo, Pane* pPane) {
    nn::font::Rectangle rect;
    CalculatePaneRect(&rect, pPane, nullptr, pPane->GetSize(),
                      GetVertexPos(pPane->GetSize(), pPane));
    pInfo->size = rect.top - rect.bottom;

    if (!IsCenterOrigin(pPane)) {
        const nn::util::Float2 basePos = GetVertexPos(pPane->GetSize(), pPane);
        const float centerX = pPane->GetSizeX() * 0.5f + basePos.x;
        const float centerY = basePos.y - pPane->GetSizeY() * 0.5f;
        MatrixT4x3fType mtx = CalculateLocalMatrix(pPane);
        MatrixClearTranslation(&mtx);
        const Vector3fType center = TransformPoint(centerX, centerY, 0.0f, mtx);
        pInfo->offset += nn::util::VectorGetY(center);
    }
}

/**
 * @brief Measures a child of a vertical alignment.
 * @param pPane Child pane.
 * @param pInfo Receives the placement data of the pane.
 * @return Whether the pane takes part in the alignment.
 */
bool GetVerticalAlignmentInfo(Pane* pPane, AlignmentPaneInfo* pInfo) {
    if (pPane->IsAlignmentIgnore()) {
        return false;
    }

    const Alignment* pAlignment = DynamicCast<const Alignment*>(pPane->GetParent());
    pInfo->pPane = pPane;
    pInfo->size = pPane->GetSizeY();
    pInfo->margin = 0.0f;
    pInfo->offset = 0.0f;
    // Whether the pane is visible and takes up space.
    bool hasContent = pPane->IsVisible();

    if (Parts* pParts = DynamicCast<Parts*>(pPane)) {
        hasContent = CalculateVerticalChildrenSize(pInfo, pParts);
    } else if (pPane->IsAlignmentNullPane() && pPane->m_Children.IsLinked()) {
        hasContent = CalculateVerticalChildrenSize(pInfo, pPane);
    } else if (const TextBox* pTextBox = DynamicCast<const TextBox*>(pPane)) {
        const Size textSize = GetTextDrawSize(pTextBox);
        nn::font::Rectangle rect;
        CalculateTextRect(&rect, pPane, pPane->GetSize(), GetVertexPos(pPane->GetSize(), pPane),
                          textSize);
        const float height = rect.top - rect.bottom;
        pInfo->size = height;
        pInfo->offset = -(height * 0.5f - (rect.top - pPane->GetPositionY()));
    } else {
        CalculateVerticalPaneSize(pInfo, pPane);
    }

    if (!hasContent) {
        pInfo->margin = 0.0f;
        return false;
    }

    if (pPane->IsAlignmentMarginEnabled()) {
        pInfo->margin = pInfo->pPane->GetAlignmentMargin();
        return true;
    }

    pInfo->margin = pAlignment->GetDefaultMargin();
    return true;
}

/**
 * @brief Widens the pane at the left end of a horizontal alignment, keeping its right edge.
 * @param pPane Pane to widen.
 * @param width New width.
 */
inline void ExtendLeftEdgePane(Pane* pPane, float width) {
    const float position = pPane->GetPositionX();
    pPane->SetSizeX(width);

    switch (pPane->GetBasePositionX()) {
    case 0:
        pPane->SetPositionX(position - width * 0.5f);
        break;
    case 1:
        pPane->SetPositionX(position - width);
        break;
    }
}

/**
 * @brief Widens the pane at the right end of a horizontal alignment, keeping its left edge.
 * @param pPane Pane to widen.
 * @param width New width.
 */
inline void ExtendRightEdgePane(Pane* pPane, float width) {
    const float position = pPane->GetPositionX();
    pPane->SetSizeX(width);

    switch (pPane->GetBasePositionX()) {
    case 0:
        pPane->SetPositionX(width * 0.5f + position);
        break;
    case 2:
        pPane->SetPositionX(width + position);
        break;
    }
}

/**
 * @brief Stretches the pane at the top end of a vertical alignment, keeping its bottom edge.
 * @param pPane Pane to stretch.
 * @param height New height.
 */
inline void ExtendTopEdgePane(Pane* pPane, float height) {
    const float position = pPane->GetPositionY();
    pPane->SetSizeY(height);

    switch (pPane->GetBasePositionY()) {
    case 0:
        pPane->SetPositionY(height * 0.5f + position);
        break;
    case 1:
        pPane->SetPositionY(height + position);
        break;
    }
}

/**
 * @brief Stretches the pane at the bottom end of a vertical alignment, keeping its top edge.
 * @param pPane Pane to stretch.
 * @param height New height.
 */
inline void ExtendBottomEdgePane(Pane* pPane, float height) {
    const float position = pPane->GetPositionY();
    pPane->SetSizeY(height);

    switch (pPane->GetBasePositionY()) {
    case 0:
        pPane->SetPositionY(position - height * 0.5f);
        break;
    case 2:
        pPane->SetPositionY(position - height);
        break;
    }
}

/**
 * @param rChildren Child list of an alignment.
 * @param pNode Link of one of the children.
 * @param alignment Alignment type of the alignment.
 * @return Whether the child sits at an edge that is extended to fill the alignment.
 */
inline bool IsExtendedEdgePane(const IntrusiveListNode& rChildren, const IntrusiveListNode* pNode,
                               u32 alignment) {
    switch (alignment) {
    case Alignment::AlignmentType_Forward:
        return rChildren.GetPrev() == pNode;
    case Alignment::AlignmentType_Center:
        return rChildren.GetNext() == pNode || rChildren.GetPrev() == pNode;
    case Alignment::AlignmentType_Reverse:
        return rChildren.GetNext() == pNode;
    default:
        return false;
    }
}
}  // namespace

/** @brief Constructs an empty horizontal alignment. */
Alignment::Alignment()
    : mAlignment(0), mDefaultMargin(0), mIsExtendEdgeEnabled(false), mAlignmentFlags(0) {}

/** @brief Destroys the alignment; the children are owned and destroyed by the layout. */
Alignment::~Alignment() {
    // The original body holds a statement that is compiled out in release builds.
    ;
}

/**
 * @brief Constructs an alignment from a layout resource.
 * @param pResAlignment Alignment block of the resource.
 * @param rBuildArgSet Build arguments of the layout.
 */
Alignment::Alignment(const ResAlignment* pResAlignment, const BuildArgSet& rBuildArgSet)
    : Pane(reinterpret_cast<const ResPane*>(pResAlignment), rBuildArgSet) {
    mAlignment = pResAlignment->alignment;
    mDefaultMargin = pResAlignment->defaultMargin;
    mIsExtendEdgeEnabled = pResAlignment->isExtendEdgeEnabled;
    mAlignmentFlags = 0;

    if (pResAlignment->alignmentFlags & 1) {
        mAlignmentFlags |= AlignmentFlag_Vertical;
    }

    RequestAlignment();
}

/** @brief Requests the children to be aligned again in the next Calculate. */
void Alignment::RequestAlignment() {
    mAlignmentFlags |= AlignmentFlag_AlignmentRequested;
}

/**
 * @brief Copies an alignment without its children.
 * @param rOther Alignment to copy.
 */
Alignment::Alignment(const Alignment& rOther) : Pane(rOther) {
    mAlignment = rOther.mAlignment;
    mDefaultMargin = rOther.mDefaultMargin;
    mIsExtendEdgeEnabled = rOther.mIsExtendEdgeEnabled;
    mAlignmentFlags = rOther.mAlignmentFlags;
}

/** @return Spacing used between children that have no margin of their own. */
float Alignment::GetDefaultMargin() const {
    return mDefaultMargin;
}

/** @return Alignment type of a horizontal alignment. */
u32 Alignment::GetHorizontalAlignment() const {
    return mAlignment;
}

/** @return Alignment type of a vertical alignment. */
u32 Alignment::GetVerticalAlignment() const {
    return mAlignment;
}

/**
 * @brief Aligns the children when requested, then calculates the pane.
 * @param rDrawInfo Draw information of the layout.
 * @param rContext Calculation context.
 * @param isDirtyParentMtx Whether the matrix of the parent changed.
 */
void Alignment::Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext,
                          bool isDirtyParentMtx) {
    if ((mAlignmentFlags & AlignmentFlag_AlignmentRequested) != 0) {
        mAlignmentFlags &= ~AlignmentFlag_AlignmentRequested;
        CalculateGlobalMatrix(rContext, true);
        MakeAlignment();
    }

    Pane::Calculate(rDrawInfo, rContext, isDirtyParentMtx);
    LoadMtx(rDrawInfo);
}

/** @brief Aligns the children along the axis of the alignment. */
void Alignment::MakeAlignment() {
    if (IsVerticalAlignment()) {
        MakeVerticalAlignment();
    } else {
        MakeHorizontalAlignment();
    }
}

/** @brief Packs the children from the left edge or around the center. */
void Alignment::MakeForwardHorizontalAlignment() {
    float position = 0.0f;

    if (mAlignment != AlignmentType_Center) {
        position = GetSizeX() * -0.5f;
    }

    IntrusiveListNode* pBegin = m_Children.GetNext();
    float totalSize = 0.0f;
    Pane* pFirstPane = nullptr;
    Pane* pLastPane = nullptr;
    bool hasPrevious = false;

    for (IntrusiveListNode* pNode = pBegin; &m_Children != pNode; pNode = pNode->GetNext()) {
        AlignmentPaneInfo info;

        if (!GetHorizontalAlignmentInfo(FromLink(pNode), &info)) {
            continue;
        }

        if (mIsExtendEdgeEnabled && m_Children.IsLinked() &&
            IsExtendedEdgePane(m_Children, pNode, mAlignment)) {
            info.offset = 0.0f;
            info.size = 0.0f;
        }

        const float halfSize = info.size * 0.5f;

        if (hasPrevious) {
            position += info.margin;
        } else {
            totalSize -= info.margin;
        }

        pLastPane = info.pPane;
        totalSize += info.margin + info.size;
        FromLink(pNode)->SetPositionX(position + halfSize - info.offset);
        position += info.size;

        if (!hasPrevious) {
            pFirstPane = info.pPane;
        }

        hasPrevious = true;
    }

    if (mAlignment == AlignmentType_Center) {
        const float halfSize = totalSize * 0.5f;

        for (IntrusiveListNode* pNode = pBegin; &m_Children != pNode; pNode = pNode->GetNext()) {
            Pane* pChild = FromLink(pNode);

            if (pChild->IsVisible() && !pChild->IsAlignmentIgnore()) {
                pChild->SetPositionX(pChild->GetPositionX() - halfSize);
            }
        }
    }

    if (mIsExtendEdgeEnabled) {
        switch (mAlignment) {
        case AlignmentType_Forward:
            ExtendRightEdgePane(pLastPane, GetSizeX() - totalSize);
            break;
        case AlignmentType_Center: {
            const float edgeSize = (GetSizeX() - totalSize) * 0.5f;
            ExtendRightEdgePane(pLastPane, edgeSize);
            ExtendLeftEdgePane(pFirstPane, edgeSize);
            break;
        }
        }
    }
}

/** @brief Packs the children from the right edge. */
void Alignment::MakeReverseHorizontalAlignment() {
    IntrusiveListNode* pFirst = m_Children.GetNext();
    float position = GetSizeX() * 0.5f;
    float totalSize = 0.0f;
    Pane* pLastPane = nullptr;
    bool hasPrevious = false;

    // Walks the children backwards; pNext is the link after the visited child.
    for (IntrusiveListNode* pNext = &m_Children; pFirst != pNext; pNext = pNext->GetPrev()) {
        AlignmentPaneInfo info;

        if (!GetHorizontalAlignmentInfo(FromLink(pNext->GetPrev()), &info)) {
            continue;
        }

        if (mIsExtendEdgeEnabled && m_Children.IsLinked() &&
            IsExtendedEdgePane(m_Children, pNext->GetPrev(), mAlignment)) {
            info.offset = 0.0f;
            info.size = 0.0f;
        }

        if (hasPrevious) {
            position -= info.margin;
        } else {
            totalSize -= info.margin;
        }

        FromLink(pNext->GetPrev())->SetPositionX(position + info.size * -0.5f - info.offset);
        totalSize += info.margin + info.size;
        position -= info.size;
        pLastPane = info.pPane;
        hasPrevious = true;
    }

    if (mIsExtendEdgeEnabled) {
        ExtendLeftEdgePane(pLastPane, GetSizeX() - totalSize);
    }
}

/** @brief Packs the children from the top edge or around the center. */
void Alignment::MakeForwardVerticalAlignment() {
    float position = 0.0f;

    if (mAlignment != AlignmentType_Center) {
        position = GetSizeY() * 0.5f;
    }

    IntrusiveListNode* pBegin = m_Children.GetNext();
    float totalSize = 0.0f;
    Pane* pFirstPane = nullptr;
    Pane* pLastPane = nullptr;
    bool hasPrevious = false;

    for (IntrusiveListNode* pNode = pBegin; &m_Children != pNode; pNode = pNode->GetNext()) {
        AlignmentPaneInfo info;

        if (!GetVerticalAlignmentInfo(FromLink(pNode), &info)) {
            continue;
        }

        if (mIsExtendEdgeEnabled && m_Children.IsLinked() &&
            IsExtendedEdgePane(m_Children, pNode, mAlignment)) {
            info.offset = 0.0f;
            info.size = 0.0f;
        }

        const float halfSize = info.size * -0.5f;

        if (hasPrevious) {
            position -= info.margin;
        } else {
            totalSize -= info.margin;
        }

        pLastPane = info.pPane;
        totalSize += info.margin + info.size;
        FromLink(pNode)->SetPositionY(position + halfSize - info.offset);
        position -= info.size;

        if (!hasPrevious) {
            pFirstPane = info.pPane;
        }

        hasPrevious = true;
    }

    if (mAlignment == AlignmentType_Center) {
        const float halfSize = totalSize * 0.5f;

        for (IntrusiveListNode* pNode = pBegin; &m_Children != pNode; pNode = pNode->GetNext()) {
            Pane* pChild = FromLink(pNode);

            if (pChild->IsVisible() && !pChild->IsAlignmentIgnore()) {
                pChild->SetPositionY(halfSize + pChild->GetPositionY());
            }
        }
    }

    if (mIsExtendEdgeEnabled) {
        switch (mAlignment) {
        case AlignmentType_Forward:
            ExtendBottomEdgePane(pLastPane, GetSizeY() - totalSize);
            break;
        case AlignmentType_Center: {
            const float edgeSize = (GetSizeY() - totalSize) * 0.5f;
            ExtendBottomEdgePane(pLastPane, edgeSize);
            ExtendTopEdgePane(pFirstPane, edgeSize);
            break;
        }
        }
    }
}

/** @brief Packs the children from the bottom edge. */
void Alignment::MakeReverseVerticalAlignment() {
    IntrusiveListNode* pFirst = m_Children.GetNext();
    float position = GetSizeY() * -0.5f;
    float totalSize = 0.0f;
    Pane* pLastPane = nullptr;
    bool hasPrevious = false;

    // Walks the children backwards; pNext is the link after the visited child.
    for (IntrusiveListNode* pNext = &m_Children; pFirst != pNext; pNext = pNext->GetPrev()) {
        AlignmentPaneInfo info;

        if (!GetVerticalAlignmentInfo(FromLink(pNext->GetPrev()), &info)) {
            continue;
        }

        if (mIsExtendEdgeEnabled && m_Children.IsLinked() &&
            IsExtendedEdgePane(m_Children, pNext->GetPrev(), mAlignment)) {
            info.offset = 0.0f;
            info.size = 0.0f;
        }

        if (hasPrevious) {
            position += info.margin;
        } else {
            totalSize -= info.margin;
        }

        FromLink(pNext->GetPrev())->SetPositionY(position + info.size * 0.5f - info.offset);
        totalSize += info.margin + info.size;
        position += info.size;
        pLastPane = info.pPane;
        hasPrevious = true;
    }

    if (mIsExtendEdgeEnabled) {
        ExtendTopEdgePane(pLastPane, GetSizeY() - totalSize);
    }
}

/** @brief Aligns the children horizontally. */
void Alignment::MakeHorizontalAlignment() {
    if (mAlignment <= AlignmentType_Center) {
        MakeForwardHorizontalAlignment();
    } else {
        MakeReverseHorizontalAlignment();
    }
}

/** @brief Aligns the children vertically. */
void Alignment::MakeVerticalAlignment() {
    if (mAlignment <= AlignmentType_Center) {
        MakeForwardVerticalAlignment();
    } else {
        MakeReverseVerticalAlignment();
    }
}

/** @return Whether the children are aligned horizontally. */
bool Alignment::IsHorizontalAlignment() const {
    return (mAlignmentFlags & AlignmentFlag_Vertical) == 0;
}

/**
 * @param rOther Copy of this alignment.
 * @return Whether the copy holds the same settings as this alignment.
 */
bool Alignment::CompareCopiedInstanceTest(const Alignment& rOther) const {
    if (mAlignment == rOther.mAlignment && mDefaultMargin == rOther.mDefaultMargin &&
        mIsExtendEdgeEnabled == rOther.mIsExtendEdgeEnabled &&
        mAlignmentFlags == rOther.mAlignmentFlags) {
        return true;
    }

    return false;
}

/** @return Whether the children are aligned vertically. */
bool Alignment::IsVerticalAlignment() const {
    return (mAlignmentFlags & AlignmentFlag_Vertical) != 0;
}

}  // namespace nn::ui2d
