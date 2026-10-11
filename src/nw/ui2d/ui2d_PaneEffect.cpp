#include <nn/ui2d/ui2d_PaneEffect.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <type_traits>
#include <nn/font/font_GpuBuffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_Shader.h>
#include <nn/gfx/gfx_StateInfo.h>
#include <nn/gfx/gfx_TextureInfo.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_CaptureTexture.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_RenderTargetTextureInfo.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_ShaderInfo.h>
#include <nn/ui2d/ui2d_TextureInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_BitUtil.h>

namespace nn::ui2d::detail {

namespace {

typedef nn::gfx::detail::CommandBufferImpl<nn::gfx::ApiVariationNvn8> CommandBufferImpl;
typedef nn::gfx::detail::ColorTargetViewImpl<nn::gfx::ApiVariationNvn8> ColorTargetViewImpl;
typedef nn::gfx::detail::ViewportScissorStateImpl<nn::gfx::ApiVariationNvn8>
    ViewportScissorStateImpl;
typedef Material::ConstantBufferForVertexShader VertexShaderConstantBuffer;
typedef SystemDataDropShadow::DropShadowType DropShadowType;

/** @brief Signature of the archive shader that draws masks. */
const u32 ArchiveShaderSignatureMask = 0x4b53414d;
/** @brief Signature of the archive shader that draws drop shadows. */
const u32 ArchiveShaderSignatureDropShadow = 0x48535244;

/** @brief Variation key of the mask capture shader. */
const u32 MaskShaderVariationKey = 0;
/** @brief Variation key of the shader that draws a captured image through a mask. */
const u32 MaskCommonShaderVariationKey = 1;
/** @brief Variation key of the shader that draws a captured image without a mask. */
const u32 DropShadowCommonShaderVariationKey = 8;

/** @brief Index of the first high quality entry of DropShadowShaderVariationKeys. */
const int DropShadowHighQualityVariationIndex = 20;
/** @brief Index of the static cache shader in DropShadowShaderVariationKeys. */
const int DropShadowStaticVariationIndex = 36;
/** @brief Index of the multiplying static cache shader in DropShadowShaderVariationKeys. */
const int DropShadowStaticMultiplyVariationIndex = 37;

/** @brief Size of the members of PaneEffect from m_Mask up to and including mFlags. */
const size_t PaneEffectStateSize =
    offsetof(PaneEffect, mFlags) + sizeof(u8) - offsetof(PaneEffect, m_Mask);

/** @brief Size of the pixel shader constant buffer of a blur pass. */
const size_t BlurPixelShaderConstantBufferSize = 0xe0;

/** @brief Number of gaussian weights evaluated for a blur. */
const int BlurWeightCount = 32;
/** @brief Smallest weight of a bilinear sample that still contributes to a blur. */
const float MinBlurWeight = 6.1035156e-05f;

/** @brief Number of bilinear samples taken on each side of a blur. */
const int BlurSampleCount = 16;

/** @brief Variation keys of the drop shadow archive shader. */
const u32 DropShadowShaderVariationKeys[PaneEffect::DropShadowShaderVariationCount] = {
    0,    1,    2,    3,    4,    5,    6,    7,    14,   15,   16,   17,   104,
    105,  106,  107,  114,  115,  116,  117,  1004, 1005, 1006, 1007, 1014, 1015,
    1016, 1017, 1104, 1105, 1106, 1107, 1114, 1115, 1116, 1117, 8,    18,
};

/** @brief Texture coordinates of a quad covering a whole texture. */
const nn::util::Float2 QuadTexCoords[1][4] = {
    {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}},
};

/** @brief Pixel shader constants of a blur pass. */
struct BlurPixelShaderConstantBuffer {
    float color[4];
    float spreadScale;
    float centerWeight;
    float alpha;
    float rcpAlpha;
    float offsetX[BlurSampleCount];
    float offsetY[BlurSampleCount];
    float weight[BlurSampleCount];
};

/**
 * @brief Cast a runtime-typed object to a derived class.
 * @param pObject Object to cast, may be nullptr.
 * @return pObject as TTo, or nullptr when the object is not of that type.
 */
template <typename TTo, typename TFrom>
TTo DynamicCast(TFrom* pObject) {
    const font::detail::RuntimeTypeInfo* pTargetType =
        std::remove_pointer<TTo>::type::GetRuntimeTypeInfoStatic();

    if (pObject != nullptr) {
        const font::detail::RuntimeTypeInfo* pType = pObject->GetRuntimeTypeInfo();
        while (pType != nullptr) {
            if (pType == pTargetType) {
                return static_cast<TTo>(pObject);
            }

            pType = pType->m_ParentTypeInfo;
        }
    }

    return nullptr;
}

/**
 * @brief Gives access to the API implementation of a command buffer.
 * @param rCommands Command buffer to access.
 * @return The implementation of rCommands.
 */
CommandBufferImpl& ToImpl(nn::gfx::CommandBuffer& rCommands) {
    return rCommands;
}

/**
 * @param pTextureList Texture table of a layout resource.
 * @return Name offsets of the texture table, relative to the offset table.
 */
const u32* GetTextureNameOffsets(const void* pTextureList) {
    return reinterpret_cast<const u32*>(static_cast<const u8*>(pTextureList) + 0xc);
}

/**
 * @brief Gives access to the API implementation of a color target view.
 * @param pView View to access.
 * @return The implementation of pView.
 */
ColorTargetViewImpl* ToImpl(nn::gfx::ColorTargetView* pView) {
    return reinterpret_cast<ColorTargetViewImpl*>(pView);
}

/**
 * @brief Gives access to the API implementation of an array of color target views.
 * @param ppViews Views to access.
 * @return The implementation of ppViews.
 */
const ColorTargetViewImpl* const* ToImpl(const nn::gfx::ColorTargetView* const* ppViews) {
    return reinterpret_cast<const ColorTargetViewImpl* const*>(ppViews);
}

/**
 * @brief Gives access to the API implementation of a viewport scissor state.
 * @param pState State to access.
 * @return The implementation of pState.
 */
const ViewportScissorStateImpl* ToImpl(const nn::gfx::ViewportScissorState* pState) {
    return reinterpret_cast<const ViewportScissorStateImpl*>(pState);
}

/**
 * @brief Look up a texture name in the name offsets of a texture table.
 * @param pOffsets Name offsets of the texture table.
 * @param index Index of the texture.
 * @return Null-terminated texture name.
 */
const char* GetTextureName(const u32* pOffsets, int index) {
    return reinterpret_cast<const char*>(pOffsets) + pOffsets[index];
}

/**
 * @brief Look up a texture name in the texture table of the layout being built.
 * @param rArgs Build arguments holding the resource tables.
 * @param index Index of the texture.
 * @return Null-terminated texture name.
 */
const char* GetTextureName(const BuildArgSet& rArgs, int index) {
    return GetTextureName(GetTextureNameOffsets(rArgs.pCurrentBuildResSet->pTextureList), index);
}

/**
 * @param pMask Mask parameters of a pane.
 * @return Texture transform of the mask texture.
 */
const ResTexSrt& GetTexSrt(const SystemDataMaskTexture* pMask) {
    return *reinterpret_cast<const ResTexSrt*>(pMask->texSrt);
}

/**
 * @param pShader Shader to query.
 * @param variation Shader variation.
 * @param index Index of the texture inside the variation.
 * @return Texture slot of the pixel shader.
 */
int GetTextureSlot(const ShaderInfo* pShader, int variation, int index) {
    const int* pSlots = &pShader->m_pTextureSlots[variation * pShader->GetTextureSlotCount()];
    return pSlots[index];
}

/**
 * @param pShader Shader to query.
 * @param variation Shader variation.
 * @return First texture slot of the pixel shader.
 */
int GetFirstTextureSlot(const ShaderInfo* pShader, u8 variation) {
    return pShader->m_pTextureSlots[variation * pShader->GetTextureSlotCount()];
}

/**
 * @brief Access a vertex shader constant buffer inside the ui2d constant buffer.
 * @param rDrawInfo Draw state holding the constant buffer.
 * @param offset Offset of the constant buffer.
 * @return The mapped constant buffer.
 */
VertexShaderConstantBuffer* GetVertexShaderConstantBuffer(const DrawInfo& rDrawInfo,
                                                          ptrdiff_t offset) {
    return reinterpret_cast<VertexShaderConstantBuffer*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) + offset);
}

/**
 * @brief Load a texture matrix into a constant buffer.
 * @param pDst Two rows of the constant buffer.
 * @param pSrc Texture matrix to load.
 */
void SetTextureMatrix(float (*pDst)[4], const float (*pSrc)[3]) {
    pDst[0][0] = pSrc[0][0];
    pDst[0][1] = pSrc[0][1];
    pDst[0][2] = 0.0f;
    pDst[0][3] = pSrc[0][2];
    pDst[1][0] = pSrc[1][0];
    pDst[1][1] = pSrc[1][1];
    pDst[1][2] = 0.0f;
    pDst[1][3] = pSrc[1][2];
}

/**
 * @brief Compute the alpha of a pane for a cached effect image.
 * @param pPane Pane to query.
 * @return Global alpha of the pane, normalized twice.
 */
float GetCachedImageAlpha(const Pane* pPane) {
    return static_cast<float>(pPane->GetGlobalAlpha()) / 255.0f / 255.0f;
}

/**
 * @brief Compute the projection matrix used to render into a capture texture.
 * @param pProjection Receives the projection matrix.
 * @param rPos Left-top position of the captured area.
 * @param rSize Size of the captured area.
 */
