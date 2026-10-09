#include <eui/euiAlignPane.h>

#include <container/seadRingBuffer.h>
#include <eui/euiArcResourceMgr.h>
#include <eui/euiCapturePane.h>
#include <eui/euiDynamicCapturePane.h>
#include <eui/euiLayoutEx.h>
#include <eui/euiNullPane.h>
#include <eui/euiPartsEx.h>
#include <eui/euiPictureEx.h>
#include <eui/euiScreen.h>
#include <eui/euiTextBoxEx.h>
#include <eui/euiWindowEx.h>
#include <filedevice/seadFileDevice.h>
#include <filedevice/seadFileDeviceMgr.h>
#include <filedevice/seadPath.h>
#include <gfx/nin/seadGraphicsNvn.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/gfx/gfx_ResShader.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_ResTextureData.h>
#include <nn/ui2d/ui2d_Alignment.h>
#include <nn/ui2d/ui2d_ArcExtractor.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_ExtUserData.h>
#include <prim/seadSafeString.h>
#include <resource/seadResourceMgr.h>

namespace eui {
namespace {
using AlignInfoBuffer = sead::FixedRingBuffer<AlignPane::AlignInfo, 128>;

/**
 * @brief Marks the screen that owns pLayout as containing an alignment pane.
 * @param pLayout Layout the pane is built into.
 */
inline void markScreenHasAlignPane(LayoutEx* pLayout) {
    Screen* screen = pLayout->getScreen();

    if (screen != nullptr) {
        screen->mFlags |= 0x20;
    }
}

/**
 * @param pPane Pane to check.
 * @return Whether the pane renders through a capture texture.
 */
inline bool isCapturePane(const nn::ui2d::Pane& rPane) {
    const auto* captureType = CapturePane::GetRuntimeTypeInfoStatic();

    for (const auto* type = rPane.GetRuntimeTypeInfo(); type != nullptr;
         type = type->m_ParentTypeInfo) {
        if (type == captureType) {
            return true;
        }
    }

    const auto* dynamicType = DynamicCapturePane::GetRuntimeTypeInfoStatic();

    for (const auto* type = rPane.GetRuntimeTypeInfo(); type != nullptr;
         type = type->m_ParentTypeInfo) {
        if (type == dynamicType) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Marks the global matrix of a pane dirty after moving or resizing it.
 * @param pPane Pane to mark.
 */
inline void setGlobalMatrixDirty(nn::ui2d::Pane* pPane) {
    u8* flags = &pPane->nn::ui2d::Pane::mFlags;
    *flags |= 0x10;
}

/**
 * @param pPane Pane to count the children of.
 * @return Number of direct children of the pane.
 */
inline int countChildren(const nn::ui2d::Pane* pPane) {
    int count = 0;

    for (const auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        count++;
    }

    return count;
}

/**
 * @param value Value to round.
 * @return value rounded to the nearest integer, halves away from zero.
 */
inline float roundPosition(float value) {
    return static_cast<int>(value + (value >= 0.0f ? 0.5f : -0.5f));
}

/**
 * @param value Value to take the magnitude of.
 * @return The absolute value.
 */
inline float absScale(float value) {
    return value > 0.0f ? value : -value;
}
}  // namespace

/**
 * @brief Creates an alignment pane from code.
 * @param kind Where the children are packed.
 * @param extendEdge Whether the edge children are stretched to fill the pane.
 * @param margin Default spacing between children.
 * @param vertical Whether the children are stacked vertically.
 */
AlignPane::AlignPane(AlignKind kind, bool extendEdge, float margin, bool vertical)
    : mScroll(0), mAppliedScroll(0), mAlignKind(kind), mDirty(true), mExtendEdge(extendEdge),
      mDefaultMargin(margin), mContentSize(-1.0f) {
    mFlags = vertical ? cFlag_Vertical : 0;
}

/**
 * @brief Creates an alignment pane from a null pane configured by extended user data.
 * @param pResource Serialized pane.
 * @param rArgs Layout build context.
 */
AlignPane::AlignPane(const nn::ui2d::ResPane* pResource, const nn::ui2d::BuildArgSet& rArgs)
    : Pane(pResource, rArgs), mScroll(0), mAppliedScroll(0), mAlignKind(cAlignKind_Center),
      mDirty(true), mExtendEdge(false), mFlags(0), mDefaultMargin(0), mContentSize(-1.0f) {
    const auto* data = FindExtUserDataByName("AlignOn");

    if (data != nullptr) {
        const sead::SafeString kind(static_cast<const char*>(data->GetData()));

        if (kind == "Center") {
            mAlignKind = cAlignKind_Center;
        } else if (kind == "Left") {
            mAlignKind = cAlignKind_Left;
        } else if (kind == "Right") {
            mAlignKind = cAlignKind_Right;
        }
    } else {
        data = FindExtUserDataByName("VerticalAlignOn");

        if (data != nullptr) {
            mFlags |= cFlag_Vertical;
            const sead::SafeString kind(static_cast<const char*>(data->GetData()));

            if (kind == "Center") {
                mAlignKind = cAlignKind_Center;
            } else if (kind == "Top") {
                mAlignKind = cAlignKind_Left;
            } else if (kind == "Bottom") {
                mAlignKind = cAlignKind_Right;
            }
        }
    }

    if (FindExtUserDataByName("AlignExtendEdge") != nullptr) {
        mExtendEdge = true;
    }

    data = FindExtUserDataByName("AlignDefaultMargin");

    if (data != nullptr) {
        mDefaultMargin = *data->GetFloatArray();
    }

    if (FindExtUserDataByName("AlignCapture") != nullptr) {
        mFlags |= cFlag_AlignCapture;
    }

    markScreenHasAlignPane(static_cast<LayoutEx*>(rArgs.m_pPartsLayout));
}

/**
 * @brief Creates an alignment pane from a native alignment pane resource.
 * @param pResource Serialized alignment pane.
 * @param rArgs Layout build context.
 */
AlignPane::AlignPane(const nn::ui2d::ResAlignment* pResource, const nn::ui2d::BuildArgSet& rArgs)
    : Pane(reinterpret_cast<const nn::ui2d::ResPane*>(pResource), rArgs), mScroll(0),
      mAppliedScroll(0), mAlignKind(cAlignKind_Center), mDirty(true),
      mExtendEdge(pResource->isExtendEdgeEnabled), mFlags(0),
      mDefaultMargin(pResource->defaultMargin), mContentSize(-1.0f) {
    // Native alignments are 0: left/top, 1: center, 2: right/bottom.
    if (pResource->alignmentFlags & 1) {
        mFlags |= cFlag_Vertical;

        switch (pResource->alignment) {
        case 0:
            mAlignKind = cAlignKind_Left;
            break;
        case 1:
            mAlignKind = cAlignKind_Center;
            break;
        case 2:
            mAlignKind = cAlignKind_Right;
            break;
        }
    } else {
        switch (pResource->alignment) {
        case 0:
            mAlignKind = cAlignKind_Left;
            break;
        case 1:
            mAlignKind = cAlignKind_Center;
            break;
        case 2:
            mAlignKind = cAlignKind_Right;
            break;
        }
    }

    mFlags |= cFlag_UseAlignment;

    if (FindExtUserDataByName("AlignCapture") != nullptr) {
        mFlags |= cFlag_AlignCapture;
    }

    markScreenHasAlignPane(static_cast<LayoutEx*>(rArgs.m_pPartsLayout));
}

/**
 * @brief Copies an alignment pane without its children.
 * @param rOther Pane to copy.
 * @param pLayout Layout the copy belongs to.
 */
AlignPane::AlignPane(const AlignPane& rOther, LayoutEx* pLayout)
    : Pane(rOther), mScroll(rOther.mScroll), mAppliedScroll(rOther.mAppliedScroll),
      mAlignKind(rOther.mAlignKind), mDirty(true), mExtendEdge(rOther.mExtendEdge),
      mFlags(rOther.mFlags), mDefaultMargin(rOther.mDefaultMargin),
      mContentSize(rOther.mContentSize) {
    markScreenHasAlignPane(pLayout);
}

AlignPane::~AlignPane() = default;

/**
 * @brief Aligns the children if needed, applies the scroll offset and calculates the pane.
 * @param rDrawInfo Draw state of the layout.
 * @param rContext Calculation context.
 * @param force Whether to force the recalculation of the pane.
 */
void AlignPane::Calculate(nn::ui2d::DrawInfo& rDrawInfo, CalculateContext& rContext, bool force) {
    updateAlign_(true);

    if (mScroll != mAppliedScroll) {
        if (isVertical()) {
            updateScrollVertical_();
        } else {
            updateScroll_();
        }

        mAppliedScroll = mScroll;
    }

    nn::ui2d::Pane::Calculate(rDrawInfo, rContext, force);
}

/**
 * @brief Aligns the children when the alignment is dirty and the pane is shown.
 * @param adjustSize Whether to adjust the pane size after a successful alignment.
 */
void AlignPane::updateAlign_(bool adjustSize) {
    if (mDirty && (nn::ui2d::Pane::mFlags & 1) && mAlpha) {
        bool dirty;

        if (!isVertical()) {
            dirty = !doAlign_();
        } else {
            dirty = !doAlignVertical_();
        }

        if (adjustSize && !dirty && (nn::ui2d::Pane::mFlags & 4)) {
            adjustPaneSize_();
        }

        mDirty = dirty;
    }
}

/** @brief Moves the children vertically by the scroll offset not yet applied. */
void AlignPane::updateScrollVertical_() {
    const float delta = mScroll - mAppliedScroll;

    for (auto* link = m_Children.GetNext(); link != &m_Children; link = link->GetNext()) {
        auto* pane = FromLink(link);
        const float position = delta + pane->mPositionY;
        setGlobalMatrixDirty(pane);
        pane->mPositionY = position;
    }
}

/** @brief Moves the children horizontally by the scroll offset not yet applied. */
void AlignPane::updateScroll_() {
    const float delta = mScroll - mAppliedScroll;

    for (auto* link = m_Children.GetNext(); link != &m_Children; link = link->GetNext()) {
        auto* pane = FromLink(link);
        const float position = delta + pane->mPositionX;
        setGlobalMatrixDirty(pane);
        pane->mPositionX = position;
    }
}

/**
 * @brief Sets where the children are packed.
 * @param kind New alignment kind.
 */
void AlignPane::setAlignKind(AlignKind kind) {
    mAlignKind = kind;
    mDirty = true;
}

/**
 * @brief Sets whether the edge children are stretched to fill the pane.
 * @param extend Whether to extend the edges.
 */
void AlignPane::setExtendEdge(bool extend) {
    mExtendEdge = extend;
    mDirty = true;
}

/**
 * @brief Sets the default spacing between children.
 * @param margin New default margin.
 */
void AlignPane::setDefaultMargin(float margin) {
    mDefaultMargin = margin;
    mDirty = true;
}

/**
 * @brief Stacks the children vertically.
 * @return Whether at least one child was aligned.
 */
bool AlignPane::doAlignVertical_() {
    AlignInfoBuffer infos;
    float total = 0.0f;
    int index = 0;

    for (auto* link = m_Children.GetNext(); link != &m_Children; link = link->GetNext()) {
        auto* pane = FromLink(link);
        float size = 0.0f;
        float margin = 0.0f;
        float offset = 0.0f;

        if (getAlignInfoVertical_(pane, &size, &margin, &offset)) {
            if (mExtendEdge && (((mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Left) &&
                                 index == countChildren(this) - 1) ||
                                ((mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Right) &&
                                 index == 0))) {
                AlignInfo* info = infos.emplaceBack();
                info->pPane = pane;
                info->size = 0.0f;
                info->margin = margin;
                info->offset = 0.0f;
                total += margin;
            } else {
                AlignInfo* info = infos.emplaceBack();
                info->pPane = pane;
                info->size = size;
                info->margin = margin;
                info->offset = offset;
                total = total + size + margin;
            }
        }

        index++;
    }

    const u32 count = infos.size();

    if (count == 0) {
        mContentSize = 0.0f;
        return false;
    }

    float contentSize;

    if (mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Left) {
        contentSize = total - infos(0).margin;
        float position = (mAlignKind == cAlignKind_Center ? contentSize : mSizeY) * 0.5f;

        for (u32 i = 0; i < count; i++) {
            if (i != 0) {
                position -= infos(i).margin;
            }

            const AlignInfo& info = infos(i);
            auto* pane = info.pPane;
            const float rounded = roundPosition(info.offset + (position - info.size * 0.5f));
            setGlobalMatrixDirty(pane);
            pane->mPositionY = rounded;
            position -= info.size;
        }

        if (mExtendEdge) {
            switch (mAlignKind) {
            case cAlignKind_Left:
                extendEdgeVertical_(&infos(count - 1), mSizeY - contentSize, false);
                break;
            case cAlignKind_Center: {
                const float size = (mSizeY - contentSize) * 0.5f;
                extendEdgeVertical_(&infos(count - 1), size, false);
                extendEdgeVertical_(&infos(0), size, true);
                break;
            }
            }
        }
    } else {
        contentSize = total - infos(count - 1).margin;
        float position = mSizeY * -0.5f;

        for (u32 n = 0, i = count - 1; n < count; n++, i--) {
            if (n != 0) {
                position += infos(i).margin;
            }

            const AlignInfo& info = infos(i);
            auto* pane = info.pPane;
            const float rounded = roundPosition(info.offset + (position + info.size * 0.5f));
            setGlobalMatrixDirty(pane);
            pane->mPositionY = rounded;
            position += info.size;
        }

        if (mExtendEdge) {
            extendEdgeVertical_(&infos(0), mSizeY - contentSize, true);
        }
    }

    mContentSize = contentSize;
    mAppliedScroll = 0.0f;
    return true;
}

/**
 * @brief Lines the children up horizontally.
 * @return Whether at least one child was aligned.
 */
bool AlignPane::doAlign_() {
    AlignInfoBuffer infos;
    float total = 0.0f;
    int index = 0;

    for (auto* link = m_Children.GetNext(); link != &m_Children; link = link->GetNext()) {
        auto* pane = FromLink(link);
        float size = 0.0f;
        float margin = 0.0f;
        float offset = 0.0f;

        if (getAlignInfo_(pane, &size, &margin, &offset)) {
            if (mExtendEdge && (((mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Left) &&
                                 index == countChildren(this) - 1) ||
                                ((mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Right) &&
                                 index == 0))) {
                AlignInfo* info = infos.emplaceBack();
                info->pPane = pane;
                info->size = 0.0f;
                info->margin = margin;
                info->offset = 0.0f;
                total += margin;
            } else {
                AlignInfo* info = infos.emplaceBack();
                info->pPane = pane;
                info->size = size;
                info->margin = margin;
                info->offset = offset;
                total = total + size + margin;
            }
        }

        index++;
    }

    const u32 count = infos.size();

    if (count == 0) {
        mContentSize = 0.0f;
        return false;
    }

    float contentSize;

    if (mAlignKind == cAlignKind_Center || mAlignKind == cAlignKind_Left) {
        contentSize = total - infos(0).margin;
        float position = (mAlignKind == cAlignKind_Center ? contentSize : mSizeX) * -0.5f;

        for (u32 i = 0; i < count; i++) {
            if (i != 0) {
                position += infos(i).margin;
            }

            const AlignInfo& info = infos(i);
            auto* pane = info.pPane;
            const float rounded = roundPosition(position + info.size * 0.5f - info.offset);
            setGlobalMatrixDirty(pane);
            pane->mPositionX = rounded;
            position += info.size;
        }

        if (mExtendEdge) {
            switch (mAlignKind) {
            case cAlignKind_Left:
                extendEdge_(&infos(count - 1), mSizeX - contentSize, false);
                break;
            case cAlignKind_Center: {
                const float size = (mSizeX - contentSize) * 0.5f;
                extendEdge_(&infos(count - 1), size, false);
                extendEdge_(&infos(0), size, true);
                break;
            }
            }
        }
    } else {
        contentSize = total - infos(count - 1).margin;
        float position = mSizeX * 0.5f;

        for (u32 n = 0, i = count - 1; n < count; n++, i--) {
            if (n != 0) {
                position -= infos(i).margin;
            }

            const AlignInfo& info = infos(i);
            auto* pane = info.pPane;
            const float rounded = roundPosition(position - info.size * 0.5f - info.offset);
            setGlobalMatrixDirty(pane);
            pane->mPositionX = rounded;
            position -= info.size;
        }

        if (mExtendEdge) {
            extendEdge_(&infos(0), mSizeX - contentSize, true);
        }
    }

    mContentSize = contentSize;
    mAppliedScroll = 0.0f;
    return true;
}

/** @brief Resizes the pane to its content (the size adjustment itself is compiled out). */
void AlignPane::adjustPaneSize_() {
    for (auto *link = mParent->m_Children.GetNext(), *end = &mParent->m_Children; link != end;
         link = link->GetNext()) {
        if (nn::ui2d::IsDerivedFrom<AlignPane>(FromLink(link))) {
        }
    }

    for (auto *link = mParent->m_Children.GetNext(), *end = &mParent->m_Children; link != end;
         link = link->GetNext()) {
        if (FromLink(link)->FindExtUserDataByName("AdjustToTextOn") != nullptr) {
        }
    }
}

/**
 * @brief Gets the horizontal layout data of a child.
 * @param pPane Child to measure.
 * @param pSize Receives the width taken by the child.
 * @param pMargin Receives the spacing before the child.
 * @param pOffset Receives the offset from the child's position to its visual center.
 * @return Whether the child takes part in the alignment.
 */
bool AlignPane::getAlignInfo_(nn::ui2d::Pane* pPane, float* pSize, float* pMargin,
                              float* pOffset) {
    const bool isTarget = (pPane->nn::ui2d::Pane::mFlags & 1) && !isPaneAlignIgnore_(pPane);
    auto* parts = nn::ui2d::DynamicCast<PartsEx*>(pPane);
    bool asParts;

    if (parts != nullptr) {
        if (!isTarget || !static_cast<LayoutEx*>(parts->m_pLayout)->isAlignAsParts()) {
            return false;
        }

        asParts = true;
    } else {
        asParts = isPaneAlignAsParts_(pPane);

        if (!isTarget) {
            return false;
        }
    }

    const float scale = absScale(pPane->mScaleX);
    const float width = pPane->mSizeX * scale;
    *pOffset = 0.0f;
    bool isValid = true;
    auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

    if (textBox != nullptr) {
        const float textWidth = scale * textBox->calcStringWidth_();
        *pSize = textWidth;

        const int position = textBox->GetTextPositionH();

        if (position == 2) {
            *pOffset = -(textWidth * 0.5f - width * 0.5f);
        } else if (position == 1) {
            *pOffset = -(width * 0.5f - textWidth * 0.5f);
        }
    } else if (asParts) {
        Bound bound = {1e32f, -1e32f};

        for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
             link = link->GetNext()) {
            calcPartsBound_(FromLink(link), &bound, 0.0f, scale);
        }

        *pSize = bound.max - bound.min;
        *pOffset = bound.min + *pSize * 0.5f;

        if (*pSize <= 0.0f) {
            isValid = false;
        }
    } else {
        *pSize = width;
    }

    switch (pPane->GetBasePositionX()) {
    case 1:
        *pOffset += width * 0.5f;
        break;
    case 2:
        *pOffset += width * -0.5f;
        break;
    }

    if (!isValid) {
        return false;
    }

    *pMargin = getPaneAlignMargin_(pPane);
    return true;
}

/**
 * @brief Stretches an edge child horizontally, keeping its outer edge in place.
 * @param pInfo Layout data of the child.
 * @param size New width of the child.
 * @param isReverse Whether the child grows towards the left.
 */
void AlignPane::extendEdge_(AlignInfo* pInfo, float size, bool isReverse) {
    pInfo->pPane->mSizeX = size;
    setGlobalMatrixDirty(pInfo->pPane);
    auto* pane = pInfo->pPane;
    const int basePosition = pane->GetBasePositionX();

    if (isReverse) {
        if (basePosition == 0) {
            pane->mPositionX = pane->mPositionX + size * -0.5f;
            setGlobalMatrixDirty(pane);
        } else if (basePosition == 1) {
            pane->mPositionX = pane->mPositionX - size;
            setGlobalMatrixDirty(pane);
        }
    } else {
        if (basePosition == 0) {
            pane->mPositionX = pane->mPositionX + size * 0.5f;
            setGlobalMatrixDirty(pane);
        } else if (basePosition == 2) {
            pane->mPositionX = pane->mPositionX + size;
            setGlobalMatrixDirty(pane);
        }
    }
}

/**
 * @param pPane Child to check.
 * @return Whether the child is excluded from the alignment.
 */
bool AlignPane::isPaneAlignIgnore_(nn::ui2d::Pane* pPane) {
    if (isUseAlignment()) {
        return pPane->IsAlignmentIgnore();
    }

    return pPane->FindExtUserDataByName("AlignIgnore") != nullptr;
}

/**
 * @param pPane Child to check.
 * @return Whether the child is measured by the bounds of its own children.
 */
bool AlignPane::isPaneAlignAsParts_(nn::ui2d::Pane* pPane) {
    if (isUseAlignment()) {
        if (pPane->IsAlignmentNullPane()) {
            return pPane->m_Children.GetNext() != &pPane->m_Children;
        }

        return false;
    }

    if (nn::ui2d::IsDerivedFrom<NullPane>(pPane) || nn::ui2d::IsDerivedFrom<AlignPane>(pPane)) {
        return pPane->FindExtUserDataByName("AlignAsParts") != nullptr;
    }

    return false;
}

/**
 * @brief Grows a horizontal bound by a pane and its descendants.
 * @param pPane Pane to measure.
 * @param pBound Bound to grow.
 * @param offset Horizontal position of the parent.
 * @param scale Accumulated horizontal scale of the parent.
 */
void AlignPane::calcPartsBound_(nn::ui2d::Pane* pPane, Bound* pBound, float offset, float scale) {
    if (!(pPane->nn::ui2d::Pane::mFlags & 1)) {
        return;
    }

    if (!isAlignCapture() && isCapturePane(*pPane)) {
        return;
    }

    auto* alignPane = nn::ui2d::DynamicCast<AlignPane*>(pPane);

    if (alignPane != nullptr) {
        alignPane->updateAlign_(true);
    }

    const float childScale = absScale(pPane->mScaleX) * scale;

    if (!isPaneAlignIgnore_(pPane)) {
        const float width = childScale * pPane->mSizeX;
        offset = pPane->mPositionX * scale + offset;
        float min;
        float max;
        auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

        if (textBox != nullptr) {
            const float textWidth = childScale * textBox->calcStringWidth_();

            switch (textBox->GetTextPositionH()) {
            case 2:
                max = width * 0.5f + offset;
                min = max - textWidth;
                break;
            case 1:
                min = offset + width * -0.5f;
                max = min + textWidth;
                break;
            default:
                min = offset - textWidth * 0.5f;
                max = offset + textWidth * 0.5f;
                break;
            }
        } else if (nn::ui2d::IsDerivedFrom<PictureEx>(pPane) ||
                   nn::ui2d::IsDerivedFrom<WindowEx>(pPane)) {
            min = offset - width * 0.5f;
            max = width * 0.5f + offset;
        } else {
            min = 0.0f;
            max = 0.0f;
        }

        if (min < max) {
            switch (pPane->GetBasePositionX()) {
            case 1:
                min = width * 0.5f + min;
                max = width * 0.5f + max;
                break;
            case 2:
                min = min - width * 0.5f;
                max = max - width * 0.5f;
                break;
            }
        }

        if (min < pBound->min) {
            pBound->min = min;
        }

        if (pBound->max < max) {
            pBound->max = max;
        }
    }

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        calcPartsBound_(FromLink(link), pBound, offset, childScale);
    }
}

/**
 * @param pPane Child to get the margin of.
 * @return Spacing before the child.
 */
float AlignPane::getPaneAlignMargin_(nn::ui2d::Pane* pPane) {
    if (isUseAlignment()) {
        if (pPane->IsAlignmentMarginEnabled()) {
            return pPane->GetAlignmentMargin();
        }

        return mDefaultMargin;
    }

    const auto* data = pPane->FindExtUserDataByName("AlignMargin");
    return *(data != nullptr ? data->GetFloatArray() : &mDefaultMargin);
}

/**
 * @brief Gets the vertical layout data of a child.
 * @param pPane Child to measure.
 * @param pSize Receives the height taken by the child.
 * @param pMargin Receives the spacing before the child.
 * @param pOffset Receives the offset from the child's position to its visual center.
 * @return Whether the child takes part in the alignment.
 */
bool AlignPane::getAlignInfoVertical_(nn::ui2d::Pane* pPane, float* pSize, float* pMargin,
                                      float* pOffset) {
    const bool isTarget = (pPane->nn::ui2d::Pane::mFlags & 1) && !isPaneAlignIgnore_(pPane);
    auto* parts = nn::ui2d::DynamicCast<PartsEx*>(pPane);
    bool asParts;

    if (parts != nullptr) {
        if (!isTarget || !static_cast<LayoutEx*>(parts->m_pLayout)->isAlignAsParts()) {
            return false;
        }

        asParts = true;
    } else {
        asParts = isPaneAlignAsParts_(pPane);

        if (!isTarget) {
            return false;
        }
    }

    const float scale = absScale(pPane->mScaleY);
    const float height = pPane->mSizeY * scale;
    *pOffset = 0.0f;
    bool isValid = true;
    auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

    if (textBox != nullptr) {
        const float textHeight = scale * textBox->calcStringWidth_();
        *pSize = textHeight;

        if (!textBox->IsTextFlag12()) {
            const int position = textBox->GetTextPositionV();

            if (position == 2) {
                *pOffset = -(textHeight * 0.5f - height * 0.5f);
            } else if (position == 1) {
                *pOffset = -(height * 0.5f - textHeight * 0.5f);
            }
        }
    } else if (asParts) {
        Bound bound = {1e32f, -1e32f};

        for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
             link = link->GetNext()) {
            calcPartsBoundVertical_(FromLink(link), &bound, 0.0f, scale);
        }

        *pSize = bound.max - bound.min;
        *pOffset = bound.min + *pSize * 0.5f;

        if (*pSize <= 0.0f) {
            isValid = false;
        }
    } else {
        *pSize = height;
    }

