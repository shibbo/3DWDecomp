#include <nn/ui2d/ui2d_Util.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <nn/font/font_GpuBuffer.h>
#include <nn/font/font_PairFont.h>
#include <nn/font/font_ResFont.h>
#include <nn/font/font_ScalableFont.h>
#include <nn/font/font_TagProcessorBase.h>
#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_ResShader.h>
#include <nn/gfx/gfx_ResShaderData-api.nvn.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_ResTextureData.h>
#include <nn/gfx/gfx_StateInfo.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/gfx/gfx_TextureInfo.h>
#include <nn/util.h>
#include <nn/util/util_MatrixApi.h>
#include <nn/ui2d/ui2d_Alignment.h>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_AnimTransform.h>
#include <nn/ui2d/ui2d_Bounding.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_Capture.h>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_LayoutPaneFactory.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_PaneEffect.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_Picture.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_ShaderInfo.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/ui2d/ui2d_TextureContainer.h>
#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/ui2d/ui2d_Window.h>
#include <nn/util/util_Constants.h>

namespace nn::ui2d {

namespace {

using MemoryPoolImpl = nn::gfx::detail::MemoryPoolImpl<nn::gfx::ApiVariationNvn8>;
using TextureImpl = nn::gfx::detail::TextureImpl<nn::gfx::ApiVariationNvn8>;
using TextureViewImpl = nn::gfx::detail::TextureViewImpl<nn::gfx::ApiVariationNvn8>;

/** @brief Character separating the names of a dynamic texture name. */
const char TextureNameSeparator = '%';

/**
 * @brief Joins a prefix and a texture name with the name separator.
 * @param pBuffer Receives the joined, terminated name.
 * @param pPrefix Prefix of the name.
 * @param prefixLength Length of pPrefix.
 * @param pName Texture name.
 * @param nameLength Length of pName.
 */
inline void JoinTextureName(char* pBuffer, const char* pPrefix, size_t prefixLength,
                            const char* pName, size_t nameLength) {
    std::memcpy(pBuffer, pPrefix, prefixLength);
    pBuffer += prefixLength;
    *pBuffer++ = TextureNameSeparator;
    std::memcpy(pBuffer, pName, nameLength);
    pBuffer[nameLength] = '\0';
}

/**
 * @brief Calculates the length of the dynamic texture prefix of nested layouts.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 * @return Length of the prefix including its terminator.
 */
inline size_t CalcTexturePrefixLength(const BuildArgSet& rArgs, int depth) {
    size_t length = 0;

    for (int i = 0; i < depth; i++) {
        length += std::strlen(rArgs.pDynamicTexturePrefixNames[i]) + 1;
    }

    return length;
}

/**
 * @brief Writes the dynamic texture prefix of nested layouts.
 * @param pBuffer Receives the prefix.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 */
inline void ConcatTexturePrefixString(char* pBuffer, const BuildArgSet& rArgs, int depth) {
    for (int i = 0; i < depth; i++) {
        const char* pName = rArgs.pDynamicTexturePrefixNames[i];
        const size_t length = std::strlen(pName);
        std::memcpy(pBuffer, pName, length);
        pBuffer += length;
        *pBuffer++ = TextureNameSeparator;
    }

    pBuffer[-1] = '\0';
}

/**
 * @brief Inverts an affine matrix.
 * @param pOut Receives the inverse; it is zero when rMtx is singular.
 * @param rMtx Matrix to invert.
 */
inline void MatrixInverse(nn::util::MatrixT4x3fType* pOut, const nn::util::MatrixT4x3fType& rMtx) {
    const float32x4_t row0 = rMtx._m.val[0];
    const float32x4_t row1 = rMtx._m.val[1];
    const float32x4_t row2 = rMtx._m.val[2];
    const float m00 = vgetq_lane_f32(row0, 0);
    const float m01 = vgetq_lane_f32(row0, 1);
    const float m02 = vgetq_lane_f32(row0, 2);
    const float m03 = vgetq_lane_f32(row0, 3);
    const float m10 = vgetq_lane_f32(row1, 0);
    const float m11 = vgetq_lane_f32(row1, 1);
    const float m12 = vgetq_lane_f32(row1, 2);
    const float m13 = vgetq_lane_f32(row1, 3);
    const float m20 = vgetq_lane_f32(row2, 0);
    const float m21 = vgetq_lane_f32(row2, 1);
    const float m22 = vgetq_lane_f32(row2, 2);
    const float m23 = vgetq_lane_f32(row2, 3);

    const float det = m00 * m11 * m22 + m01 * m12 * m20 + m02 * m10 * m21 - m11 * m20 * m02 -
                      m01 * m10 * m22 - m00 * m21 * m12;
    float invDet;

    if (det == 0.0f) {
        invDet = 0.0f;
    } else {
        invDet = 1.0f / det;
    }

    const float i00 = (m11 * m22 - m12 * m21) * invDet;
    const float i01 = (m02 * m21 - m01 * m22) * invDet;
    const float i02 = (m01 * m12 - m02 * m11) * invDet;
    const float i10 = (m12 * m20 - m10 * m22) * invDet;
    const float i11 = (m00 * m22 - m02 * m20) * invDet;
    const float i12 = (m02 * m10 - m00 * m12) * invDet;
    const float i20 = (m10 * m21 - m11 * m20) * invDet;
    const float i21 = (m01 * m20 - m00 * m21) * invDet;
    const float i22 = (m00 * m11 - m01 * m10) * invDet;
    const float i03 = -(m03 * i00) - i01 * m13 - i02 * m23;
    const float i13 = -(m03 * i10) - i11 * m13 - i12 * m23;
    const float i23 = -(m03 * i20) - i21 * m13 - i22 * m23;

    pOut->_m.val[0] = float32x4_t{i00, i01, i02, i03};
    pOut->_m.val[1] = float32x4_t{i10, i11, i12, i13};
    pOut->_m.val[2] = float32x4_t{i20, i21, i22, i23};
}

/**
 * @brief Transforms a point by an affine matrix.
 * @param x X coordinate of the point.
 * @param y Y coordinate of the point.
 * @param z Z coordinate of the point.
 * @param rMtx Matrix to apply.
 * @return The transformed point.
 */
inline float32x4_t TransformPoint(float x, float y, float z,
                                  const nn::util::MatrixT4x3fType& rMtx) {
    float32x4x4_t matrix;
    matrix.val[0] = rMtx._m.val[0];
    matrix.val[1] = rMtx._m.val[1];
    matrix.val[2] = rMtx._m.val[2];
    matrix.val[3] = vdupq_n_f32(0.0f);
    const float32x4x4_t transposed = nn::util::detail::Matrix4x4fTranspose(matrix);

    float32x4_t value = vmulq_n_f32(transposed.val[0], x);
    value = vfmaq_n_f32(value, transposed.val[1], y);
    value = vfmaq_n_f32(value, transposed.val[2], z);
    return vaddq_f32(transposed.val[3], value);
}

/**
 * @brief Checks whether the vertical axis of a draw is flipped.
 * @param rDrawInfo Draw information of the layout.
 * @return True when the Y axis is flipped.
 */
inline bool IsYAxisFlipped(const DrawInfo& rDrawInfo) {
    return (rDrawInfo.mFlags & 8) != 0;
}

/**
 * @brief Checks whether a frame lies in the range of a parameterized animation.
 * @param rParameter Animation to check.
 * @param frame Frame to check.
 * @return True when frame is between the first and the last frame of the animation.
 */
inline bool IsFrameInRange(const ResParameterizedAnimParameter& rParameter, float frame) {
    return rParameter.offset <= frame && frame <= rParameter.offset + rParameter.duration;
}

/**
 * @brief Draws the box of a null or bounding pane.
 * @param rCommandBuffer Command buffer the box is drawn with.
 * @param rDrawInfo Draw information of the layout.
 * @param rMaterial Material the box is drawn with.
 * @param pPane Pane whose rectangle is drawn.
 * @param rColor Color of the box.
 */
inline void DrawPaneBox(nn::gfx::CommandBuffer& rCommandBuffer, DrawInfo& rDrawInfo,
                        Material& rMaterial, const Pane* pPane, const nn::util::Unorm8x4& rColor) {
    rDrawInfo.m_ModelViewMtx = pPane->GetGlobalMatrix();
    rDrawInfo.mModelViewLoaded = false;

    const nn::font::Rectangle rect = pPane->GetPaneRect();
    const nn::util::Float2 basePos = {{{rect.left, rect.top}}};
    detail::DrawBox(rCommandBuffer, rDrawInfo, rMaterial, basePos, pPane->GetSize(), rColor);
    rDrawInfo.mVertexBufferDirty = true;
}

/**
 * @brief Counts the children of a pane.
 * @param pPane Pane whose children are counted.
 * @return Number of children.
 */
inline int CountChildren(const Pane* pPane) {
    int count = 0;

    for (const nn::util::IntrusiveListNode* pNode = pPane->m_Children.GetNext();
         pNode != &pPane->m_Children; pNode = pNode->GetNext()) {
        count++;
    }

    return count;
}

/**
 * @brief Writes one vertex of a shape centered on the origin.
 * @param pShape Shape whose vertex format selects the attributes.
 * @param pVertex Destination of the vertex.
 * @param x X coordinate between -1 and 1.
 * @param y Y coordinate between -1 and 1.
 * @return The end of the written vertex.
 */
inline float* WriteShapeVertex(const nn::gfx::util::PrimitiveShape* pShape, float* pVertex,
                               float x, float y) {
    if (pShape->GetVertexFormat() & nn::gfx::util::PrimitiveShapeFormat_Pos) {
        pVertex[0] = x;
        pVertex[1] = y;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (pShape->GetVertexFormat() & nn::gfx::util::PrimitiveShapeFormat_Uv) {
        pVertex[0] = x * 0.5f + 0.5f;
        pVertex[1] = 1.0f - (y * 0.5f + 0.5f);
        pVertex += 2;
    }

    return pVertex;
}

/** @brief Number of rounded corners of a rounded rectangle. */
const int CornerCount = 4;

/**
 * @brief Gets the size of one index of an index format.
 * @param format Index format.
 * @return Size of one index in bytes.
 */
inline size_t GetIndexSize(nn::gfx::IndexFormat format) {
    switch (format) {
    case nn::gfx::IndexFormat_Uint16:
        return sizeof(uint16_t);
    case nn::gfx::IndexFormat_Uint32:
        return sizeof(uint32_t);
    default:
        return sizeof(uint8_t);
    }
}

/** @brief Signature of a complex font (bfcpx) file. */
const uint32_t ComplexFontSignature = 0x58504346;

/** @brief Version of the complex font format before scalable font metrics were added. */
const uint32_t ComplexFontVersionOld = 0x08000000;

/** @brief Version of the complex font format that added the scale height of scalable fonts. */
const uint32_t ComplexFontVersionScaleHeight = 0x09000000;

/** @brief Resource type of a bitmap font. */
const uint32_t ResourceTypeFont = 0x666f6e74;

/** @brief Resource type of a scalable font. */
const uint32_t ResourceTypeScalableFont = 0x73636674;

/**
 * @brief Checks the signature of a binary file.
 * @param signature Signature of the file.
 * @param expected Expected signature.
 */
inline void CheckSignature(uint32_t signature, uint32_t expected) {
    if (signature != expected) {
        char message[256];
        nn::util::SNPrintf(message, sizeof(message),
                           "Signature check failed ('%c%c%c%c' must be '%c%c%c%c').",
                           (signature >> 24) & 0xff, (signature >> 16) & 0xff,
                           (signature >> 8) & 0xff, signature & 0xff, (expected >> 24) & 0xff,
                           (expected >> 16) & 0xff, (expected >> 8) & 0xff, expected & 0xff);
    }
}

/**
 * @brief Checks whether font data is a plain TrueType or OpenType font.
 * @param pFontData Font data.
 * @return True when the data is not encrypted.
 */
inline bool IsPlainFontData(const void* pFontData) {
    const uint32_t signature = *static_cast<const uint32_t*>(pFontData);
    return signature == 0x00000100 || signature == 0x66637474 || signature == 0x4f54544f;
}

/**
 * @brief Sets the metrics of a scalable font of a complex font before version 8.0.1, which has
 *        none.
 * @param pArg Texture cache arguments to fill.
 * @param face Font face of the scalable font.
 * @param index Index of the font in the font face.
 * @param pDesc Scalable font of the complex font.
 * @param version Version of the complex font file.
 */
inline void SetupScalableFontMetrics(nn::font::TextureCache::InitializeArg* pArg, int face,
                                     int index,
                                     const nn::font::ResScalableFontDescriptionOld* pDesc,
                                     uint32_t version) {}

/**
 * @brief Sets the metrics of a scalable font of a complex font.
 * @param pArg Texture cache arguments to fill.
 * @param face Font face of the scalable font.
 * @param index Index of the font in the font face.
 * @param pDesc Scalable font of the complex font.
 * @param version Version of the complex font file.
 */
inline void SetupScalableFontMetrics(nn::font::TextureCache::InitializeArg* pArg, int face,
                                     int index, const nn::font::ResScalableFontDescription* pDesc,
                                     uint32_t version) {
    pArg->scaleWidths[face][index] = pDesc->scaleWidth;
    pArg->scaleHeights[face][index] =
        version > ComplexFontVersionScaleHeight ? pDesc->scaleHeight : 1.0f;
    pArg->ignorePalt[face][index] = (pDesc->flags & 1) != 0;
    pArg->isWidthFromBoundingBox[face][index] = (pDesc->flags >> 1 & 1) != 0;
    pArg->letterSpacings[face][index] = pDesc->letterSpacing;
    pArg->isFixedWidth[face][index] = (pDesc->flags >> 2 & 1) != 0;
    pArg->fixedWidths[face][index] = pDesc->fixedWidth;
    pArg->baselineOffsets[face][index] = pDesc->baselineOffset;
    pArg->overwrittenAscents[face][index] = pDesc->overwrittenAscent;
    pArg->overwrittenDescents[face][index] = pDesc->overwrittenDescent;
}

/**
 * @brief Sets up the scalable font arguments of a scalable font node before version 8.0.1.
 * @param pArg Receives the arguments.
 * @param pNode Scalable font node.
 */
inline void SetupScalableFontNodeArg(nn::font::ScalableFont::InitializeArg* pArg,
                                     const nn::font::ResMultiScalableFontOld* pNode) {}

/**
 * @brief Sets up the scalable font arguments of a scalable font node.
 * @param pArg Receives the arguments.
 * @param pNode Scalable font node.
 */
inline void SetupScalableFontNodeArg(nn::font::ScalableFont::InitializeArg* pArg,
                                     const nn::font::ResMultiScalableFont* pNode) {
    pArg->lineFeedOffset = pNode->lineFeedOffset;

    if (pNode->alternateChar != 0) {
        pArg->alternateChar = pNode->alternateChar;
    }
}

}  // namespace

/**
 * @brief Initializes the texture and texture view of a texture resource.
 * @param pTextureInfo Receives the texture resource.
 * @param pDevice Device the texture is created on.
 * @param pResource Texture resource (nn::gfx::ResTexture).
 * @return Always true.
 */
bool LoadTexture(ResourceTextureInfo* pTextureInfo, nn::gfx::Device* pDevice,
                 const void* pResource) {
    nn::gfx::ResTexture* pResTexture =
        static_cast<nn::gfx::ResTexture*>(const_cast<void*>(pResource));
    pTextureInfo->m_pResource = pResTexture;
    nn::gfx::ResTextureData& rData = pResTexture->ToData();
    auto* pTexture = static_cast<nn::gfx::Texture*>(rData.pTexture.Get());

    if (pTexture != nullptr && pTexture->ToData()->state != 0) {
        return true;
    }

    nn::gfx::ResTextureContainerData* pContainer = rData.pResTextureContainerData.Get();
    const ptrdiff_t offset =
        pContainer->memoryPoolOffsetBase -
        reinterpret_cast<uintptr_t>(static_cast<u8*>(pContainer->pTextureData.Get()) + 0x10) +
        reinterpret_cast<uintptr_t>(rData.pMipPtrArray.Get()->Get());
    static_cast<TextureImpl*>(pTexture)->Initialize(
        pDevice, *reinterpret_cast<const nn::gfx::TextureInfo*>(&rData.textureInfoData),
        static_cast<MemoryPoolImpl*>(pContainer->pCurrentMemoryPool.Get()), offset,
        rData.textureDataSize);

    nn::gfx::TextureViewInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetImageDimension(static_cast<nn::gfx::ImageDimension>(rData.imageDimension));
    std::memcpy(info.ToData()->channelMapping, rData.channelMapping, sizeof(rData.channelMapping));
    info.SetImageFormat(static_cast<nn::gfx::ImageFormat>(rData.textureInfoData.imageFormat));
    info.SetTexturePtr(rData.pTexture.Get());
    info.EditSubresourceRange().EditArrayRange().SetArrayLength(
        rData.textureInfoData.arrayLength);
    info.EditSubresourceRange().EditMipRange().SetMipCount(rData.textureInfoData.mipCount);
    static_cast<TextureViewImpl*>(rData.pTextureView.Get())->Initialize(pDevice, info);
    return true;
}

/**
 * @brief Initializes a shader archive with its variation table.
 * @param pShaderInfo Receives the shader.
 * @param pDevice Device the shader is created on.
 * @param pShader Shader archive.
 * @param pVariationTable Variation table of the archive.
 * @param pMemoryPool Memory pool holding the archive, or nullptr.
 * @param memoryPoolOffset Offset of the archive in pMemoryPool.
 * @param memoryPoolSize Size of the archive in pMemoryPool.
 * @param codeType Code type of the shader.
 */
void LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, void* pShader,
                       const void* pVariationTable, nn::gfx::MemoryPool* pMemoryPool,
                       ptrdiff_t memoryPoolOffset, size_t memoryPoolSize, int codeType) {
    pShaderInfo->InitializeWithVariationTable(pDevice, pShader, pVariationTable, pMemoryPool,
                                              memoryPoolOffset, memoryPoolSize, codeType);
}

/**
 * @brief Finalizes a shader archive initialized by LoadArchiveShader.
 * @param pDevice Device the shader was created on.
 * @param pShaderInfo Shader to finalize.
 */
void FreeArchiveShader(nn::gfx::Device* pDevice, ShaderInfo* pShaderInfo) {
    pShaderInfo->Finalize(pDevice, false);
}

/**
 * @brief Searches a shader variation table for a key set.
 * @param pVariationTable Variation table of a shader archive.
 * @param signature Signature of the shader.
 * @param keyCount Number of keys.
 * @param pKeys Keys of the variation.
 * @return Index of the first shader of the variation, or -1 when it is missing.
 */
int SearchShaderVariationIndexFromTable(const void* pVariationTable, uint32_t signature,
                                        size_t keyCount, const uint32_t* pKeys) {
    const uint32_t* pTable = static_cast<const uint32_t*>(pVariationTable);
    const uint32_t variationCount = *pTable++;

    for (uint32_t i = 0; i < variationCount; i++) {
        const uint32_t variationSignature = pTable[0];
        const uint32_t variationKeyCount = pTable[1];
        const uint32_t* pVariationKeys = pTable + 2;
        const size_t keySize = variationKeyCount * sizeof(uint32_t);

        if (variationSignature == signature && variationKeyCount == keyCount &&
            std::memcmp(pVariationKeys, pKeys, keySize) == 0) {
            return i * 3;
        }

        pTable = reinterpret_cast<const uint32_t*>(reinterpret_cast<const u8*>(pVariationKeys) +
                                                   keySize);
    }

    return -1;
}

/**
 * @brief Binds an animation to the panes of a group.
 * @param pAnimTrans Animation to bind.
 * @param pGroup Group whose panes are bound.
 * @param isEnabled Whether the animation is applied.
 */
void BindAnimation(AnimTransform* pAnimTrans, Group* pGroup, bool isEnabled) {
    pAnimTrans->BindGroup(pGroup);
    pAnimTrans->SetEnabled(isEnabled);
}

/**
 * @brief Unbinds an animation from the panes of a group.
 * @param pAnimTrans Animation to unbind.
 * @param pGroup Group whose panes are unbound.
 */
void UnbindAnimation(AnimTransform* pAnimTrans, Group* pGroup) {
    pAnimTrans->UnbindGroup(pGroup);
}

/**
 * @brief Checks whether a position lies inside the rectangle of a pane.
 * @param pPane Pane to test.
 * @param rPos Position in the coordinate system of the global matrix.
 * @return True when the position is inside the pane.
 */
bool IsContain(const Pane* pPane, const nn::util::Float2& rPos) {
    nn::util::MatrixT4x3fType invGlobalMtx;
    MatrixInverse(&invGlobalMtx, pPane->GetGlobalMatrix());

    const float32x4_t localPos = TransformPoint(rPos.x, rPos.y, 0.0f, invGlobalMtx);

    const nn::font::Rectangle rect = pPane->GetPaneRect();
    const float x = vgetq_lane_f32(localPos, 0);
    const float y = vgetq_lane_f32(localPos, 1);
    return rect.left <= x && rect.right >= x && rect.bottom <= y && rect.top >= y;
}

/**
 * @brief Finds the topmost bounding pane of a pane tree that contains a position.
 * @param pPane Root of the pane tree.
 * @param rPos Position to test.
 * @return The bounding pane hit, or nullptr when none is hit.
 */
Pane* FindHitPane(Pane* pPane, const nn::util::Float2& rPos) {
    if (!pPane->IsVisible()) {
        return nullptr;
    }

    nn::util::IntrusiveListNode* pNode = &pPane->m_Children;

    while (pPane->m_Children.GetNext() != pNode) {
        Pane* pHitPane = FindHitPane(Pane::FromLink(pNode->GetPrev()), rPos);

        if (pHitPane != nullptr) {
            return pHitPane;
        }

        pNode = pNode->GetPrev();
    }

    Bounding* pBounding = DynamicCast<Bounding*>(pPane);

    if (pBounding != nullptr && IsContain(pBounding, rPos)) {
        return pBounding;
    }

    return nullptr;
}

/**
 * @brief Finds the topmost bounding pane of a pane tree that contains a position.
 * @param pPane Root of the pane tree.
 * @param rPos Position to test.
 * @return The bounding pane hit, or nullptr when none is hit.
 */
const Pane* FindHitPane(const Pane* pPane, const nn::util::Float2& rPos) {
    return FindHitPane(const_cast<Pane*>(pPane), rPos);
}

/**
 * @brief Finds the topmost bounding pane of a layout that contains a position.
 * @param pLayout Layout to search.
 * @param rPos Position to test.
 * @return The bounding pane hit, or nullptr when none is hit.
 */
Pane* FindHitPane(Layout* pLayout, const nn::util::Float2& rPos) {
    return FindHitPane(pLayout->GetRootPane(), rPos);
}

/**
 * @brief Finds the topmost bounding pane of a layout that contains a position.
 * @param pLayout Layout to search.
 * @param rPos Position to test.
 * @return The bounding pane hit, or nullptr when none is hit.
 */
const Pane* FindHitPane(const Layout* pLayout, const nn::util::Float2& rPos) {
    return FindHitPane(pLayout->GetRootPane(), rPos);
}

/**
 * @brief Steps a depth-first traversal of a pane tree.
 * @param pPane Current pane of the traversal.
 * @return The next pane, or nullptr when the traversal is over.
 */
Pane* GetNextPane(Pane* pPane) {
    auto* pChild = pPane->m_Children.GetNext();

    if (pChild != &pPane->m_Children) {
        return Pane::FromLink(pChild);
    }

    while (pPane->GetParent() != nullptr) {
        auto* pNext = pPane->m_Link.GetNext();
        auto* pParent = pPane->GetParent();

        if (pNext != &pParent->m_Children) {
            return Pane::FromLink(pNext);
        }

        pPane = pParent;
    }

    return nullptr;
}

/**
 * @brief Clones a pane tree.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @return The clone of the tree.
 */
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice) {
    return detail::ClonePaneTreeImpl_(pSrcPane, pDevice, nullptr);
}