void CalculateCaptureProjection(nn::util::MatrixT4x4fType* pProjection,
                                const nn::util::Float2& rPos, const Size& rSize) {
    const float left = rPos.x;
    const float top = rPos.y;
    const float right = left + rSize.width;
    const float bottom = top - rSize.height;

    const float rcpWidth = 1.0f / (right - left);
    const float translateX = -((left + right) * rcpWidth);
    const float scaleX = rcpWidth + rcpWidth;
    pProjection->_m.val[0] =
        vcombine_f32(float32x2_t{scaleX, 0.0f}, float32x2_t{0.0f, translateX});

    const float rcpHeight = 1.0f / (bottom - top);
    const float translateY = -((top + bottom) * rcpHeight);
    const float scaleY = rcpHeight + rcpHeight;
    pProjection->_m.val[1] =
        vcombine_f32(float32x2_t{0.0f, scaleY}, float32x2_t{0.0f, translateY});
    pProjection->_m.val[2] = float32x4_t{0.0f, 0.0f, -0.002f, -0.0f};
    pProjection->_m.val[3] = float32x4_t{0.0f, 0.0f, 0.0f, 1.0f};
}

/**
 * @brief Load the projection and model view matrices of a capture into a constant buffer.
 * @param pConstantBuffer Constant buffer receiving the matrices.
 * @param rProjection Projection matrix of the capture.
 * @param rRootMtx Model view matrix of the capture root.
 */
void SetCaptureMatrices(VertexShaderConstantBuffer* pConstantBuffer,
                        const nn::util::MatrixT4x4fType& rProjection,
                        const nn::util::MatrixT4x3fType& rRootMtx) {
    float32x4_t* pProjection = reinterpret_cast<float32x4_t*>(pConstantBuffer->projection);
    const float32x4_t projection0 = rProjection._m.val[0];
    const float32x4_t projection1 = rProjection._m.val[1];
    const float32x4_t projection2 = rProjection._m.val[2];
    const float32x4_t projection3 = rProjection._m.val[3];
    pProjection[0] = projection0;
    pProjection[1] = projection1;
    pProjection[2] = projection2;
    pProjection[3] = projection3;

    float32x4_t* pModelView = reinterpret_cast<float32x4_t*>(pConstantBuffer->modelView);
    const float32x4_t modelView0 = rRootMtx._m.val[0];
    const float32x4_t modelView1 = rRootMtx._m.val[1];
    const float32x4_t modelView2 = rRootMtx._m.val[2];
    pModelView[0] = modelView0;
    pModelView[1] = modelView1;
    pModelView[2] = modelView2;
}

/**
 * @brief Describe a render target texture used by a pane effect.
 * @param pTextureInfo Receives the description.
 * @param format Image format of the texture.
 * @param rSize Size of the texture in pixels.
 */
void SetupRenderTargetInfo(nn::gfx::TextureInfo* pTextureInfo, nn::gfx::ImageFormat format,
                           const Size& rSize) {
    pTextureInfo->SetDefault();
    pTextureInfo->SetWidth(static_cast<int>(rSize.width));
    pTextureInfo->SetHeight(static_cast<int>(rSize.height));
    pTextureInfo->SetImageFormat(format);
    pTextureInfo->SetGpuAccessFlags(nn::gfx::GpuAccess_Texture | nn::gfx::GpuAccess_ColorBuffer);
    pTextureInfo->SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
    pTextureInfo->SetMipCount(1);
}

/**
 * @brief Describe a render target texture holding a blurred drop shadow layer.
 * @param pTextureInfo Receives the description.
 * @param format Image format of the texture.
 * @param rSize Size of the captured image.
 * @param padding Padding added around the captured image for the blur.
 */
void SetupBlurRenderTargetInfo(nn::gfx::TextureInfo* pTextureInfo, nn::gfx::ImageFormat format,
                               const Size& rSize, int padding) {
    pTextureInfo->SetDefault();
    pTextureInfo->SetWidth(static_cast<int>(rSize.width + static_cast<float>(padding)));
    pTextureInfo->SetHeight(static_cast<int>(rSize.height + static_cast<float>(padding)));
    pTextureInfo->SetImageFormat(format);
    pTextureInfo->SetGpuAccessFlags(nn::gfx::GpuAccess_Texture | nn::gfx::GpuAccess_ColorBuffer);
    pTextureInfo->SetImageStorageDimension(nn::gfx::ImageStorageDimension_2d);
    pTextureInfo->SetMipCount(1);
}

/**
 * @brief Destroy a render target texture created by CreateRenderTarget.
 * @param pDevice Device that created the texture.
 * @param ppTarget Texture to destroy; reset to nullptr.
 */
void DestroyRenderTarget(nn::gfx::Device* pDevice, RenderTargetTextureInfo** ppTarget) {
    if (*ppTarget != nullptr) {
        if ((*ppTarget)->IsValid()) {
            (*ppTarget)->Finalize(pDevice);
        }

        Layout::FreeMemory(*ppTarget);
        *ppTarget = nullptr;
    }
}

/**
 * @brief Make the contents rendered into textures visible to later texture reads.
 * @param rCommands Command buffer to record into.
 */
void EndRenderToTexture(nn::gfx::CommandBuffer& rCommands) {
    rCommands.FlushMemory(nn::gfx::GpuAccess_ColorBuffer);
    rCommands.InvalidateMemory(nn::gfx::GpuAccess_Texture);
}

}  // namespace

/** @brief Construct a pane effect without any function. */
PaneEffect::PaneEffect() : mPane(nullptr) {
    std::memset(&m_Mask, 0, PaneEffectStateSize);
    mFlags = 0;
}

/**
 * @brief Initialize the effect functions of a pane from its system extended user data.
 * @param pResult Receives the required resource sizes.
 * @param pDevice Device creating the resources.
 * @param pPane Pane the effect is applied to.
 * @param rArgs Build arguments of the pane.
 */
void PaneEffect::Initialize(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                            Pane* pPane, const BuildArgSet& rArgs) {
    mPane = pPane;
    std::memset(&m_Mask, 0, sizeof(m_Mask) + sizeof(m_DropShadow));

    const SystemDataMaskTexture* pMask = static_cast<const SystemDataMaskTexture*>(
        pPane->GetSystemExtDataByType(PaneSystemDataType_Mask));
    const SystemDataDropShadow* pDropShadow = static_cast<const SystemDataDropShadow*>(
        mPane->GetSystemExtDataByType(PaneSystemDataType_DropShadow));

    if (pMask != nullptr) {
        m_Mask.pData = pMask;
        InitializeMaskFunction(pResult, pDevice, pMask, rArgs);
    }

    float scale = 1.0f;
    if (pDropShadow != nullptr) {
        m_DropShadow.pData = pDropShadow;
        InitializeDropShadowFunction(pDevice, pDropShadow, rArgs);
        scale = CalculateCaptureTextureScale(
            FindCaptureTextureResource(rArgs, pDropShadow->captureTextureIndex));
    }

    InitializeCommonShader(pDevice, rArgs.m_pPartsLayout, pMask, pDropShadow);
    CreatePaneEffectTempTextures(pDevice, rArgs.m_pPartsLayout, pMask, pDropShadow, scale);

    m_Mask.pData = nullptr;
    m_DropShadow.pData = nullptr;
}

/**
 * @brief Initialize the mask function.
 * @param pResult Receives the required resource sizes.
 * @param pDevice Device creating the resources.
 * @param pMask Mask parameters of the pane.
 * @param rArgs Build arguments of the pane.
 */
void PaneEffect::InitializeMaskFunction(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                        const SystemDataMaskTexture* pMask,
                                        const BuildArgSet& rArgs) {
    const BuildResSet* pResSet = rArgs.pCurrentBuildResSet;
    const u32* pTextureNameOffsets = GetTextureNameOffsets(pResSet->pTextureList);
    m_Mask.pData = pMask;

    if ((pMask->captureFlags & SystemDataMaskTexture::CaptureFlag_UseCaptureTexture) != 0) {
        CaptureTexture* pCaptureTexture = CreateCaptureTexture(rArgs, pMask->captureTextureIndex);
        m_Mask.captureTexMap.Set(static_cast<const TextureInfo*>(pCaptureTexture->_08));
    }

    m_Mask.captureTexMap.SetWrapMode(static_cast<TexWrap>(pMask->captureWrapAndFilterS & 3),
                                     static_cast<TexWrap>(pMask->captureWrapAndFilterT & 3));
    m_Mask.captureTexMap.SetFilter(
        static_cast<TexFilter>((pMask->captureWrapAndFilterS >> 2) & 3),
        static_cast<TexFilter>((pMask->captureWrapAndFilterT >> 2) & 3));

    const char* pTextureName = GetTextureName(pTextureNameOffsets, pMask->textureIndex);
    if ((pMask->textureFlags & SystemDataMaskTexture::TextureFlag_CaptureTexture) != 0) {
        CaptureTexture* pCaptureTexture =
            rArgs.pBuildPaneTreeContext->GetCurrentTextureShareInfo()->FindCaptureTexture(
                pTextureName);
        if (pCaptureTexture == nullptr) {
            pCaptureTexture = CreateCaptureTexture(rArgs, pMask->textureIndex);
            mFlags |= Flag_MaskTextureOwner;
        }

        m_Mask.maskTexMap.Set(static_cast<const TextureInfo*>(pCaptureTexture->_08));
    } else if ((pMask->textureFlags & SystemDataMaskTexture::TextureFlag_VectorGraphicsTexture) !=
               0) {
        DynamicTextureShareInfo* pShareInfo =
            rArgs.pBuildPaneTreeContext->GetCurrentTextureShareInfo();
        VectorGraphicsTexture* pTexture = pShareInfo->CreateVectorGraphicsTexture(pTextureName);
        pShareInfo->InitializeVectorGraphicsTexture(pResult, pDevice, rArgs.m_pPartsLayout,
                                                    pResSet->pResAccessor, pTextureName);
        m_Mask.maskTexMap.Set(static_cast<const TextureInfo*>(pTexture->_08));
        mFlags |= Flag_MaskTextureOwner;
    } else {
        m_Mask.maskTexMap.Set(pResSet->pResAccessor->AcquireTexture(pDevice, pTextureName));
    }

    m_Mask.maskTexMap.SetWrapMode(static_cast<TexWrap>(pMask->wrapAndFilterS & 3),
                                  static_cast<TexWrap>(pMask->wrapAndFilterT & 3));
    m_Mask.maskTexMap.SetFilter(static_cast<TexFilter>((pMask->wrapAndFilterS >> 2) & 3),
                                static_cast<TexFilter>((pMask->wrapAndFilterT >> 2) & 3));

    const u32 key = MaskShaderVariationKey;
    m_Mask.pShader =
        rArgs.m_pPartsLayout->AcquireArchiveShader(pDevice, ArchiveShaderSignatureMask, 1, &key);
    m_Mask.shaderVariation = SearchShaderVariationIndexFromTable(
        m_Mask.pShader->m_pVariationTable, ArchiveShaderSignatureMask, 1, &key);
}