    switch (pPane->GetBasePositionY()) {
    case 1:
        *pOffset += height * 0.5f;
        break;
    case 2:
        *pOffset += height * -0.5f;
        break;
    }

    if (!isValid) {
        return false;
    }

    *pMargin = getPaneAlignMargin_(pPane);
    return true;
}

/**
 * @brief Stretches an edge child vertically, keeping its outer edge in place.
 * @param pInfo Layout data of the child.
 * @param size New height of the child.
 * @param isReverse Whether the child grows upwards.
 */
void AlignPane::extendEdgeVertical_(AlignInfo* pInfo, float size, bool isReverse) {
    pInfo->pPane->mSizeY = size;
    setGlobalMatrixDirty(pInfo->pPane);
    auto* pane = pInfo->pPane;
    const int basePosition = pane->GetBasePositionY();

    if (isReverse) {
        if (basePosition == 0) {
            pane->mPositionY = pane->mPositionY + size * 0.5f;
            setGlobalMatrixDirty(pane);
        } else if (basePosition == 1) {
            pane->mPositionY = pane->mPositionY + size;
            setGlobalMatrixDirty(pane);
        }
    } else {
        if (basePosition == 0) {
            pane->mPositionY = pane->mPositionY + size * -0.5f;
            setGlobalMatrixDirty(pane);
        } else if (basePosition == 2) {
            pane->mPositionY = pane->mPositionY - size;
            setGlobalMatrixDirty(pane);
        }
    }
}

