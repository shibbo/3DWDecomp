#pragma once
#include <nn/font/font_Font.h>
#include <nn/font/font_TextureCache.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/gfx/util/gfx_PrimitiveShape.h>
#include <nn/util/util_MathTypes.h>
#include <algorithm>

namespace nn {
namespace font {
class Font;
class TextureCache;

/** @brief Kind of a node of a complex font tree. */
enum ComplexFontNodeType {
    ComplexFontNodeType_Bitmap,
    ComplexFontNodeType_MultiScalable,
    ComplexFontNodeType_Pair,
};

/** @brief Header of a complex font (bfcpx) file. */
struct ResComplexFontHeader {
    uint32_t signature;
    uint16_t byteOrder;
    uint16_t headerSize;
    uint32_t version;
    uint32_t fileSize;
    uint16_t dataBlockCount;
    uint16_t reserved;
    uint32_t rootNodeOffset;

    /** @return The root node of the font tree. */
    const void* GetRootNode() const {
        return reinterpret_cast<const uint8_t*>(&rootNodeOffset) + rootNodeOffset;
    }
};

/** @brief Range of character codes a font of a complex font provides. */
struct ResCharCodeRange {
    uint32_t first;
    uint32_t last;
};

/** @brief Bitmap font node of a complex font tree. */
struct ResBitmapFont {
    uint32_t type;
    uint32_t nameOffset;
    int32_t charCodeRangeCount;
    uint32_t charCodeRangeOffset;
};

/** @brief Pair font node of a complex font tree. */
struct ResPairFont {
    uint32_t type;
    uint32_t firstOffset;
    uint32_t secondOffset;
};

/** @brief Scalable font of a multi scalable font node, before version 8.0.1. */
struct ResScalableFontDescriptionOld {
    float boldWeight;
    uint32_t fontIndex;
    uint32_t nameOffset;
    uint8_t borderWidth;
    uint8_t padding[3];
    int32_t charCodeRangeCount;
    uint32_t charCodeRangeOffset;
};

/** @brief Scalable font node of a complex font tree, before version 8.0.1. */
struct ResMultiScalableFontOld {
    uint32_t type;
    float size;
    uint32_t descriptionCount;
    uint32_t descriptionOffset;
};

/** @brief Scalable font of a multi scalable font node. */
struct ResScalableFontDescription {
    float boldWeight;
    float scaleWidth;
    uint32_t fontIndex;
    uint32_t nameOffset;
    uint8_t borderWidth;
    uint8_t flags;
    int16_t baselineOffset;
    float scaleHeight;
    int16_t letterSpacing;
    int16_t fixedWidth;
    uint16_t overwrittenAscent;
    uint16_t overwrittenDescent;
    int32_t charCodeRangeCount;
    uint32_t charCodeRangeOffset;
};

/** @brief Scalable font node of a complex font tree. */
struct ResMultiScalableFont {
    uint32_t type;
    float size;
    int32_t lineFeedOffset;
    uint32_t descriptionCount;
    uint32_t descriptionOffset;
    uint16_t alternateChar;
    uint16_t padding;
};
}  // namespace font
}  // namespace nn