namespace detail {

/**
 * @brief Clones a pane tree with a temporary build context.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeImpl_(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout) {
    void* pContextMemory =
        __builtin_alloca(BuildPaneTreeContext::CalculateContextRequireMemorySize());
    BuildPaneTreeContext context(pContextMemory,
                                 BuildPaneTreeContext::CalculateContextRequireMemorySize());
    context.Initialize();
    context.PushCache(pLayout, nullptr);
    Pane* pPane = Layout::g_pLayoutPaneFactory->ClonePaneTree(pSrcPane, pDevice, pLayout, &context);
    pLayout->AggregateDynamicTextureList(context.GetCurrentTextureShareInfo());
    context.PopCache();
    context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
    context.Finalize();
    return pPane;
}

}  // namespace detail

/**
 * @brief Clones a pane tree.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @return The clone of the tree.
 */
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout) {
    return detail::ClonePaneTreeImpl_(pSrcPane, pDevice, pLayout);
}

/**
 * @brief Clones a pane tree.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @param pResAccessor Unused resource accessor.
 * @param pNewRootName Unused name of the new root.
 * @return The clone of the tree.
 */
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice,
                    ResourceAccessor* pResAccessor, const char* pNewRootName) {
    return detail::ClonePaneTreeImpl_(pSrcPane, pDevice, nullptr);
}