/**
 * @brief Grows a vertical bound by a pane and its descendants.
 * @param pPane Pane to measure.
 * @param pBound Bound to grow.
 * @param offset Vertical position of the parent, growing downwards.
 * @param scale Accumulated vertical scale of the parent.
 */
void AlignPane::calcPartsBoundVertical_(nn::ui2d::Pane* pPane, Bound* pBound, float offset,
                                        float scale) {
    if (!(pPane->nn::ui2d::Pane::mFlags & 1)) {
        return;
    }

    if (!isAlignCapture() && isCapturePane(*pPane)) {
        return;
    }

    auto* alignPane = nn::ui2d::DynamicCast<AlignPane*>(pPane);

    if (alignPane != nullptr) {
        alignPane->updateAlign_(true);
    }

    const float childScale = absScale(pPane->mScaleY) * scale;

    if (!isPaneAlignIgnore_(pPane)) {
        const float height = childScale * pPane->mSizeY;
        offset = offset - pPane->mPositionY * scale;
        float min;
        float max;
        auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

        if (textBox != nullptr) {
            const float textHeight = childScale * textBox->calcStringWidth_();
            int position = textBox->IsTextFlag12() ? 0 : textBox->GetTextPositionV();

            switch (position) {
            case 2:
                max = height * 0.5f + offset;
                min = max - textHeight;
                break;
            case 1:
                min = offset + height * -0.5f;
                max = min + textHeight;
                break;
            default:
                min = offset - textHeight * 0.5f;
                max = offset + textHeight * 0.5f;
                break;
            }
        } else if (nn::ui2d::IsDerivedFrom<PictureEx>(pPane) ||
                   nn::ui2d::IsDerivedFrom<WindowEx>(pPane)) {
            min = offset - height * 0.5f;
            max = height * 0.5f + offset;
        } else {
            min = 0.0f;
            max = 0.0f;
        }

        if (min < max) {
            switch (pPane->GetBasePositionY()) {
            case 1:
                min = height * 0.5f + min;
                max = height * 0.5f + max;
                break;
            case 2:
                min = min - height * 0.5f;
                max = max - height * 0.5f;
                break;
            }
        }

        if (min < pBound->min) {
            pBound->min = min;
        }

        if (pBound->max < max) {
            pBound->max = max;
        }
    }

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        calcPartsBoundVertical_(FromLink(link), pBound, offset, childScale);
    }
}