namespace nn {
namespace gfx {
class ResShaderProgram;
class ResShaderContainer;
class ResShaderVariation;
class ViewportStateInfo;
class ScissorStateInfo;
};  // namespace gfx
namespace ui2d {
struct BuildArgSet;
class Pane;
class ResourceTextureInfo;
class TextureInfo;
class ShaderInfo;
class Material;
class DrawInfo;
struct ResExtUserData;
struct ResExtUserDataList;
struct ResHermiteKey;

/** @brief Easing curve of a parameterized animation. */
enum ParameterizedAnimType {
    ParameterizedAnimType_Linear,
    ParameterizedAnimType_SineIn,
    ParameterizedAnimType_SineOut,
    ParameterizedAnimType_SineInOut,
    ParameterizedAnimType_CubicIn,
    ParameterizedAnimType_CubicOut,
    ParameterizedAnimType_CubicInOut,
    ParameterizedAnimType_QuintIn,
    ParameterizedAnimType_QuintOut,
    ParameterizedAnimType_QuintInOut,
    ParameterizedAnimType_BackIn,
    ParameterizedAnimType_BackOut,
    ParameterizedAnimType_BackInOut,
    ParameterizedAnimType_ElasticIn,
    ParameterizedAnimType_ElasticOut,
    ParameterizedAnimType_ElasticInOut,
    ParameterizedAnimType_BounceIn,
    ParameterizedAnimType_BounceOut,
    ParameterizedAnimType_BounceInOut,
    ParameterizedAnimType_Max
};

/** @brief Start and target values of a parameterized animation. */
struct ResParameterizedAnimParameterValue {
    float startValue;
    float targetValue;
};

/** @brief One easing segment of a parameterized animation. */
struct ResParameterizedAnimParameter {
    u8 parameterizedAnimType;
    u8 padding[7];
    ResParameterizedAnimParameterValue value;
    float offset;
    float duration;
};

/** @brief Easing segments that animate one target. */
struct ResParameterizedAnim {
    u16 parameterCount;
    u8 padding[2];
    u32 parameterOffsets[1];

    /** @return The segment at index, stored relative to this block. */
    const ResParameterizedAnimParameter* GetParameter(int index) const {
        return reinterpret_cast<const ResParameterizedAnimParameter*>(
            reinterpret_cast<const u8*>(this) + parameterOffsets[index]);
    }
};

/** @brief Capture texture of a layout resource. */
struct ResCaptureTexture {
    u32 nameOffset;
    u8 _04[0x28];
    float textureScale;
};

/** @brief Capture textures of a layout resource. */
struct ResCaptureTextureList {
    u32 signature;
    u32 blockSize;
    u16 textureCount;
    u16 padding;

    /** @return The first capture texture of the list. */
    const ResCaptureTexture* GetTextureArray() const {
        return reinterpret_cast<const ResCaptureTexture*>(reinterpret_cast<const u8*>(this) + 12);
    }

    /** @return The name of pTexture, stored relative to the list. */
    const char* GetName(const ResCaptureTexture* pTexture) const {
        return reinterpret_cast<const char*>(this) + pTexture->nameOffset;
    }
};

struct ResVectorGraphicsTexture;

/** @brief Vector graphics textures of a layout resource. */
struct ResVectorGraphicsTextureList {
    /** @brief Size of one texture entry. */
    static const int TextureStride = 16;

    u32 signature;
    u32 blockSize;
    u16 textureCount;
    u16 padding;

    /** @return The first byte of the texture array. */
    const u8* GetTextureArray() const { return reinterpret_cast<const u8*>(this) + 12; }

    /** @return The texture at index. */
    const ResVectorGraphicsTexture* GetTexture(int index) const {
        return reinterpret_cast<const ResVectorGraphicsTexture*>(GetTextureArray() +
                                                                 index * TextureStride);
    }

    /** @return The bnvg file name of pTexture, stored relative to the texture array. */
    const char* GetFileName(const ResVectorGraphicsTexture* pTexture) const {
        return reinterpret_cast<const char*>(GetTextureArray()) +
               *reinterpret_cast<const u32*>(pTexture);
    }
};

/** @brief Builds the fonts of a complex font (bfcpx) file. */
class ComplexFontHelper {
public:
    /** @brief Loads the data of a font of a complex font. */
    typedef void* (*LoadFontCallback)(size_t* pSize, const char* pName, uint32_t type,
                                      void* pUserData);

