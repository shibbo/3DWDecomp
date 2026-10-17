#pragma once
#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
struct ResAlignment;
}

namespace eui {
class LayoutEx;

/** @brief Pane that lines its visible children up horizontally or vertically. */
class AlignPane : public nn::ui2d::Pane {
public:
    /** @brief Where the children are packed along the alignment axis. */
    enum AlignKind {
        cAlignKind_Center,
        cAlignKind_Left,   ///< Top for vertical alignment.
        cAlignKind_Right,  ///< Bottom for vertical alignment.
    };

    /** @brief Bits stored in mFlags. */
    enum Flag {
        cFlag_Vertical = 1 << 0,
        cFlag_UseAlignment = 1 << 1,  ///< Built from a native alignment pane.
        cFlag_AlignCapture = 1 << 2,  ///< Capture panes take part in parts bounds.
    };

    /** @brief Layout data of one aligned child. */
    struct AlignInfo {
        nn::ui2d::Pane* pPane;
        float size;
        float margin;
        float offset;
    };

    /** @brief Extent of a parts pane along the alignment axis. */
    struct Bound {
        float min;
        float max;
    };

    AlignPane(AlignKind kind, bool extendEdge, float margin, bool vertical);
    AlignPane(const nn::ui2d::ResPane* pResource, const nn::ui2d::BuildArgSet& rArgs);
    AlignPane(const nn::ui2d::ResAlignment* pResource, const nn::ui2d::BuildArgSet& rArgs);
    AlignPane(const AlignPane& rOther, LayoutEx* pLayout);
    ~AlignPane() override;
    NN_RUNTIME_TYPEINFO(nn::ui2d::Pane);
    void Calculate(nn::ui2d::DrawInfo& rDrawInfo, CalculateContext& rContext, bool force) override;
    void updateAlign_(bool adjustSize);
    void updateScrollVertical_();
    void updateScroll_();
    void setAlignKind(AlignKind kind);
    void setExtendEdge(bool extend);
    void setDefaultMargin(float margin);
    bool doAlignVertical_();
    bool doAlign_();
    void adjustPaneSize_();
    bool getAlignInfo_(nn::ui2d::Pane* pPane, float* pSize, float* pMargin, float* pOffset);
    void extendEdge_(AlignInfo* pInfo, float size, bool isReverse);
    bool isPaneAlignIgnore_(nn::ui2d::Pane* pPane);
    bool isPaneAlignAsParts_(nn::ui2d::Pane* pPane);
    void calcPartsBound_(nn::ui2d::Pane* pPane, Bound* pBound, float offset, float scale);
    float getPaneAlignMargin_(nn::ui2d::Pane* pPane);
    bool getAlignInfoVertical_(nn::ui2d::Pane* pPane, float* pSize, float* pMargin,
                               float* pOffset);
    void extendEdgeVertical_(AlignInfo* pInfo, float size, bool isReverse);
    void calcPartsBoundVertical_(nn::ui2d::Pane* pPane, Bound* pBound, float offset, float scale);

    bool isVertical() const { return (mFlags & cFlag_Vertical) != 0; }
    bool isUseAlignment() const { return (mFlags & cFlag_UseAlignment) != 0; }
    bool isAlignCapture() const { return (mFlags & cFlag_AlignCapture) != 0; }

    float mScroll;
    float mAppliedScroll;
    u8 mAlignKind;
    bool mDirty;
    bool mExtendEdge;
    u8 mFlags;
    float mDefaultMargin;
    float mContentSize;
};

static_assert(sizeof(AlignPane) == 0xe8, "AlignPane size");
}  // namespace eui