// The functions below belong to eui::ArcResourceMgr; the current split places them in this unit.

namespace {
using MemoryPoolImpl = nn::gfx::detail::MemoryPoolImpl<nn::gfx::ApiVariationNvn8>;
using MemoryPoolData = nn::gfx::MemoryPoolImplData<nn::gfx::ApiVariationNvn8>;
using ShaderImpl = nn::gfx::detail::ShaderImpl<nn::gfx::ApiVariationNvn8>;
using ShaderData = nn::gfx::ShaderImplData<nn::gfx::ApiVariationNvn8>;
using ShaderContainerImpl = nn::gfx::detail::ResShaderContainerImpl;
using GfxDevice = sead::GraphicsNvn::GfxDevice;

/** @brief Memory pool block referenced by a shader container. */
struct ResShaderBinaryPoolData {
    u8 _0[0x20];
    nn::util::BinTPtr<void> pMemoryPool;
};

/** @brief Signature of a shader file inside a layout archive. */
constexpr u32 cShaderFileSignature = 'HSNB';

/**
 * @return The gfx device of the graphics system.
 */
inline GfxDevice* getGfxDevice() {
    return sead::GraphicsNvn::instance()->getGfxDevice();
}

/**
 * @param pFile Texture file.
 * @return The texture container of the file.
 */
inline nn::gfx::ResTextureContainerData& getContainer(nn::gfx::ResTextureFile* pFile) {
    return pFile->ToData().textureContainerData;
}

/**
 * @param pPool Memory pool object.
 * @return The state of the memory pool.
 */
inline u8 getMemoryPoolState(void* pPool) {
    return static_cast<MemoryPoolImpl*>(pPool)->ToData()->state;
}

/**
 * @param pContainer Shader container.
 * @return The state of the memory pool holding the shader binaries of the container.
 */
inline u8 getShaderPoolState(nn::gfx::ResShaderContainer* pContainer) {
    auto* pool =
        reinterpret_cast<ResShaderBinaryPoolData*>(pContainer->ToData().pShaderBinaryPool.Get());
    return getMemoryPoolState(pool->pMemoryPool.Get());
}

/**
 * @brief Initializes the memory pool of a texture container on its texture data.
 * @param rContainer Texture container.
 */
inline void initializeTextureMemoryPool(nn::gfx::ResTextureContainerData& rContainer) {
    GfxDevice* device = getGfxDevice();
    nn::gfx::MemoryPoolInfo info;
    info.SetMemoryPoolProperty(0x21);
    auto* block = static_cast<nn::util::BinaryBlockHeader*>(rContainer.pTextureData.Get());
    info.SetPoolMemory(reinterpret_cast<u8*>(block) + sizeof(nn::util::BinaryBlockHeader),
                       block->GetBlockSize() - sizeof(nn::util::BinaryBlockHeader));
    static_cast<MemoryPoolImpl*>(rContainer.pTextureMemoryPool.Get())->Initialize(device, info);
}

/**
 * @param pShaderFile Shader file.
 * @param index Index of the shader variation.
 * @return The shader object of the binary program of the variation.
 */
inline ShaderImpl* getBinaryShader(nn::gfx::ResShaderFile* pShaderFile, int index) {
    auto* program = pShaderFile->GetShaderContainer()->GetResShaderVariation(index)->GetResShaderProgram(
        nn::gfx::ShaderCodeType_Binary);
    return program->GetShader();
}
}  // namespace

