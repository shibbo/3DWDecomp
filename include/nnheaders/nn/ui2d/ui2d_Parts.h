/**
 * @file Parts.h
 * @brief Layout parts.
 */

#pragma once

#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_Resources.h>

namespace nn {
namespace ui2d {
struct BuildArgSet;

/** @brief Per-pane override entry of a parts pane resource. */
struct ResPartsProperty {
    char name[24];
    u8 usageFlag;
    u8 basicUsageFlag;
    u8 materialUsageFlag;
    u8 systemExtUserDataOverrideFlag;
    u32 propertyOffset;
    u32 extUserDataOffset;
    u32 paneBasicInfoOffset;
};

/** @brief Parts pane block; ResPartsProperty entries and the layout name follow it. */
struct ResParts : ResPane {
    u32 propertyCount;
    nn::util::Float2 magnify;
};

class Parts : public nn::ui2d::Pane {
public:
    NN_RUNTIME_TYPEINFO(nn::ui2d::Pane);

    Parts();
    Parts(nn::ui2d::ResParts const*, nn::ui2d::ResParts const*, nn::ui2d::BuildArgSet const&);
    Parts(nn::ui2d::Parts const&);

    virtual ~Parts();

    Pane* FindPaneByNameRecursive(const char*) override;
    const Pane* FindPaneByNameRecursive(const char*) const override;
    Material* FindMaterialByNameRecursive(const char*) override;
    const Material* FindMaterialByNameRecursive(const char*) const override;
    bool CompareCopiedInstanceTest(const Parts&) const;

    nn::util::IntrusiveListNode m_PartsList;
    Layout* m_pLayout;
};
}  // namespace ui2d
}  // namespace nn
