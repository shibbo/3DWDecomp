/**
 * @file ui2d_CaptureTexture.h
 * @brief Textures rendered at runtime from panes of a layout.
 */

#pragma once

#include <nn/font/font_Util.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>

namespace nn::ui2d {
class DrawInfo;
class Layout;
class Pane;
struct ResCaptureTexture;

namespace detail {
/** @brief Texture whose contents are generated while the layout is drawn. */
class DynamicRenderingTexture {
public:
    explicit DynamicRenderingTexture(const char* pName);
    NN_RUNTIME_TYPEINFO_BASE();
    virtual ~DynamicRenderingTexture();
    virtual void Finalize(nn::gfx::Device* pDevice);
    virtual void Calculate(DrawInfo& rDrawInfo);
    virtual void Draw(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    virtual Pane* GetTargetPane() const;
    virtual void ResetFirstFrameCaptureUpdatedFlag();

    /**
     * @brief Access the texture name used for sharing between layouts.
     * @return Null-terminated texture name.
     */
    const char* GetName() const { return m_pName; }
    /**
     * @brief Access the draw ordering key of the texture.
     * @return Priority; textures with different priorities are separated by memory barriers.
     */
    u8 GetDrawPriority() const { return m_DrawPriority; }

    void* _08;
    const char* m_pName;
    u8 m_DrawPriority;
};
}  // namespace detail

/** @brief Texture captured from a pane of the layout. */
class CaptureTexture : public detail::DynamicRenderingTexture {
public:
    NN_RUNTIME_TYPEINFO(detail::DynamicRenderingTexture);
    explicit CaptureTexture(const char* pName);

    void Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, const ResCaptureTexture* pResource,
                    Pane* pTargetPane, bool isAllocateInitialized);
    void Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, const CaptureTexture& rSource,
                    Pane* pTargetPane);

    /**
     * @brief Check whether the render target of the texture was created.
     * @return True once Initialize allocated the texture resources.
     */
    bool IsInitialized() const { return m_pTexture != nullptr; }

    unsigned char _20[0x18];
    void* m_pTexture;
    unsigned char _40[0x20];
    u16 m_Flags;
    unsigned char _62[0xe];
};
}  // namespace nn::ui2d