/** @brief Creates an empty archive manager. */
ArcResourceMgr::ArcResourceMgr() {
    mList.initOffset(offsetof(ArcResource, mListNode));
}

ArcResourceMgr::~ArcResourceMgr() = default;

/**
 * @brief Loads every archive file directly inside a directory.
 * @param pHeap Heap for the archives.
 * @param rPath Path of the directory.
 */
void ArcResourceMgr::loadArchivesInDirectory(sead::Heap* pHeap, const sead::SafeString& rPath) {
    sead::FixedSafeString<256> path;
    sead::FileDevice* device = sead::FileDeviceMgr::instance()->findDeviceFromPath(rPath, &path);
    sead::DirectoryHandle handle;

    if (device->tryOpenDirectory(&handle, path) != nullptr) {
        sead::DirectoryEntry entry;

        while (handle.read(&entry, 1) != 0) {
            if (!entry.is_directory) {
                sead::FormatFixedSafeString<256> filePath("%s/%s", path.cstr(), entry.name.cstr());
                loadArchive(pHeap, filePath);
            }
        }
    }
}

/**
 * @brief Loads an archive file and adds it to the archive list.
 * @param pHeap Heap for the archive.
 * @param rPath Path of the archive file.
 */
void ArcResourceMgr::loadArchive(sead::Heap* pHeap, const sead::SafeString& rPath) {
    sead::ResourceMgr::LoadArg arg;
    OneTimeBinaryResourceFactory factory;
    bool hasTriedCreateWithDecomp = false;
    arg.path = rPath;
    arg.instance_heap = pHeap;
    arg.load_data_heap = pHeap;
    arg.instance_alignment = 4;
    arg.load_data_alignment = 0x1000;
    arg.factory = &factory;
    arg.has_tried_create_with_decomp = &hasTriedCreateWithDecomp;
    sead::Resource* resource = sead::ResourceMgr::instance()->tryLoad(arg, "", nullptr);
    auto* directResource = sead::DynamicCast<sead::DirectResource>(resource);

    if (directResource != nullptr) {
        sead::FixedSafeString<64> name;
        sead::Path::getBaseFileName(&name, rPath);
        auto* archive = new (pHeap, 8) ArcResource(this, name, directResource->getRawData());
        mList.pushBack(archive);
    }
}

