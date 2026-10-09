/**
 * @file Material.h
 * @brief UI Material implementation.
 */

#pragma once

#include <nn/types.h>
#include <nn/ui2d/ui2d_Types.h>
#include <nn/ui2d/ui2d_Resources.h>
#include <nn/ui2d/ui2d_TexMap.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/gfx/gfx_State.h>

namespace nn {
namespace ui2d {
class AnimTransform;
class BuildResultInformation;
struct UserShaderInformation;
class TextureInfo;
class DrawInfo;
struct Size;
struct ResMaterial;
struct ResExtUserData;
struct ResExtUserDataList;
class CaptureTexture;
struct BuildArgSet;
struct BuildResSet;
class Layout;
class ResourceAccessor;
namespace detail {
class BuildPaneTreeContext;
}

/** @brief Objects a material copy needs to rebuild its device resources. */
struct MaterialCopyContext {
    nn::gfx::Device* pDevice;
    ResourceAccessor* pResourceAccessor;
    const Layout* pLayout;
    detail::BuildPaneTreeContext* pBuildPaneTreeContext;
};
class ShaderInfo;

enum ShaderVariation {
    ShaderVariation_Standard,
    ShaderVariation_WithoutVertexColor,
};

/** @brief Interpolation colours held by a material. */
enum MaterialColor {
    MaterialColor_Black,
    MaterialColor_White,
    MaterialColor_Max
};

const int TexMapMax = 3;
const int TevStageMax = 6;
const int ShaderKeyMax = 24;

/** @brief Source of generated texture coordinates. */
enum TexGenSrc {
    TexGenSrc_Tex0,
    TexGenSrc_Tex1,
    TexGenSrc_Tex2,
    TexGenSrc_OrthogonalProjection,
    TexGenSrc_PaneBasedProjection,
    TexGenSrc_PerspectiveProjection,
    TexGenSrc_PaneBasedPerspectiveProjection,
    TexGenSrc_BrickRepeat,
    TexGenSrc_MaxTexGenSrc
};

/** @brief Colour combiner modes of the built-in shaders. */
enum TevMode {
    TevMode_Replace,
    TevMode_Modulate,
    TevMode_Add,
    TevMode_Exclusion,
    TevMode_Interpolate,
    TevMode_Subtract,
    TevMode_Dodge,
    TevMode_Burn,
    TevMode_Overlay,
    TevMode_Lighten,
    TevMode_Darken,
    TevMode_Indirect,
    TevMode_BlendIndirect,
    TevMode_EachIndirect,
    TevMode_MaxTevMode
};

/** @brief Built-in shader used by a material. */
enum ShaderId {
    ShaderId_NoTexture = 0,
    ShaderId_SingleTexture = 1,
    ShaderId_DoubleTextureBase = 2,
    ShaderId_IndirectBlend = 13,
    ShaderId_Undefined = 14,
    ShaderId_Archive = 15
};

/** @brief Two-component vector stored in layout resources. */
struct ResVec2 {
    float x;
    float y;
};

/** @brief Texture reference and sampler settings of a material resource. */
struct ResTexMap {
    u16 texIdx;
    u8 wrapSflt;
    u8 wrapTflt;

    /** @return The horizontal wrap mode. */
    TexWrap GetWarpModeS() const { return TexWrap(detail::GetBits<u8>(wrapSflt, 0, 2)); }
    /** @return The vertical wrap mode. */
    TexWrap GetWarpModeT() const { return TexWrap(detail::GetBits<u8>(wrapTflt, 0, 2)); }
    /** @return The minification filter. */
    TexFilter GetMinFilter() const { return TexFilter(detail::GetBits<u8>(wrapSflt, 2, 2)); }
    /** @return The magnification filter. */
    TexFilter GetMagFilter() const { return TexFilter(detail::GetBits<u8>(wrapTflt, 2, 2)); }
};

/** @brief Says where a material texture is generated at runtime. */
struct ResTexMapAdditionalInfo {
    enum Info {
        Info_CaptureTexture = 1 << 0,
        Info_VectorGraphicsTexture = 1 << 1,
    };

    u32 info;
};

/** @brief Texture scale/rotate/translate parameters. */
struct ResTexSrt {
    ResVec2 translate;
    float rotate;
    ResVec2 scale;
};

struct ResProjectionTexGenParameters;

/** @brief Texture coordinate generation parameters. */
struct ResTexCoordGen {
    u8 texGenType;
    u8 texGenSrc;
    u8 reserve[2];
    u32 reserve2;
    const ResProjectionTexGenParameters* pProjectionTexGenParameters;

    ResTexCoordGen() : texGenType(0), texGenSrc(0), pProjectionTexGenParameters(nullptr) {
        reserve[0] = 0;
        reserve[1] = 0;
    }

    /** @return The source of the generated coordinates. */
    TexGenSrc GetTexGenSrc() const { return static_cast<TexGenSrc>(texGenSrc); }