/**
 * @brief Initialize the drop shadow function.
 * @param pDevice Device creating the resources.
 * @param pDropShadow Drop shadow parameters of the pane.
 * @param rArgs Build arguments of the pane.
 */
void PaneEffect::InitializeDropShadowFunction(nn::gfx::Device* pDevice,
                                              const SystemDataDropShadow* pDropShadow,
                                              const BuildArgSet& rArgs) {
    m_DropShadow.pData = pDropShadow;

    CaptureTexture* pCaptureTexture = CreateCaptureTexture(rArgs, pDropShadow->captureTextureIndex);
    m_DropShadow.captureTexMap.Set(static_cast<const TextureInfo*>(pCaptureTexture->_08));
    m_DropShadow.captureTexMap.SetWrapMode(
        static_cast<TexWrap>(pDropShadow->captureWrapAndFilterS & 3),
        static_cast<TexWrap>(pDropShadow->captureWrapAndFilterT & 3));
    m_DropShadow.captureTexMap.SetFilter(
        static_cast<TexFilter>((pDropShadow->captureWrapAndFilterS >> 2) & 3),
        static_cast<TexFilter>((pDropShadow->captureWrapAndFilterT >> 2) & 3));
    m_DropShadow.pCaptureTextureInfo = pCaptureTexture->_20;

    const bool isHighQuality =
        (m_DropShadow.pData->flags & SystemDataDropShadow::Flag_HighQualityBlur) != 0;
    const u32 key =
        DropShadowShaderVariationKeys[isHighQuality ? DropShadowHighQualityVariationIndex : 0];
    m_DropShadow.pShader = rArgs.m_pPartsLayout->AcquireArchiveShader(
        pDevice, ArchiveShaderSignatureDropShadow, 1, &key);

    const void* pVariationTable = m_DropShadow.pShader->m_pVariationTable;
    for (int i = 0; i < DropShadowShaderVariationCount; i++) {
        const u32 variationKey = DropShadowShaderVariationKeys[i];
        m_DropShadow.shaderVariations[i] = SearchShaderVariationIndexFromTable(
            pVariationTable, ArchiveShaderSignatureDropShadow, 1, &variationKey);
        if (m_DropShadow.shaderVariations[i] >= 0) {
            m_DropShadow.pixelShaderSlots[i] =
                m_DropShadow.pShader->GetPixelShader(m_DropShadow.shaderVariations[i])
                    ->GetInterfaceSlot(nn::gfx::ShaderStage_Pixel,
                                       nn::gfx::ShaderInterfaceType_ConstantBuffer,
                                       "uDropShadowBlur");
        }
    }
}

/**
 * @brief Find the capture texture resource of a texture of the layout.
 * @param rArgs Build arguments holding the resource tables.
 * @param textureIndex Index of the texture.
 * @return The capture texture resource, or nullptr when there is none.
 */
const ResCaptureTexture* PaneEffect::FindCaptureTextureResource(const BuildArgSet& rArgs,
                                                                int textureIndex) const {
    const char* pName = GetTextureName(rArgs, textureIndex);
    return detail::FindCaptureTextureResource(
        static_cast<const ResCaptureTextureList*>(rArgs.pCurrentBuildResSet->pCaptureTextureList),
        pName);
}

/**
 * @brief Acquire the shader drawing the result image of the effect.
 * @param pDevice Device creating the resources.
 * @param pLayout Layout owning the shader.
 * @param pMask Mask parameters of the pane, may be nullptr.
 * @param pDropShadow Drop shadow parameters of the pane, may be nullptr.
 */
void PaneEffect::InitializeCommonShader(nn::gfx::Device* pDevice, const Layout* pLayout,
                                        const SystemDataMaskTexture* pMask,
                                        const SystemDataDropShadow* pDropShadow) {
    if (pMask != nullptr) {
        const u32 key = MaskCommonShaderVariationKey;
        m_pCommonShader =
            pLayout->AcquireArchiveShader(pDevice, ArchiveShaderSignatureMask, 1, &key);
        m_CommonShaderVariation = SearchShaderVariationIndexFromTable(
            m_pCommonShader->m_pVariationTable, ArchiveShaderSignatureMask, 1, &key);
    } else if (pDropShadow != nullptr) {
        const u32 key = DropShadowCommonShaderVariationKey;
        m_pCommonShader =
            pLayout->AcquireArchiveShader(pDevice, ArchiveShaderSignatureDropShadow, 1, &key);
        m_CommonShaderVariation = SearchShaderVariationIndexFromTable(
            m_pCommonShader->m_pVariationTable, ArchiveShaderSignatureDropShadow, 1, &key);
    }

    m_CommonShaderTextureSlot = GetTextureSlot(m_pCommonShader, m_CommonShaderVariation, 0);
}

/**
 * @brief Create the render targets the effect renders into.
 * @param pDevice Device creating the textures.
 * @param pLayout Layout owning the textures.
 * @param pMask Mask parameters of the pane, may be nullptr.
 * @param pDropShadow Drop shadow parameters of the pane, may be nullptr.
 * @param scale Scale of the captured image of the drop shadow.
 */
void PaneEffect::CreatePaneEffectTempTextures(nn::gfx::Device* pDevice, const Layout* pLayout,
                                              const SystemDataMaskTexture* pMask,
                                              const SystemDataDropShadow* pDropShadow,
                                              float scale) {
    Size size;
    mPane->GetSizeWithCaptureEffect(&size);

    if (pMask != nullptr &&
        (pDropShadow != nullptr || (pMask->flags & SystemDataMaskTexture::Flag_StaticCache) != 0)) {
        m_Mask.pCaptureTarget = Layout::NewObj<RenderTargetTextureInfo>();
        nn::gfx::TextureInfo textureInfo;
        SetupRenderTargetInfo(&textureInfo, nn::gfx::ImageFormat_R8_G8_B8_A8_Unorm, size);
        m_Mask.pCaptureTarget->Initialize(pDevice, pLayout, textureInfo,
                                          static_cast<RenderTargetTextureLifetime>(0));
    } else {
        m_Mask.pCaptureTarget = nullptr;
    }

    size.width *= scale;
    size.height *= scale;

    if (pDropShadow != nullptr) {
        m_DropShadow.pBlurTarget = Layout::NewObj<RenderTargetTextureInfo>();
        const int padding = pDropShadow->blurPaddingSize * 2;
        nn::gfx::TextureInfo textureInfo;
        SetupBlurRenderTargetInfo(&textureInfo, nn::gfx::ImageFormat_R8_Unorm, size, padding);
        m_DropShadow.pBlurTarget->Initialize(pDevice, pLayout, textureInfo,
                                             static_cast<RenderTargetTextureLifetime>(0));
    }

    if (pDropShadow == nullptr ||
        (pDropShadow->flags & SystemDataDropShadow::Flag_StaticCache) == 0) {
        return;
    }

    for (int i = 0; i < SystemDataDropShadow::DropShadowType_Max; i++) {
        if ((pDropShadow->flags & (1 << i)) != 0) {
            m_DropShadow.pStaticCacheTargets[i] = Layout::NewObj<RenderTargetTextureInfo>();
            const int padding = pDropShadow->blurPaddingSize * 2;
            nn::gfx::TextureInfo textureInfo;
            SetupBlurRenderTargetInfo(&textureInfo, nn::gfx::ImageFormat_R8_G8_B8_A8_Unorm,
                                      size, padding);
            m_DropShadow.pStaticCacheTargets[i]->Initialize(
                pDevice, pLayout, textureInfo, static_cast<RenderTargetTextureLifetime>(0));
        } else {
            m_DropShadow.pStaticCacheTargets[i] = nullptr;
        }
    }
}

/**
 * @brief Copy the effect of another pane.
 * @param rSource Effect to copy.
 * @param pPane Pane the effect is applied to.
 * @param pDevice Device creating the resources.
 * @param pLayout Layout owning the resources.
 * @param pContext Context sharing the dynamic textures of the copied pane tree.
 */