/**
 * @param rName Base file name of the archive.
 * @return The data of the archive, or nullptr if it is not loaded.
 */
void* ArcResourceMgr::findArchiveData(const sead::SafeString& rName) const {
    for (auto& archive : mList) {
        if (archive.mName == rName) {
            return archive.mData;
        }
    }

    return nullptr;
}

/**
 * @param rName Base file name of the archive.
 * @return The archive, or nullptr if it is not loaded.
 */
ArcResourceMgr::ArcResource* ArcResourceMgr::findArcResource(const sead::SafeString& rName) const {
    for (auto& archive : mList) {
        if (archive.mName == rName) {
            return &archive;
        }
    }

    return nullptr;
}

/** @brief Destroys every archive and unloads its file data. */
void ArcResourceMgr::unloadAllArchives() {
    for (auto& archive : mList.robustRange()) {
        u8* data = static_cast<u8*>(archive.mData);
        delete &archive;
        sead::FileDeviceMgr::instance()->unload(data);
    }
}

/**
 * @param pArchive Archive to add to the list.
 */
void ArcResourceMgr::addArchiveToList(ArcResource* pArchive) {
    mList.pushBack(pArchive);
}

/**
 * @param pArchive Archive to remove from the list.
 */
void ArcResourceMgr::eraseArchiveFromList(ArcResource* pArchive) {
    mList.erase(pArchive);
}

