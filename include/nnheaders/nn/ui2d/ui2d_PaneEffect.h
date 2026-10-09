#pragma once
#include <nn/ui2d/ui2d_TexMap.h>
namespace nn::ui2d {
class Pane;
class DrawInfo;
namespace detail {
class PaneEffect {
public:
    PaneEffect();
    void* GetPrivateTexturePtr(const TexMap& map);
    void CaptureMaskedImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawDropShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);

    /** @return Whether the capture effect spreads the pane size to fit italic text. */
    bool IsSpreadPaneSizeOfItalicEnabled() const { return (mParameters._70[0x1b8] >> 1) & 1; }

    /** @param isEnabled Whether the capture effect spreads the pane size to fit italic text. */
    void SetSpreadPaneSizeOfItalicEnabled(bool isEnabled) {
        mParameters._70[0x1b8] = (mParameters._70[0x1b8] & ~2) | (isEnabled << 1);
    }

    Pane* mPane;
    struct Parameters {
        u8 _08[8];
        TexMap maskTexture, shadowTexture;
        u8 _30[0x30];
        TexMap effectTexture;
        u8 _70[0x1b9];
    } mParameters;
};
}
}