    /** @return Whether the coordinates are projected from the layout, pane or texture. */
    bool IsProjection() const {
        const TexGenSrc src = GetTexGenSrc();
        return src == TexGenSrc_PaneBasedPerspectiveProjection ||
               (src >= TexGenSrc_OrthogonalProjection && src <= TexGenSrc_PerspectiveProjection);
    }

    /** @return Whether the coordinates use a perspective projection. */
    bool IsPerspectiveProjection() const {
        const TexGenSrc src = GetTexGenSrc();
        return src >= TexGenSrc_PerspectiveProjection &&
               src <= TexGenSrc_PaneBasedPerspectiveProjection;
    }
};

/** @brief Alpha test settings. */
struct ResAlphaCompare {
    u8 func;
    float ref;

    ResAlphaCompare() {}
    /**
     * @param aFunc Comparison function.
     * @param aRef Reference value.
     */
    ResAlphaCompare(AlphaTest aFunc, float aRef) : func(aFunc), ref(aRef) {}
};

/** @brief Indirect texture parameters. */
struct ResIndirectParameter {
    float rotate;
    ResVec2 scale;
};

/** @brief Colour combiner stage. */
struct ResTevStage {
    u8 combineRgb;
    u8 combineAlpha;
    u8 reserve[2];

    /** @return Whether the stage blends through an indirect texture. */
    bool IsIndirectBlend() const {
        return combineRgb >= TevMode_Indirect && combineRgb <= TevMode_EachIndirect;
    }
};

/** @brief Parameters of a projected texture coordinate. */
struct ResProjectionTexGenParameters {
    ResVec2 translate;
    ResVec2 scale;
    u8 flag;
    u8 reserve[3];
};

/** @brief Parameters of a brick repeated texture coordinate. */
struct ResBrickRepeatTexGenParameters {
    float params[6];
    float angle;
    u8 reserve0[0x14];
    float offset[2];
    float range[2];
    u8 flag;
    u8 reserve1[0x17];
};

/** @brief Font shadow interpolation colours. */
struct ResFontShadowParameter {
    u8 blackInterporateColor[3];
    u8 whiteInterporateColor[4];
    u8 reserve;
};

/** @brief Global settings of the detailed combiner. */
struct ResDetailedCombinerStageInfo {
    u32 bits;
    util::Unorm8x4 constantColor[5];
    u32 reserve;
};

/** @brief One stage of the detailed combiner. */
struct ResDetailedCombinerStage {
    u32 bits[4];
};

/** @brief Settings of the combiner user shader. */
struct ResCombinerUserShader {
    u32 keys[24];
    u8 constantColor[5][4];
};

/** @brief Vector graphics texture used by a material texture. */
struct ResVectorGraphicsTextureInfo {
    float time;
    u8 color[4];
    u8 reserve[8];
};

/** @brief Shader replacement chosen by the application for a material. */
struct UserShaderInformation {
    char userShaderName[8];
    u32 vertexShaderConstantBufferExtendSize;
    u32 geometryShaderConstantBufferExtendSize;
    u32 pixelShaderConstantBufferExtendSize;

    void SetDefault();
    void SetShaderName(const char* pName);
};

typedef bool (*GetUserShaderInformationFromUserDataCallback)(UserShaderInformation& rInfo,
                                                             const ResExtUserDataList* pList,
                                                             void* pUserData);

/** @brief Constant buffer location of the brick repeat pixel shader. */
struct BrickRepeatShaderInfo {
    size_t constantBufferOffset;
    int slot;
};

/** @brief Interpolation colours of a material resource, stored as bytes or floats. */
struct ResMaterialColor {
    u8 byteColorFlags;
    u8 count;
    u8 offsets[MaterialColor_Max];

    /**
     * @param colorIdx Colour to get.
     * @return The colour stored as bytes.
     */
    const nn::util::Unorm8x4* GetByteColor(int colorIdx) const {
        return reinterpret_cast<const nn::util::Unorm8x4*>(reinterpret_cast<const u8*>(this) +
                                                          offsets[colorIdx]);
    }

    /**
     * @param colorIdx Colour to get.
     * @return The colour stored as floats.
     */
    const nn::util::Float4* GetFloatColor(int colorIdx) const {
        return reinterpret_cast<const nn::util::Float4*>(reinterpret_cast<const u8*>(this) +
                                                        offsets[colorIdx]);
    }

    /** @return Size of the colour block. */
    size_t GetSize() const {
        size_t size = count + 2;
        for (int i = 0; i < count; i++) {
            size += (byteColorFlags & (1 << i)) ? sizeof(nn::util::Float4) : sizeof(nn::util::Unorm8x4);
        }

        return size;
    }
};

/** @brief Material resource; the parameter blocks follow the colour block. */
struct ResMaterial {
    /** @brief Bits of flags. */
    enum Flag {
        Flag_AlphaCompare = 1 << 9,
        Flag_BlendMode = 1 << 10,
        Flag_TextureOnly = 1 << 11,
        Flag_BlendModeAlpha = 1 << 12,
        Flag_IndirectParameter = 1 << 14,
        Flag_FontShadowParameter = 1 << 17,
        Flag_ThresholdingAlphaInterpolation = 1 << 18,
        Flag_DetailedCombiner = 1 << 19,
        Flag_CombinerUserShader = 1 << 20,
        Flag_TexMapAdditionalInfo = 1 << 21,
    };

