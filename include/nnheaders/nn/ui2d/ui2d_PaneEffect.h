/**
 * @file ui2d_PaneEffect.h
 * @brief Capture based effects (mask and drop shadow) applied to a pane.
 */

#pragma once

#include <cstddef>
#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_TexMap.h>
#include <nn/util/util_MathTypes.h>

namespace nn::gfx {
class ViewportStateInfo;
class ScissorStateInfo;
}  // namespace nn::gfx

namespace nn::ui2d {
class CaptureTexture;
class DrawInfo;
class Layout;
class Pane;
class RenderTargetTextureInfo;
class ShaderInfo;
class TextureInfo;
struct BuildArgSet;
struct BuildResultInformation;
struct ResCaptureTexture;
struct Size;

namespace detail {
/** @brief Blend mode of a drop shadow layer as stored in the resource. */
enum DropShadowBlendMode {
    DropShadowBlendMode_Normal,
    DropShadowBlendMode_Multiply,
    DropShadowBlendMode_Add,
    DropShadowBlendMode_Subtract,
    DropShadowBlendMode_SemitransparencyMaxAlpha,
};

/** @brief Parameters of one blurred drop shadow layer. */
struct BlurParams {
    float angle;
    float distance;
    nn::util::Float4 color;
    float spread;
    float size;
};

size_t AllocAndSetupVertexShaderConstantBuffer(DrawInfo& rDrawInfo, const Pane* pPane);
void SetupViewportScissorStateInfo(nn::gfx::ViewportStateInfo* pViewportInfo,
                                   nn::gfx::ScissorStateInfo* pScissorInfo, float width,
                                   float height);
float CalculateCaptureTextureScale(const ResCaptureTexture* pResource);

class PaneEffect {
public:
    /** @brief Bits of mFlags. */
    enum Flag {
        Flag_MaskTextureOwner = 1 << 0,
        Flag_SpreadPaneSizeOfItalic = 1 << 1,
    };

    /** @brief Number of shader variations used by the drop shadow. */
    static const int DropShadowShaderVariationCount = 38;

    /** @brief State of the mask function. */
    struct MaskParameters {
        const SystemDataMaskTexture* pData;
        TexMap maskTexMap;
        TexMap captureTexMap;
        int vertexShaderOffset;
        const ShaderInfo* pShader;
        int shaderVariation;
        RenderTargetTextureInfo* pCaptureTarget;
        int captureVertexShaderOffset;
        bool isStaticCacheUpdated;
        u8 _4D[3];
    };

    /** @brief State of the drop shadow function. */
    struct DropShadowParameters {
        const SystemDataDropShadow* pData;
        TexMap captureTexMap;
        const TextureInfo* pCaptureTextureInfo;
        RenderTargetTextureInfo* pBlurTarget;
        RenderTargetTextureInfo* pStaticCacheTargets[SystemDataDropShadow::DropShadowType_Max];
        bool isStaticCacheUpdated;
        int horizontalBlurOffsets[SystemDataDropShadow::DropShadowType_Max];
        int verticalBlurOffsets[SystemDataDropShadow::DropShadowType_Max];
        int staticRenderingOffsets[SystemDataDropShadow::DropShadowType_Max];
        int pixelShaderOffsets[SystemDataDropShadow::DropShadowType_Max];
        int blurLevels[SystemDataDropShadow::DropShadowType_Max];
        u32 vertexShaderOffset;
        const ShaderInfo* pShader;
        int shaderVariations[DropShadowShaderVariationCount];
        int pixelShaderSlots[DropShadowShaderVariationCount];
    };

    PaneEffect();
    PaneEffect(const PaneEffect& rSource, Pane* pPane, nn::gfx::Device* pDevice,
               const Layout* pLayout, BuildPaneTreeContext* pContext);

