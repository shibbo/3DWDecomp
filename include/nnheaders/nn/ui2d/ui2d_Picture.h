#pragma once

#include <nn/ui2d/ui2d_Common.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_Resources.h>

namespace nn::gfx::util {
class PrimitiveShape;
}

namespace nn::ui2d {
class TextureInfo;
class BuildResultInformation;
struct BuildResSet;

/** @brief Kind of the primitive shape a picture can be drawn with. */
enum ShapeType {
    ShapeType_GfxPrimitiveRoundRect = 1,
    ShapeType_GfxPrimitiveCircle = 2,
};

/** @brief Shape resource referenced by a picture that is drawn as a primitive shape. */
struct ResShapeInfo {
    u16 shapeType;
    u16 reserve;
};

/** @brief Parameters of a circle shape. */
struct ResShapeInfoCircle : ResShapeInfo {
    u32 sliceCount;
};

/** @brief Parameters of a rectangle shape with rounded corners. */
struct ResShapeInfoRoundRect : ResShapeInfo {
    u32 radius;
    u32 sliceCount;
};

/** @brief Picture pane resource; the texture coordinates follow the fixed part. */
struct ResPicture : ResPane {
    /** @brief Bits of flags. */
    enum Flag {
        Flag_ShapeBinaryIndex = 1 << 0,
    };

    nn::util::Unorm8x4 vtxCols[4];
    u16 materialIdx;
    u8 texCoordCount;
    u8 flags;

    /** @return Whether the picture is drawn as a primitive shape. */
    bool IsShape() const { return (flags & Flag_ShapeBinaryIndex) != 0; }

    /** @return The texture coordinate sets, four corners each. */
    const nn::util::Float2 (*GetTexCoords() const)[4] {
        return reinterpret_cast<const nn::util::Float2(*)[4]>(this + 1);
    }

    /**
     * @param texCoordCount Number of texture coordinate sets stored before the index.
     * @return Index of the shape resource of the picture.
     */
    u32 GetShapeBinaryIndex(int texCoordCount) const {
        const u32 offset = sizeof(ResPicture) + sizeof(nn::util::Float2[4]) * texCoordCount;
        return *reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(this) + offset);
    }
};

namespace detail {
/** @brief Blend modes of the procedural shape effects. */
enum ProceduralShapeEffectBlendMode {
    ProceduralShapeEffectBlendMode_Normal,
    ProceduralShapeEffectBlendMode_Multiply,
    ProceduralShapeEffectBlendMode_Add,
    ProceduralShapeEffectBlendMode_Subtract,
};

const ResShapeInfo* GetResShapeInfo(const BuildResSet* pBuildResSet, u32 index);
}  // namespace detail

class Picture : public Pane {
public:
    /** @brief Vertex and index buffers of a picture drawn as a primitive shape. */
    struct ShapeDrawInfo {
        u16 shapeType;
        ptrdiff_t vertexBufferGpuMemoryOffset;
        ptrdiff_t indexBufferGpuMemoryOffset;
        nn::gfx::util::PrimitiveShape* pShape;
    };

    /** @brief Pixel shader constants of a procedural shape. */
    struct ProceduralShapeConstantBuffer {
        /** @brief Shape of one corner. */
        struct Corner {
            float exp;
            float radius;
            float innerSize[2];
        };

        float rcpHalfSize[2];
        float isEffectOnly;
        float reserve0c;
        Corner corners[4];
        int innerStrokeBlendType;
        int shadowBlendType;
        int colorOverlayBlendType;
        int gradationOverlayBlendType;
        float innerStrokeSize;
        float innerStrokeAntiAliasScale;
        float reserve68[2];
        nn::util::Float4 innerStrokeColor;
        float shadowSoftness;
        float shadowOffset[2];
        float shadowType;
        nn::util::Float4 shadowColor;
        float shadowChannelWeight[4];
        nn::util::Float4 colorOverlayColor;
        float gradationControlPoint[4];
        nn::util::Float4 gradationColor[4];
        float gradationDirection[2];
        float reserve118[2];
    };