    char name[28];
    u32 flags;
    ResMaterialColor color;

    /**
     * @param offset Byte offset from the start of the resource.
     * @return Pointer to the parameter block stored at offset.
     */
    template <typename T>
    const T* GetPtr(u32 offset) const {
        return reinterpret_cast<const T*>(reinterpret_cast<const u8*>(this) + offset);
    }
};

namespace detail {
class VectorGraphicsTexture;

const int CombinerUserShaderConstantColorMax = 5;

/** @brief Counts of the parameter blocks stored in a material's memory. */
struct MatMemCount {
    u32 texMap : 2;
    u32 texSrt : 2;
    u32 texCoordGen : 2;
    u32 tevStage : 3;
    u32 alpComp : 1;
    u32 blendMode : 2;
    u32 indirectParameter : 1;
    u32 projectionTexGen : 2;
    u32 fontShadowParameter : 1;
    u32 detailedCombinerParameter : 1;
    u32 combinerUserShaderParameter : 1;
    u32 vectorGraphicsTexture : 2;
    u32 brickRepeatParameter : 2;
};

/** @brief Runtime link between a material texture and a vector graphics texture. */
struct RefVectorGraphicsTextureInfo {
    float time;
    nn::util::Float4 color;
    VectorGraphicsTexture* pTexture;
};
}  // namespace detail

class Material {
public:
    Material();
    Material(const Material& rOther, MaterialCopyContext& rContext);

    /** @brief Builds a material from its resource and an optional override resource. */
    Material(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const ResMaterial* pBaseRes,
             const ResMaterial* pOverrideRes, const BuildArgSet& rArgs) {
        InitializeMaterialImpl(pResult, pDevice, pBaseRes, pOverrideRes, rArgs);
    }

    void SetColorElement(int colorType, int value);
    void Initialize();
    bool ReserveMem(s32, s32, s32, s32, bool, s32, bool, s32, bool, bool, bool, s32, s32);
    void InitializeMaterialImpl(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                const ResMaterial* pBaseRes, const ResMaterial* pOverrideRes,
                                const BuildArgSet& rArgs);
    void InitializeDynamicRenderingTexture(BuildResultInformation* pResult,
                                           nn::gfx::Device* pDevice, const BuildArgSet& rArgs);
    void Finalize(nn::gfx::Device* pDevice);
    void AllocateConstantBuffer(DrawInfo& rDrawInfo);
    void SetupBlendState(const DrawInfo* pDrawInfo);
    void* GetConstantBufferForVertexShader(const DrawInfo& rDrawInfo) const;
    void* GetConstantBufferForPixelShader(const DrawInfo& rDrawInfo) const;
    void SetShader(nn::gfx::CommandBuffer& rCommands) const;
    void SetCommandBuffer(nn::gfx::CommandBuffer& rCommands, DrawInfo& rDrawInfo) const;
    void SetCommandBufferOnlyBlend(nn::gfx::CommandBuffer& rCommands) const;
    void SetupSubmaterialOf_Texture(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) const;
    void ApplyVertexShaderConstantBuffer(nn::gfx::CommandBuffer& rCommands,
                                         DrawInfo& rDrawInfo) const;
    void ApplyGeometryShaderConstantBuffer(nn::gfx::CommandBuffer& rCommands,
                                           DrawInfo& rDrawInfo) const;
    void ApplyPixelShaderConstantBuffer(nn::gfx::CommandBuffer& rCommands,
                                        DrawInfo& rDrawInfo) const;
    bool CompareCopiedInstanceTest(const Material& rTarget) const;
    void SetupUserShaderConstantBufferInformation(nn::ui2d::UserShaderInformation const&);
    size_t GetVertexShaderConstantBufferSize() const;
    size_t GetPixelShaderConstantBufferSize() const;
    size_t GetGeometryShaderConstantBufferSize() const;
    size_t GetPixelShaderDetailedCombinerConstantBufferSize() const;
    size_t GetPixelShaderCombinerUserShaderConstantBufferSize() const;
    bool IsUseFramebufferTexture() const;

    struct ConstantBufferForVertexShader;

    virtual ~Material();
    virtual void BindAnimation(nn::ui2d::AnimTransform*);
    virtual void UnbindAnimation(nn::ui2d::AnimTransform*);
    virtual void SetupGraphics(DrawInfo& rDrawInfo, u8 alpha, ShaderVariation variation,
                               bool isGlobalMatrixUpdate, const nn::util::MatrixT4x3fType& rGlobalMtx,
                               const Size* pSize, const ResExtUserData* pExtUserData,
                               u16 extUserDataCount);