PaneEffect::PaneEffect(const PaneEffect& rSource, Pane* pPane, nn::gfx::Device* pDevice,
                       const Layout* pLayout, BuildPaneTreeContext* pContext)
    : mPane(pPane) {
    std::memset(&m_Mask, 0, PaneEffectStateSize);

    const SystemDataMaskTexture* pMask = static_cast<const SystemDataMaskTexture*>(
        mPane->GetSystemExtDataByType(PaneSystemDataType_Mask));
    const SystemDataDropShadow* pDropShadow = static_cast<const SystemDataDropShadow*>(
        mPane->GetSystemExtDataByType(PaneSystemDataType_DropShadow));
    DynamicTextureShareInfo* pShareInfo = pContext->GetCurrentTextureShareInfo();

    if (pMask != nullptr) {
        m_Mask = rSource.m_Mask;

        if ((pMask->captureFlags & SystemDataMaskTexture::CaptureFlag_UseCaptureTexture) != 0) {
            CaptureTexture* pSourceTexture = DynamicCast<CaptureTexture*>(
                static_cast<DynamicRenderingTexture*>(
                    rSource.m_Mask.captureTexMap.m_pTextureInfo->GetPrivateTextureInstancePtr()));
            CaptureTexture* pTexture = pShareInfo->CopyCaptureTexture(nullptr, pSourceTexture);
            m_Mask.captureTexMap.Set(static_cast<const TextureInfo*>(pTexture->_08));
        }

        if ((pMask->textureFlags & SystemDataMaskTexture::TextureFlag_CaptureTexture) != 0) {
            bool isCreated = false;
            CaptureTexture* pSourceTexture = DynamicCast<CaptureTexture*>(
                static_cast<DynamicRenderingTexture*>(
                    rSource.m_Mask.maskTexMap.m_pTextureInfo->GetPrivateTextureInstancePtr()));
            CaptureTexture* pTexture = pShareInfo->CopyCaptureTexture(&isCreated, pSourceTexture);
            m_IsMaskTextureOwner = isCreated;
            m_Mask.maskTexMap.Set(static_cast<const TextureInfo*>(pTexture->_08));
        } else if ((pMask->textureFlags &
                    SystemDataMaskTexture::TextureFlag_VectorGraphicsTexture) != 0) {
            VectorGraphicsTexture* pSourceTexture = DynamicCast<VectorGraphicsTexture*>(
                static_cast<DynamicRenderingTexture*>(
                    rSource.m_Mask.maskTexMap.m_pTextureInfo->GetPrivateTextureInstancePtr()));
            VectorGraphicsTexture* pTexture =
                pShareInfo->CopyVectorGraphicsTexture(pDevice, pLayout, pSourceTexture);
            m_Mask.maskTexMap.Set(static_cast<const TextureInfo*>(pTexture->_08));
            mFlags |= Flag_MaskTextureOwner;
        }

        m_Mask.isStaticCacheUpdated = false;
    }

    float scale = 1.0f;
    if (pDropShadow != nullptr) {
        m_DropShadow = rSource.m_DropShadow;

        CaptureTexture* pSourceTexture = DynamicCast<CaptureTexture*>(
            static_cast<DynamicRenderingTexture*>(
                rSource.m_DropShadow.captureTexMap.m_pTextureInfo->GetPrivateTextureInstancePtr()));
        CaptureTexture* pTexture = pShareInfo->CopyCaptureTexture(nullptr, pSourceTexture);
        m_DropShadow.captureTexMap.Set(static_cast<const TextureInfo*>(pTexture->_08));
        m_DropShadow.pCaptureTextureInfo = pTexture->_20;
        scale = pSourceTexture->_28;
        m_DropShadow.isStaticCacheUpdated = false;
    }

    InitializeCommonShader(pDevice, pLayout, pMask, pDropShadow);
    CreatePaneEffectTempTextures(pDevice, pLayout, pMask, pDropShadow, scale);

    m_Mask.pData = nullptr;
    m_DropShadow.pData = nullptr;
}

/**
 * @brief Access the dynamic texture instance behind a texture map.
 * @param rTexMap Texture map to query.
 * @return The dynamic rendering texture of the map, or nullptr.
 */
void* PaneEffect::GetPrivateTexturePtr(const TexMap& rTexMap) {
    return (rTexMap.m_pTextureInfo != nullptr) ?
               rTexMap.m_pTextureInfo->GetPrivateTextureInstancePtr() :
               nullptr;
}

/**
 * @brief Destroy the dynamic texture behind a texture map.
 * @param pDevice Device that created the texture.
 * @param rTexMap Texture map to release.
 */
void PaneEffect::FinalizeTexMapDynamicRenderingTexture(nn::gfx::Device* pDevice,
                                                       const TexMap& rTexMap) {
    DynamicRenderingTexture* pTexture =
        static_cast<DynamicRenderingTexture*>(GetPrivateTexturePtr(rTexMap));
    if (pTexture != nullptr) {
        pTexture->Finalize(pDevice);
        Layout::DeleteObj(pTexture);
    }
}

/**
 * @brief Destroy the dynamic textures owned by the effect.
 * @param pDevice Device that created the textures.
 */
void PaneEffect::FinalizeDynamicRenderingTexture(nn::gfx::Device* pDevice) {
    FinalizeTexMapDynamicRenderingTexture(pDevice, m_Mask.captureTexMap);

    if ((mFlags & Flag_MaskTextureOwner) != 0) {
        FinalizeTexMapDynamicRenderingTexture(pDevice, m_Mask.maskTexMap);
    }

    FinalizeTexMapDynamicRenderingTexture(pDevice, m_DropShadow.captureTexMap);
}

/**
 * @brief Destroy the resources of the effect.
 * @param pDevice Device that created the resources.
 */
void PaneEffect::Finalize(nn::gfx::Device* pDevice) {
    FinalizeDynamicRenderingTexture(pDevice);

    DestroyRenderTarget(pDevice, &m_Mask.pCaptureTarget);
    for (int i = 0; i < SystemDataDropShadow::DropShadowType_Max; i++) {
        DestroyRenderTarget(pDevice, &m_DropShadow.pStaticCacheTargets[i]);
    }

    DestroyRenderTarget(pDevice, &m_DropShadow.pBlurTarget);
}

/**
 * @brief Compute the constant buffers of the effect.
 * @param rDrawInfo Draw state.
 */
void PaneEffect::Calculate(DrawInfo& rDrawInfo) {
    const nn::util::MatrixT4x3fType globalMtx = mPane->GetGlobalMatrix();

    if (m_Mask.pData == nullptr &&
        mPane->GetSystemExtDataByType(PaneSystemDataType_Mask) != nullptr) {
        m_Mask.pData = static_cast<const SystemDataMaskTexture*>(
            mPane->GetSystemExtDataByType(PaneSystemDataType_Mask));
    }

    if (m_DropShadow.pData == nullptr &&
        mPane->GetSystemExtDataByType(PaneSystemDataType_DropShadow) != nullptr) {
        m_DropShadow.pData = static_cast<const SystemDataDropShadow*>(
            mPane->GetSystemExtDataByType(PaneSystemDataType_DropShadow));
    }

    if (m_Mask.pData != nullptr) {
        rDrawInfo.m_ModelViewMtx = globalMtx;
        rDrawInfo.mModelViewLoaded = false;
        CalculateMaskConstantBuffer(rDrawInfo);

        VectorGraphicsTexture* pTexture = DynamicCast<VectorGraphicsTexture*>(
            static_cast<DynamicRenderingTexture*>(GetPrivateTexturePtr(m_Mask.maskTexMap)));
        if (pTexture != nullptr) {
            pTexture->SetUpdateRequested();
        }
    }

    if (m_DropShadow.pData != nullptr) {
        rDrawInfo.m_ModelViewMtx = globalMtx;
        rDrawInfo.mModelViewLoaded = false;
        CalculateDropShadowConstantBuffer(rDrawInfo);
    }
}

/**
 * @brief Compute the constant buffers drawing the masked image.
 * @param rDrawInfo Draw state.
 */
void PaneEffect::CalculateMaskConstantBuffer(DrawInfo& rDrawInfo) {
    const u32 offset = AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
    m_Mask.vertexShaderOffset = offset;

    u8* pMappedPointer = static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer());
    if (pMappedPointer != nullptr) {
        VertexShaderConstantBuffer* pConstantBuffer =
            reinterpret_cast<VertexShaderConstantBuffer*>(pMappedPointer + offset);

        if (!IsMaskStaticCacheEnabled() || !m_Mask.isStaticCacheUpdated) {
            ApplyMaskParameters(pConstantBuffer);
        }

        if (IsMaskStaticCacheEnabled()) {
            pConstantBuffer->color[3] = GetCachedImageAlpha(mPane);
        }

        const nn::util::Float2 basePos = mPane->GetVertexPos();
        CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, basePos, mPane->GetSize(), 1,
                                   QuadTexCoords);
    }

    if (m_DropShadow.pData != nullptr ||
        (IsMaskStaticCacheEnabled() && !m_Mask.isStaticCacheUpdated)) {
        const int captureOffset = AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
        m_Mask.captureVertexShaderOffset = captureOffset;
        CalculateMaskCaptureDrawConstantBuffer(rDrawInfo, captureOffset);
    }
}

/**
 * @brief Compute the constant buffers drawing the drop shadow.
 * @param rDrawInfo Draw state.
 */