/**
 * @brief Clones a pane tree.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @param pResAccessor Unused resource accessor.
 * @param pNewRootName Unused name of the new root.
 * @param pLayout Unused layout.
 * @return The clone of the tree.
 */
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice,
                    ResourceAccessor* pResAccessor, const char* pNewRootName,
                    const Layout* pLayout) {
    return detail::ClonePaneTreeImpl_(pSrcPane, pDevice, nullptr);
}

/**
 * @brief Clones a pane tree that belongs to a parts layout.
 * @param pSrcPane Root of the tree to clone.
 * @param pPartsLayout Parts layout of the tree.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout) {
    return detail::ClonePaneTreeWithPartsLayoutImpl_(pSrcPane, pPartsLayout, pDevice, pLayout);
}

namespace detail {

/**
 * @brief Clones a pane tree of a parts layout with a temporary build context.
 * @param pSrcPane Root of the tree to clone.
 * @param pPartsLayout Parts layout of the tree.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeWithPartsLayoutImpl_(const Pane* pSrcPane, Layout* pPartsLayout,
                                        nn::gfx::Device* pDevice, Layout* pLayout) {
    void* pContextMemory =
        __builtin_alloca(BuildPaneTreeContext::CalculateContextRequireMemorySize());
    BuildPaneTreeContext context(pContextMemory,
                                 BuildPaneTreeContext::CalculateContextRequireMemorySize());
    context.Initialize();
    context.PushCache(pLayout, nullptr);
    Pane* pPane = Layout::g_pLayoutPaneFactory->ClonePaneTreeWithPartsLayout(
        pSrcPane, pPartsLayout, pDevice, pLayout, &context);
    pLayout->AggregateDynamicTextureList(context.GetCurrentTextureShareInfo());
    context.PopCache();
    context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
    context.Finalize();
    return pPane;
}

}  // namespace detail

/**
 * @brief Clones a pane tree that belongs to a parts layout.
 * @param pSrcPane Root of the tree to clone.
 * @param pPartsLayout Parts layout of the tree.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @param pResAccessor Unused resource accessor.
 * @param pNewRootName Unused name of the new root.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout,
                                   ResourceAccessor* pResAccessor, const char* pNewRootName) {
    return detail::ClonePaneTreeWithPartsLayoutImpl_(pSrcPane, pPartsLayout, pDevice, pLayout);
}

/**
 * @brief Clones a pane tree that belongs to a parts layout.
 * @param pSrcPane Root of the tree to clone.
 * @param pPartsLayout Parts layout of the tree.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @param pResAccessor Unused resource accessor.
 * @param pNewRootName Unused name of the new root.
 * @param pSrcLayout Unused source layout.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout,
                                   ResourceAccessor* pResAccessor, const char* pNewRootName,
                                   const Layout* pSrcLayout) {
    return detail::ClonePaneTreeWithPartsLayoutImpl_(pSrcPane, pPartsLayout, pDevice, pLayout);
}

/**
 * @brief Checks whether a pane tree is an exact copy of another one.
 * @param pLhs Root of the first tree.
 * @param pRhs Root of the second tree.
 * @return True when both trees are equal.
 */