    /** @return Whether the owner pane must not delete this material. */
    bool IsUserAllocated() const { return m_IsUserAllocated != 0; }
    /** @return The number of texture maps in use. */
    int GetTexMapCount() const { return m_MemNum.texMap; }
    /** @return Whether memory for combiner user shader parameters is reserved. */
    bool IsCombinerUserShaderCapable() const { return m_MemCap.combinerUserShaderParameter != 0; }
    /** @return The material name. */
    const char* GetName() const { return mName; }
    /** @return The shader used to draw this material. */
    const ShaderInfo* GetShaderInfo() const { return static_cast<const ShaderInfo*>(mShaderInfo); }
    /** @return The variation index within the shader. */
    u16 GetShaderVariation() const { return mShaderVariation; }

    void SetTextureNum(u8 count) { mTextureCount = (mTextureCount & 0xf) | (count << 4); }
    TexMap* GetFirstTexMap() { return m_pTexMaps; }
    const u8* GetWhiteColor() const { return &_08[4]; }
    // index selects the material slot; pInfo supplies its texture description.
    void SetTextureInfo(int index, const TextureInfo* pInfo) {
        m_pTexMaps[index].m_pTextureInfo = pInfo;
    }

    struct UserShaderConstantBufferInformation {
        u32 vertexSize, pixelSize, geometrySize, flags;
    };

    /** @return Number of texture maps in use. */
    u8 GetTexMapNum() const { return static_cast<u8>(m_MemNum.texMap); }
    /** @return Number of texture maps the memory can hold. */
    u8 GetTexMapCap() const { return static_cast<u8>(m_MemCap.texMap); }
    /** @return Number of texture SRT parameters in use. */
    u8 GetTexSrtNum() const { return static_cast<u8>(m_MemNum.texSrt); }
    /** @return Number of texture SRT parameters the memory can hold. */
    u8 GetTexSrtCap() const { return static_cast<u8>(m_MemCap.texSrt); }
    /** @return Number of texture coordinate generators in use. */
    u8 GetTexCoordGenNum() const { return static_cast<u8>(m_MemNum.texCoordGen); }
    /** @return Number of texture coordinate generators the memory can hold. */
    u8 GetTexCoordGenCap() const { return static_cast<u8>(m_MemCap.texCoordGen); }
    /** @return Number of combiner stages in use. */
    u8 GetTevStageNum() const { return static_cast<u8>(m_MemNum.tevStage); }
    /** @return Number of combiner stages the memory can hold. */
    u8 GetTevStageCap() const { return static_cast<u8>(m_MemCap.tevStage); }
    /** @return Number of projection texture generation parameters in use. */
    u8 GetProjectionTexGenNum() const { return static_cast<u8>(m_MemNum.projectionTexGen); }
    /** @return Number of projection texture generation parameters the memory can hold. */
    u8 GetProjectionTexGenCap() const { return static_cast<u8>(m_MemCap.projectionTexGen); }
    /** @return Number of vector graphics texture references in use. */
    u8 GetVectorGraphicsTextureNum() const {
        return static_cast<u8>(m_MemNum.vectorGraphicsTexture);
    }
    /** @return Number of brick repeat texture generation parameters in use. */
    u8 GetBrickRepeatNum() const { return static_cast<u8>(m_MemNum.brickRepeatParameter); }
    /** @return Number of brick repeat texture generation parameters the memory can hold. */
    u8 GetBrickRepeatCap() const { return static_cast<u8>(m_MemCap.brickRepeatParameter); }