void PaneEffect::CalculateDropShadowConstantBuffer(DrawInfo& rDrawInfo) {
    m_DropShadow.vertexShaderOffset = AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
    for (int i = 0; i < SystemDataDropShadow::DropShadowType_Max; i++) {
        m_DropShadow.blurLevels[i] = -1;
    }

    if ((m_DropShadow.pData->flags & SystemDataDropShadow::Flag_StrokeEnabled) != 0 &&
        m_DropShadow.pData->strokeColor.w > 0.0f) {
        m_DropShadow.blurLevels[SystemDataDropShadow::DropShadowType_Stroke] =
            MakeDropShadowConstantBufferSet(rDrawInfo, SystemDataDropShadow::DropShadowType_Stroke);
    }

    if ((m_DropShadow.pData->flags & SystemDataDropShadow::Flag_OuterGlowEnabled) != 0 &&
        m_DropShadow.pData->outerGlowColor.w > 0.0f) {
        m_DropShadow.blurLevels[SystemDataDropShadow::DropShadowType_OuterGlow] =
            MakeDropShadowConstantBufferSet(rDrawInfo,
                                            SystemDataDropShadow::DropShadowType_OuterGlow);
    }

    if ((m_DropShadow.pData->flags & SystemDataDropShadow::Flag_DropShadowEnabled) != 0 &&
        m_DropShadow.pData->dropShadowColor.w > 0.0f) {
        m_DropShadow.blurLevels[SystemDataDropShadow::DropShadowType_DropShadow] =
            MakeDropShadowConstantBufferSet(rDrawInfo,
                                            SystemDataDropShadow::DropShadowType_DropShadow);
    }

    VertexShaderConstantBuffer* pConstantBuffer = reinterpret_cast<VertexShaderConstantBuffer*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
        m_DropShadow.vertexShaderOffset);
    if (IsDropShadowStaticCacheEnabled()) {
        pConstantBuffer->color[3] = GetCachedImageAlpha(mPane);
    }

    Size size;
    mPane->GetSizeWithCaptureEffect(&size);
    nn::util::Float2 basePos;
    CalcuVertexPosOfPaneParam(&basePos);
    CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, basePos, size, 1, QuadTexCoords);
}

/**
 * @brief Draw the effect and the image of the pane.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    CaptureMaskedImage(rDrawInfo, rCommands);

    if (m_DropShadow.pData != nullptr) {
        DrawDropShadow(rDrawInfo, rCommands);
        if (IsDropShadowStaticCacheEnabled() && m_DropShadow.isStaticCacheUpdated) {
            DrawStaticCachedShadow(rDrawInfo, rCommands);
        }
    }

    if (m_Mask.pData != nullptr) {
        if (IsDropShadowOnlyEnabled()) {
            return;
        }

        if (IsMaskStaticCacheEnabled() && m_Mask.isStaticCacheUpdated) {
            const nn::gfx::DescriptorSlot samplerSlot =
                rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
                    TexWrap_Clamp, TexWrap_Clamp, TexFilter_Linear, TexFilter_Linear);
            DrawImage(rDrawInfo, rCommands, m_Mask.vertexShaderOffset,
                      m_Mask.pCaptureTarget->mDescriptor, samplerSlot);
        } else {
            DrawMaskedImage(rDrawInfo, rCommands);
        }
    } else if (m_DropShadow.pData != nullptr && !IsDropShadowOnlyEnabled()) {
        const nn::gfx::DescriptorSlot samplerSlot =
            rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
                TexWrap_Clamp, TexWrap_Clamp, TexFilter_Linear, TexFilter_Linear);
        DrawImage(rDrawInfo, rCommands, m_DropShadow.vertexShaderOffset,
                  m_DropShadow.pCaptureTextureInfo->mDescriptor, samplerSlot);
    }
}

/**
 * @brief Render the masked image of the pane into its capture texture.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::CaptureMaskedImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (m_Mask.pData == nullptr) {
        return;
    }

    if (IsMaskStaticCacheEnabled()) {
        if (m_Mask.isStaticCacheUpdated) {
            return;
        }
    } else if (m_DropShadow.pData == nullptr) {
        return;
    }

    nn::gfx::ClearColorValue clearColor = {};
    const nn::gfx::ColorTargetView* pColorTarget = &m_Mask.pCaptureTarget->mColorTarget;
    ToImpl(rCommands).ClearColorTarget(ToImpl(const_cast<nn::gfx::ColorTargetView*>(pColorTarget)),
                                       clearColor, nullptr);
    ToImpl(rCommands).SetRenderTargets(1, ToImpl(&pColorTarget), nullptr);
    ToImpl(rCommands).SetViewportScissorState(
        ToImpl(&m_Mask.pCaptureTarget->mViewportScissorState));
    SetupMaskShader(rDrawInfo, rCommands);
    DrawCommonImpl(rDrawInfo, rCommands, m_Mask.pShader, m_Mask.shaderVariation,
                   m_Mask.captureVertexShaderOffset);
    EndRenderToTexture(rCommands);
    rDrawInfo.ResetRenderTarget(rCommands);

    if (IsMaskStaticCacheEnabled()) {
        m_Mask.isStaticCacheUpdated = true;
    }
}

/**
 * @brief Draw the blurred layers of the drop shadow.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::DrawDropShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (IsDropShadowStaticCacheEnabled() && m_DropShadow.isStaticCacheUpdated) {
        return;
    }

    const nn::gfx::ColorTargetView* pColorTarget = rDrawInfo.m_pColorTarget;
    const nn::gfx::ScissorStateInfo scissor = rDrawInfo.mScissor;
    const nn::gfx::ViewportStateInfo viewport = rDrawInfo.mViewport;

    for (int i = SystemDataDropShadow::DropShadowType_Max - 1; i >= 0; i--) {
        const DropShadowType type = static_cast<DropShadowType>(i);
        if ((m_DropShadow.pData->flags & (1 << type)) == 0 || m_DropShadow.blurLevels[type] < 0) {
            continue;
        }

        nn::gfx::ScissorStateInfo targetScissor;
        nn::gfx::ViewportStateInfo targetViewport;
        PresetBlendStateId blendStateId;
        if (IsDropShadowStaticCacheEnabled()) {
            nn::gfx::ColorTargetView* pTargetView =
                &m_DropShadow.pStaticCacheTargets[type]->mColorTarget;

            nn::gfx::ClearColorValue clearColor;
            clearColor.valueUint[0] = 0xff;
            clearColor.valueUint[1] = 0xff;
            clearColor.valueUint[2] = 0xff;
            clearColor.valueUint[3] = 0xff;
            ToImpl(rCommands).ClearColorTarget(ToImpl(pTargetView), clearColor, nullptr);

            const TextureSize textureSize = m_DropShadow.pStaticCacheTargets[type]->GetSize();
            SetupViewportScissorStateInfo(&targetViewport, &targetScissor,
                                          static_cast<float>(textureSize.width),
                                          static_cast<float>(textureSize.height));
            rDrawInfo.m_pColorTarget = pTargetView;
            rDrawInfo.mViewport = targetViewport;
            rDrawInfo.mScissor = targetScissor;
            blendStateId = PresetBlendStateId_OpaqueOrAlphaTest;
        } else {
            blendStateId = ConvertBlendType(
                static_cast<DropShadowBlendMode>(m_DropShadow.pData->blendMode[type]));
        }

        DrawBluredShadow(
            rDrawInfo, rCommands, type, blendStateId,
            (m_DropShadow.pData->flags & SystemDataDropShadow::Flag_KnockoutEnabled) != 0);
    }

    if (IsDropShadowStaticCacheEnabled()) {
        EndRenderToTexture(rCommands);
        m_DropShadow.isStaticCacheUpdated = true;
    }

    rDrawInfo.m_pColorTarget = pColorTarget;
    rDrawInfo.mViewport = viewport;
    rDrawInfo.mScissor = scissor;
    rDrawInfo.ResetRenderTarget(rCommands);
}

/**
 * @brief Draw the drop shadow layers rendered into the static caches.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::DrawStaticCachedShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    for (int i = SystemDataDropShadow::DropShadowType_Max - 1; i >= 0; i--) {
        if ((m_DropShadow.pData->flags & (1 << i)) == 0) {
            continue;
        }

        const PresetBlendStateId blendStateId =
            ConvertBlendType(static_cast<DropShadowBlendMode>(m_DropShadow.pData->blendMode[i]));
        const int variation =
            blendStateId == PresetBlendStateId_Multiplication ?
                m_DropShadow.shaderVariations[DropShadowStaticMultiplyVariationIndex] :
                m_DropShadow.shaderVariations[DropShadowStaticVariationIndex];
        const ShaderInfo* pShader = m_DropShadow.pShader;
        SetupShaderWithShaderCache(rDrawInfo, rCommands, pShader, variation);

        const nn::gfx::DescriptorSlot samplerSlot =
            rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
                TexWrap_Clamp, TexWrap_Clamp, TexFilter_Linear, TexFilter_Linear);
        rCommands.SetTextureAndSampler(GetFirstTextureSlot(pShader, variation),
                                       nn::gfx::ShaderStage_Pixel,
                                       m_DropShadow.pStaticCacheTargets[i]->mDescriptor,
                                       samplerSlot);
        rCommands.SetBlendState(
            const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                ->GetPresetBlendState(blendStateId));
        DrawCommonImpl(rDrawInfo, rCommands, pShader, variation,
                       m_DropShadow.staticRenderingOffsets[i]);
    }
}

/**
 * @brief Draw an image of the pane with the common shader.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 * @param vertexShaderOffset Offset of the vertex shader constant buffer.
 * @param rTextureSlot Descriptor of the image.
 * @param rSamplerSlot Descriptor of the sampler of the image.
 */
void PaneEffect::DrawImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                           int vertexShaderOffset, const nn::gfx::DescriptorSlot& rTextureSlot,
                           const nn::gfx::DescriptorSlot& rSamplerSlot) {
    SetupShaderWithShaderCache(rDrawInfo, rCommands, m_pCommonShader, m_CommonShaderVariation);
    rCommands.SetTextureAndSampler(m_CommonShaderTextureSlot, nn::gfx::ShaderStage_Pixel,
                                   rTextureSlot, rSamplerSlot);
    mPane->SetupPaneEffectSourceImageRenderState(rCommands);
    DrawCommonImpl(rDrawInfo, rCommands, m_pCommonShader, m_CommonShaderVariation,
                   vertexShaderOffset);
}