    static int SetupTextureCacheArg(nn::font::TextureCache::InitializeArg* pArg,
                                    LoadFontCallback pLoadFunction, void* pUserData,
                                    const void* pComplexFontData);
    static nn::font::Font* InitializeComplexFontTree(
        nn::gfx::Device* pDevice, nn::font::RegisterTextureViewSlot pRegisterFunction,
        void* pRegisterUserData, nn::font::TextureCache* pTextureCache, int fontFaceHead,
        LoadFontCallback pLoadFunction, void* pUserData, const void* pComplexFontData);
    static void FinalizeComplexFontTree(nn::gfx::Device* pDevice, nn::font::Font* pFont,
                                        nn::font::UnregisterTextureViewSlot pUnregisterFunction,
                                        void* pUserData);

private:
    template <typename TMultiScalableFont, typename TScalableFontDescription>
    static void BuildTextureCacheArg(const void* pNode, int* pFontFace,
                                     nn::font::TextureCache::InitializeArg* pArg,
                                     LoadFontCallback pLoadFunction, void* pUserData,
                                     uint32_t version);

    template <typename TMultiScalableFont>
    static nn::font::Font* BuildFontTree(nn::gfx::Device* pDevice,
                                         nn::font::RegisterTextureViewSlot pRegisterFunction,
                                         void* pRegisterUserData, const void* pNode,
                                         int* pFontFace, nn::font::TextureCache* pTextureCache,
                                         LoadFontCallback pLoadFunction, void* pUserData,
                                         uint32_t version);

    static void DestroyFontTree(nn::gfx::Device* pDevice, nn::font::Font* pFont,
                                nn::font::UnregisterTextureViewSlot pUnregisterFunction,
                                void* pUserData);
    static bool CheckExt(const char* pName, const char* pExt);
};

/** @brief Rectangle with rounded corners, centered on the origin and two units wide. */
class RoundRectShape : public nn::gfx::util::PrimitiveShape {
public:
    RoundRectShape(nn::gfx::util::PrimitiveShapeFormat vertexFormat,
                   nn::gfx::PrimitiveTopology topology, float cornerSizeX, float cornerSizeY,
                   uint32_t sliceCount);
    RoundRectShape(nn::gfx::util::PrimitiveShapeFormat vertexFormat,
                   nn::gfx::PrimitiveTopology topology);
    ~RoundRectShape() override;

    void CopyParams(const RoundRectShape& rSource);

protected:
    int CalculateVertexCount();
    int CalculateIndexCount();
    void CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                       size_t indexSize) override;

private:
    void* CalculateVertexBuffer();

    template <typename T>
    void CalculateIndexBuffer();

    /** @return Size of the corners along X, clamped to the full width. */
    float GetClampedCornerSizeX() const { return std::min(m_CornerSizeX, 1.0f); }

    /** @return Size of the corners along Y, clamped to the full height. */
    float GetClampedCornerSizeY() const { return std::min(m_CornerSizeY, 1.0f); }