/**
 * @brief Finalizes the initialized shaders and shader pools of every shader file in an archive.
 * @param pArchive Layout archive.
 */
void ArcResourceMgr::finalizeInitializedShaderResource(void* pArchive) {
    nn::ui2d::ArcExtractor extractor(pArchive);
    GfxDevice* device = getGfxDevice();
    const int fileCount = extractor.GetFileCount();

    for (int i = 0; i < fileCount; i++) {
        void* file = extractor.GetFileFast(nullptr, i);

        if (*static_cast<u32*>(file) != cShaderFileSignature) {
            continue;
        }

        auto* shaderFile = nn::gfx::ResShaderFile::ResCast(file);
        const int variationCount = shaderFile->GetShaderContainer()->GetShaderVariationCount();

        for (int j = 0; j < variationCount; j++) {
            ShaderImpl* shader = getBinaryShader(shaderFile, j);

            if (shader->ToData()->state != ShaderData::State_NotInitialized) {
                shader->Finalize(device);
            }
        }

        nn::gfx::ResShaderContainer* container = shaderFile->GetShaderContainer();

        if (getShaderPoolState(container) == MemoryPoolData::State_Initialized) {
            ShaderContainerImpl::Finalize(container, device);
        }
    }
}

/**
 * @brief Initializes again the shader pools and shaders of every shader file in an archive.
 * @param pArchive Layout archive.
 */