/**
 * @brief Draw the image of the pane through the mask.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::DrawMaskedImage(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    SetupMaskShader(rDrawInfo, rCommands);
    mPane->SetupPaneEffectSourceImageRenderState(rCommands);
    DrawCommonImpl(rDrawInfo, rCommands, m_Mask.pShader, m_Mask.shaderVariation,
                   m_Mask.vertexShaderOffset);
}

/**
 * @brief Compute the size of the ui2d constant buffer needed by an effect.
 * @param pDevice Device the buffer is created for.
 * @param pMask Mask parameters of the pane, may be nullptr.
 * @param pDropShadow Drop shadow parameters of the pane, may be nullptr.
 * @return Required size in bytes.
 */
size_t PaneEffect::GetRequiredConstantBufferSize(nn::gfx::Device* pDevice,
                                                 const SystemDataMaskTexture* pMask,
                                                 const SystemDataDropShadow* pDropShadow) {
    size_t size = 0;
    if (pMask != nullptr) {
        size = GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                    sizeof(VertexShaderConstantBuffer));
        if (pDropShadow != nullptr ||
            (pMask->flags & SystemDataMaskTexture::Flag_StaticCache) != 0) {
            size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                         sizeof(VertexShaderConstantBuffer));
        }
    }

    if (pDropShadow != nullptr) {
        for (int i = 0; i < SystemDataDropShadow::DropShadowType_Max; i++) {
            if ((pDropShadow->flags & (1 << i)) != 0) {
                const size_t count =
                    (pDropShadow->flags & SystemDataDropShadow::Flag_StaticCache) != 0 ? 3 : 2;
                size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                             sizeof(VertexShaderConstantBuffer)) *
                        count;
                size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                             BlurPixelShaderConstantBufferSize);
            }
        }

        size += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                     sizeof(VertexShaderConstantBuffer));
    }

    return size;
}

/**
 * @brief Create the capture texture of a texture of the layout.
 * @param rArgs Build arguments holding the resource tables.
 * @param textureIndex Index of the texture.
 * @return The created capture texture.
 */
CaptureTexture* PaneEffect::CreateCaptureTexture(const BuildArgSet& rArgs,
                                                 int textureIndex) const {
    const char* pName = GetTextureName(rArgs, textureIndex);
    CaptureTexture* pTexture =
        rArgs.pBuildPaneTreeContext->GetCurrentTextureShareInfo()->CreateAndSetupCaptureTexture(
            pName);
    pTexture->m_DrawPriority = rArgs.captureNestDepth > 0 ? 1 : 3;
    return pTexture;
}

/**
 * @brief Load the texture matrix of the mask into a constant buffer.
 * @param pConstantBuffer Constant buffer receiving the matrix.
 */
void PaneEffect::ApplyMaskParameters(VertexShaderConstantBuffer* pConstantBuffer) {
    float texMtx[2][3];
    Material::CalculateTextureMtx(texMtx, GetTexSrt(m_Mask.pData), m_Mask.maskTexMap);

    if ((m_Mask.pData->flags & SystemDataMaskTexture::Flag_TextureMatrixToSlot0) != 0) {
        SetTextureMatrix(pConstantBuffer->texMtx0, texMtx);
    } else {
        SetTextureMatrix(pConstantBuffer->texMtx1, texMtx);
    }
}

/**
 * @brief Compute the constant buffer rendering the masked image into the capture texture.
 * @param rDrawInfo Draw state.
 * @param offset Offset of the vertex shader constant buffer.
 */
void PaneEffect::CalculateMaskCaptureDrawConstantBuffer(DrawInfo& rDrawInfo, int offset) {
    Size size;
    size.width = m_Mask.pCaptureTarget->GetSize().width;
    size.height = m_Mask.pCaptureTarget->GetSize().height;
    nn::util::Float2 basePos;
    basePos.x = size.width * -0.5f;
    basePos.y = size.height * 0.5f;

    VertexShaderConstantBuffer* pConstantBuffer = GetVertexShaderConstantBuffer(rDrawInfo, offset);
    ApplyMaskParameters(pConstantBuffer);
    CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, basePos, size, 1, QuadTexCoords);

    nn::util::MatrixT4x4fType projection;
    CalculateCaptureProjection(&projection, basePos, size);
    nn::util::MatrixT4x3fType rootMtx;
    CalculateCaptureRootMatrix(rootMtx, rDrawInfo);
    SetCaptureMatrices(pConstantBuffer, projection, rootMtx);
}

/**
 * @brief Bind the mask shader and its textures.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void PaneEffect::SetupMaskShader(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    SetupShaderWithShaderCache(rDrawInfo, rCommands, m_Mask.pShader, m_Mask.shaderVariation);

    const nn::gfx::DescriptorSlot maskSamplerSlot =
        rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
            static_cast<TexWrap>(m_Mask.maskTexMap.mWrapS),
            static_cast<TexWrap>(m_Mask.maskTexMap.mWrapT),
            static_cast<TexFilter>(m_Mask.maskTexMap.mMinFilter),
            static_cast<TexFilter>(m_Mask.maskTexMap.mMagFilter));
    const nn::gfx::DescriptorSlot captureSamplerSlot =
        rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
            static_cast<TexWrap>(m_Mask.captureTexMap.mWrapS),
            static_cast<TexWrap>(m_Mask.captureTexMap.mWrapT),
            static_cast<TexFilter>(m_Mask.captureTexMap.mMinFilter),
            static_cast<TexFilter>(m_Mask.captureTexMap.mMagFilter));

    if ((m_Mask.pData->flags & SystemDataMaskTexture::Flag_TextureMatrixToSlot0) != 0) {
        rCommands.SetTextureAndSampler(
            GetTextureSlot(m_Mask.pShader, m_Mask.shaderVariation, 0), nn::gfx::ShaderStage_Pixel,
            m_Mask.maskTexMap.m_pTextureInfo->mDescriptor, maskSamplerSlot);
        rCommands.SetTextureAndSampler(
            GetTextureSlot(m_Mask.pShader, m_Mask.shaderVariation, 1), nn::gfx::ShaderStage_Pixel,
            m_Mask.captureTexMap.m_pTextureInfo->mDescriptor, captureSamplerSlot);
    } else {
        rCommands.SetTextureAndSampler(
            GetTextureSlot(m_Mask.pShader, m_Mask.shaderVariation, 0), nn::gfx::ShaderStage_Pixel,
            m_Mask.captureTexMap.m_pTextureInfo->mDescriptor, captureSamplerSlot);
        rCommands.SetTextureAndSampler(
            GetTextureSlot(m_Mask.pShader, m_Mask.shaderVariation, 1), nn::gfx::ShaderStage_Pixel,
            m_Mask.maskTexMap.m_pTextureInfo->mDescriptor, maskSamplerSlot);
    }

    rCommands.SetBlendState(const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                                ->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
}

/**
 * @brief Draw a quad with a vertex shader constant buffer.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 * @param pShader Shader drawing the quad.
 * @param variation Shader variation drawing the quad.
 * @param vertexShaderOffset Offset of the vertex shader constant buffer.
 */
void PaneEffect::DrawCommonImpl(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                                const ShaderInfo* pShader, int variation, int vertexShaderOffset) {
    nn::gfx::GpuAddress address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
    address.Offset(vertexShaderOffset);
    const nn::gfx::GpuAddress& rIndexBufferAddress =
        rDrawInfo.GetGraphicsResource()->m_IndexBufferGpuAddress;
    rCommands.SetConstantBuffer(pShader->GetVertexShaderSlot(variation),
                                nn::gfx::ShaderStage_Vertex, address,
                                sizeof(VertexShaderConstantBuffer));
    rCommands.DrawIndexed(nn::gfx::PrimitiveTopology_TriangleList, nn::gfx::IndexFormat_Uint16,
                          rIndexBufferAddress, 6, 0);
}

/**
 * @brief Compute the pixel shader constant buffer of a blur.
 * @param rDrawInfo Draw state.
 * @param offset Offset of the constant buffer.
 * @param rParams Parameters of the blurred layer.
 * @return Number of bilinear samples with a significant weight.
 */
int PaneEffect::MakeBlurPixelShaderConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                                  const BlurParams& rParams) const {
    BlurPixelShaderConstantBuffer* pConstantBuffer =
        reinterpret_cast<BlurPixelShaderConstantBuffer*>(
            static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) + offset);
    std::memset(pConstantBuffer, 0, sizeof(BlurPixelShaderConstantBuffer));

    pConstantBuffer->spreadScale =
        1.0f / std::max(rParams.spread / -100.0f + 1.0f, 0.0039f);
    pConstantBuffer->color[0] = rParams.color.x / 255.0f;
    pConstantBuffer->color[1] = rParams.color.y / 255.0f;
    pConstantBuffer->color[2] = rParams.color.z / 255.0f;
    pConstantBuffer->color[3] = rParams.color.w / 255.0f;

    if (rParams.size != 0.0f) {
        float weights[BlurWeightCount] = {};
        float totalWeight = 0.0f;
        for (int i = 0; i < BlurWeightCount; i++) {
            const float x = static_cast<float>(i) / rParams.size * 2.8f;
            weights[i] = std::exp(x * -0.5f * x);
            totalWeight += i == 0 ? weights[i] : weights[i] * 2.0f;
        }

        for (int i = 0; i < BlurWeightCount; i++) {
            weights[i] /= totalWeight;
        }

        Size size;
        mPane->GetSizeWithCaptureEffect(&size);
        const float rcpWidth = 1.0f / size.width;
        const float rcpHeight =
            1.0f / (size.height + static_cast<float>(m_DropShadow.pData->blurPaddingSize * 2));

        pConstantBuffer->centerWeight = weights[0];
        for (int i = 0; i < BlurSampleCount; i++) {
            const int index = i * 2 + 1;
            if (index + 1 < BlurWeightCount) {
                const float weight = weights[index] + weights[index + 1];
                float sampleOffset = 0.0f;
                if (weight > 0.0f) {
                    sampleOffset = weights[index + 1] / weight + static_cast<float>(index);
                }

                pConstantBuffer->offsetX[i] = rcpWidth * sampleOffset;
                pConstantBuffer->offsetY[i] = rcpHeight * sampleOffset;
                pConstantBuffer->weight[i] = weight;
            } else {
                pConstantBuffer->offsetX[i] = rcpWidth * static_cast<float>(index);
                pConstantBuffer->offsetY[i] = rcpHeight * static_cast<float>(index);
                pConstantBuffer->weight[i] = weights[index];
            }
        }
    } else {
        for (int i = 0; i < BlurSampleCount; i++) {
            pConstantBuffer->weight[i] = 0.0f;
        }

        pConstantBuffer->centerWeight = 1.0f;
    }

    const u8 alpha = mPane->GetGlobalAlpha();
    pConstantBuffer->alpha = 1.0f;
    pConstantBuffer->rcpAlpha = 1.0f;
    if (alpha != 0 && !IsDropShadowStaticCacheEnabled()) {
        pConstantBuffer->alpha = static_cast<float>(alpha) / 255.0f;
        pConstantBuffer->rcpAlpha = 255.0f / static_cast<float>(alpha);
    }

    int sampleCount = 0;
    bool isSignificant = true;
    for (int i = 0; i < BlurSampleCount && isSignificant; i++) {
        isSignificant = !(pConstantBuffer->weight[i] < MinBlurWeight);
        if (isSignificant) {
            sampleCount++;
        }
    }

    return sampleCount;
}

