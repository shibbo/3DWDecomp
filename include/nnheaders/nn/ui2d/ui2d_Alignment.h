#pragma once

#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
/** @brief Serialized alignment pane: the pane block followed by the alignment settings. */
struct ResAlignment {
    u8 paneBlock[0x54];
    u32 alignment;
    float defaultMargin;
    bool isExtendEdgeEnabled;
    u8 alignmentFlags;
};

/** @brief Pane that lines up its children horizontally or vertically. */
class Alignment : public Pane {
public:
    /** @brief Where the children are packed along the alignment axis. */
    enum AlignmentType {
        /** @brief Left edge, or top edge for a vertical alignment. */
        AlignmentType_Forward = 0,
        AlignmentType_Center = 1,
        /** @brief Right edge, or bottom edge for a vertical alignment. */
        AlignmentType_Reverse = 2,
    };

    /** @brief Bits of mAlignmentFlags. */
    enum AlignmentFlag {
        AlignmentFlag_AlignmentRequested = 1 << 0,
        AlignmentFlag_Vertical = 1 << 1,
    };

    Alignment();
    Alignment(const ResAlignment* pResAlignment, const BuildArgSet& rBuildArgSet);
    Alignment(const Alignment& rOther);
    ~Alignment() override;
    NN_RUNTIME_TYPEINFO(Pane);
    bool CompareCopiedInstanceTest(const Alignment& rOther) const;
    void Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext,
                   bool isDirtyParentMtx) override;
    void RequestAlignment();
    float GetDefaultMargin() const;
    u32 GetHorizontalAlignment() const;
    u32 GetVerticalAlignment() const;
    bool IsHorizontalAlignment() const;
    bool IsVerticalAlignment() const;
    void MakeAlignment();
    void MakeHorizontalAlignment();
    void MakeVerticalAlignment();
    void MakeForwardHorizontalAlignment();
    void MakeReverseHorizontalAlignment();
    void MakeForwardVerticalAlignment();
    void MakeReverseVerticalAlignment();

    u32 mAlignment;
    float mDefaultMargin;
    bool mIsExtendEdgeEnabled;
    u8 mAlignmentFlags;
};
static_assert(sizeof(Alignment) == 0xe0, "Alignment size");
}  // namespace nn::ui2d