    explicit Picture(int);
    explicit Picture(const TextureInfo&);
    Picture(BuildResultInformation*, nn::gfx::Device*, const ResPicture*, const ResPicture*,
            const BuildArgSet&);
    Picture(const Picture& rOther, nn::gfx::Device* pDevice)
        : Pane(rOther), m_pMaterial(nullptr), m_SharedMemory() {
        CopyImpl(rOther, pDevice, nullptr, nullptr);
    }
    Picture(const Picture& rOther, nn::gfx::Device* pDevice, Layout* pLayout);
    ~Picture() override;
    NN_RUNTIME_TYPEINFO(Pane);
    bool CompareCopiedInstanceTest(const Picture& rOther) const;
    void Finalize(nn::gfx::Device*) override;
    nn::util::Unorm8x4 GetVertexColor(int) const override;
    void SetVertexColor(int, const nn::util::Unorm8x4&) override;
    u8 GetVertexColorElement(int) const override;
    void SetVertexColorElement(int, u8) override;
    u32 GetMaterialCount() const override;
    Material* GetMaterial(int) const override;
    using Pane::GetMaterial;
    void Calculate(DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(DrawInfo&, nn::gfx::CommandBuffer&) override;
    void SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer&) const override;
    void LoadMtx(DrawInfo&) override;
    virtual void Append(const TextureInfo&);

    void Initialize(int texCoordCount);
    void InitializeMaterial(int texCoordCount);
    void InitializeShape(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                         const ResPicture* pBlock, const BuildArgSet& rBuildArgSet,
                         u32 shapeBinaryIndex);
    void InitializeProceduralShape(BuildResultInformation* pResult, nn::gfx::Device* pDevice);
    void CopyImpl(const Picture&, nn::gfx::Device*, const Layout*, detail::BuildPaneTreeContext*);
    void CopyImpl(const Picture& rOther, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                  const char* pNewRootName, const Layout* pLayout);
    void ReserveTexCoord(int count);
    void CopyShapeInfo(const Picture& rOther);
    void SetMaterial(Material* pMaterial);
    void SetTexCoordCount(int count);
    int GetTexCoordCount() const;
    void GetTexCoord(nn::util::Float2* pCoords, int index) const;
    void SetTexCoord(int index, const nn::util::Float2* pCoords);
    void CopyShapeVertexData(DrawInfo& rDrawInfo);
    void CalculateProceduralShape(DrawInfo& rDrawInfo);
    float CalculateProceduralShapeBasicParams(ProceduralShapeConstantBuffer* pConstantBuffer,
                                              const SystemDataProceduralShape* pShape);
    void CalculateProceduralShapeShadow(ProceduralShapeConstantBuffer* pConstantBuffer,
                                        float radius, bool isEnabled, bool isKnockout,
                                        float size, int blendType, float angle, float distance,
                                        const float* pColor, int shadowType, bool isDropShadow);
    void CalculateProceduralShapeGradationOverlay(ProceduralShapeConstantBuffer* pConstantBuffer,
                                                  const SystemDataProceduralShape* pShape);
    void CalculateProceduralShapeDropShadow(DrawInfo& rDrawInfo,
                                            const SystemDataProceduralShape* pShape,
                                            SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo);
    PresetBlendStateId ConvertProceduralShapeEffectBlendType(
        detail::ProceduralShapeEffectBlendMode blendMode) const;
    void DrawProceduralShapeDropShadow(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
                                       const SystemDataProceduralShape* pShape,
                                       SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo);
    void SetupProceduralShapeConstantBuffer(
        DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
        const SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo) const;
    bool CheckDrawOriginalPaneByProceduralShapeState(const SystemDataProceduralShape* pShape) const;

    /**
     * @param type Kind of system data.
     * @return Whether the extended user data of the pane holds a system data of that kind.
     */
    bool IsSystemExtDataEnabled(PaneSystemDataType type) const {
        return ((m_SystemExtDataFlag >> type) & 1) != 0;
    }

    /** @return Whether all four vertex colors are opaque white. */
    bool IsVertexColorWhite() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (m_VertexColors[i].v[j] != 0xff) {
                    return false;
                }
            }
        }

        return true;
    }

    /** @return Whether the shared memory holds texture coordinates rather than a shape. */
    bool IsTexCoordArrayEnabled() const {
        return m_SharedMemory.texCoordArray.GetCapacity() <= TexCoordCountMax;
    }

    /** @brief Texture coordinates and shape data share the same storage. */
    union SharedMemory {
        detail::TexCoordArray texCoordArray;
        struct {
            u8 kind;
            ShapeDrawInfo* pShapeDrawInfo;
        } shape;
    };

    /** @brief Largest number of texture coordinate sets; larger kinds mean a shape. */
    static const u8 TexCoordCountMax = 3;
    /** @brief Kind stored in the shared memory of a picture drawn as a shape. */
    static const u8 SharedMemoryKind_Shape = TexCoordCountMax + 1;

    Material* m_pMaterial;
    nn::util::Unorm8x4 m_VertexColors[4];
    SharedMemory m_SharedMemory;
};
}  // namespace nn::ui2d