    /** @return Byte offset of the texture SRT array within the parameter memory. */
    int GetTexSrtOffset() const { return sizeof(TexMap) * m_MemCap.texMap; }
    /** @return Byte offset of the texture coordinate generator array. */
    int GetTexCoordGenOffset() const {
        return GetTexSrtOffset() + sizeof(ResTexSrt) * m_MemCap.texSrt;
    }
    /** @return Byte offset of the alpha compare settings. */
    int GetAlphaCompareOffset() const {
        return GetTexCoordGenOffset() + sizeof(ResTexCoordGen) * m_MemCap.texCoordGen;
    }
    /** @return Byte offset of the blend mode array. */
    int GetBlendModeOffset() const {
        return GetAlphaCompareOffset() + sizeof(ResAlphaCompare) * m_MemCap.alpComp;
    }
    /** @return Byte offset of the indirect parameter. */
    int GetIndirectParameterOffset() const {
        return GetBlendModeOffset() + sizeof(ResBlendMode) * m_MemCap.blendMode;
    }
    /** @return Byte offset of the combiner stage array. */
    int GetTevStageOffset() const {
        return GetIndirectParameterOffset() +
               sizeof(ResIndirectParameter) * m_MemCap.indirectParameter;
    }
    /** @return Byte offset of the projection texture generation parameters. */
    int GetProjectionTexGenOffset() const {
        return GetTevStageOffset() + sizeof(ResTevStage) * m_MemCap.tevStage;
    }
    /** @return Byte offset of the font shadow parameter. */
    int GetFontShadowParameterOffset() const {
        return GetProjectionTexGenOffset() +
               sizeof(ResProjectionTexGenParameters) * m_MemCap.projectionTexGen;
    }
    /** @return Byte offset of the detailed combiner settings. */
    int GetDetailedCombinerStageInfoOffset() const {
        return GetFontShadowParameterOffset() +
               sizeof(ResFontShadowParameter) * m_MemCap.fontShadowParameter;
    }
    /** @return Byte offset of the detailed combiner stage array. */
    int GetDetailedCombinerStageOffset() const {
        return GetDetailedCombinerStageInfoOffset() +
               sizeof(ResDetailedCombinerStageInfo) * m_MemCap.detailedCombinerParameter;
    }
    /** @return Byte offset of the combiner user shader settings. */
    int GetCombinerUserShaderOffset() const {
        return GetDetailedCombinerStageOffset() + sizeof(ResDetailedCombinerStage) *
                                                      m_MemCap.detailedCombinerParameter *
                                                      m_MemCap.tevStage;
    }
    /** @return Byte offset of the vector graphics texture references. */
    int GetVectorGraphicsTextureRefInfoOffset() const {
        return GetCombinerUserShaderOffset() +
               sizeof(ResCombinerUserShader) * m_MemCap.combinerUserShaderParameter;
    }
    /** @return Byte offset of the brick repeat shader information. */
    int GetBrickRepeatShaderInfoOffset() const {
        return GetVectorGraphicsTextureRefInfoOffset() +
               sizeof(detail::RefVectorGraphicsTextureInfo) * m_MemCap.vectorGraphicsTexture;
    }
    /** @return Byte offset of the brick repeat texture generation parameters. */
    int GetBrickRepeatTexGenParametersOffset() const {
        return GetBrickRepeatShaderInfoOffset() +
               (m_MemCap.brickRepeatParameter != 0 ? sizeof(BrickRepeatShaderInfo) : 0);
    }

    /**
     * @param offset Byte offset within the parameter memory.
     * @return Pointer to the parameter block stored at offset.
     */
    template <typename T>
    T* GetMemPtr(int offset) const {
        return reinterpret_cast<T*>(static_cast<u8*>(m_pMem) + offset);
    }

    /** @return The texture maps of the material. */
    TexMap* GetTexMapAry() const { return static_cast<TexMap*>(m_pMem); }
    /** @return The texture SRT parameters. */
    ResTexSrt* GetTexSrtAry() const { return GetMemPtr<ResTexSrt>(GetTexSrtOffset()); }
    /** @return The texture coordinate generators. */
    ResTexCoordGen* GetTexCoordGenAry() const {
        return GetMemPtr<ResTexCoordGen>(GetTexCoordGenOffset());
    }
    /** @return The alpha compare settings. */
    ResAlphaCompare* GetAlphaComparePtr() const {
        return GetMemPtr<ResAlphaCompare>(GetAlphaCompareOffset());
    }
    /** @return The blend modes (colour, then alpha). */
    ResBlendMode* GetBlendModeAry() const { return GetMemPtr<ResBlendMode>(GetBlendModeOffset()); }
    /** @return The indirect texture parameter. */
    ResIndirectParameter* GetIndirectParameterPtr() const {
        return GetMemPtr<ResIndirectParameter>(GetIndirectParameterOffset());
    }
    /** @return The combiner stages. */
    ResTevStage* GetTevStageAry() const { return GetMemPtr<ResTevStage>(GetTevStageOffset()); }
    /** @return The projection texture generation parameters. */
    ResProjectionTexGenParameters* GetProjectionTexGenAry() const {
        return GetMemPtr<ResProjectionTexGenParameters>(GetProjectionTexGenOffset());
    }
    /** @return The font shadow parameter. */
    ResFontShadowParameter* GetFontShadowParameterPtr() const {
        return GetMemPtr<ResFontShadowParameter>(GetFontShadowParameterOffset());
    }
    /** @return The detailed combiner settings. */
    ResDetailedCombinerStageInfo* GetDetailedCombinerStageInfoPtr() const {
        return GetMemPtr<ResDetailedCombinerStageInfo>(GetDetailedCombinerStageInfoOffset());
    }
    /** @return The detailed combiner stages. */
    ResDetailedCombinerStage* GetDetailedCombinerStageAry() const {
        return GetMemPtr<ResDetailedCombinerStage>(GetDetailedCombinerStageOffset());
    }
    /** @return The combiner user shader settings. */
    ResCombinerUserShader* GetCombinerUserShaderPtr() const {
        return GetMemPtr<ResCombinerUserShader>(GetCombinerUserShaderOffset());
    }
    /** @return The vector graphics texture references. */
    detail::RefVectorGraphicsTextureInfo* GetVectorGraphicsTextureRefInfoAry() const {
        return GetMemPtr<detail::RefVectorGraphicsTextureInfo>(
            GetVectorGraphicsTextureRefInfoOffset());
    }
    /** @return The brick repeat shader information. */
    BrickRepeatShaderInfo* GetBrickRepeatShaderInfoPtr() const {
        return GetMemPtr<BrickRepeatShaderInfo>(GetBrickRepeatShaderInfoOffset());
    }
    /** @return The brick repeat texture generation parameters. */
    ResBrickRepeatTexGenParameters* GetBrickRepeatTexGenParametersAry() const {
        return GetMemPtr<ResBrickRepeatTexGenParameters>(GetBrickRepeatTexGenParametersOffset());
    }

