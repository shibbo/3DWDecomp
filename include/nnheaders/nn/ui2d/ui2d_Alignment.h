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

class Alignment : public Pane {
public:
    Alignment();
    ~Alignment() override;
    NN_RUNTIME_TYPEINFO(Pane);
    bool CompareCopiedInstanceTest(const Alignment& rOther) const;
    void Calculate(DrawInfo&, CalculateContext&, bool) override;
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
    u8 mMode, mAlignmentFlags;
};
static_assert(sizeof(Alignment) == 0xe0, "Alignment size");
}