/**
 * @brief Compute the vertex shader constant buffer of a horizontal blur pass.
 * @param rDrawInfo Draw state.
 * @param offset Offset of the constant buffer.
 * @param rPos Left-top position of the blurred area.
 * @param rSize Size of the blurred area.
 * @param rTexCoordOffset Texture coordinate padding of the blurred area.
 */
void PaneEffect::MakeHorizontalBlurConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                                  const nn::util::Float2& rPos, const Size& rSize,
                                                  const nn::util::Float2& rTexCoordOffset) const {
    VertexShaderConstantBuffer* pConstantBuffer = GetVertexShaderConstantBuffer(rDrawInfo, offset);

    nn::util::Float2 texCoords[1][4];
    texCoords[0][0].x = 0.0f - rTexCoordOffset.x;
    texCoords[0][1].x = rTexCoordOffset.x + 1.0f;
    texCoords[0][2].x = 0.0f - rTexCoordOffset.x;
    texCoords[0][3].x = rTexCoordOffset.x + 1.0f;
    texCoords[0][0].y = 0.0f - rTexCoordOffset.y;
    texCoords[0][1].y = 0.0f - rTexCoordOffset.y;
    texCoords[0][2].y = rTexCoordOffset.y + 1.0f;
    texCoords[0][3].y = rTexCoordOffset.y + 1.0f;

    CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, rPos, rSize, 1, texCoords);

    nn::util::MatrixT4x4fType projection;
    CalculateCaptureProjection(&projection, rPos, rSize);
    nn::util::MatrixT4x3fType rootMtx;
    CalculateCaptureRootMatrix(rootMtx, rDrawInfo);
    SetCaptureMatrices(pConstantBuffer, projection, rootMtx);
}

/**
 * @brief Compute the vertex shader constant buffer of a vertical blur pass.
 * @param rDrawInfo Draw state.
 * @param offset Offset of the constant buffer.
 * @param rParams Parameters of the blurred layer.
 * @param rPos Left-top position of the blurred area in the static cache.
 * @param rSize Size of the blurred area.
 * @param rTexCoordScale Ratio of the blurred area to the pane area.
 * @param rTexCoordOffset Texture coordinate padding of the blurred area.
 */
void PaneEffect::MakeVerticalBlurConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                                const BlurParams& rParams,
                                                const nn::util::Float2& rPos, const Size& rSize,
                                                const nn::util::Float2& rTexCoordScale,
                                                const nn::util::Float2& rTexCoordOffset) const {
    VertexShaderConstantBuffer* pConstantBuffer = GetVertexShaderConstantBuffer(rDrawInfo, offset);

    nn::util::Float2 drawPos = CalculateDropShadowEffectVertexPos(rSize);
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (rParams.distance > 0.0f) {
        const float angle = nn::util::DegreeToRadian(rParams.angle);
        offsetX = std::cos(angle) * rParams.distance;
        offsetY = std::sin(angle) * rParams.distance;
        drawPos.x -= offsetX;
        drawPos.y -= offsetY;
    }

    const float texOffsetX = offsetX / rSize.width * rTexCoordScale.x;
    const float texOffsetY = offsetY / rSize.height * rTexCoordScale.y;

    nn::util::Float2 texCoords[2][4] = {};
    std::memcpy(texCoords[0], QuadTexCoords[0], sizeof(texCoords[0]));
    texCoords[1][0].x = 0.0f - rTexCoordOffset.x - texOffsetX;
    texCoords[1][1].x = rTexCoordOffset.x + 1.0f - texOffsetX;
    texCoords[1][2].x = 0.0f - rTexCoordOffset.x - texOffsetX;
    texCoords[1][3].x = rTexCoordOffset.x + 1.0f - texOffsetX;
    texCoords[1][0].y = texOffsetY + (0.0f - rTexCoordOffset.y);
    texCoords[1][1].y = texOffsetY + (0.0f - rTexCoordOffset.y);
    texCoords[1][2].y = texOffsetY + (rTexCoordOffset.y + 1.0f);
    texCoords[1][3].y = texOffsetY + (rTexCoordOffset.y + 1.0f);

    rDrawInfo.m_TexCoordSrc[0] = 0;
    rDrawInfo.m_TexCoordSrc[1] = 1;

    if (!IsDropShadowStaticCacheEnabled()) {
        CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, drawPos, rSize, 2, texCoords);
    } else {
        CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, rPos, rSize, 2, texCoords);

        nn::util::MatrixT4x4fType projection;
        CalculateCaptureProjection(&projection, rPos, rSize);
        nn::util::MatrixT4x3fType rootMtx;
        CalculateCaptureRootMatrix(rootMtx, rDrawInfo);
        SetCaptureMatrices(pConstantBuffer, projection, rootMtx);
    }
}

/**
 * @brief Compute the left-top position of a drop shadow layer.
 * @param rSize Size of the layer.
 * @return Position centering the layer on the pane.
 */
nn::util::Float2 PaneEffect::CalculateDropShadowEffectVertexPos(const Size& rSize) const {
    Size paneSize;
    mPane->GetSizeWithCaptureEffect(&paneSize);
    nn::util::Float2 pos;
    CalcuVertexPosOfPaneParam(&pos);
    pos.x -= (rSize.width - paneSize.width) * 0.5f;
    pos.y += (rSize.height - paneSize.height) * 0.5f;
    return pos;
}

/**
 * @brief Compute the vertex shader constant buffer drawing a static cache.
 * @param rDrawInfo Draw state.
 * @param offset Offset of the constant buffer.
 * @param rParams Parameters of the blurred layer.
 * @param rSize Size of the layer.
 */
void PaneEffect::MakeStaticRenderingConstantBuffer(DrawInfo& rDrawInfo, int offset,
                                                   const BlurParams& rParams,
                                                   const Size& rSize) const {
    VertexShaderConstantBuffer* pConstantBuffer = GetVertexShaderConstantBuffer(rDrawInfo, offset);

    nn::util::Float2 pos = CalculateDropShadowEffectVertexPos(rSize);
    if (rParams.distance > 0.0f) {
        const float angle = nn::util::DegreeToRadian(rParams.angle);
        pos.x -= std::cos(angle) * rParams.distance;
        pos.y -= std::sin(angle) * rParams.distance;
    }

    pConstantBuffer->color[3] = GetCachedImageAlpha(mPane);
    CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, pos, rSize, 1, QuadTexCoords);
}

/**
 * @brief Compute every constant buffer of a drop shadow layer.
 * @param rDrawInfo Draw state.
 * @param type Layer to compute.
 * @return Blur level of the layer.
 */
int PaneEffect::MakeDropShadowConstantBufferSet(DrawInfo& rDrawInfo, DropShadowType type) {
    AllocateDropShadowEffectConstantBufferSet(rDrawInfo, type);

    BlurParams params;
    MakeBlurParams(&params, m_DropShadow.pData, type);

    Size paneSize;
    mPane->GetSizeWithCaptureEffect(&paneSize);
    const float padding = static_cast<float>(m_DropShadow.pData->blurPaddingSize) * 2.0f;
    Size size;
    size.width = paneSize.width + padding;
    size.height = paneSize.height + padding;

    int sampleCount = 0;
    if (!IsDropShadowStaticCacheEnabled() || !m_DropShadow.isStaticCacheUpdated) {
        nn::util::Float2 pos;
        pos.x = size.width * -0.5f;
        pos.y = size.height * 0.5f;
        nn::util::Float2 texCoordScale;
        texCoordScale.x = size.width / paneSize.width;
        texCoordScale.y = size.height / paneSize.height;
        nn::util::Float2 texCoordOffset;
        texCoordOffset.x = (texCoordScale.x - 1.0f) * 0.5f;
        texCoordOffset.y = (texCoordScale.y - 1.0f) * 0.5f;

        MakeHorizontalBlurConstantBuffer(rDrawInfo, m_DropShadow.horizontalBlurOffsets[type], pos,
                                         size, texCoordOffset);
        MakeVerticalBlurConstantBuffer(rDrawInfo, m_DropShadow.verticalBlurOffsets[type], params,
                                       pos, size, texCoordScale, texCoordOffset);
        sampleCount = MakeBlurPixelShaderConstantBuffer(
            rDrawInfo, m_DropShadow.pixelShaderOffsets[type], params);
    }

    if (IsDropShadowStaticCacheEnabled()) {
        MakeStaticRenderingConstantBuffer(rDrawInfo, m_DropShadow.staticRenderingOffsets[type],
                                          params, size);
    }

    return sampleCount > 0 ? (sampleCount - 1) / 4 : 0;
}

