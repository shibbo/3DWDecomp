#pragma once

#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
struct BuildResSet;
struct ResExtUserDataList;
class ControlCreator;
class TextSearcher;
namespace detail {
class BuildPaneTreeContext;
}

class ResourceAccessor;

/** @brief Font name table of a layout resource. */
struct ResFontList {
    u32 signature;
    u32 blockSize;
    u16 fontCount;
    u16 padding;
    u32 nameOffsets[1];

    /** @return The name of the font at index, stored relative to the offset table. */
    const char* GetFontName(int index) const {
        return reinterpret_cast<const char*>(nameOffsets) + nameOffsets[index];
    }
};

/** @brief Resource tables of the layout a pane is built from. */
struct BuildResSet {
    const void* pTextureList;
    const ResFontList* pFontList;
    const void* pMaterialList;
    const void* pShapeInfoList;
    const void* pCaptureTextureList;
    const void* pVectorGraphicsTextureList;
    ResourceAccessor* pResAccessor;
    Layout* pLayout;
};

struct BuildArgSet {
    nn::util::Float2 magnify;
    nn::util::Float2 partsSize;
    ControlCreator* pControlCreator;
    TextSearcher* pTextSearcher;
    Layout* m_pPartsLayout;
    Layout* m_pLayout;
    const BuildResSet* pCurrentBuildResSet;
    const BuildResSet* pOverrideBuildResSet;
    u16 overridePaneUsageFlag;
    u16 overrideBasicUsageFlag;
    u16 overrideUsageFlag;
    const void* pOverridePartsPaneBasicInfo;
    const char* pDynamicTexturePrefixNames[8];
    int mDynamicTexturePrefixDepth;
    int mAlternateDynamicTexturePrefixDepth;
    Pane* pParentPane;
    detail::BuildPaneTreeContext* pBuildPaneTreeContext;
    bool _A8;
    void* _B0;
    void* _B8;
    const ResExtUserDataList* pExtUserDataList;
    const ResExtUserDataList* pOverrideExtUserDataList;
    bool isRootPaneParts;
    bool isUtf8;
    bool _D2;
    u32 resourceVersion;
    bool _D8;
    bool isCaptureTextureAllAllocateInitialized;
    bool _DA;
    bool isIgnoreCaptureEffectFirstFrameOnly;
    bool _DC;
    int captureNestDepth;
};

}  // namespace nn::ui2d