void ArcResourceMgr::reinitializeShaderResource(void* pArchive) {
    nn::ui2d::ArcExtractor extractor(pArchive);
    GfxDevice* device = getGfxDevice();
    const int fileCount = extractor.GetFileCount();

    for (int i = 0; i < fileCount; i++) {
        void* file = extractor.GetFileFast(nullptr, i);

        if (*static_cast<u32*>(file) != cShaderFileSignature) {
            continue;
        }

        auto* shaderFile = nn::gfx::ResShaderFile::ResCast(file);

        nn::gfx::ResShaderContainer* container = shaderFile->GetShaderContainer();

        if (getShaderPoolState(container) == MemoryPoolData::State_NotInitialized) {
            ShaderContainerImpl::Initialize(container, device);
        }

        const int variationCount = shaderFile->GetShaderContainer()->GetShaderVariationCount();

        for (int j = 0; j < variationCount; j++) {
            auto* program = shaderFile->GetShaderContainer()->GetResShaderVariation(j)->GetResShaderProgram(
                nn::gfx::ShaderCodeType_Binary);
            ShaderImpl* shader = program->GetShader();

            if (shader->ToData()->state == ShaderData::State_NotInitialized) {
                shader->Initialize(device, *program->GetShaderInfo());
            }
        }
    }
}

/**
 * @brief Registers a loaded archive and sets up the memory pool of its combined texture file.
 * @param pMgr Manager owning the archive.
 * @param rName Base file name of the archive.
 * @param pData Data of the archive.
 */
ArcResourceMgr::ArcResource::ArcResource(ArcResourceMgr* pMgr, const sead::SafeString& rName,
                                         void* pData)
    : mMgr(pMgr), mName(rName), mData(pData), mTextureFile(nullptr) {
    nn::ui2d::ArcExtractor extractor(pData);
    nn::ui2d::ArcFileInfo fileInfo;
    const int entryId = extractor.ConvertPathToEntryId("timg/__Combined.bntx");

    if (entryId < 0) {
        return;
    }

    void* file = extractor.GetFileFast(&fileInfo, entryId);

    if (file == nullptr || fileInfo.GetLength() == 0) {
        return;
    }

    mTextureFile = nn::gfx::ResTextureFile::ResCast(file);
    nn::gfx::ResTextureContainerData& rContainer = getContainer(mTextureFile);

    if (getMemoryPoolState(rContainer.pTextureMemoryPool.Get()) !=
        MemoryPoolData::State_NotInitialized) {
        return;
    }

    initializeTextureMemoryPool(rContainer);
    rContainer.pCurrentMemoryPool.Set(rContainer.pTextureMemoryPool.Get());
    rContainer.memoryPoolOffsetBase = 0;
}

/** @brief Releases the texture memory pool and shaders of the archive and unregisters it. */
ArcResourceMgr::ArcResource::~ArcResource() {
    if (mTextureFile != nullptr) {
        nn::gfx::ResTextureContainerData& rContainer = getContainer(mTextureFile);

        if (getMemoryPoolState(rContainer.pTextureMemoryPool.Get()) !=
            MemoryPoolData::State_NotInitialized) {
            if (rContainer.pCurrentMemoryPool.Get() == rContainer.pTextureMemoryPool.Get()) {
                static_cast<MemoryPoolImpl*>(rContainer.pCurrentMemoryPool.Get())
                    ->Finalize(getGfxDevice());
            }

            rContainer.pCurrentMemoryPool.Set(nullptr);
        }
    }

    if (mData != nullptr) {
        finalizeInitializedShaderResource(mData);
    }

    mMgr->eraseArchiveFromList(this);
}

/**
 * @brief Creates the single resource of the factory in its own buffer.
 * @param pHeap Unused.
 * @param alignment Unused.
 * @return The resource.
 */
sead::DirectResource* ArcResourceMgr::OneTimeBinaryResourceFactory::newResource_(sead::Heap* pHeap,
                                                                                  s32 alignment) {
    mIsCreated = true;
    return new (mResourceBuffer) sead::DirectResource();
}
}  // namespace eui