/**
 * @brief Allocate the constant buffers of a drop shadow layer.
 * @param rDrawInfo Draw state holding the ui2d constant buffer.
 * @param type Layer to allocate the buffers for.
 */
void PaneEffect::AllocateDropShadowEffectConstantBufferSet(DrawInfo& rDrawInfo,
                                                           DropShadowType type) {
    if (!m_DropShadow.isStaticCacheUpdated) {
        const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
        m_DropShadow.horizontalBlurOffsets[type] =
            AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
        m_DropShadow.verticalBlurOffsets[type] =
            AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
        m_DropShadow.pixelShaderOffsets[type] =
            rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(
                nn::util::align_up(BlurPixelShaderConstantBufferSize, alignment));
    }

    if (IsDropShadowStaticCacheEnabled()) {
        m_DropShadow.staticRenderingOffsets[type] =
            AllocAndSetupVertexShaderConstantBuffer(rDrawInfo, mPane);
    }
}

/**
 * @brief Collect the parameters of a drop shadow layer.
 * @param pParams Receives the parameters.
 * @param pDropShadow Drop shadow parameters of the pane.
 * @param type Layer to collect.
 */
void PaneEffect::MakeBlurParams(BlurParams* pParams, const SystemDataDropShadow* pDropShadow,
                                DropShadowType type) const {
    switch (type) {
    case SystemDataDropShadow::DropShadowType_Stroke:
        pParams->color = pDropShadow->strokeColor;
        pParams->size = pDropShadow->strokeSize;
        pParams->spread = 100.0f;
        pParams->angle = 0.0f;
        pParams->distance = 0.0f;
        break;
    case SystemDataDropShadow::DropShadowType_OuterGlow:
        pParams->color = pDropShadow->outerGlowColor;
        pParams->size = pDropShadow->outerGlowSize;
        pParams->spread = pDropShadow->outerGlowSpread;
        pParams->angle = 0.0f;
        pParams->distance = 0.0f;
        break;
    case SystemDataDropShadow::DropShadowType_DropShadow:
        pParams->color = pDropShadow->dropShadowColor;
        pParams->size = pDropShadow->dropShadowSize;
        pParams->spread = pDropShadow->dropShadowSpread;
        pParams->angle = pDropShadow->dropShadowAngle;
        pParams->distance = pDropShadow->dropShadowDistance;
        break;
    default:
        break;
    }
}

/**
 * @brief Compute the left-top position of the pane including italic text.
 * @param pPos Receives the position.
 */
void PaneEffect::CalcuVertexPosOfPaneParam(nn::util::Float2* pPos) const {
    mPane->GetVertexPosWithCaptureEffect(pPos);
    const float italicSize = mPane->GetItalicSize();

    switch (mPane->GetBasePositionX()) {
    case 0:
        break;
    case 2:
        pPos->x += italicSize;
        break;
    default:
        pPos->x -= italicSize;
        break;
    }
}

/**
 * @brief Select the shader of the vertical blur pass.
 * @param blendStateId Blend state the layer is drawn with.
 * @param isKnockoutEnabled Whether the pane image is cut out of the layer.
 * @param isHighQuality Whether the high quality shaders are used.
 * @return Index of the shader in the variation table.
 */
int PaneEffect::CalculateVerticalBlurShaderId(PresetBlendStateId blendStateId,
                                              bool isKnockoutEnabled, bool isHighQuality) const {
    int shaderId;
    if (blendStateId == PresetBlendStateId_Multiplication) {
        shaderId = isKnockoutEnabled ? 16 : 8;
    } else {
        shaderId = isKnockoutEnabled ? 12 : 4;
    }

    if (isHighQuality) {
        shaderId += 16;
    }

    return shaderId;
}

/**
 * @brief Draw one blurred drop shadow layer.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 * @param type Layer to draw.
 * @param blendStateId Blend state the layer is drawn with.
 * @param isKnockoutEnabled Whether the pane image is cut out of the layer.
 */
void PaneEffect::DrawBluredShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                                  DropShadowType type, PresetBlendStateId blendStateId,
                                  bool isKnockoutEnabled) {
    const int blurLevel = m_DropShadow.blurLevels[type];
    const int horizontalOffset = m_DropShadow.horizontalBlurOffsets[type];
    const int verticalOffset = m_DropShadow.verticalBlurOffsets[type];
    const int pixelShaderOffset = m_DropShadow.pixelShaderOffsets[type];
    const TextureInfo* pSourceTexture;
    if (m_Mask.pData != nullptr) {
        pSourceTexture = m_Mask.pCaptureTarget;
    } else {
        pSourceTexture = m_DropShadow.captureTexMap.m_pTextureInfo;
    }

    const ShaderInfo* pShader = m_DropShadow.pShader;

    nn::gfx::ClearColorValue clearColor = {};
    const nn::gfx::ColorTargetView* pColorTarget = &m_DropShadow.pBlurTarget->mColorTarget;
    ToImpl(rCommands).ClearColorTarget(ToImpl(const_cast<nn::gfx::ColorTargetView*>(pColorTarget)),
                                       clearColor, nullptr);
    ToImpl(rCommands).SetRenderTargets(1, ToImpl(&pColorTarget), nullptr);
    ToImpl(rCommands).SetViewportScissorState(
        ToImpl(&m_DropShadow.pBlurTarget->mViewportScissorState));

    const int horizontalVariation = m_DropShadow.shaderVariations[blurLevel];
    SetupShaderWithShaderCache(rDrawInfo, rCommands, pShader, horizontalVariation);
    const nn::gfx::DescriptorSlot samplerSlot =
        rDrawInfo.GetGraphicsResource()->GetSamplerDescriptorSlot(
            static_cast<PresetSamplerId>(36));
    rCommands.SetTextureAndSampler(GetFirstTextureSlot(pShader, horizontalVariation),
                                   nn::gfx::ShaderStage_Pixel, pSourceTexture->mDescriptor,
                                   samplerSlot);

    nn::gfx::GpuAddress pixelShaderAddress = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
    pixelShaderAddress.Offset(pixelShaderOffset);
    if (m_DropShadow.pixelShaderSlots[blurLevel] >= 0) {
        rCommands.SetConstantBuffer(m_DropShadow.pixelShaderSlots[blurLevel],
                                    nn::gfx::ShaderStage_Pixel, pixelShaderAddress,
                                    BlurPixelShaderConstantBufferSize);
    }

    rCommands.SetBlendState(const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                                ->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
    DrawCommonImpl(rDrawInfo, rCommands, pShader, horizontalVariation, horizontalOffset);
    EndRenderToTexture(rCommands);
    rDrawInfo.ResetRenderTarget(rCommands);

    const int shaderId =
        CalculateVerticalBlurShaderId(
            blendStateId, isKnockoutEnabled,
            (m_DropShadow.pData->flags & SystemDataDropShadow::Flag_HighQualityBlur) != 0) +
        blurLevel;
    const int verticalVariation = m_DropShadow.shaderVariations[shaderId];
    SetupShaderWithShaderCache(rDrawInfo, rCommands, pShader, verticalVariation);
    rCommands.SetTextureAndSampler(GetFirstTextureSlot(pShader, verticalVariation),
                                   nn::gfx::ShaderStage_Pixel,
                                   m_DropShadow.pBlurTarget->mDescriptor, samplerSlot);

    if ((m_DropShadow.pData->flags & SystemDataDropShadow::Flag_KnockoutEnabled) != 0) {
        if (m_Mask.pData != nullptr) {
            rCommands.SetTextureAndSampler(
                GetTextureSlot(pShader, static_cast<u8>(verticalVariation), 1),
                nn::gfx::ShaderStage_Pixel, m_Mask.pCaptureTarget->mDescriptor, samplerSlot);
        } else {
            rCommands.SetTextureAndSampler(
                GetTextureSlot(pShader, static_cast<u8>(verticalVariation), 1),
                nn::gfx::ShaderStage_Pixel, m_DropShadow.captureTexMap.m_pTextureInfo->mDescriptor,
                samplerSlot);
        }
    }

    if (m_DropShadow.pixelShaderSlots[shaderId] >= 0) {
        rCommands.SetConstantBuffer(m_DropShadow.pixelShaderSlots[shaderId],
                                    nn::gfx::ShaderStage_Pixel, pixelShaderAddress,
                                    BlurPixelShaderConstantBufferSize);
    }

    rCommands.SetBlendState(const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                                ->GetPresetBlendState(blendStateId));
    DrawCommonImpl(rDrawInfo, rCommands, pShader, verticalVariation, verticalOffset);
}

/**
 * @brief Convert the blend mode of a drop shadow layer to a preset blend state.
 * @param blendMode Blend mode of the layer.
 * @return The preset blend state.
 */
PresetBlendStateId PaneEffect::ConvertBlendType(DropShadowBlendMode blendMode) const {
    PresetBlendStateId blendStateId = PresetBlendStateId_Default;
    switch (blendMode) {
    case DropShadowBlendMode_Multiply:
        blendStateId = PresetBlendStateId_Multiplication;
        break;
    case DropShadowBlendMode_Add:
        blendStateId = PresetBlendStateId_Addition;
        break;
    case DropShadowBlendMode_Subtract:
        blendStateId = PresetBlendStateId_Subtraction;
        break;
    case DropShadowBlendMode_SemitransparencyMaxAlpha:
        blendStateId = PresetBlendStateId_SemitransparencyMaxAlpha;
        break;
    default:
        break;
    }

    return blendStateId;
}

}  // namespace nn::ui2d::detail