    float m_CornerSizeX;
    float m_CornerSizeY;
    uint32_t m_SliceCount;
};

/** @brief Circle shape whose buffers can be copied from another circle. */
class Ui2dCircleShape : public nn::gfx::util::CircleShape {
public:
    void CopyParams(const Ui2dCircleShape& rSource);
};

bool IsContain(const Pane* pPane, const nn::util::Float2& rPos);
bool LoadTexture(ResourceTextureInfo* pTextureInfo, nn::gfx::Device* pDevice,
                 const void* pResource);
void LoadArchiveShader(ShaderInfo* pShaderInfo, nn::gfx::Device* pDevice, void* pShader,
                       const void* pVariationTable, nn::gfx::MemoryPool* pMemoryPool,
                       ptrdiff_t memoryPoolOffset, size_t memoryPoolSize, int codeType);
void FreeArchiveShader(nn::gfx::Device* pDevice, ShaderInfo* pShaderInfo);
size_t CalcCaptureTexturePrefixLength(const BuildArgSet& args, int depth);
void ConcatCaptureTexturePrefixString(char* buffer, size_t size, const BuildArgSet& args,
                                      int depth);
// args contains the nested layout names; depth selects how many form the prefix.
size_t CalcDynamicGenerateTexturePrefixLength(const BuildArgSet& args, int depth);
// buffer/size describe the destination for the prefix selected by args and depth.
void ConcatDynamicGenerateTexturePrefixString(char* buffer, size_t size, const BuildArgSet& args,
                                              int depth);
void MakeCaptureTextureName(char* pBuffer, size_t bufferSize, const char* pPrefix,
                            const char* pName);
void MakeDynamicGenerateTextureName(char* pBuffer, size_t bufferSize, const char* pPrefix,
                                    const char* pName);
size_t GetAlignedBufferSize(nn::gfx::Device* pDevice, nn::gfx::GpuAccess gpuAccess, size_t size);
bool IsResShaderProgramInitialized(nn::gfx::ResShaderProgram*);

class AnimTransform;
class AnimResource;
class Group;
class Layout;
class ResourceAccessor;

struct Size;

bool ComparePaneTreeTest(const Pane* pLhs, const Pane* pRhs);
void DrawNullAndBoundingPane(nn::gfx::CommandBuffer& rCommandBuffer, DrawInfo& rDrawInfo,
                             Material& rMaterial, const Pane* pPane,
                             const nn::util::Unorm8x4& rNullColor,
                             const nn::util::Unorm8x4& rBoundingColor);
bool CheckFrameBufferTextureDescriptorSlotRequired(const Layout* pLayout);
bool CheckFrameBufferTextureDescriptorSlotRequired(const Pane* pPane);
void BindAnimation(AnimTransform* pTransform, Group* pGroup, bool isEnabled);
void UnbindAnimation(AnimTransform* pTransform, Group* pGroup);
Pane* FindHitPane(Pane* pPane, const nn::util::Float2& rPos);
const Pane* FindHitPane(const Pane* pPane, const nn::util::Float2& rPos);
Pane* FindHitPane(Layout* pLayout, const nn::util::Float2& rPos);
const Pane* FindHitPane(const Layout* pLayout, const nn::util::Float2& rPos);
Pane* GetNextPane(Pane* pPane);
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice);
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout);
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice,
                    ResourceAccessor* pResAccessor, const char* pNewRootName);
Pane* ClonePaneTree(const Pane* pSrcPane, nn::gfx::Device* pDevice,
                    ResourceAccessor* pResAccessor, const char* pNewRootName,
                    const Layout* pLayout);
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout);
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout,
                                   ResourceAccessor* pResAccessor, const char* pNewRootName);
Pane* ClonePaneTreeWithPartsLayout(const Pane* pSrcPane, Layout* pPartsLayout,
                                   nn::gfx::Device* pDevice, Layout* pLayout,
                                   ResourceAccessor* pResAccessor, const char* pNewRootName,
                                   const Layout* pSrcLayout);
float GetHermiteCurveValue(float frame, const ResHermiteKey* pKeys, int keyCount);
float GetParameterizedAnimValue(float frame, float defaultValue,
                                const ResParameterizedAnim* pAnim);
float GetParameterizedAnimValueAtFrameClamped(float frame,
                                              const ResParameterizedAnimParameter* pParameter);
float GetParameterizedAnimValueAtFrame_(float frame, float duration, float ratio,
                                        float startValue, float targetValue, u8 curveType);
float GetParameterizedAnimValueAtFrameClamped(float frame, float duration, float offset,
                                              float startValue, float targetValue,
                                              u8 curveType);
float GetParameterizedAnimValueAtFrame(float frame,
                                       const ResParameterizedAnimParameter* pParameter);
const ResExtUserData* GetExtUserData(const ResExtUserDataList* pList, const char* pName);
TextureInfo* AcquireCaptureTextureWithResolvePrefix(char* pOutName, int outNameSize,
                                                    const BuildArgSet& rArgs, bool isAlternate,
                                                    nn::gfx::Device* pDevice,
                                                    ResourceAccessor* pResAccessor,
                                                    const char* pName);
TextureInfo* AcquireDynamicGenerateTextureWithResolvePrefix(char* pOutName, int outNameSize,
                                                            const BuildArgSet& rArgs,
                                                            bool isAlternate,
                                                            nn::gfx::Device* pDevice,
                                                            ResourceAccessor* pResAccessor,
                                                            const char* pName);
