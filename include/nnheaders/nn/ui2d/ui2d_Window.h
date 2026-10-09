#pragma once

#include <nn/ui2d/ui2d_Common.h>
#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
struct ResPane;
struct ResMaterial;
struct BuildResultInformation;

/** @brief Signed inflation of the content area beyond the frame edges, in pixels. */
struct ResWindowInflation {
    s16 left;
    s16 right;
    s16 top;
    s16 bottom;
};

/** @brief Frame thickness of each window edge, in pixels. */
struct ResWindowFrameSize {
    u16 left;
    u16 right;
    u16 top;
    u16 bottom;
};

/** @brief Frame thickness of each window edge, as used while calculating vertices. */
struct WindowFrameSize {
    float left;
    float right;
    float top;
    float bottom;
};

/** @brief Content block of a window resource; the texture coordinates follow it directly. */
struct ResWindowContent {
    u32 vtxCols[4];
    u16 materialIdx;
    u8 texCoordCount;
    u8 padding;
};

/** @brief Frame block of a window resource. */
struct ResWindowFrame {
    u16 materialIdx;
    u8 textureFlip;
    u8 padding;
};

/** @brief Window pane resource block; the pane block occupies its first 0x54 bytes. */
struct ResWindow {
    unsigned char paneBlock[0x54];
    ResWindowInflation inflation;
    ResWindowFrameSize frameSize;
    u8 frameCount;
    u8 windowFlags;
    u16 padding;
    u32 contentOffset;
    u32 frameOffsetTableOffset;

    /** @return The pane part of this block. */
    const ResPane* GetPane() const { return reinterpret_cast<const ResPane*>(this); }

    /** @return The content block. */
    const ResWindowContent* GetContent() const {
        return reinterpret_cast<const ResWindowContent*>(reinterpret_cast<const u8*>(this) +
                                                         contentOffset);
    }

    /** @return The table of frame block offsets, relative to this block. */
    const u32* GetFrameOffsetTable() const {
        return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) +
                                            frameOffsetTableOffset);
    }
};

enum WindowFrame {
    WindowFrame_LeftTop,
    WindowFrame_RightTop,
    WindowFrame_LeftBottom,
    WindowFrame_RightBottom,
    WindowFrame_Left,
    WindowFrame_Right,
    WindowFrame_Top,
    WindowFrame_Bottom,
    WindowFrame_MaxWindowFrame
};

enum TextureFlip {
    TextureFlip_None,
    TextureFlip_FlipH,
    TextureFlip_FlipV,
    TextureFlip_Rotate90,
    TextureFlip_Rotate180,
    TextureFlip_Rotate270,
    TextureFlip_MaxTextureFlip
};

enum WindowKind {
    WindowKind_Around,
    WindowKind_Horizontal,
    WindowKind_HorizontalNoContent,
};

class Window : public Pane {
public:
    /** @brief One frame part of the window and the material that draws it. */
    struct Frame {
        Frame() : textureFlip(0), pMaterial(nullptr) {}
        ~Frame();

        /** @return How the frame texture is flipped or rotated. */
        TextureFlip GetTextureFlip() const { return static_cast<TextureFlip>(textureFlip); }

        u8 textureFlip;
        Material* pMaterial;
    };

    /** @brief Content area colors and texture coordinates. */
    struct Content {
        nn::util::Unorm8x4 vtxColors[4];
        detail::TexCoordArray texCoordArray;
    };

    enum Flag {
        Flag_UseVertexColorAll = 1 << 0,
        Flag_UseOneMaterialForAll = 1 << 1,
        Flag_NotDrawContent = 1 << 2,
        Flag_ContentInflationEnabled = 1 << 3,
    };

    Window(int contentTexCount, int frameTexCount);
    Window(int contentTexCount, int frameLtTexCount, int frameRtTexCount, int frameRbTexCount,
           int frameLbTexCount);
    Window(int contentTexCount, int frameLtTexCount, int frameRtTexCount, int frameRbTexCount,
           int frameLbTexCount, int frameLTexCount, int frameTTexCount, int frameRTexCount,
           int frameBTexCount);
    Window(BuildResultInformation*, nn::gfx::Device*, const ResWindow*, const ResWindow*,
           const BuildArgSet&);
    Window(const Window& rOther, nn::gfx::Device* pDevice) : Pane(rOther) {
        CopyImpl(rOther, pDevice, nullptr, nullptr);
    }
    Window(const Window& rOther, nn::gfx::Device* pDevice, Layout* pLayout);
    ~Window() override;
    NN_RUNTIME_TYPEINFO(Pane);
    void Finalize(nn::gfx::Device*) override;
    nn::util::Unorm8x4 GetVertexColor(int) const override;
    void SetVertexColor(int, const nn::util::Unorm8x4&) override;
    u8 GetVertexColorElement(int) const override;
    void SetVertexColorElement(int, u8) override;
    u32 GetMaterialCount() const override;
    Material* GetMaterial(int) const override;
    using Pane::GetMaterial;
    Material* FindMaterialByName(const char*, bool) override;
    const Material* FindMaterialByName(const char*, bool) const override;
    void SetFrameMaterial(WindowFrame frameIdx, Material* pMaterial);
    void SetContentMaterial(Material* pMaterial);
    void Calculate(DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(DrawInfo&, nn::gfx::CommandBuffer&) override;
    void SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer&) const override;
    bool CompareCopiedInstanceTest(const Window& rTarget) const;