bool ComparePaneTreeTest(const Pane* pLhs, const Pane* pRhs) {
    if (const Picture* pPicture = DynamicCast<const Picture*>(pLhs)) {
        const Picture* pOther = DynamicCast<const Picture*>(pRhs);

        if (pOther == nullptr || !pPicture->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const TextBox* pTextBox = DynamicCast<const TextBox*>(pLhs)) {
        const TextBox* pOther = DynamicCast<const TextBox*>(pRhs);

        if (pOther == nullptr || !pTextBox->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Window* pWindow = DynamicCast<const Window*>(pLhs)) {
        const Window* pOther = DynamicCast<const Window*>(pRhs);

        if (pOther == nullptr || !pWindow->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Bounding* pBounding = DynamicCast<const Bounding*>(pLhs)) {
        const Bounding* pOther = DynamicCast<const Bounding*>(pRhs);

        if (pOther == nullptr || !pBounding->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Capture* pCapture = DynamicCast<const Capture*>(pLhs)) {
        const Capture* pOther = DynamicCast<const Capture*>(pRhs);

        if (pOther == nullptr || !pCapture->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Parts* pParts = DynamicCast<const Parts*>(pLhs)) {
        const Parts* pOther = DynamicCast<const Parts*>(pRhs);

        if (pOther == nullptr || !pParts->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Alignment* pAlignment = DynamicCast<const Alignment*>(pLhs)) {
        const Alignment* pOther = DynamicCast<const Alignment*>(pRhs);

        if (pOther == nullptr || !pAlignment->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    } else if (const Pane* pPane = DynamicCast<const Pane*>(pLhs)) {
        const Pane* pOther = DynamicCast<const Pane*>(pRhs);

        if (pOther == nullptr || !pPane->CompareCopiedInstanceTest(*pOther)) {
            return false;
        }
    }

    if (CountChildren(pLhs) != CountChildren(pRhs)) {
        return false;
    }

    const nn::util::IntrusiveListNode* pRhsNode = pRhs->m_Children.GetNext();

    for (const nn::util::IntrusiveListNode* pLhsNode = pLhs->m_Children.GetNext();
         pLhsNode != &pLhs->m_Children;
         pLhsNode = pLhsNode->GetNext(), pRhsNode = pRhsNode->GetNext()) {
        if (!ComparePaneTreeTest(Pane::FromLink(pLhsNode), Pane::FromLink(pRhsNode))) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Draws the boxes of the null and bounding panes of a pane tree.
 * @param rCommandBuffer Command buffer the boxes are drawn with.
 * @param rDrawInfo Draw information of the layout.
 * @param rMaterial Material the boxes are drawn with.
 * @param pPane Root of the pane tree.
 * @param rNullColor Color of the null pane boxes.
 * @param rBoundingColor Color of the bounding pane boxes.
 */
void DrawNullAndBoundingPane(nn::gfx::CommandBuffer& rCommandBuffer, DrawInfo& rDrawInfo,
                             Material& rMaterial, const Pane* pPane,
                             const nn::util::Unorm8x4& rNullColor,
                             const nn::util::Unorm8x4& rBoundingColor) {
    if (pPane->GetParent() != nullptr) {
        if (pPane->GetRuntimeTypeInfo() == Pane::GetRuntimeTypeInfoStatic()) {
            DrawPaneBox(rCommandBuffer, rDrawInfo, rMaterial, pPane, rNullColor);
        } else if (pPane->GetRuntimeTypeInfo() == Bounding::GetRuntimeTypeInfoStatic()) {
            DrawPaneBox(rCommandBuffer, rDrawInfo, rMaterial, pPane, rBoundingColor);
        }
    }

    for (const nn::util::IntrusiveListNode* pNode = pPane->m_Children.GetNext();
         pNode != &pPane->m_Children; pNode = pNode->GetNext()) {
        DrawNullAndBoundingPane(rCommandBuffer, rDrawInfo, rMaterial, Pane::FromLink(pNode),
                                rNullColor, rBoundingColor);
    }
}

/**
 * @brief Evaluates a hermite curve.
 * @param frame Frame to evaluate.
 * @param pKeys Keys of the curve, sorted by frame.
 * @param keyCount Number of keys.
 * @return Value of the curve at frame.
 */
float GetHermiteCurveValue(float frame, const ResHermiteKey* pKeys, int keyCount) {
    if (keyCount == 1 || frame <= pKeys[0].frame) {
        return pKeys[0].value;
    }

    if (pKeys[keyCount - 1].frame <= frame) {
        return pKeys[keyCount - 1].value;
    }

    u32 left = 0;
    u32 right = keyCount - 1;

    while (left != right - 1 && left != right) {
        const int center = static_cast<int>(left + right) / 2;

        if (frame <= pKeys[center].frame) {
            right = center;
        } else {
            left = center;
        }
    }

    const ResHermiteKey& rKey0 = pKeys[left];
    const ResHermiteKey& rKey1 = pKeys[right];
    const float FrameTolerance = 0.001f;
    const float diff = frame - rKey1.frame;

    if (-FrameTolerance < diff && diff < FrameTolerance) {
        if (right < keyCount - 1 && rKey1.frame == pKeys[right + 1].frame) {
            return pKeys[right + 1].value;
        }

        return rKey1.value;
    }

    const float t1 = frame - rKey0.frame;
    const float t2 = 1.0f / (rKey1.frame - rKey0.frame);
    const float v0 = rKey0.value;
    const float v1 = rKey1.value;
    const float s0 = rKey0.slope;
    const float s1 = rKey1.slope;
    const float t1t1t2 = t1 * t1 * t2;
    const float t1t1t2t2 = t1t1t2 * t2;
    const float t1t1t1t2t2 = t1 * t1t1t2t2;
    const float t1t1t1t2t2t2 = t1t1t1t2t2 * t2;

    return v0 * (2.0f * t1t1t1t2t2t2 - 3.0f * t1t1t2t2 + 1.0f) +
           v1 * (-2.0f * t1t1t1t2t2t2 + 3.0f * t1t1t2t2) +
           s0 * (t1t1t1t2t2 - 2.0f * t1t1t2 + t1) + s1 * (t1t1t1t2t2 - t1t1t2);
}

/**
 * @brief Evaluates the parameterized animation that is active at a frame.
 * @param frame Frame to evaluate.
 * @param defaultValue Value returned when no animation is active.
 * @param pAnim Parameterized animations of a target.
 * @return Value of the active animation, or defaultValue.
 */
float GetParameterizedAnimValue(float frame, float defaultValue,
                                const ResParameterizedAnim* pAnim) {
    for (int i = 0; i < pAnim->parameterCount; i++) {
        const ResParameterizedAnimParameter* pParameter = pAnim->GetParameter(i);

        if (IsFrameInRange(*pParameter, frame)) {
            return GetParameterizedAnimValueAtFrameClamped(frame, pParameter);
        }
    }

    return defaultValue;
}

/**
 * @brief Evaluates a parameterized animation with the frame clamped to its range.
 * @param frame Frame to evaluate.
 * @param pParameter Animation to evaluate.
 * @return Value of the animation.
 */
float GetParameterizedAnimValueAtFrameClamped(float frame,
                                              const ResParameterizedAnimParameter* pParameter) {
    return GetParameterizedAnimValueAtFrameClamped(frame, pParameter->duration, pParameter->offset,
                                                   pParameter->value.startValue,
                                                   pParameter->value.targetValue,
                                                   pParameter->parameterizedAnimType);
}

/**
 * @brief Evaluates an easing curve.
 * @param frame Frame relative to the start of the animation.
 * @param duration Length of the animation in frames.
 * @param ratio Progress of the animation between 0 and 1.
 * @param startValue Value at the start of the animation.
 * @param targetValue Value at the end of the animation.
 * @param curveType Easing curve (ParameterizedAnimType).
 * @return Value of the curve.
 */
float GetParameterizedAnimValueAtFrame_(float frame, float duration, float ratio,
                                        float startValue, float targetValue, u8 curveType) {
    const float delta = targetValue - startValue;

    switch (curveType) {
    case ParameterizedAnimType_Linear:
        return delta * ratio + startValue;
    case ParameterizedAnimType_SineIn:
        return startValue - delta * std::cos(nn::util::FloatPi * 0.5f * ratio) + delta;
    case ParameterizedAnimType_SineOut:
        return delta * std::sin(nn::util::FloatPi * 0.5f * ratio) + startValue;
    case ParameterizedAnimType_SineInOut:
        return -delta * 0.5f * (std::cos(nn::util::FloatPi * ratio) - 1.0f) + startValue;
    case ParameterizedAnimType_CubicIn:
        return delta * ratio * ratio * ratio + startValue;
    case ParameterizedAnimType_CubicOut: {
        const float t = ratio - 1.0f;
        return delta * (t * t * t + 1.0f) + startValue;
    }
    case ParameterizedAnimType_CubicInOut: {
        float t = frame / (duration * 0.5f);

        if (t < 1.0f) {
            return delta * 0.5f * t * t * t + startValue;
        }

        t -= 2.0f;
        return delta * 0.5f * (t * t * t + 2.0f) + startValue;
    }
    case ParameterizedAnimType_QuintIn: {
        const float t = frame / duration;
        return delta * (t * t * t * t * t) + startValue;
    }
    case ParameterizedAnimType_QuintOut: {
        const float t = frame / duration - 1.0f;
        return delta * (t * t * t * t * t + 1.0f) + startValue;
    }
    case ParameterizedAnimType_QuintInOut: {
        float t = frame / (duration * 0.5f);

        if (t < 1.0f) {
            return delta * 0.5f * t * t * t * t * t + startValue;
        }

        t -= 2.0f;
        return delta * 0.5f * (t * t * t * t * t + 2.0f) + startValue;
    }
    case ParameterizedAnimType_BackIn: {
        const float s = 1.70158f;
        const float t = frame / duration;
        return delta * t * t * ((s + 1.0f) * t - s) + startValue;
    }
    case ParameterizedAnimType_BackOut: {
        const float s = 1.70158f;
        const float t = frame / duration - 1.0f;
        return delta * (t * t * ((s + 1.0f) * t + s) + 1.0f) + startValue;
    }
    case ParameterizedAnimType_BackInOut: {
        const float s = 1.70158f * 1.525f;
        float t = frame / (duration * 0.5f);

        if (t < 1.0f) {
            return delta * 0.5f * (t * t * ((s + 1.0f) * t - s)) + startValue;
        }

        t -= 2.0f;
        return delta * 0.5f * (t * t * ((s + 1.0f) * t + s) + 2.0f) + startValue;
    }
    case ParameterizedAnimType_ElasticIn: {
        if (frame == 0.0f) {
            return startValue;
        }

        float t = frame / duration;

        if (t == 1.0f) {
            return startValue + delta;
        }

        const float p = duration * 0.3f;
        const float s = p / 4.0f;
        t -= 1.0f;
        return startValue -
               static_cast<float>(delta * std::pow(2.0, 10.0 * t) *
                                  std::sin((t * duration - s) * (2.0 * nn::util::FloatPi) / p));
    }
    case ParameterizedAnimType_ElasticOut: {
        if (frame == 0.0f) {
            return startValue;
        }

        const float t = frame / duration;

        if (t == 1.0f) {
            return startValue + delta;
        }

        const float p = duration * 0.3f;
        const float s = p / 4.0f;
        return delta * static_cast<float>(
                           std::pow(2.0, -10.0 * t) *
                           std::sin((t * duration - s) * (2.0 * nn::util::FloatPi) / p)) +
               startValue + delta;
    }
    case ParameterizedAnimType_ElasticInOut: {
        if (frame == 0.0f) {
            return startValue;
        }

        float t = frame / (duration * 0.5f);

        if (t == 2.0f) {
            return startValue + delta;
        }

        const float p = duration * 0.3f * 1.5f;
        const float s = p / 4.0f;
        float amplitude;
        float base = startValue;

        if (t < 1.0f) {
            t -= 1.0f;
            amplitude = delta * static_cast<float>(std::pow(2.0, 10.0 * t)) * -0.5f;
        } else {
            t -= 1.0f;
            amplitude = delta * static_cast<float>(std::pow(2.0, -10.0 * t)) * 0.5f;
            base = delta + startValue;
        }

        return base + amplitude * static_cast<float>(std::sin(
                                      (t * duration - s) * (2.0 * nn::util::FloatPi) / p));
    }
    case ParameterizedAnimType_BounceIn:
    case ParameterizedAnimType_BounceOut:
    case ParameterizedAnimType_BounceInOut:
        return 0.0f;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * @brief Evaluates an easing curve with the frame clamped to its range.
 * @param frame Frame to evaluate.
 * @param duration Length of the animation in frames.
 * @param offset First frame of the animation.
 * @param startValue Value at the start of the animation.
 * @param targetValue Value at the end of the animation.
 * @param curveType Easing curve (ParameterizedAnimType).
 * @return Value of the curve.
 */
float GetParameterizedAnimValueAtFrameClamped(float frame, float duration, float offset,
                                              float startValue, float targetValue,
                                              u8 curveType) {
    float time = frame - offset;
    float ratio = time / duration;
    time = std::min(std::fmax(time, 0.0f), duration);
    ratio = std::min(std::fmax(ratio, 0.0f), 1.0f);
    return GetParameterizedAnimValueAtFrame_(time, duration, ratio, startValue, targetValue,
                                             curveType);
}

/**
 * @brief Evaluates a parameterized animation.
 * @param frame Frame to evaluate.
 * @param pParameter Animation to evaluate.
 * @return Value of the animation.
 */
float GetParameterizedAnimValueAtFrame(float frame,
                                       const ResParameterizedAnimParameter* pParameter) {
    const float time = frame - pParameter->offset;
    return GetParameterizedAnimValueAtFrame_(
        time, pParameter->duration, (time - pParameter->offset) / pParameter->duration,
        pParameter->value.startValue, pParameter->value.targetValue,
        pParameter->parameterizedAnimType);
}

/**
 * @brief Finds an extended user data entry by name.
 * @param pList Extended user data list, or nullptr.
 * @param pName Name of the entry.
 * @return The entry, or nullptr when it is missing.
 */
const ResExtUserData* GetExtUserData(const ResExtUserDataList* pList, const char* pName) {
    if (pList == nullptr) {
        return nullptr;
    }

    const ResExtUserData* pData = pList->entries;

    for (int i = 0; i < pList->count; i++, pData++) {
        if (std::strcmp(pName, pData->GetName()) == 0) {
            return pData;
        }
    }

    return nullptr;
}

/**
 * @brief Rounds a buffer size up to the alignment the device requires.
 * @param pDevice Device the buffer is created on.
 * @param gpuAccess GPU access flags of the buffer.
 * @param size Size of the buffer.
 * @return The aligned size.
 */
size_t GetAlignedBufferSize(nn::gfx::Device* pDevice, nn::gfx::GpuAccess gpuAccess, size_t size) {
    nn::gfx::BufferInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetSize(size);
    info.SetGpuAccessFlags(gpuAccess);
    const size_t alignment = nn::gfx::Buffer::GetBufferAlignment(pDevice, info);
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Selects the default shader of a material from its texture count.
 * @param pMaterial Material to update.
 * @param texCount Number of textures of the material.
 */
void SetDefaultShaderId(Material* pMaterial, int texCount) {
    switch (texCount) {
    case 0:
        pMaterial->m_ShaderId = ShaderId_NoTexture;
        break;
    case 1:
        pMaterial->m_ShaderId = ShaderId_SingleTexture;
        break;
    default:
        break;
    }
}

/**
 * @brief Checks whether the shader binary pool of a shader container is initialized.
 * @param pContainer Shader container.
 * @return True when the memory pool of the container is initialized.
 */
bool IsResShaderContainerInitialized(nn::gfx::ResShaderContainer* pContainer) {
    auto* pShaderPool =
        static_cast<nn::gfx::NvnShaderPool*>(pContainer->ToData().pShaderBinaryPool.Get());

    if (pShaderPool == nullptr) {
        return false;
    }

    auto* pMemoryPool = static_cast<nn::gfx::MemoryPool*>(pShaderPool->pMemoryPool.Get());

    if (pMemoryPool == nullptr) {
        return false;
    }

    return pMemoryPool->ToData()->state ==
           nn::gfx::MemoryPoolImplData<nn::gfx::ApiVariationNvn8>::State_Initialized;
}

/**
 * @brief Checks whether the shader of a shader program is initialized.
 * @param pProgram Shader program, or nullptr.
 * @return True when the shader is initialized.
 */
bool IsResShaderProgramInitialized(nn::gfx::ResShaderProgram* pProgram) {
    return pProgram != nullptr &&
           static_cast<nn::gfx::Shader*>(pProgram->GetShader())->ToData()->state !=
               nn::gfx::ShaderImplData<nn::gfx::ApiVariationNvn8>::State_NotInitialized;
}

/**
 * @brief Builds the name of a capture texture.
 * @param pBuffer Receives the name.
 * @param bufferSize Size of pBuffer.
 * @param pPrefix Prefix of the nested layouts.
 * @param pName Name of the capture texture.
 */
void MakeCaptureTextureName(char* pBuffer, size_t bufferSize, const char* pPrefix,
                            const char* pName) {
    JoinTextureName(pBuffer, pPrefix, std::strlen(pPrefix), pName, std::strlen(pName));
}

/**
 * @brief Builds the name of a dynamically generated texture.
 * @param pBuffer Receives the name.
 * @param bufferSize Size of pBuffer.
 * @param pPrefix Prefix of the nested layouts.
 * @param pName Name of the texture.
 */
void MakeDynamicGenerateTextureName(char* pBuffer, size_t bufferSize, const char* pPrefix,
                                    const char* pName) {
    JoinTextureName(pBuffer, pPrefix, std::strlen(pPrefix), pName, std::strlen(pName));
}

/**
 * @brief Calculates the length of the capture texture prefix of nested layouts.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 * @return Length of the prefix including its terminator.
 */
size_t CalcCaptureTexturePrefixLength(const BuildArgSet& rArgs, int depth) {
    return CalcTexturePrefixLength(rArgs, depth);
}

/**
 * @brief Calculates the length of the dynamic texture prefix of nested layouts.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 * @return Length of the prefix including its terminator.
 */
size_t CalcDynamicGenerateTexturePrefixLength(const BuildArgSet& rArgs, int depth) {
    return CalcTexturePrefixLength(rArgs, depth);
}

/**
 * @brief Writes the capture texture prefix of nested layouts.
 * @param pBuffer Receives the prefix.
 * @param bufferSize Size of pBuffer.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 */
void ConcatCaptureTexturePrefixString(char* pBuffer, size_t bufferSize, const BuildArgSet& rArgs,
                                      int depth) {
    ConcatTexturePrefixString(pBuffer, rArgs, depth);
}

/**
 * @brief Writes the dynamic texture prefix of nested layouts.
 * @param pBuffer Receives the prefix.
 * @param bufferSize Size of pBuffer.
 * @param rArgs Build arguments holding the nested layout names.
 * @param depth Number of names that form the prefix.
 */
void ConcatDynamicGenerateTexturePrefixString(char* pBuffer, size_t bufferSize,
                                              const BuildArgSet& rArgs, int depth) {
    ConcatTexturePrefixString(pBuffer, rArgs, depth);
}

/**
 * @brief Acquires a capture texture whose name is prefixed by the nested layouts.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param rArgs Build arguments holding the nested layout names.
 * @param isAlternate Whether the alternate prefix depth is used.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireCaptureTextureWithResolvePrefix(char* pOutName, int outNameSize,
                                                    const BuildArgSet& rArgs, bool isAlternate,
                                                    nn::gfx::Device* pDevice,
                                                    ResourceAccessor* pResAccessor,
                                                    const char* pName) {
    return detail::AcquireDynamicGenerateTextureWithResolvePrefixImpl(
        pOutName, outNameSize, rArgs, isAlternate, pDevice, pResAccessor, pName);
}

namespace detail {

/**
 * @brief Acquires a texture whose name is prefixed by the nested layouts.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param rArgs Build arguments holding the nested layout names.
 * @param isAlternate Whether the alternate prefix depth is used.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireDynamicGenerateTextureWithResolvePrefixImpl(
    char* pOutName, int outNameSize, const BuildArgSet& rArgs, bool isAlternate,
    nn::gfx::Device* pDevice, ResourceAccessor* pResAccessor, const char* pName) {
    int depth;
    char* pPrefix = nullptr;

    if (isAlternate ? (depth = rArgs.mAlternateDynamicTexturePrefixDepth) > 0
                    : (depth = rArgs.mDynamicTexturePrefixDepth) > 0) {
        const size_t length = CalcDynamicGenerateTexturePrefixLength(rArgs, depth);
        pPrefix = static_cast<char*>(__builtin_alloca(length));
        ConcatDynamicGenerateTexturePrefixString(pPrefix, length, rArgs, depth);
    }

    return AcquireDynamicGenerateTextureImpl(pOutName, outNameSize, pDevice, pResAccessor, pPrefix,
                                             pName);
}

}  // namespace detail

/**
 * @brief Acquires a texture whose name is prefixed by the nested layouts.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param rArgs Build arguments holding the nested layout names.
 * @param isAlternate Whether the alternate prefix depth is used.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireDynamicGenerateTextureWithResolvePrefix(char* pOutName, int outNameSize,
                                                            const BuildArgSet& rArgs,
                                                            bool isAlternate,
                                                            nn::gfx::Device* pDevice,
                                                            ResourceAccessor* pResAccessor,
                                                            const char* pName) {
    return detail::AcquireDynamicGenerateTextureWithResolvePrefixImpl(
        pOutName, outNameSize, rArgs, isAlternate, pDevice, pResAccessor, pName);
}

/**
 * @brief Acquires a capture texture.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pPrefix Prefix of the nested layouts, or nullptr.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireCaptureTexture(char* pOutName, int outNameSize, nn::gfx::Device* pDevice,
                                   ResourceAccessor* pResAccessor, const char* pPrefix,
                                   const char* pName) {
    return detail::AcquireDynamicGenerateTextureImpl(pOutName, outNameSize, pDevice, pResAccessor,
                                                     pPrefix, pName);
}

namespace detail {

/**
 * @brief Acquires a texture whose name is prefixed by a nested layout prefix.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pPrefix Prefix of the nested layouts, or nullptr.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireDynamicGenerateTextureImpl(char* pOutName, int outNameSize,
                                               nn::gfx::Device* pDevice,
                                               ResourceAccessor* pResAccessor,
                                               const char* pPrefix, const char* pName) {
    const char* pTextureName = pName;

    if (pPrefix != nullptr) {
        const size_t prefixLength = std::strlen(pPrefix);

        if (prefixLength != 0) {
            const size_t nameLength = std::strlen(pName);
            char* pBuffer =
                static_cast<char*>(__builtin_alloca(prefixLength + nameLength + 2));
            JoinTextureName(pBuffer, pPrefix, prefixLength, pName, nameLength);
            pTextureName = pBuffer;
        }
    }

    TextureInfo* pTexture = pResAccessor->AcquireTexture(pDevice, pTextureName);

    if (pOutName != nullptr) {
        std::strncpy(pOutName, pTextureName, outNameSize);
        pOutName[outNameSize - 1] = '\0';
    }

    return pTexture;
}

}  // namespace detail

/**
 * @brief Acquires a dynamically generated texture.
 * @param pOutName Receives the resolved name, or nullptr.
 * @param outNameSize Size of pOutName.
 * @param pDevice Device the texture is created on.
 * @param pResAccessor Accessor the texture is acquired from.
 * @param pPrefix Prefix of the nested layouts, or nullptr.
 * @param pName Name of the texture.
 * @return The texture.
 */
TextureInfo* AcquireDynamicGenerateTexture(char* pOutName, int outNameSize,
                                           nn::gfx::Device* pDevice,
                                           ResourceAccessor* pResAccessor, const char* pPrefix,
                                           const char* pName) {
    return detail::AcquireDynamicGenerateTextureImpl(pOutName, outNameSize, pDevice, pResAccessor,
                                                     pPrefix, pName);
}

/**
 * @brief Initializes the first shader program of a variation that the device accepts.
 * @param pDevice Device the shader is created on.
 * @param pVariation Shader variation.
 * @return Code type of the initialized program.
 */
nn::gfx::ShaderCodeType TryInitializeAndGetShaderCodeType(nn::gfx::Device* pDevice,
                                                          nn::gfx::ResShaderVariation* pVariation) {
    nn::gfx::ResShaderProgram* pBinaryProgram =
        pVariation->GetResShaderProgram(nn::gfx::ShaderCodeType_Binary);

    if (pBinaryProgram != nullptr) {
        if (IsResShaderProgramInitialized(pBinaryProgram) ||
            pBinaryProgram->Initialize(pDevice) == nn::gfx::ShaderInitializeResult_Success) {
            return nn::gfx::ShaderCodeType_Binary;
        }
    }

    nn::gfx::ResShaderProgram* pIrProgram =
        pVariation->GetResShaderProgram(nn::gfx::ShaderCodeType_Ir);

    if (pIrProgram != nullptr) {
        if (IsResShaderProgramInitialized(pIrProgram) ||
            pIrProgram->Initialize(pDevice) == nn::gfx::ShaderInitializeResult_Success) {
            return nn::gfx::ShaderCodeType_Ir;
        }
    }

    nn::gfx::ResShaderProgram* pSourceProgram =
        pVariation->GetResShaderProgram(nn::gfx::ShaderCodeType_Source);

    if (!IsResShaderProgramInitialized(pSourceProgram)) {
        pSourceProgram->Initialize(pDevice);
    }

    return nn::gfx::ShaderCodeType_Source;
}

namespace detail {

/**
 * @brief Calculates the root matrix used when rendering a capture texture.
 * @param rMtx Receives the matrix.
 * @param rDrawInfo Draw information of the layout.
 */
void CalculateCaptureRootMatrix(nn::util::MatrixT4x3fType& rMtx, const DrawInfo& rDrawInfo) {
    rMtx._m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
    rMtx._m.val[1] = float32x4_t{0.0f, 1.0f, 0.0f, 0.0f};
    rMtx._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};

    if (IsYAxisFlipped(rDrawInfo)) {
        rMtx._m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
        rMtx._m.val[1] = float32x4_t{0.0f, -1.0f, 0.0f, 0.0f};
        rMtx._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
    }
}

/**
 * @brief Finds a capture texture resource by name.
 * @param pList Capture texture list of the layout.
 * @param pName Name of the capture texture.
 * @return The resource, or nullptr when it is missing.
 */
const ResCaptureTexture* FindCaptureTextureResource(const ResCaptureTextureList* pList,
                                                    const char* pName) {
    const ResCaptureTexture* pTexture = pList->GetTextureArray();

    for (int i = 0; i < pList->textureCount; i++, pTexture++) {
        if (std::strcmp(pList->GetName(pTexture), pName) == 0) {
            return pTexture;
        }
    }

    return nullptr;
}

/**
 * @brief Gets the scale of a capture texture.
 * @param pTexture Capture texture resource.
 * @return The scale of the texture against the captured area.
 */
float CalculateCaptureTextureScale(const ResCaptureTexture* pTexture) {
    return pTexture->textureScale;
}

/**
 * @brief Finds a vector graphics texture resource by name.
 * @param pList Vector graphics texture list of the layout.
 * @param pName Name of the texture.
 * @return The resource, or nullptr when it is missing.
 */
const ResVectorGraphicsTexture* FindVectorGraphicsTextureResource(
    const ResVectorGraphicsTextureList* pList, const char* pName) {
    for (int i = 0; i < pList->textureCount; i++) {
        const ResVectorGraphicsTexture* pTexture = pList->GetTexture(i);
        char name[71];
        VectorGraphicsTexture::MakeRefTextureName(name, sizeof(name), pList->GetFileName(pTexture),
                                                  pTexture);

        if (std::strcmp(name, pName) == 0) {
            return pTexture;
        }
    }

    return nullptr;
}

/**
 * @brief Finds the base name of a capture texture name overwritten by nested layouts.
 * @param pName Overwritten name.
 * @return The part of pName after the last separator.
 */
const char* FindCaptureTextureBaseNameFromOverwriteString(const char* pName) {
    const int length = std::strlen(pName);
    int baseNameStart = 0;

    for (int i = 0; i < length; i++) {
        if (pName[length - 1 - i] == TextureNameSeparator) {
            baseNameStart = length - i;
            break;
        }
    }

    return &pName[baseNameStart];
}

/**
 * @brief Counts the nested layouts of an overwritten capture texture name.
 * @param pName Overwritten name.
 * @return Number of separators in pName.
 */
int CalcCaptureTextureNameOverwriteDepth(const char* pName) {
    const int length = std::strlen(pName);
    int depth = 0;

    for (int i = 0; i < length; i++) {
        if (pName[i] == TextureNameSeparator) {
            depth++;
        }
    }

    return depth;
}

/**
 * @brief Clamps a value in place.
 * @param rValue Value to clamp.
 * @param minimum Lower bound.
 * @param maximum Upper bound.
 */
void ClampValue(float& rValue, float minimum, float maximum) {
    if (rValue < minimum) {
        rValue = minimum;
    }

    if (rValue > maximum) {
        rValue = maximum;
    }
}

/**
 * @brief Allocates a vertex shader constant buffer and fills it for drawing a pane.
 * @param rDrawInfo Draw information that owns the constant buffer.
 * @param pPane Pane whose global matrix is used as the model view matrix.
 * @return Offset of the allocated buffer.
 */
size_t AllocAndSetupVertexShaderConstantBuffer(DrawInfo& rDrawInfo, const Pane* pPane) {
    typedef Material::ConstantBufferForVertexShader ConstantBuffer;

    const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
    nn::font::GpuBuffer* pGpuBuffer = rDrawInfo.GetUi2dConstantBuffer();
    const uint64_t offset =
        pGpuBuffer->AllocateWithoutAlignment(nn::util::align_up(sizeof(ConstantBuffer), alignment));
    u8* pMapped = static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer());

    if (pMapped != nullptr) {
        ConstantBuffer* pConstantBuffer = reinterpret_cast<ConstantBuffer*>(pMapped + offset);
        std::memset(pConstantBuffer, 0, sizeof(ConstantBuffer));

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                pConstantBuffer->vertexColor[i][j] = 255.0f;
            }
        }

        const nn::util::MatrixT4x3fType& rGlobalMtx = pPane->GetGlobalMatrix();
        const float32x4_t modelView0 = rGlobalMtx._m.val[0];
        const float32x4_t modelView1 = rGlobalMtx._m.val[1];
        const float32x4_t modelView2 = rGlobalMtx._m.val[2];

        float (*pTexMtxs[])[4] = {pConstantBuffer->texMtx0, pConstantBuffer->texMtx1,
                                  pConstantBuffer->texMtx2};
        const float texMtxRow0[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        const float texMtxRow1[4] = {0.0f, -1.0f, 0.0f, 1.0f};

        for (int i = 0; i < 3; i++) {
            std::memcpy(pTexMtxs[i][0], texMtxRow0, sizeof(texMtxRow0));
            std::memcpy(pTexMtxs[i][1], texMtxRow1, sizeof(texMtxRow1));
        }

        vst1q_f32(pConstantBuffer->modelView[0], modelView0);
        vst1q_f32(pConstantBuffer->modelView[1], modelView1);
        vst1q_f32(pConstantBuffer->modelView[2], modelView2);

        rDrawInfo.m_TexCoordSrc[0] = 0;
        rDrawInfo.m_TexCoordSrc[1] = 0;
        rDrawInfo.m_TexCoordSrc[2] = 0;

        for (int i = 0; i < 4; i++) {
            pConstantBuffer->color[i] = 1.0f / 255.0f;
        }

        rDrawInfo.LoadProjectionMtx(pConstantBuffer->projection);
    }

    return offset;
}

/**
 * @brief Sets up a shader, reusing the one already bound when possible.
 * @param rDrawInfo Draw information that caches the current shader.
 * @param rCommandBuffer Command buffer the shader is set on.
 * @param pShaderInfo Shader to set.
 * @param variation Variation of the shader.
 */
void SetupShaderWithShaderCache(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer,
                                const ShaderInfo* pShaderInfo, int variation) {
    if (rDrawInfo.RecordCurrentShader(pShaderInfo, static_cast<u8>(variation))) {
        pShaderInfo->SetShader(rCommandBuffer, variation);
    }

    rDrawInfo.SetupProgram(&rCommandBuffer);
}

/**
 * @brief Sets up a viewport and a scissor that cover a render target.
 * @param pViewportInfo Receives the viewport.
 * @param pScissorInfo Receives the scissor.
 * @param width Width of the render target.
 * @param height Height of the render target.
 */
void SetupViewportScissorStateInfo(nn::gfx::ViewportStateInfo* pViewportInfo,
                                   nn::gfx::ScissorStateInfo* pScissorInfo, float width,
                                   float height) {
    pScissorInfo->SetOriginX(0);
    pScissorInfo->SetOriginY(0);
    pScissorInfo->SetWidth(static_cast<int>(width));
    pScissorInfo->SetHeight(static_cast<int>(height));
    pViewportInfo->SetOriginX(0.0f);
    pViewportInfo->SetOriginY(0.0f);
    pViewportInfo->SetWidth(width);
    pViewportInfo->SetHeight(height);
}

/**
 * @brief Clones a pane tree with the build context of the caller.
 * @param pSrcPane Root of the tree to clone.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @param pContext Build context of the caller.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeImpl_(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout,
                         BuildPaneTreeContext* pContext) {
    return Layout::g_pLayoutPaneFactory->ClonePaneTree(pSrcPane, pDevice, pLayout, pContext);
}

/**
 * @brief Clones a pane tree of a parts layout with the build context of the caller.
 * @param pSrcPane Root of the tree to clone.
 * @param pPartsLayout Parts layout of the tree.
 * @param pDevice Device the clone is created on.
 * @param pLayout Layout that owns the clone.
 * @param pContext Build context of the caller.
 * @return The clone of the tree.
 */
Pane* ClonePaneTreeWithPartsLayoutImpl_(const Pane* pSrcPane, Layout* pPartsLayout,
                                        nn::gfx::Device* pDevice, Layout* pLayout,
                                        BuildPaneTreeContext* pContext) {
    return Layout::g_pLayoutPaneFactory->ClonePaneTreeWithPartsLayout(pSrcPane, pPartsLayout,
                                                                      pDevice, pLayout, pContext);
}

/**
 * @brief Copies a string into memory allocated by the layout allocator.
 * @param pSource Terminated string to copy.
 * @return The copy.
 */
char* AllocateAndCopyString(const char* pSource) {
    const size_t size = std::strlen(pSource) + 1;
    return std::strncpy(static_cast<char*>(Layout::AllocateMemory(size)), pSource, size);
}

}  // namespace detail

/**
 * @brief Adds the scalable fonts of a complex font to the arguments of a texture cache.
 * @param pArg Texture cache arguments to fill.
 * @param pLoadFunction Loads the data of a font.
 * @param pUserData User data passed to pLoadFunction.
 * @param pComplexFontData Complex font (bfcpx) file.
 * @return Font face of the first scalable font of the complex font.
 */
int ComplexFontHelper::SetupTextureCacheArg(nn::font::TextureCache::InitializeArg* pArg,
                                            LoadFontCallback pLoadFunction, void* pUserData,
                                            const void* pComplexFontData) {
    const nn::font::ResComplexFontHeader* pHeader =
        static_cast<const nn::font::ResComplexFontHeader*>(pComplexFontData);
    const uint32_t version = pHeader->version;

    if (version > ComplexFontVersionOld) {
        CheckSignature(pHeader->signature, ComplexFontSignature);
    }

    const void* pRootNode = pHeader->GetRootNode();
    int fontFace = pArg->fontFaceCount;

    if (fontFace == 1) {
        const uint32_t innerFontCount = pArg->innerFontCounts[0];

        if (innerFontCount == 0 ||
            (innerFontCount == 1 && pArg->pFontDatas[0][0] == nullptr)) {
            fontFace = 0;
        }
    }

    const int fontFaceHead = fontFace;

    if (version <= ComplexFontVersionOld) {
        BuildTextureCacheArg<nn::font::ResMultiScalableFontOld,
                             nn::font::ResScalableFontDescriptionOld>(
            pRootNode, &fontFace, pArg, pLoadFunction, pUserData, pHeader->version);
    } else {
        BuildTextureCacheArg<nn::font::ResMultiScalableFont,
                             nn::font::ResScalableFontDescription>(
            pRootNode, &fontFace, pArg, pLoadFunction, pUserData, pHeader->version);
    }

    pArg->fontFaceCount = fontFace;
    return fontFaceHead;
}

/**
 * @brief Adds the scalable fonts of a complex font tree node to the arguments of a texture cache.
 * @tparam TMultiScalableFont Scalable font node of the format version.
 * @tparam TScalableFontDescription Scalable font of the format version.
 * @param pNode Node of the font tree.
 * @param pFontFace Font face of the next scalable font node; advanced for every one.
 * @param pArg Texture cache arguments to fill.
 * @param pLoadFunction Loads the data of a font.
 * @param pUserData User data passed to pLoadFunction.
 * @param version Version of the complex font file.
 */
template <typename TMultiScalableFont, typename TScalableFontDescription>
void ComplexFontHelper::BuildTextureCacheArg(const void* pNode, int* pFontFace,
                                             nn::font::TextureCache::InitializeArg* pArg,
                                             LoadFontCallback pLoadFunction, void* pUserData,
                                             uint32_t version) {
    const uint8_t* pNodeBase = static_cast<const uint8_t*>(pNode);

    switch (*static_cast<const uint32_t*>(pNode)) {
    case nn::font::ComplexFontNodeType_MultiScalable: {
        const TMultiScalableFont* pMulti = static_cast<const TMultiScalableFont*>(pNode);
        const TScalableFontDescription* pDescriptions =
            reinterpret_cast<const TScalableFontDescription*>(pNodeBase +
                                                              pMulti->descriptionOffset);

        for (int i = 0; i < pMulti->descriptionCount; i++) {
            const TScalableFontDescription* pDesc = &pDescriptions[i];
            const char* pName = reinterpret_cast<const char*>(pNodeBase + pDesc->nameOffset);
            size_t size = 0;
            const void* pFontData =
                pLoadFunction(&size, pName, ResourceTypeScalableFont, pUserData);
            pArg->pFontDatas[*pFontFace][i] = pFontData;
            pArg->fontDataSizes[*pFontFace][i] = size;
            pArg->boldWeights[*pFontFace][i] = pDesc->boldWeight;
            pArg->borderWidths[*pFontFace][i] = pDesc->borderWidth;
            pArg->fontIndexes[*pFontFace][i] = pDesc->fontIndex;
            SetupScalableFontMetrics(pArg, *pFontFace, i, pDesc, version);

            uint32_t fontDataType;

            if (CheckExt(pName, ".bfttf")) {
                if (IsPlainFontData(pFontData)) {
                    fontDataType = nn::font::TextureCache::FontDataType_Ttf;
                } else {
                    fontDataType = nn::font::TextureCache::FontDataType_Bfttf;
                }
            } else if (CheckExt(pName, ".bfotf")) {
                if (IsPlainFontData(pFontData)) {
                    fontDataType = nn::font::TextureCache::FontDataType_Otf;
                } else {
                    fontDataType = nn::font::TextureCache::FontDataType_Bfotf;
                }
            } else if (CheckExt(pName, ".otf")) {
                fontDataType = nn::font::TextureCache::FontDataType_Otf;
            } else {
                fontDataType = nn::font::TextureCache::FontDataType_Ttf;
            }

            pArg->fontDataTypes[*pFontFace][i] = fontDataType;
            pArg->charCodeRangeCounts[*pFontFace][i] = pDesc->charCodeRangeCount;

            const nn::font::ResCharCodeRange* pRanges =
                reinterpret_cast<const nn::font::ResCharCodeRange*>(pNodeBase +
                                                                    pDesc->charCodeRangeOffset);

            for (int j = 0; j < nn::font::TextureCache::CharCodeRangeCountMax; j++) {
                if (j < pDesc->charCodeRangeCount) {
                    pArg->charCodeRangeFirsts[*pFontFace][i][j] = pRanges[j].first;
                    pArg->charCodeRangeLasts[*pFontFace][i][j] = pRanges[j].last;
                } else {
                    pArg->charCodeRangeFirsts[*pFontFace][i][j] = 0;
                    pArg->charCodeRangeLasts[*pFontFace][i][j] = 0;
                }
            }
        }

        pArg->innerFontCounts[*pFontFace] = pMulti->descriptionCount;
        (*pFontFace)++;
        break;
    }
    case nn::font::ComplexFontNodeType_Pair: {
        const nn::font::ResPairFont* pPair = static_cast<const nn::font::ResPairFont*>(pNode);
        BuildTextureCacheArg<TMultiScalableFont, TScalableFontDescription>(
            pNodeBase + pPair->firstOffset, pFontFace, pArg, pLoadFunction, pUserData, version);
        BuildTextureCacheArg<TMultiScalableFont, TScalableFontDescription>(
            pNodeBase + pPair->secondOffset, pFontFace, pArg, pLoadFunction, pUserData, version);
        break;
    }
    default:
        break;
    }
}


/**
 * @brief Builds the font tree of a complex font.
 * @param pDevice Device the fonts are created on.
 * @param pRegisterFunction Registers the textures of the bitmap fonts.
 * @param pRegisterUserData User data passed to pRegisterFunction.
 * @param pTextureCache Texture cache of the scalable fonts.
 * @param fontFaceHead Font face of the first scalable font of the complex font.
 * @param pLoadFunction Loads the data of a font.
 * @param pUserData User data passed to pLoadFunction.
 * @param pComplexFontData Complex font (bfcpx) file.
 * @return Root of the font tree.
 */
nn::font::Font* ComplexFontHelper::InitializeComplexFontTree(
    nn::gfx::Device* pDevice, nn::font::RegisterTextureViewSlot pRegisterFunction,
    void* pRegisterUserData, nn::font::TextureCache* pTextureCache, int fontFaceHead,
    LoadFontCallback pLoadFunction, void* pUserData, const void* pComplexFontData) {
    const nn::font::ResComplexFontHeader* pHeader =
        static_cast<const nn::font::ResComplexFontHeader*>(pComplexFontData);
    const uint32_t version = pHeader->version;

    if (version != ComplexFontVersionOld) {
        CheckSignature(pHeader->signature, ComplexFontSignature);
    }

    const void* pRootNode = pHeader->GetRootNode();
    int fontFace = fontFaceHead;

    if (version == ComplexFontVersionOld) {
        return BuildFontTree<nn::font::ResMultiScalableFontOld>(
            pDevice, pRegisterFunction, pRegisterUserData, pRootNode, &fontFace,
            pTextureCache, pLoadFunction, pUserData, pHeader->version);
    } else {
        return BuildFontTree<nn::font::ResMultiScalableFont>(
            pDevice, pRegisterFunction, pRegisterUserData, pRootNode, &fontFace,
            pTextureCache, pLoadFunction, pUserData, pHeader->version);
    }
}

/**
 * @brief Builds the fonts of a complex font tree node.
 * @tparam TMultiScalableFont Scalable font node of the format version.
 * @param pDevice Device the fonts are created on.
 * @param pRegisterFunction Registers the textures of the bitmap fonts.
 * @param pRegisterUserData User data passed to pRegisterFunction.
 * @param pNode Node of the font tree.
 * @param pFontFace Font face of the next scalable font node; advanced for every one.
 * @param pTextureCache Texture cache of the scalable fonts.
 * @param pLoadFunction Loads the data of a font.
 * @param pUserData User data passed to pLoadFunction.
 * @param version Version of the complex font file.
 * @return The font of the node, or nullptr for an unknown node.
 */
template <typename TMultiScalableFont>
nn::font::Font* ComplexFontHelper::BuildFontTree(
    nn::gfx::Device* pDevice, nn::font::RegisterTextureViewSlot pRegisterFunction,
    void* pRegisterUserData, const void* pNode, int* pFontFace,
    nn::font::TextureCache* pTextureCache, LoadFontCallback pLoadFunction, void* pUserData,
    uint32_t version) {
    const uint8_t* pNodeBase = static_cast<const uint8_t*>(pNode);

    switch (*static_cast<const uint32_t*>(pNode)) {
    case nn::font::ComplexFontNodeType_Bitmap: {
        const nn::font::ResBitmapFont* pBitmap =
            static_cast<const nn::font::ResBitmapFont*>(pNode);
        const char* pName = reinterpret_cast<const char*>(pNodeBase + pBitmap->nameOffset);
        size_t size = 0;
        void* pFontData = pLoadFunction(&size, pName, ResourceTypeFont, pUserData);
        nn::font::ResFont* pResFont = Layout::NewObj<nn::font::ResFont>();
        pResFont->SetResource(pDevice, pFontData, nullptr, 0, 0);
        pResFont->RegisterTextureViewToDescriptorPool(pRegisterFunction, pRegisterUserData);

        const int rangeCount = pBitmap->charCodeRangeCount;

        if (rangeCount > 0) {
            const nn::font::ResCharCodeRange* pRanges =
                reinterpret_cast<const nn::font::ResCharCodeRange*>(pNodeBase +
                                                                    pBitmap->charCodeRangeOffset);
            uint32_t firsts[nn::font::TextureCache::CharCodeRangeCountMax];
            uint32_t lasts[nn::font::TextureCache::CharCodeRangeCountMax];

            for (int i = 0; i < rangeCount; i++) {
                firsts[i] = pRanges[i].first;
                lasts[i] = pRanges[i].last;
            }

            pResFont->SetCharCodeRange(rangeCount, firsts, lasts);
        }

        return pResFont;
    }
    case nn::font::ComplexFontNodeType_MultiScalable: {
        const TMultiScalableFont* pMulti = static_cast<const TMultiScalableFont*>(pNode);
        nn::font::ScalableFont::InitializeArg arg;
        arg.SetDefault();
        arg.pTextureCache = pTextureCache;
        arg.fontSize = static_cast<int>(pMulti->size);
        arg.fontFace = static_cast<u16>(*pFontFace);
        SetupScalableFontNodeArg(&arg, pMulti);
        arg.isDrawWhiteSpaceWhenGlyphNotReady = true;

        nn::font::ScalableFont* pScalableFont = Layout::NewObj<nn::font::ScalableFont>();
        pScalableFont->Initialize(arg);
        (*pFontFace)++;
        return pScalableFont;
    }
    case nn::font::ComplexFontNodeType_Pair: {
        const nn::font::ResPairFont* pPair = static_cast<const nn::font::ResPairFont*>(pNode);
        nn::font::Font* pFirstFont = BuildFontTree<TMultiScalableFont>(
            pDevice, pRegisterFunction, pRegisterUserData, pNodeBase + pPair->firstOffset,
            pFontFace, pTextureCache, pLoadFunction, pUserData, version);
        nn::font::Font* pSecondFont = BuildFontTree<TMultiScalableFont>(
            pDevice, pRegisterFunction, pRegisterUserData, pNodeBase + pPair->secondOffset,
            pFontFace, pTextureCache, pLoadFunction, pUserData, version);
        nn::font::PairFont* pPairFont = Layout::NewObj<nn::font::PairFont>();
        pPairFont->SetFont(pFirstFont, pSecondFont);
        return pPairFont;
    }
    default:
        return nullptr;
    }
}

/**
 * @brief Destroys a font tree built by InitializeComplexFontTree.
 * @param pDevice Device the fonts were created on.
 * @param pFont Root of the font tree, or nullptr.
 * @param pUnregisterFunction Unregisters the textures of the bitmap fonts.
 * @param pUserData User data passed to pUnregisterFunction.
 */
void ComplexFontHelper::FinalizeComplexFontTree(
    nn::gfx::Device* pDevice, nn::font::Font* pFont,
    nn::font::UnregisterTextureViewSlot pUnregisterFunction, void* pUserData) {
    if (pFont != nullptr) {
        DestroyFontTree(pDevice, pFont, pUnregisterFunction, pUserData);
        Layout::DeleteObj(pFont);
    }
}

/**
 * @brief Finalizes the fonts of a font tree and deletes the fonts below its root.
 * @param pDevice Device the fonts were created on.
 * @param pFont Root of the font tree.
 * @param pUnregisterFunction Unregisters the textures of the bitmap fonts.
 * @param pUserData User data passed to pUnregisterFunction.
 */
void ComplexFontHelper::DestroyFontTree(nn::gfx::Device* pDevice, nn::font::Font* pFont,
                                        nn::font::UnregisterTextureViewSlot pUnregisterFunction,
                                        void* pUserData) {
    if (nn::font::PairFont* pPairFont = DynamicCast<nn::font::PairFont*>(pFont)) {
        DestroyFontTree(pDevice, pPairFont->GetFirstFont(), pUnregisterFunction, pUserData);
        Layout::DeleteObj(pPairFont->GetFirstFont());
        DestroyFontTree(pDevice, pPairFont->GetSecondFont(), pUnregisterFunction, pUserData);
        Layout::DeleteObj(pPairFont->GetSecondFont());
    } else if (nn::font::ResFont* pResFont = DynamicCast<nn::font::ResFont*>(pFont)) {
        pResFont->UnregisterTextureViewFromDescriptorPool(pUnregisterFunction, pUserData);
        pResFont->RemoveResource(pDevice);
    }

    pFont->Finalize(pDevice);
}

/**
 * @brief Checks the extension of a file name.
 * @param pName File name.
 * @param pExt Extension including its dot.
 * @return True when pName ends with pExt.
 */
bool ComplexFontHelper::CheckExt(const char* pName, const char* pExt) {
    const char* pFound = std::strstr(pName, pExt);
    return pFound != nullptr && std::strlen(pFound) == std::strlen(pExt);
}

/**
 * @brief Constructs a rounded rectangle.
 * @param vertexFormat Attributes written for every vertex.
 * @param topology Primitive topology of the shape.
 * @param cornerSizeX Half the size of the corners along X, relative to the half width.
 * @param cornerSizeY Half the size of the corners along Y, relative to the half height.
 * @param sliceCount Number of subdivisions of each corner.
 */
RoundRectShape::RoundRectShape(nn::gfx::util::PrimitiveShapeFormat vertexFormat,
                               nn::gfx::PrimitiveTopology topology, float cornerSizeX,
                               float cornerSizeY, uint32_t sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_CornerSizeX(cornerSizeX * 2.0f),
      m_CornerSizeY(cornerSizeY * 2.0f), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(GetIndexSize(GetIndexBufferFormat()) * GetIndexCount());
}

/** @return The number of vertices of the rounded rectangle. */
int RoundRectShape::CalculateVertexCount() {
    return m_SliceCount * 4 + 20;
}

/** @return The number of indices of the rounded rectangle. */
int RoundRectShape::CalculateIndexCount() {
    return m_SliceCount * 12 + 18;
}

/**
 * @brief Constructs an empty rounded rectangle whose parameters are copied later.
 * @param vertexFormat Attributes written for every vertex.
 * @param topology Primitive topology of the shape.
 */
RoundRectShape::RoundRectShape(nn::gfx::util::PrimitiveShapeFormat vertexFormat,
                               nn::gfx::PrimitiveTopology topology)
    : PrimitiveShape(vertexFormat, topology), m_CornerSizeX(0.0f), m_CornerSizeY(0.0f),
      m_SliceCount(1) {}

/** @brief Destroys the rounded rectangle. */
RoundRectShape::~RoundRectShape() {}

/**
 * @brief Copies the parameters and the buffers of another rounded rectangle.
 * @param rSource Rounded rectangle to copy.
 */
void RoundRectShape::CopyParams(const RoundRectShape& rSource) {
    m_CornerSizeX = rSource.m_CornerSizeX;
    m_CornerSizeY = rSource.m_CornerSizeY;
    m_SliceCount = rSource.m_SliceCount;
    SetVertexBufferSize(rSource.GetVertexBufferSize());
    SetIndexBufferSize(rSource.GetIndexBufferSize());
    SetVertexCount(rSource.GetVertexFormat());
    SetIndexCount(rSource.GetIndexCount());

    void* pVertexBuffer = Layout::AllocateMemory(rSource.GetVertexBufferSize());
    std::memcpy(pVertexBuffer, rSource.GetVertexBuffer(), rSource.GetVertexBufferSize());
    SetVertexBuffer(pVertexBuffer);

    void* pIndexBuffer = Layout::AllocateMemory(rSource.GetIndexBufferSize());
    std::memcpy(pIndexBuffer, rSource.GetIndexBuffer(), rSource.GetIndexBufferSize());
    SetIndexBuffer(pIndexBuffer);
}

/**
 * @brief Writes the vertices of the rounded rectangle into the vertex buffer.
 * @return The end of the written vertices.
 */
void* RoundRectShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());
    const float radiusX = GetClampedCornerSizeX();
    const float radiusY = GetClampedCornerSizeY();
    const uint32_t arcVertexCount = m_SliceCount + 1;
    const float centerX = 1.0f - radiusX;
    const float centerY = 1.0f - radiusY;
    const float quarterAngle = nn::util::FloatPi * 0.5f;

    const float signX[CornerCount] = {1.0f, -1.0f, -1.0f, 1.0f};
    const float signY[CornerCount] = {1.0f, 1.0f, -1.0f, -1.0f};

    for (int corner = 0; corner < CornerCount; corner++) {
        pVertex = WriteShapeVertex(this, pVertex, centerX * signX[corner], centerY * signY[corner]);

        float angle = 0.0f;

        for (uint32_t i = 0; i < arcVertexCount; i++) {
            const float cos = std::cos(angle);
            const float sin = std::sin(angle);
            const float x = centerX + radiusX * cos;
            const float y = centerY + radiusY * sin;
            pVertex = WriteShapeVertex(this, pVertex, x * signX[corner], y * signY[corner]);
            angle += quarterAngle * (1.0f / m_SliceCount);
        }
    }

    if (radiusX * 2.0f < 2.0f) {
        for (int corner = 0; corner < CornerCount; corner++) {
            pVertex = WriteShapeVertex(this, pVertex, centerX * signX[corner], signY[corner]);
            pVertex = WriteShapeVertex(this, pVertex, centerX * signX[corner],
                                       centerY * signY[corner]);
        }
    }

    if (radiusY * 2.0f < 2.0f) {
        for (int corner = 0; corner < CornerCount; corner++) {
            pVertex = WriteShapeVertex(this, pVertex, signX[corner], centerY * signY[corner]);
        }
    }

    return pVertex;
}

/**
 * @brief Fills the vertex and index buffers of the rounded rectangle.
 * @param pVertexMemory Destination of the vertices.
 * @param vertexSize Size of the vertex memory.
 * @param pIndexMemory Destination of the indices.
 * @param indexSize Size of the index memory.
 */
void RoundRectShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                                   size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case nn::gfx::IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case nn::gfx::IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * @brief Writes the indices of the rounded rectangle into the index buffer.
 * @tparam T Index type.
 */
template <typename T>
void RoundRectShape::CalculateIndexBuffer() {
    T* pIndex = static_cast<T*>(GetIndexBuffer());
    const float sizeX = m_CornerSizeX > 1.0f ? 2.0f : m_CornerSizeX * 2.0f;
    const float sizeY = m_CornerSizeY > 1.0f ? 2.0f : m_CornerSizeY * 2.0f;
    const uint32_t arcVertexCount = m_SliceCount + 1;
    const uint32_t cornerVertexCount = m_SliceCount + 2;

    for (int corner = 0; corner < CornerCount; corner++) {
        const uint32_t center = corner * cornerVertexCount;

        for (uint32_t i = 0; i < arcVertexCount; i++) {
            if (i != 0) {
                pIndex[0] = center;
                pIndex[1] = center + i;
                pIndex[2] = center + i + 1;
                pIndex += 3;
            }
        }
    }

    uint32_t base = arcVertexCount * CornerCount + CornerCount;

    if (sizeX < 2.0f) {
        pIndex[0] = base;
        pIndex[1] = base + 2;
        pIndex[2] = base + 3;
        pIndex[3] = base;
        pIndex[4] = base + 3;
        pIndex[5] = base + 1;
        pIndex[6] = base + 4;
        pIndex[7] = base + 7;
        pIndex[8] = base + 6;
        pIndex[9] = base + 4;
        pIndex[10] = base + 5;
        pIndex[11] = base + 7;
        pIndex += 12;
        base += 8;
    }

    if (sizeY < 2.0f) {
        pIndex[0] = base;
        pIndex[1] = base + 1;
        pIndex[2] = base + 2;
        pIndex[3] = base;
        pIndex[4] = base + 2;
        pIndex[5] = base + 3;
    }
}

/**
 * @brief Copies the buffers of another circle.
 * @param rSource Circle to copy.
 */
void Ui2dCircleShape::CopyParams(const Ui2dCircleShape& rSource) {
    SetVertexBufferSize(rSource.GetVertexBufferSize());
    SetIndexBufferSize(rSource.GetIndexBufferSize());
    SetVertexCount(rSource.GetVertexFormat());
    SetIndexCount(rSource.GetIndexCount());

    void* pVertexBuffer = Layout::AllocateMemory(rSource.GetVertexBufferSize());
    std::memcpy(pVertexBuffer, rSource.GetVertexBuffer(), rSource.GetVertexBufferSize());
    SetVertexBuffer(pVertexBuffer);

    void* pIndexBuffer = Layout::AllocateMemory(rSource.GetIndexBufferSize());
    std::memcpy(pIndexBuffer, rSource.GetIndexBuffer(), rSource.GetIndexBufferSize());
    SetIndexBuffer(pIndexBuffer);
}

/**
 * @brief Checks whether a layout draws a material that samples the frame buffer.
 * @param pLayout Layout to check.
 * @return True when a frame buffer texture descriptor slot is required.
 */
bool CheckFrameBufferTextureDescriptorSlotRequired(const Layout* pLayout) {
    return CheckFrameBufferTextureDescriptorSlotRequired(pLayout->GetRootPane());
}

/**
 * @brief Checks whether a pane tree draws a material that samples the frame buffer.
 * @param pPane Root of the pane tree.
 * @return True when a frame buffer texture descriptor slot is required.
 */
bool CheckFrameBufferTextureDescriptorSlotRequired(const Pane* pPane) {
    const u8 materialCount = static_cast<u8>(pPane->GetMaterialCount());

    for (int i = 0; i < materialCount; i++) {
        const Material* pMaterial = pPane->GetMaterial(i);

        if (pMaterial != nullptr && pMaterial->IsUseFramebufferTexture()) {
            const nn::gfx::Shader* pShader =
                pMaterial->GetShaderInfo()->GetPixelShader(pMaterial->GetShaderVariation());

            if (pShader->GetInterfaceSlot(nn::gfx::ShaderStage_Pixel,
                                          nn::gfx::ShaderInterfaceType_Sampler,
                                          "uTexture3") >= 0) {
                return true;
            }
        }
    }

    for (const nn::util::IntrusiveListNode* pNode = pPane->m_Children.GetNext();
         pNode != &pPane->m_Children; pNode = pNode->GetNext()) {
        if (CheckFrameBufferTextureDescriptorSlotRequired(Pane::FromLink(pNode))) {
            return true;
        }
    }

    return false;
}

namespace detail {

/**
 * @brief Formats a UTF-16 string.
 * @param pBuffer Receives the formatted string.
 * @param bufferLength Length of pBuffer in characters.
 * @param pFormat Format string.
 * @return Return value of vsnprintf for the UTF-8 conversion of the format.
 */
int VSNPrintf(uint16_t* pBuffer, size_t bufferLength, const uint16_t* pFormat, ...) {
    const int BufferSize = 1024;
    std::va_list args;
    va_start(args, pFormat);
    std::va_list formatArgs;
    va_copy(formatArgs, args);

    char format[BufferSize];
    nn::util::ConvertStringUtf16NativeToUtf8(format, BufferSize, pFormat, bufferLength - 1);

    char buffer[BufferSize];
    const int result = std::vsnprintf(buffer, bufferLength - 1, format, formatArgs);
    buffer[bufferLength - 1] = '\0';
    nn::util::ConvertStringUtf8ToUtf16Native(pBuffer, bufferLength, buffer, BufferSize);
    va_end(formatArgs);
    va_end(args);
    return result;
}

}  // namespace detail

}  // namespace nn::ui2d