TextureInfo* AcquireCaptureTexture(char* pOutName, int outNameSize, nn::gfx::Device* pDevice,
                                   ResourceAccessor* pResAccessor, const char* pPrefix,
                                   const char* pName);
TextureInfo* AcquireDynamicGenerateTexture(char* pOutName, int outNameSize,
                                           nn::gfx::Device* pDevice,
                                           ResourceAccessor* pResAccessor, const char* pPrefix,
                                           const char* pName);

namespace detail {
class BuildPaneTreeContext;

Pane* ClonePaneTreeImpl_(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout);
Pane* ClonePaneTreeImpl_(const Pane* pSrcPane, nn::gfx::Device* pDevice, Layout* pLayout,
                         BuildPaneTreeContext* pContext);
Pane* ClonePaneTreeWithPartsLayoutImpl_(const Pane* pSrcPane, Layout* pPartsLayout,
                                        nn::gfx::Device* pDevice, Layout* pLayout);
Pane* ClonePaneTreeWithPartsLayoutImpl_(const Pane* pSource, Layout* pPartsLayout,
                                        nn::gfx::Device* pDevice, Layout* pLayout,
                                        BuildPaneTreeContext* pContext);
TextureInfo* AcquireDynamicGenerateTextureWithResolvePrefixImpl(
    char* pOutName, int outNameSize, const BuildArgSet& rArgs, bool isAlternate,
    nn::gfx::Device* pDevice, ResourceAccessor* pResAccessor, const char* pName);
TextureInfo* AcquireDynamicGenerateTextureImpl(char* pOutName, int outNameSize,
                                               nn::gfx::Device* pDevice,
                                               ResourceAccessor* pResAccessor,
                                               const char* pPrefix, const char* pName);
const ResCaptureTexture* FindCaptureTextureResource(const ResCaptureTextureList* pList,
                                                    const char* pName);
float CalculateCaptureTextureScale(const ResCaptureTexture* pTexture);
const ResVectorGraphicsTexture* FindVectorGraphicsTextureResource(
    const ResVectorGraphicsTextureList* pList, const char* pName);
const char* FindCaptureTextureBaseNameFromOverwriteString(const char* pName);
int CalcCaptureTextureNameOverwriteDepth(const char* pName);
void ClampValue(float& rValue, float minimum, float maximum);
void SetupViewportScissorStateInfo(nn::gfx::ViewportStateInfo* pViewportInfo,
                                   nn::gfx::ScissorStateInfo* pScissorInfo, float width,
                                   float height);
char* AllocateAndCopyString(const char* pSource);
void DrawBox(nn::gfx::CommandBuffer& rCommandBuffer, DrawInfo& rDrawInfo, Material& rMaterial,
             const nn::util::Float2& rPos, const Size& rSize, const nn::util::Unorm8x4& rColor);
int VSNPrintf(uint16_t* pBuffer, size_t bufferLength, const uint16_t* pFormat, ...);

/** @brief Pane subtree whose animation contents are shared with other panes. */
class AnimPaneTree {
public:
    AnimPaneTree(Pane* pTargetPane, const AnimResource& rResource);
    void Bind(nn::gfx::Device* pDevice, Layout* pLayout, Pane* pTargetPane,
              ResourceAccessor* pResourceAccessor) const;

    /**
     * @brief Check whether the source pane tree has any animation contents.
     * @return True when at least one animation content targets the tree.
     */
    bool IsEnabled() const { return m_AnimCount != 0; }

    unsigned char _00[0x70];
    u16 m_AnimCount;
    unsigned char _72[0xe];
};
}  // namespace detail

bool IsResShaderContainerInitialized(nn::gfx::ResShaderContainer*);

nn::gfx::ShaderCodeType TryInitializeAndGetShaderCodeType(nn::gfx::Device*,
                                                          nn::gfx::ResShaderVariation*);
};  // namespace ui2d
};  // namespace nn