    /** @brief Pixel shader constants used by the combiner user shader. */
    struct ConstantBufferForCombinerUserShaderPixelShader {
        int frameCount;
        float paneSize[2];
        float reserve0;
        float colors[MaterialColor_Max][4];
        float constantColors[detail::CombinerUserShaderConstantColorMax][4];
        float paneMtx[3][4];
        float globalMtx[3][4];
        float viewMtx[3][4];
        float cameraPosition[3];
        float reserve11c;
        float extFloat[4];
        float extVec2[4][4];
        float extVec3[4][4];
        float extRgba[4][4];
        float textureSize[TexMapMax][4];
    };

    /** @brief Pixel shader constants of the built-in combiners. */
    struct ConstantBufferForPixelShader {
        nn::util::Float4 interpolateWidth;
        nn::util::Float4 interpolateOffset;
        float indirectMtx0[4];
        float indirectMtx1[4];
        u8 reserve40[0x40];
        u32 tevAlphaFlags;
        u32 reserve84;
        u8 reserve88[8];
    };

    /** @brief Pixel shader constants of the detailed combiner. */
    struct ConstantBufferForDetailedCombinerPixelShader {
        int stageCount;
        int reserve;
        u8 reserve8[8];
        nn::util::Float4 colors[MaterialColor_Max];
        nn::util::Float4 constantColors[detail::CombinerUserShaderConstantColorMax];
        u32 stageBits[TevStageMax][4];
    };

    /** @brief Pixel shader constants of the brick repeated textures. */
    struct BrickRepeatConstantBuffer {
        float params0[TexMapMax][4];
        float params1[TexMapMax][4];
        float ranges[TexMapMax][4];
        float flags[TexMapMax][4];
    };

    static const size_t BrickRepeatConstantBufferSize = 0xc0;

    /** @return The colour blend mode. */
    const ResBlendMode* GetBlendMode() const { return &GetBlendModeAry()[0]; }
    /** @return The alpha blend mode, or the colour blend mode when none is set. */
    const ResBlendMode* GetBlendModeAlpha() const {
        return m_MemCap.blendMode == 2 ? &GetBlendModeAry()[1] : &GetBlendModeAry()[0];
    }

    /**
     * @param colorIdx Colour to get.
     * @return The colour normalized to [0, 1].
     */
    nn::util::Float4 GetColorFloat(int colorIdx) const {
        nn::util::Float4 color;
        if (colorIdx == MaterialColor_Black ? m_IsBlackColorFloat : m_IsWhiteColorFloat) {
            color = m_pFloatColors[colorIdx];
        } else {
            color.x = m_ByteColors[colorIdx].v[0] * (1.0f / 255.0f);
            color.y = m_ByteColors[colorIdx].v[1] * (1.0f / 255.0f);
            color.z = m_ByteColors[colorIdx].v[2] * (1.0f / 255.0f);
            color.w = m_ByteColors[colorIdx].v[3] * (1.0f / 255.0f);
        }

        return color;
    }

    Material(const Material& rOther, nn::gfx::Device* pDevice);