    void Initialize(BuildResultInformation* pResult, nn::gfx::Device* pDevice, Pane* pPane,
                    const BuildArgSet& rArgs);
    void InitializeMaskFunction(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                const SystemDataMaskTexture* pMask, const BuildArgSet& rArgs);
    void InitializeDropShadowFunction(nn::gfx::Device* pDevice,
                                      const SystemDataDropShadow* pDropShadow,
                                      const BuildArgSet& rArgs);
    const ResCaptureTexture* FindCaptureTextureResource(const BuildArgSet& rArgs,
                                                        int textureIndex) const;
    void InitializeCommonShader(nn::gfx::Device* pDevice, const Layout* pLayout,
                                const SystemDataMaskTexture* pMask,
                                const SystemDataDropShadow* pDropShadow);
    void CreatePaneEffectTempTextures(nn::gfx::Device* pDevice, const Layout* pLayout,
                                      const SystemDataMaskTexture* pMask,
                                      const SystemDataDropShadow* pDropShadow, float scale);
    void* GetPrivateTexturePtr(const TexMap& rTexMap);
    void FinalizeTexMapDynamicRenderingTexture(nn::gfx::Device* pDevice, const TexMap& rTexMap);
    void FinalizeDynamicRenderingTexture(nn::gfx::Device* pDevice);
    void Finalize(nn::gfx::Device* pDevice);
    void Calculate(DrawInfo& rDrawInfo);
    void CalculateMaskConstantBuffer(DrawInfo& rDrawInfo);
    void CalculateDropShadowConstantBuffer(DrawInfo& rDrawInfo);
    void Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void CaptureMaskedImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawDropShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawStaticCachedShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands, int vertexShaderOffset,
                   const nn::gfx::DescriptorSlot& rTextureSlot,
                   const nn::gfx::DescriptorSlot& rSamplerSlot);
    void DrawMaskedImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    static size_t GetRequiredConstantBufferSize(nn::gfx::Device* pDevice,
                                                const SystemDataMaskTexture* pMask,
                                                const SystemDataDropShadow* pDropShadow);
    CaptureTexture* CreateCaptureTexture(const BuildArgSet& rArgs, int textureIndex) const;
    void ApplyMaskParameters(Material::ConstantBufferForVertexShader* pConstantBuffer);
    void CalculateMaskCaptureDrawConstantBuffer(DrawInfo& rDrawInfo, int offset);
    void SetupMaskShader(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DrawCommonImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                        const ShaderInfo* pShader, int variation, int vertexShaderOffset);
    int MakeBlurPixelShaderConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                          const BlurParams& rParams) const;
    void MakeHorizontalBlurConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                          const nn::util::Float2& rPos, const Size& rSize,
                                          const nn::util::Float2& rTexCoordOffset) const;
    void MakeVerticalBlurConstantBuffer(DrawInfo& rDrawInfo, int offset, const BlurParams& rParams,
                                        const nn::util::Float2& rPos, const Size& rSize,
                                        const nn::util::Float2& rTexCoordScale,
                                        const nn::util::Float2& rTexCoordOffset) const;
    nn::util::Float2 CalculateDropShadowEffectVertexPos(const Size& rSize) const;
    void MakeStaticRenderingConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                           const BlurParams& rParams, const Size& rSize) const;
    int MakeDropShadowConstantBufferSet(DrawInfo& rDrawInfo,
                                        SystemDataDropShadow::DropShadowType type);
    void AllocateDropShadowEffectConstantBufferSet(DrawInfo& rDrawInfo,
                                                   SystemDataDropShadow::DropShadowType type);
    void MakeBlurParams(BlurParams* pParams, const SystemDataDropShadow* pDropShadow,
                        SystemDataDropShadow::DropShadowType type) const;
    void CalcuVertexPosOfPaneParam(nn::util::Float2* pPos) const;
    int CalculateVerticalBlurShaderId(PresetBlendStateId blendStateId, bool isKnockoutEnabled,
                                      bool isHighQuality) const;
    void DrawBluredShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                          SystemDataDropShadow::DropShadowType type,
                          PresetBlendStateId blendStateId, bool isKnockoutEnabled);
    PresetBlendStateId ConvertBlendType(DropShadowBlendMode blendMode) const;

    /** @return Whether the mask is rendered once into a static cache. */
    bool IsMaskStaticCacheEnabled() const {
        return m_Mask.pData != nullptr &&
               (m_Mask.pData->flags & SystemDataMaskTexture::Flag_StaticCache) != 0;
    }

    /** @return Whether the drop shadow is rendered once into a static cache. */
    bool IsDropShadowStaticCacheEnabled() const {
        return m_DropShadow.pData != nullptr &&
               (m_DropShadow.pData->flags & SystemDataDropShadow::Flag_StaticCache) != 0;
    }

    /** @return Whether only the drop shadow is drawn, without the source image. */
    bool IsDropShadowOnlyEnabled() const {
        return m_DropShadow.pData != nullptr &&
               (m_DropShadow.pData->flags & SystemDataDropShadow::Flag_OnlyEffectEnabled) != 0;
    }

    /** @return Whether the capture effect spreads the pane size to fit italic text. */
    bool IsSpreadPaneSizeOfItalicEnabled() const {
        return (mFlags >> 1) & 1;
    }

    /** @param isEnabled Whether the capture effect spreads the pane size to fit italic text. */
    void SetSpreadPaneSizeOfItalicEnabled(bool isEnabled) {
        mFlags = (mFlags & ~Flag_SpreadPaneSizeOfItalic) | (isEnabled << 1);
    }

    Pane* mPane;
    MaskParameters m_Mask;
    DropShadowParameters m_DropShadow;
    const ShaderInfo* m_pCommonShader;
    int m_CommonShaderVariation;
    int m_CommonShaderTextureSlot;
    union {
        u8 mFlags;
        struct {
            u8 m_IsMaskTextureOwner : 1;
            u8 m_IsSpreadPaneSizeOfItalic : 1;
        };
    };
};

static_assert(offsetof(SystemDataMaskTexture, texSrt) == 0x18, "SystemDataMaskTexture layout");
static_assert(offsetof(SystemDataDropShadow, dropShadowSize) == 0x68,
              "SystemDataDropShadow layout");
static_assert(sizeof(PaneEffect::MaskParameters) == 0x50, "MaskParameters size");
static_assert(sizeof(PaneEffect::DropShadowParameters) == 0x1c0, "DropShadowParameters size");
static_assert(sizeof(PaneEffect) == 0x230, "PaneEffect size");
}  // namespace detail
}  // namespace nn::ui2d