    /** @return The material that draws the content area. */
    Material* GetContentMaterial() const { return m_pMaterial; }

    /** @return The material that draws frame @p frameIdx. */
    Material* GetFrameMaterial(WindowFrame frameIdx) const {
        return m_pFrames[frameIdx].pMaterial;
    }

    /** @return The number of frame parts. */
    int GetFrameCount() const { return m_FrameCount; }

    /** @return The sizes of the window frame. */
    const WindowFrameSize GetFrameSize() const {
        const WindowFrameSize frameSize = {
            static_cast<float>(m_WindowSize.frameSize.left),
            static_cast<float>(m_WindowSize.frameSize.right),
            static_cast<float>(m_WindowSize.frameSize.top),
            static_cast<float>(m_WindowSize.frameSize.bottom),
        };
        return frameSize;
    }

    /** @return Whether every vertex color of the content area is opaque white. */
    bool IsContentVertexColorWhite() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (m_Content.vtxColors[i].v[j] != 0xff) {
                    return false;
                }
            }
        }

        return true;
    }

    void CopyImpl(const Window&, nn::gfx::Device*, const Layout*, detail::BuildPaneTreeContext*);

private:
    void Initialize();
    void InitializeTexCount(int contentTexCount, int* pFrameTexCounts, int frameCount);
    void InitializeContent(int texCount);
    void InitializeSize(const ResWindow* pBlock);
    void InitializeFrame(int frameCount);
    void InitializeUseLeftTopMaterialEmulation(BuildResultInformation* pResult,
                                               nn::gfx::Device* pDevice);
    /** @return The number of extra draws that emulate the single frame on other corners. */
    int GetUseLeftTopMaterialEmulationCount() const {
        switch (m_WindowKind) {
        case WindowKind_Horizontal:
        case WindowKind_HorizontalNoContent:
            return 1;
        default:
            return 3;
        }
    }
    void CalculateAroundFrameWindow(DrawInfo& rDrawInfo);
    void CalculateHorizontalFrameWindow(DrawInfo& rDrawInfo);
    void CalculateHorizontalFrameNocontentWindow(DrawInfo& rDrawInfo);
    void DrawSharedMaterialImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawNormalImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void CalculateFrame(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos, const Frame& rFrame,
                        const WindowFrameSize& rFrameSize, u8 alpha);
    void CalculateFrame4(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                         const Frame* pFrames, const WindowFrameSize& rFrameSize, u8 alpha);
    void CalculateFrame8(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                         const Frame* pFrames, const WindowFrameSize& rFrameSize, u8 alpha);
    void CalculateContent(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                          const WindowFrameSize& rFrameSize, u8 alpha);
    void CalculateHorizontalFrame(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                                  const Frame& rFrame, const WindowFrameSize& rFrameSize,
                                  u8 alpha);
    void CalculateHorizontalFrame2(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                                   const Frame* pFrames, const WindowFrameSize& rFrameSize,
                                   u8 alpha);
    void CalculateHorizontalNocontentFrame(DrawInfo& rDrawInfo, const nn::util::Float2& rBasePos,
                                           const Frame& rFrame, const WindowFrameSize& rFrameSize,
                                           u8 alpha);
    void CalculateHorizontalNocontentFrame2(DrawInfo& rDrawInfo,
                                            const nn::util::Float2& rBasePos,
                                            const Frame* pFrames,
                                            const WindowFrameSize& rFrameSize, u8 alpha);

public:
    /** @brief Content inflation and frame sizes, copied and compared as one block. */
    struct WindowSize {
        ResWindowInflation inflation;
        ResWindowFrameSize frameSize;
    };

    WindowSize m_WindowSize;
    Content m_Content;
    u8 m_WindowKind;
    s8 m_FrameCount;
    u8 m_WindowFlags;
    Frame* m_pFrames;
    Material* m_pMaterial;
    u32* m_pUseLeftTopEmulationConstantBufferOffsets;
    u32 m_UseLeftTopEmulationConstantBufferCount;
};

static_assert(sizeof(Window) == 0x130, "Window size");
}  // namespace nn::ui2d