    void InitializeMatMemCount(detail::MatMemCount* pCount) const;
    void InitializeTexMap(nn::gfx::Device* pDevice, const BuildResSet* pBuildResSet,
                          const BuildArgSet& rBuildArgSet, const ResMaterial* pResMaterial,
                          const ResTexMap* pResTexMap,
                          const ResTexMapAdditionalInfo* pAdditionalInfo,
                          const ResVectorGraphicsTextureInfo* pVectorGraphicsInfo);
    void SetupShader(nn::gfx::Device* pDevice, const BuildArgSet& rBuildArgSet,
                     const BuildResSet* pBuildResSet);
    void CollectConstantBufferSize(BuildResultInformation* pResult, nn::gfx::Device* pDevice) const;
    void InitializeBlendInformationImpl(nn::gfx::Device* pDevice);
    void FinalizeBlendInformationImpl(nn::gfx::Device* pDevice);
    void SetVectorGraphicsTexturRefInfo(detail::RefVectorGraphicsTextureInfo* pRefInfo,
                                        const ResVectorGraphicsTextureInfo& rResInfo,
                                        detail::VectorGraphicsTexture* pTexture) const;
    CaptureTexture* InitializeCaptureTextureReference(TexMap* pTexMap, const BuildResSet* pBuildResSet,
                                              const BuildArgSet& rBuildArgSet, const char* pName);
    detail::VectorGraphicsTexture* InitializeVectorGraphicsTextureReference(
        TexMap* pTexMap, const BuildResSet* pBuildResSet, const BuildArgSet& rBuildArgSet,
        const char* pName);
    bool TryToSetupUserShader(nn::gfx::Device* pDevice, const BuildArgSet& rBuildArgSet,
                              const BuildResSet* pBuildResSet);
    bool TryToSetupDetailedCombinerOrCombinerUserShader(nn::gfx::Device* pDevice,
                                                        const BuildResSet* pBuildResSet);
    bool TryToSetupSharedShader(size_t* pKeyCount, u32* pKeys, const BuildArgSet& rBuildArgSet);
    void SetupArchiveShader(nn::gfx::Device* pDevice, const BuildResSet* pBuildResSet,
                            size_t keyCount, const u32* pKeys);
    bool IsBrickRepeatTextureUsed() const;
    bool IsIndirectBlendUsed() const;
    bool IsPerspectiveTextureProjectionUsed() const;
    u32 GetProceduralShapeVariationBitFlags(const BuildArgSet& rBuildArgSet) const;
    int GetArchiveShaderVariation(int variation, int baseVariation) const;
    void CopyUserShaderConstantBufferInformation(const Material& rSrc);
    int GetBlendStateId() const;
    void CopyMaterialImpl(const Material& rSrc, MaterialCopyContext& rContext);
    void CopyDynamicRenderingTexture(const Material& rSrc, MaterialCopyContext& rContext);
    void FinalizeTexMap(nn::gfx::Device* pDevice);
    size_t CalculateReserveMemSize(int texMapNum, int texSrtNum, int texCoordGenNum,
                                   int tevStageNum, int alpCompNum, int blendModeNum,
                                   int indirectParameterNum, int projectionTexGenNum,
                                   int fontShadowParameterNum, int detailedCombinerNum,
                                   int combinerUserShaderNum, int vectorGraphicsTextureNum,
                                   int brickRepeatNum);
    void ResParameterToMemory(int texMapNum, int texSrtNum, int texCoordGenNum, int tevStageNum,
                              int alpCompNum, int blendModeNum, int indirectParameterNum,
                              int projectionTexGenNum, int fontShadowParameterNum,
                              int detailedCombinerNum, int combinerUserShaderNum,
                              int vectorGraphicsTextureNum, int brickRepeatNum);
    int GetProjectionTexGenParametersIdxFromTexCoordGenIdx(int texCoordGenIdx) const;
    int GetBrickRepeatTexGenParametersIdxFromTexCoordGenIdx(int texCoordGenIdx) const;
    static void CalculateTextureMtx(float (*pMtx)[3], const ResTexSrt& rTexSrt,
                                    const TexMap& rTexMap);
    static void CalculateIndirectMtx(float (*pMtx)[3], float rotate, const ResVec2& rScale);
    void SetupSubmaterialOf_TextureMatrix(DrawInfo& rDrawInfo,
                                          const nn::util::MatrixT4x3fType& rGlobalMtx,
                                          const Size* pSize);
    void SetRcpTexSize(const DrawInfo& rDrawInfo) const;
    void SetupSubmaterialOf_Tev(DrawInfo& rDrawInfo, const ResExtUserData* pExtUserData,
                                u16 extUserDataCount, const Size* pSize) const;
    void SetupSubmaterialOf_TextureCoordGenerateParams(
        DrawInfo& rDrawInfo, const nn::util::MatrixT4x3fType& rGlobalMtx, const Size* pSize) const;
    void* GetConstantBufferForBrickRepeatPixelShader(const DrawInfo& rDrawInfo) const;
    void SetupSubmaterialOf_TextureProjectionMatrix(
        DrawInfo& rDrawInfo, const nn::util::MatrixT4x3fType& rGlobalMtx, const Size* pSize,
        const ResProjectionTexGenParameters& rParameters, int texCoordGenIdx) const;
    void SetupSubmaterialOf_TextureBrickRepeatParams(
        DrawInfo& rDrawInfo, const ResBrickRepeatTexGenParameters& rParameters,
        int texCoordGenIdx) const;
    void SetupSubmaterialOf_FramebufferTexture(DrawInfo& rDrawInfo,
                                               nn::gfx::CommandBuffer& rCommands) const;
    void SetupSubmaterialOf_DetailedCombiner(DrawInfo& rDrawInfo) const;
    void SetupSubmaterialOf_CombinerUserShader(DrawInfo& rDrawInfo,
                                               const ResExtUserData* pExtUserData,
                                               u16 extUserDataCount, const Size* pSize) const;
    void* GetConstantBufferForDetailedCombinerPixelShader(const DrawInfo& rDrawInfo) const;
    void SetupConstantBufferColor_for_CombinerUserShader(
        ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer) const;
    void SetupConstantBufferPosture_for_CombinerUserShader(
        ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer,
        DrawInfo& rDrawInfo) const;
    void SetupConstantBufferExData_for_CombinerUserShader(
        ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer,
        const ResExtUserData* pExtUserData, u16 extUserDataCount) const;
    void SetupConstantBufferTextureData_for_CombinerUserShader(
        ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer) const;
    void* GetConstantBufferForCombinerUserShaderPixelShader(const DrawInfo& rDrawInfo) const;
    void* GetConstantBufferForUserVertexShader(const DrawInfo& rDrawInfo) const;
    void* GetConstantBufferForUserPixelShader(const DrawInfo& rDrawInfo) const;
    void* GetConstantBufferForUserGeometryShader(const DrawInfo& rDrawInfo) const;
    void EnableAlphaTest(nn::gfx::CommandBuffer& rCommands) const;
    void DisableAlphaTest() const;
    void ApplyPixelShaderConstantBufferDefault(nn::gfx::CommandBuffer& rCommands,
                                               DrawInfo& rDrawInfo) const;
    void ApplyPixelShaderDetailedCombinerConstantBuffer(nn::gfx::CommandBuffer& rCommands,
                                                        DrawInfo& rDrawInfo) const;
    void ApplyPixelShaderCombinerUserShaderConstantBuffer(nn::gfx::CommandBuffer& rCommands,
                                                          DrawInfo& rDrawInfo) const;

    union {
        unsigned char _08[8];
        struct {
            u32 mBlackColor;
            u32 mWhiteColor;
        };
        nn::util::Unorm8x4 m_ByteColors[MaterialColor_Max];
        nn::util::Float4* m_pFloatColors;
    };
    union {
        u32 mResourceCapacity;
        detail::MatMemCount m_MemCap;
    };
    union {
        u32 mResourceCounts;
        detail::MatMemCount m_MemNum;
    };
    union {
        TexMap* m_pTexMaps;
        void* m_pMem;
    };
    union {
        unsigned char _20[8];
        void* mShaderInfo;
        const ShaderInfo* m_pShaderInfo;
    };
    const char* mName;
    union {
        void* mUserShader;
        struct {
            u32 m_VertexShaderConstantBufferOffset;
            u32 m_PixelShaderConstantBufferOffset;
        };
    };
    UserShaderConstantBufferInformation* mUserShaderConstantBufferInformation;
    union {
        void* mDetailedCombiner;
        nn::gfx::BlendState* m_pBlendState;
    };
    union {
        u32 mFlags;
        struct {
            u8 mTextureCount;
            u8 mOwnershipFlags;
            u16 mShaderVariation;
        };
        struct {
            s8 m_DrawTextureNum : 4;
            u8 m_ShaderId : 4;
            u8 : 0;
            u8 m_IsUserAllocated : 1;
            u8 m_IsTextureOnly : 1;
            u8 m_IsThresholdingAlphaInterpolation : 1;
            u8 m_IsBlackColorFloat : 1;
            u8 m_IsWhiteColorFloat : 1;
            u8 m_IsFloatColorAllocated : 1;
            u8 m_IsDynamicTextureCreator : 1;
            u8 m_Reserved : 1;
        };
    };
};

/** @brief Vertex shader constants of a material. */
struct Material::ConstantBufferForVertexShader {
    float projection[4][4];
    float modelView[3][4];
    float texMtx0[2][4];
    float texMtx1[2][4];
    float texMtx2[2][4];
    float color[4];
    float brickRepeatMtx0[4][4];
    float brickRepeatMtx1[4][4];
    float brickRepeatMtx2[4][4];
    float reserve1a0[2][4];
    float rcpTexSize0[4];
    float vertexColor[4][4];
    float halfSize[2];
    float reserve218[2];
    int generatingTexCoord[TexMapMax];
    int frameSpec;
};

void SetDefaultShaderId(Material* pMaterial, int texCount);

namespace detail {
const ResMaterial* GetResMaterial(const BuildResSet* pResSet, u16 materialIdx);
void SetupMaterialRenderState(nn::gfx::CommandBuffer& rCommands, DrawInfo& rDrawInfo,
                              Material& rMaterial);
void DrawQuad(nn::gfx::CommandBuffer& rCommands, DrawInfo& rDrawInfo);
void CalculateQuad(DrawInfo& rDrawInfo, Material::ConstantBufferForVertexShader* pConstantBuffer,
                   const nn::util::Float2& rBasePos, const Size& rSize);
void CalculateQuadWithTexCoords(DrawInfo& rDrawInfo,
                                Material::ConstantBufferForVertexShader* pConstantBuffer,
                                const nn::util::Float2& rBasePos, const Size& rSize,
                                int texCoordCount, const nn::util::Float2 (*pTexCoords)[4]);
}  // namespace detail
}  // namespace ui2d
}  // namespace nn
