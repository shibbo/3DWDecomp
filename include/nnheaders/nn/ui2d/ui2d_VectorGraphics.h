#pragma once

#include <cstdio>
#include <new>
#include "nn/ui2d/ui2d_DrawInfo.h"
#include "nn/ui2d/ui2d_Resources.h"
#include "nn/ui2d/ui2d_Types.h"
#include "nn/ui2d/ui2d_CaptureTexture.h"
#include "nn/ui2d/ui2d_GraphicsResource.h"
#include "nn/ui2d/ui2d_RenderTargetTextureInfo.h"
#include "nn/util/util_BinaryFormat.h"
#include "nn/util/util_IntrusiveList.h"
#include "nn/util/util_Vector.h"
#include "nn/util/util_VectorApi.h"

namespace nn {
namespace ui2d {
struct BuildResultInformation;
class ResourceAccessor;
struct ResVectorGraphicsTexture;
struct BnvgFileHeader;

/** @brief One bezier-eased key of an animated scalar. */
struct ResBnvgAnimationKey {
    float frame;
    nn::util::Float2 easeOut;
    nn::util::Float2 easeIn;
    float startValue;
    float endValue;
    bool isHold;
    uint8_t padding[3];
};

/** @brief A scalar that is either constant or driven by a list of keys. */
struct ResBnvgAnimatableValue {
    uint16_t keyCount;
    uint8_t padding[6];
    union {
        float value;
        const ResBnvgAnimationKey* pKeys;
    };
};

/** @brief One key of an animated 2D value moving along a cubic bezier. */
struct ResBnvgBezierAnimationKey2D {
    float frame;
    float reserved[4];
    nn::util::Float2 startValue;
    nn::util::Float2 endValue;
    nn::util::Float2 startTangent;
    nn::util::Float2 endTangent;
    uint32_t padding;
};

/** @brief A 2D value that is either constant or driven by bezier keys. */
struct ResBnvgBezierAnimatableValue2D {
    uint16_t keyCount;
    uint8_t padding[6];
    union {
        nn::util::Float2 value;
        const ResBnvgBezierAnimationKey2D* pKeys;
    };
};

/** @brief Draw flags shared by every shape path. */
enum BnvgShapePathFlag {
    BnvgShapePathFlag_Fill = 1 << 0,
    BnvgShapePathFlag_Stroke = 1 << 1,
};

/** @brief Header shared by every shape path resource. */
struct ResBnvgShapePath {
    uint8_t type;
    uint8_t flags;
    uint16_t effectStartIndex;
    uint16_t drawFlags;
    uint8_t reserved[14];
};

/** @brief One animated control point of a free-form path. */
struct ResBnvgControlPoint {
    ResBnvgAnimatableValue inTangentX;
    ResBnvgAnimatableValue inTangentY;
    ResBnvgAnimatableValue outTangentX;
    ResBnvgAnimatableValue outTangentY;
    ResBnvgAnimatableValue positionX;
    ResBnvgAnimatableValue positionY;
};

/** @brief Free-form path made of cubic bezier segments. */
struct ResBnvgShapePathData : ResBnvgShapePath {
    uint32_t reserved2;
    uint32_t controlPointCount;
    uint8_t isClosed;
    uint8_t padding2[3];
    const ResBnvgControlPoint* pControlPoints;
};

/** @brief Ellipse path. */
struct ResBnvgShapePathEllipse : ResBnvgShapePath {
    ResBnvgAnimatableValue positionX;
    ResBnvgAnimatableValue positionY;
    ResBnvgAnimatableValue sizeX;
    ResBnvgAnimatableValue sizeY;
};

/** @brief Rectangle path with optional rounded corners. */
struct ResBnvgShapePathRect : ResBnvgShapePath {
    ResBnvgAnimatableValue positionX;
    ResBnvgAnimatableValue positionY;
    ResBnvgAnimatableValue sizeX;
    ResBnvgAnimatableValue sizeY;
    ResBnvgAnimatableValue cornerRadius;
};

/** @brief Star or polygon path. */
struct ResBnvgShapePathStar : ResBnvgShapePath {
    uint32_t isPolygon;
    uint32_t reserved2[2];
    ResBnvgAnimatableValue positionX;
    ResBnvgAnimatableValue positionY;
    ResBnvgAnimatableValue rotation;
    ResBnvgAnimatableValue pointCount;
    ResBnvgAnimatableValue outerRadius;
    ResBnvgAnimatableValue outerRoundness;
    ResBnvgAnimatableValue innerRadius;
    ResBnvgAnimatableValue innerRoundness;
};

/** @brief A mask made from one animated path. */
struct ResBnvgMaskInfo {
    uint32_t reserved;
    uint32_t isEnabled;
    uint32_t controlPointCount;
    uint8_t isClosed;
    uint8_t padding[3];
    const ResBnvgControlPoint* pControlPoints;
    ResBnvgAnimatableValue opacity;
    uint8_t reserved2[16];
};

/** @brief The masks applied to one layer. */
struct ResBnvgMaskInfoSet {
    uint16_t maskCount;
    uint8_t padding[6];
    const ResBnvgMaskInfo* pMaskInfos;
};

class ShaderInfo;

int SearchShaderVariationIndexFromTable(const void* pVariationTable, uint32_t signature,
                                        size_t keyCount, const uint32_t* pKeys);

/** @brief Animated position, anchor, scale and rotation of a layer, group or shape. */
struct ResBnvgTransform {
    ResBnvgBezierAnimatableValue2D position;
    ResBnvgBezierAnimatableValue2D anchor;
    ResBnvgAnimatableValue scaleX;
    ResBnvgAnimatableValue scaleY;
    ResBnvgAnimatableValue reserved;
    ResBnvgAnimatableValue rotation;
};

enum BnvgLayerType {
    BnvgLayerType_Composition,
    BnvgLayerType_Solid,
    BnvgLayerType_Image,
    BnvgLayerType_Null,
    BnvgLayerType_Shape,
    BnvgLayerType_Text,
    BnvgLayerType_Mask,
};

/** @brief Data shared by every layer resource. */
struct ResBnvgLayerBasicInfo {
    uint32_t type;
    uint32_t reserved;
    ResBnvgTransform transform;
    ResBnvgAnimatableValue opacity;
    int layerId;
    float inFrame;
    float outFrame;
    uint8_t reserved2[8];
    int parentLayerId;
    uint8_t reserved3;
    uint8_t flags;
    uint16_t maskInfoIndex;
    uint32_t reserved4;
    uint32_t compositionIndex;
    uint32_t reserved5;
};

struct ResBnvgMaskInfoSet;

/** @brief A layer holding the masks other layers refer to. */
struct ResBnvgMaskInfoLayer : ResBnvgLayerBasicInfo {
    const ResBnvgMaskInfoSet* pMaskInfoSets;
};

enum BnvgShapeEffectType {
    BnvgShapeEffectType_Fill,
    BnvgShapeEffectType_Stroke,
    BnvgShapeEffectType_Trim,
    BnvgShapeEffectType_Max
};

/** @brief Header shared by every shape effect. */
struct ResBnvgShapeEffect {
    uint8_t type;
    uint8_t flags;
    uint8_t padding[2];
};

/** @brief Fills the paths of a shape with a color. */
struct ResBnvgShapeFillEffect : ResBnvgShapeEffect {
    uint32_t reserved;
    ResBnvgAnimatableValue colorR;
    ResBnvgAnimatableValue colorG;
    ResBnvgAnimatableValue colorB;
    ResBnvgAnimatableValue colorA;
};

enum BnvgLineCapType { BnvgLineCapType_Butt, BnvgLineCapType_Round, BnvgLineCapType_Square };

enum BnvgLineJoinType { BnvgLineJoinType_Miter, BnvgLineJoinType_Round, BnvgLineJoinType_Bevel };

/** @brief Outlines the paths of a shape. */
struct ResBnvgShapeStrokeEffect : ResBnvgShapeEffect {
    uint32_t lineCap;
    uint32_t lineJoin;
    uint32_t reserved;
    ResBnvgAnimatableValue width;
    ResBnvgAnimatableValue miterLimit;
    ResBnvgAnimatableValue colorR;
    ResBnvgAnimatableValue colorG;
    ResBnvgAnimatableValue colorB;
    ResBnvgAnimatableValue colorA;
};

/** @brief Draws only part of the paths of a shape. */
struct ResBnvgShapeTrimEffect : ResBnvgShapeEffect {
    uint32_t reserved;
    ResBnvgAnimatableValue start;
    ResBnvgAnimatableValue end;
    ResBnvgAnimatableValue offset;
};

/** @brief One shape: a list of paths and the effects drawn on them. */
struct ResBnvgShapeInfo {
    uint16_t pathCount;
    uint16_t groupIndex;
    uint32_t reserved;
    const ResBnvgShapePath* const* ppPaths;
    uint16_t effectCount;
    uint8_t padding[6];
    const ResBnvgShapeEffect* const* ppEffects;
    uint8_t reserved2[16];
    ResBnvgTransform transform;
    uint8_t reserved3[16];
};

/** @brief A group of shapes sharing a transform and an opacity. */
struct ResBnvgShapeGroup {
    int parentIndex;
    uint32_t reserved;
    ResBnvgTransform transform;
    ResBnvgAnimatableValue opacity;
};

/** @brief A layer made of shapes. */
struct ResBnvgShapeLayer : ResBnvgLayerBasicInfo {
    const ResBnvgShapeGroup* pGroups;
    uint32_t shapeCount;
    uint16_t groupCount;
    uint16_t padding;
    const ResBnvgShapeInfo* pShapes;
};

/** @brief Block holding the layers of one composition. */
struct BnvgCompositionDataBlock : nn::util::BinaryBlockHeader {
    /** @brief Signature of a composition block. */
    static const int32_t Signature = 0x50434756;

    uint32_t layerCount;
    uint32_t padding;
    const ResBnvgLayerBasicInfo* const* ppLayers;
};

/** @brief Header of a bnvg file. */
struct BnvgFileHeader : nn::util::BinaryFileHeader {
    float width;
    float height;
};

/** @brief Reference from a layout to one texture rendered from a bnvg file. */
struct ResVectorGraphicsTexture {
    uint32_t reserved;
    int index;
    uint16_t reserved2;
    uint16_t multiSampleType;
};

namespace detail {

struct StrokeSegmentVertexInfo {
    nn::util::Float2 start;
    nn::util::Float2* pCurvePoints;
    uint32_t count;
    bool ignore;
};

/** @brief Control point of a free-form path after evaluating its animation. */
struct EvaluatedControlPoint {
    nn::util::Float2 inTangent;
    nn::util::Float2 outTangent;
    nn::util::Float2 position;
};

/** @brief Vertex and index buffers of one shape, carved from the constant buffer. */
struct ShapeMeshBufferInfo {
    nn::util::Float2* pFillVertex;
    ptrdiff_t fillVertexOffset;
    uint32_t* pFillIndex;
    ptrdiff_t fillIndexOffset;
    nn::util::Float2* pStrokeVertex;
    ptrdiff_t strokeVertexOffset;
    uint32_t* pStrokeIndex;
    ptrdiff_t strokeIndexOffset;
    StrokeSegmentVertexInfo* pStrokeSegment;
};

/** @brief Shader variation, selected by the number of masks applied. */
enum VectorGraphicsShaderVariation {
    VectorGraphicsShaderVariation_NoMask,
    VectorGraphicsShaderVariation_Mask1,
    VectorGraphicsShaderVariation_Mask2,
    VectorGraphicsShaderVariation_Mask3,
    VectorGraphicsShaderVariation_Max
};

typedef void (*ClearStencilCallback)(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer,
                                     void* pUserData);

/** @brief Drawing state shared by every layer of a vector graphics scene. */
class VectorGraphicsDrawInfo {
public:
    /** @brief Signature of the variation table keys used by vector graphics shaders. */
    static const uint32_t VariationTableSignature = 0x52475456;

    struct ShaderVariationInfo {
        int variationIndex;
        int vertexConstantSlot;
        int pixelConstantSlot;
        int textureSlots[3];
    };

    VectorGraphicsDrawInfo();
    ~VectorGraphicsDrawInfo();

    void Initialize(const ShaderInfo* pShaderInfo);
    void Finalize();
    void PushMaskTexture(nn::gfx::DescriptorSlot* pSlot);
    void PopMaskTexture();
    void ResetMaskTextureStack();

    nn::util::MatrixT4x4fType m_ProjectionMatrix;
    const ShaderInfo* m_pShaderInfo;
    ClearStencilCallback m_pClearStencilCallback;
    void* m_pClearStencilCallbackUserData;
    ShaderVariationInfo m_ShaderVariations[VectorGraphicsShaderVariation_Max];
    nn::gfx::DescriptorSlot* m_pMaskTextureStack[3];
    int m_MaskTextureStackCount;
};

void ClampValue(float& value, float minimum, float maximum);

void SetupShaderWithShaderCache(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer,
                                const ShaderInfo* pShaderInfo, int variation);

class ReservedVectorGraphicsSceneMemory {
public:
    ReservedVectorGraphicsSceneMemory();
    ~ReservedVectorGraphicsSceneMemory();

    void Initialize(size_t);
    void Finalize();
    size_t GetReservedSize() const;
    size_t GetAllocatedSize() const;
    void* Allocate(size_t);
    void* Allocate(size_t, size_t);

    /**
     * @brief Constructs an object in the arena.
     * @param args Constructor arguments.
     * @return Constructed object, or nullptr when the arena is exhausted.
     */
    template <typename T, typename... Args>
    T* New(Args... args) {
        void* pMemory = Allocate(sizeof(T));

        if (pMemory == nullptr) {
            return nullptr;
        }

        return new (pMemory) T(args...);
    }

    /**
     * @brief Constructs a value-initialized array in the arena.
     * @param count Number of elements.
     * @return Constructed array, or nullptr when the arena is exhausted.
     */
    template <typename T>
    T* NewArray(int count) {
        void* pMemory = Allocate(sizeof(T) * count);

        if (pMemory == nullptr) {
            return nullptr;
        }

        return new (pMemory) T[count]();
    }

    /**
     * @brief Constructs a value-initialized, aligned array in the arena.
     * @param count Number of elements.
     * @param alignment Alignment of the array.
     * @return Constructed array, or nullptr when the arena is exhausted.
     */
    template <typename T>
    T* NewArray(int count, size_t alignment) {
        void* pMemory = Allocate(sizeof(T) * count, alignment);

        if (pMemory == nullptr) {
            return nullptr;
        }

        return new (pMemory) T[count]();
    }

    char* mMemory;
    size_t mReservedSize;
    char* mNext;
};

class VectorGraphicsShapePathProcessor {
public:
    enum TrimDirection { TrimDirection_Right, TrimDirection_Left, TrimDirection_Max };

    /** @brief Starts with an untrimmed path of unknown length. */
    explicit VectorGraphicsShapePathProcessor(ReservedVectorGraphicsSceneMemory* pReservedMemory)
        : m_TrimStart(0.0f), m_TrimEnd(1.0f), m_PathLength(0.0f), m_ShapeAnimated(false),
          m_pReservedMemory(pReservedMemory) {}

    virtual ~VectorGraphicsShapePathProcessor() {}
    virtual void Initialize() = 0;
    virtual void Finalize() = 0;
    virtual BnvgShapePathType GetType() const = 0;
    virtual void EvaluateParams(DrawInfo& drawInfo, float time) = 0;
    virtual const nn::util::Float2 GetPosition() const { return nn::util::MakeFloat2(0.0f, 0.0f); }
    virtual uint32_t GetControlPointCount() const = 0;
    virtual uint32_t CalculatePathDivideVertexCount(int) const = 0;
    virtual uint32_t CalculateVertexCount() const = 0;
    virtual bool IsPathClosed() const = 0;
    virtual bool IsPathTrimed() const { return m_TrimStart > 0.0f || m_TrimEnd < 1.0f; }
    virtual void SetTrimParams(float, float);
    virtual int GenerateAndWritePathVertex(nn::util::Float2**, int*, uint32_t**, int*,
                                           StrokeSegmentVertexInfo*) = 0;
    virtual TrimDirection TrimPathDirection() const { return TrimDirection_Right; }

    void AdjustTrimmedVertexPosition(nn::util::Float2* pVertex, int startIndex, float startRate,
                                     int endIndex, float endRate) const;
    int TrimStrokeVertexInfo(nn::util::Float2* pVertex, int vertexCount, int startIndex,
                             StrokeSegmentVertexInfo* pStrokeInfo, int strokeInfoCount) const;
    int Trim(nn::util::Float2* pVertex, int* pVertexCount, StrokeSegmentVertexInfo* pStrokeInfo,
             int* pStrokeInfoCount);
    void GenerateFillIndex(uint32_t** ppIndex, int* pIndexCount, int baseIndex,
                           int triangleCount) const;

    float m_TrimStart;
    float m_TrimEnd;
    float m_PathLength;
    bool m_ShapeAnimated;
    ReservedVectorGraphicsSceneMemory* m_pReservedMemory;
};

class VectorGraphicsShapePathData : public VectorGraphicsShapePathProcessor {
public:
    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgShapePathData* pRes);

    VectorGraphicsShapePathData(const ResBnvgShapePathData* pRes,
                                ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~VectorGraphicsShapePathData() override;

    void Initialize() override;
    void Finalize() override;
    BnvgShapePathType GetType() const override {
        return static_cast<BnvgShapePathType>(m_pRes->type);
    }
    void EvaluateParams(DrawInfo& drawInfo, float time) override;
    uint32_t GetControlPointCount() const override;
    uint32_t CalculatePathDivideVertexCount(int index) const override;
    uint32_t CalculateVertexCount() const override;
    bool IsPathClosed() const override { return m_pRes->isClosed & 1; }
    int GenerateAndWritePathVertex(nn::util::Float2** ppVertex, int* pVertexCount,
                                   uint32_t** ppIndex, int* pIndexCount,
                                   StrokeSegmentVertexInfo* pStrokeInfo) override;
    TrimDirection TrimPathDirection() const override;

    /** @brief Sets the maximum allowed distance between a curve and its line segments. */
    void SetTolerance(float tolerance) { m_Tolerance = tolerance; }

    const ResBnvgShapePathData* m_pRes;
    EvaluatedControlPoint* m_pEvaluatedControlPoints;
    float m_Tolerance;
};

class VectorGraphicsShapePathEllipse : public VectorGraphicsShapePathProcessor {
public:
    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgShapePathEllipse* pRes);

    VectorGraphicsShapePathEllipse(const ResBnvgShapePathEllipse* pRes,
                                   ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~VectorGraphicsShapePathEllipse() override;

    void Initialize() override;
    void Finalize() override;
    BnvgShapePathType GetType() const override {
        return static_cast<BnvgShapePathType>(m_pRes->type);
    }
    void EvaluateParams(DrawInfo& drawInfo, float time) override;
    const nn::util::Float2 GetPosition() const override { return m_Position; }
    uint32_t GetControlPointCount() const override { return 1; }
    uint32_t CalculatePathDivideVertexCount(int index) const override;
    uint32_t CalculateVertexCount() const override { return CalculatePathDivideVertexCount(0) + 1; }
    bool IsPathClosed() const override { return true; }
    int GenerateAndWritePathVertex(nn::util::Float2** ppVertex, int* pVertexCount,
                                   uint32_t** ppIndex, int* pIndexCount,
                                   StrokeSegmentVertexInfo* pStrokeInfo) override;
    TrimDirection TrimPathDirection() const override;

    const ResBnvgShapePathEllipse* m_pRes;
    nn::util::Float2 m_Position;
    nn::util::Float2 m_Size;
};

class VectorGraphicsShapePathRect : public VectorGraphicsShapePathProcessor {
public:
    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgShapePathRect* pRes);

    VectorGraphicsShapePathRect(const ResBnvgShapePathRect* pRes,
                                ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~VectorGraphicsShapePathRect() override;

    void Initialize() override;
    void Finalize() override;
    BnvgShapePathType GetType() const override {
        return static_cast<BnvgShapePathType>(m_pRes->type);
    }
    void EvaluateParams(DrawInfo& drawInfo, float time) override;
    const nn::util::Float2 GetPosition() const override { return m_Position; }
    uint32_t GetControlPointCount() const override;
    uint32_t CalculatePathDivideVertexCount(int index) const override;
    uint32_t CalculateVertexCount() const override;
    bool IsPathClosed() const override { return true; }
    int GenerateAndWritePathVertex(nn::util::Float2** ppVertex, int* pVertexCount,
                                   uint32_t** ppIndex, int* pIndexCount,
                                   StrokeSegmentVertexInfo* pStrokeInfo) override;
    TrimDirection TrimPathDirection() const override;

    bool IsCurveDivideEnabled() const;
    float CalculateRadius() const;
    const nn::util::Float2 CalculateOffset(int corner) const;
    void GenerateRoundRectVertex(nn::util::Float2** ppVertex, int* pVertexCount) const;

    const ResBnvgShapePathRect* m_pRes;
    nn::util::Float2 m_Position;
    nn::util::Float2 m_Size;
    float m_CornerRadius;
};

class VectorGraphicsShapePathStar : public VectorGraphicsShapePathProcessor {
public:
    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgShapePathStar* pRes);

    VectorGraphicsShapePathStar(const ResBnvgShapePathStar* pRes,
                                ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~VectorGraphicsShapePathStar() override;

    void Initialize() override;
    void Finalize() override;
    BnvgShapePathType GetType() const override {
        return static_cast<BnvgShapePathType>(m_pRes->type);
    }
    void EvaluateParams(DrawInfo& drawInfo, float time) override;
    const nn::util::Float2 GetPosition() const override { return m_Position; }
    uint32_t GetControlPointCount() const override;
    uint32_t CalculatePathDivideVertexCount(int index) const override;
    uint32_t CalculateVertexCount() const override;
    bool IsPathClosed() const override { return true; }
    int GenerateAndWritePathVertex(nn::util::Float2** ppVertex, int* pVertexCount,
                                   uint32_t** ppIndex, int* pIndexCount,
                                   StrokeSegmentVertexInfo* pStrokeInfo) override;
    TrimDirection TrimPathDirection() const override;

    /** @brief Sets the maximum allowed distance between a curve and its line segments. */
    void SetTolerance(float tolerance) { m_Tolerance = tolerance; }

    const ResBnvgShapePathStar* m_pRes;
    EvaluatedControlPoint* m_pEvaluatedControlPoints;
    int m_ControlPointCount;
    float m_Tolerance;
    nn::util::Float2 m_Position;
};

/** @brief One rendering entry of a shape: built by Calculate and consumed by Draw. */
struct VectorGraphicsPathDrawInfo {
    BnvgShapeEffectType type;
    int indexStart;
    int indexCount;
    size_t pixelShaderOffset;
};

/** @brief Draws the masks of a layer into a coverage texture. */
class MaskDrawer {
public:
    /** @brief Per-mask drawing state. */
    struct MaskData {
        size_t vertexShaderOffset;
        VectorGraphicsShapePathProcessor* pPath;
        VectorGraphicsPathDrawInfo drawInfo;
    };

    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgMaskInfoSet* pMaskInfoSet);
    static int CalculateValidMaskCount(const ResBnvgMaskInfoSet* pMaskInfoSet);

    explicit MaskDrawer(ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~MaskDrawer();

    size_t GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const;
    void Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, int width, int height,
                    const ResBnvgMaskInfoSet* pMaskInfoSet);
    void Finalize(nn::gfx::Device* pDevice);
    void Calculate(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                   const nn::util::MatrixT4x3fType& rMatrix, float time);
    void DrawMaskPolygonFill(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                             nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
                             const VectorGraphicsPathDrawInfo& rPathDrawInfo,
                             PresetBlendStateId blendStateId) const;
    void DrawMaskPolygonStencil(DrawInfo& rDrawInfo,
                                VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
                                const VectorGraphicsPathDrawInfo& rPathDrawInfo) const;
    void DrawMaskShape(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                       nn::gfx::CommandBuffer& rCommandBuffer,
                       PresetBlendStateId blendStateId) const;
    void Draw(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
              nn::gfx::CommandBuffer& rCommandBuffer) const;

    /** @brief Returns the number of masks that are drawn. */
    int GetValidMaskCount() const { return m_ValidMaskCount; }

    RenderTargetTextureInfo* m_pRenderTarget;
    const ResBnvgMaskInfo* m_pMaskInfos;
    int m_ValidMaskCount;
    MaskData* m_pMasks;
    ResBnvgShapePathData* m_pPathResources;
    int m_VertexCount;
    int m_IndexCount;
    ptrdiff_t m_VertexBufferOffset;
    ptrdiff_t m_IndexBufferOffset;
    ReservedVectorGraphicsSceneMemory* m_pReservedMemory;
};

/** @brief A node of the layer tree of a vector graphics scene. */
class VectorGraphicsLayer {
public:

    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgLayerBasicInfo* pBasicInfo,
                                                     const ResBnvgMaskInfoLayer* pMaskInfoLayer);

    VectorGraphicsLayer(const ResBnvgLayerBasicInfo* pBasicInfo,
                        const ResBnvgMaskInfoLayer* pMaskInfoLayer,
                        ReservedVectorGraphicsSceneMemory* pReservedMemory);
    virtual ~VectorGraphicsLayer();

    virtual void Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, int width,
                            int height);
    virtual void Finalize(nn::gfx::Device* pDevice);
    virtual void Calculate(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                           float time);
    virtual void Draw(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                      nn::gfx::CommandBuffer& rCommandBuffer);
    virtual size_t GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const;
    virtual void DrawLayerImpl(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                               nn::gfx::CommandBuffer& rCommandBuffer) {}

    void CalcTransform(nn::util::MatrixT4x3fType* pMatrix, const ResBnvgTransform& rTransform,
                       float time) const;

    /**
     * @brief Returns the layer a child list node belongs to.
     * @param pNode Node of the layer's m_Link member.
     */
    static VectorGraphicsLayer* FromLink(nn::util::IntrusiveListNode* pNode) {
        return reinterpret_cast<VectorGraphicsLayer*>(reinterpret_cast<char*>(pNode) -
                                                      sizeof(void*));
    }

    /**
     * @brief Appends a layer to the children of this layer.
     * @param pChild Layer to link.
     */
    void AppendChild(VectorGraphicsLayer* pChild) {
        m_Children.LinkPrev(&pChild->m_Link);
        pChild->m_pParent = this;
    }

    nn::util::IntrusiveListNode m_Link;
    float m_Opacity;
    bool m_IsVisible;
    nn::util::MatrixT4x3fType* m_pMatrix;
    const ResBnvgLayerBasicInfo* m_pBasicInfo;
    const ResBnvgMaskInfoLayer* m_pMaskInfoLayer;
    VectorGraphicsLayer* m_pParent;
    MaskDrawer m_MaskDrawer;
    ReservedVectorGraphicsSceneMemory* m_pReservedMemory;
    nn::util::IntrusiveListNode m_Children;
};

/** @brief Stroke parameters of one effect, evaluated every frame. */
struct StrokeInfo {
    float halfWidth;
    int roundDivideCount;
    const ResBnvgShapeEffect* pStrokeEffect;
};

/** @brief Geometry of one stroke join. */
struct StrokeJoinInfo {
    nn::util::Vector2f center;
    nn::util::Vector2f prevOuter;
    nn::util::Vector2f prevInner;
    nn::util::Vector2f prevTangent;
    nn::util::Vector2f nextOuter;
    nn::util::Vector2f nextInner;
    nn::util::Vector2f nextTangent;
};

/** @brief A layer made of shapes, each a list of paths drawn with fill and stroke effects. */
class VectorGraphicsShapeLayer : public VectorGraphicsLayer {
public:
    /** @brief Per-shape drawing state. */
    struct ShapeDrawInfo {
        size_t vertexShaderOffset;
        size_t* pPixelShaderOffsets;
        StrokeInfo* pStrokeInfos;
        VectorGraphicsShapePathProcessor** ppPathProcessors;
        int pathDrawInfoCountMax;
        int pathDrawInfoCount;
        VectorGraphicsPathDrawInfo* pPathDrawInfos;
    };

    /** @brief Evaluated transform and opacity of a shape group. */
    struct GroupInfo {
        nn::util::MatrixT4x3fType matrix;
        float opacity;
    };

    static bool IsDrawableEffect(BnvgShapeEffectType type);
    static int CalculateDrawablePathCount(const ResBnvgShapeInfo& rShapeInfo);
    static size_t CalculateRequiredDynamicMemorySize(const ResBnvgShapeLayer* pShapeLayer,
                                                     const ResBnvgMaskInfoLayer* pMaskInfoLayer);

    VectorGraphicsShapeLayer(const ResBnvgShapeLayer* pShapeLayer,
                             const ResBnvgMaskInfoLayer* pMaskInfoLayer,
                             ReservedVectorGraphicsSceneMemory* pReservedMemory);
    ~VectorGraphicsShapeLayer() override;

    void Initialize(nn::gfx::Device* pDevice, const Layout* pLayout, int width,
                    int height) override;
    void Finalize(nn::gfx::Device* pDevice) override;
    void Calculate(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                   float time) override;
    size_t GetRequiredConstantBufferSize(nn::gfx::Device* pDevice) const override;
    void DrawLayerImpl(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                       nn::gfx::CommandBuffer& rCommandBuffer) override;

    void InitializeShapeDrawInfo();
    VectorGraphicsShapePathProcessor* CreateAndInitializePathProcessor(const ResBnvgShapePath* pPath);
    void SetupConstantBuffer(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                             int shapeIndex, const ResBnvgShapeInfo& rShapeInfo, float time);
    bool IsShapeNeedRendering(const ResBnvgShapeInfo& rShapeInfo) const;
    bool IsPathNeedRendering(const ResBnvgShapePath& rPath) const;
    void CalculateShapeStrokeEffectSegmentVertexInfoCount(int* pVertexCount, int* pIndexCount,
                                                          int shapeIndex, int effectStart,
                                                          int pointCount) const;
    void CalculateShapeStrokeEffectLineCapVertexInfoCount(int* pVertexCount, int* pIndexCount,
                                                          int shapeIndex, int effectStart) const;
    void CalculateMaxGenerateVertexInfo(int* pFillVertexCount, int* pFillIndexCount,
                                        int* pStrokeVertexCount, int* pStrokeIndexCount) const;
    uint32_t FindMaxControlPointCount() const;
    void ResetVertexInfo();
    void SetPathDrawInfo(VectorGraphicsPathDrawInfo* pPathDrawInfo, BnvgShapeEffectType type,
                         int indexStart, int indexCount, size_t pixelShaderOffset) const;
    void MakeStrokeMiterJoinPolygon(nn::util::Float2** ppVertex, uint32_t** ppIndex,
                                    StrokeInfo& rStrokeInfo, float time,
                                    const StrokeJoinInfo& rJoinInfo, bool isClockwise,
                                    int baseIndex);
    void MakeStrokeCirclePolygon(nn::util::Float2** ppVertex, uint32_t** ppIndex,
                                 const nn::util::Vector2f& rCenter,
                                 const nn::util::Vector2f& rStart, float radius,
                                 float angle, int divideCount);
    void MakeStrokeRoundJoinPolygon(nn::util::Float2** ppVertex, uint32_t** ppIndex,
                                    StrokeInfo& rStrokeInfo, const nn::util::Vector2f& rCenter,
                                    const nn::util::Vector2f& rNormal, bool isClockwise);
    void MakeStrokeLineCapPolygon(nn::util::Float2** ppVertex, uint32_t** ppIndex,
                                  StrokeInfo& rStrokeInfo, const nn::util::Vector2f& rPoint,
                                  const nn::util::Vector2f& rNormal,
                                  const nn::util::Vector2f& rTangent, bool isStart);
    void CalculateStrokeNormalAndTangent(nn::util::Vector2f* pNormal, nn::util::Vector2f* pTangent,
                                         const VectorGraphicsShapePathProcessor* pPath,
                                         const nn::util::Vector2f& rStart,
                                         const nn::util::Vector2f& rEnd);
    void MakeStrokePolygon(const VectorGraphicsShapePathProcessor* pPath,
                           nn::util::Float2** ppVertex, uint32_t** ppIndex,
                           StrokeInfo& rStrokeInfo, float time,
                           const StrokeSegmentVertexInfo* pSegments, int segmentCount,
                           bool isClosed);
    void CalculateGroupInfo(float time);
    void CalculateTrimEffect(float* pTrimStart, float* pTrimEnd,
                             const ResBnvgShapeInfo& rShapeInfo, const ResBnvgShapePath& rPath,
                             float time) const;
    void EvaluateParams(DrawInfo& rDrawInfo, float time);
    void CalculateShapeDrawInfo(DrawInfo& rDrawInfo,
                                VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo, float time);
    void DrawFillPolygon(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                         nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
                         const VectorGraphicsPathDrawInfo& rPathDrawInfo);
    void DrawFillPolygonStencil(DrawInfo& rDrawInfo,
                                VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
                                const VectorGraphicsPathDrawInfo& rPathDrawInfo);
    void DrawStrokePolygon(DrawInfo& rDrawInfo, VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                           nn::gfx::CommandBuffer& rCommandBuffer, size_t vertexShaderOffset,
                           const VectorGraphicsPathDrawInfo& rPathDrawInfo);
    void DrawVectorGraphicsTexture(DrawInfo& rDrawInfo,
                                   VectorGraphicsDrawInfo& rVectorGraphicsDrawInfo,
                                   nn::gfx::CommandBuffer& rCommandBuffer);

    const ResBnvgShapeInfo* m_pShapes;
    ShapeDrawInfo* m_pShapeDrawInfos;
    const ResBnvgShapeGroup* m_pGroups;
    GroupInfo* m_pGroupInfos;
    int m_ShapeCount;
    int m_GroupCount;
    float m_Tolerance;
    int m_FillVertexCount;
    int m_FillIndexCount;
    ptrdiff_t m_FillVertexOffset;
    ptrdiff_t m_FillIndexOffset;
    int m_StrokeVertexCount;
    int m_StrokeIndexCount;
    ptrdiff_t m_StrokeVertexOffset;
    ptrdiff_t m_StrokeIndexOffset;
};

/** @brief A vector graphics animation: a tree of layers drawn into the current render target. */
class VectorGraphicsScene {
public:
    VectorGraphicsScene();
    VectorGraphicsScene(const VectorGraphicsScene& rSource, nn::gfx::Device* pDevice);
    VectorGraphicsScene(const VectorGraphicsScene& rSource, nn::gfx::Device* pDevice,
                        const Layout* pLayout);
    ~VectorGraphicsScene();

    size_t BuildComposition(nn::gfx::Device* pDevice, const Layout* pLayout,
                            VectorGraphicsLayer* pParent, const BnvgCompositionDataBlock* pBlock,
                            int width, int height);
    bool Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
               ResourceAccessor* pResourceAccessor, const BnvgFileHeader* pFileHeader);
    bool Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const Layout* pLayout,
               ResourceAccessor* pResourceAccessor, const BnvgFileHeader* pFileHeader);
    bool Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const Layout* pLayout,
               ResourceAccessor* pResourceAccessor, const BnvgFileHeader* pFileHeader, int width,
               int height);
    static size_t CaclulateRequiredMemorySize(const BnvgFileHeader* pFileHeader);
    static size_t CaclulateRequiredMemorySize(const BnvgFileHeader* pFileHeader,
                                              const BnvgCompositionDataBlock* pBlock);
    void Finalize();
    void Finalize(nn::gfx::Device* pDevice);
    void Calculate(DrawInfo& rDrawInfo, float time);
    void Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer);

    const BnvgFileHeader* m_pFileHeader;
    VectorGraphicsLayer* m_pRootLayer;
    int m_Width;
    int m_Height;
    VectorGraphicsDrawInfo m_DrawInfo;
    ReservedVectorGraphicsSceneMemory m_ReservedMemory;
};

/** @brief Texture rendered from a vector graphics (bnvg) resource. */
class VectorGraphicsTexture : public DynamicRenderingTexture {
public:
    static void MakeRefTextureName(char* pName, int nameLength, const char* pFileName,
                                   const ResVectorGraphicsTexture* pResource);

    explicit VectorGraphicsTexture(const char* pName);
    VectorGraphicsTexture(const VectorGraphicsTexture& rSource, nn::gfx::Device* pDevice,
                          const Layout* pLayout);
    ~VectorGraphicsTexture() override;

    void InitializeParams();
    void InitializeResources(nn::gfx::Device* pDevice, const Layout* pLayout);
    bool Initialize(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                    ResourceAccessor* pResourceAccessor, const Layout* pLayout,
                    const ResVectorGraphicsTexture* pResource, const char* pFileName,
                    const BnvgFileHeader* pFileHeader);
    NN_RUNTIME_TYPEINFO(DynamicRenderingTexture);
    int ConvertMultiSampleTypeToMultiSampleCount(int multiSampleType) const;
    void Finalize(nn::gfx::Device* pDevice) override;
    void FinalizeResources(nn::gfx::Device* pDevice);
    void Calculate(DrawInfo& rDrawInfo) override;
    void Draw(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
              nn::gfx::CommandBuffer& rCommandBuffer) override;
    void BeginRendering(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer);
    void EndRendering(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommandBuffer);
    void SetTime(float time);

    /**
     * @brief Set the colour the texture is cleared with.
     * @param rColor Clear colour.
     */
    void SetColor(const nn::util::Float4& rColor) { m_Color = rColor; }
    /** @brief Request the texture to be redrawn. */
    void SetUpdateRequested() { m_IsUpdateRequested = 1; }

    /** @brief Returns the render target the scene is drawn into. */
    RenderTargetTextureInfo* GetRenderTarget() const {
        return static_cast<RenderTargetTextureInfo*>(_08);
    }

    VectorGraphicsScene m_Scene;
    nn::util::Float4 m_Color;
    float m_Time;
    int m_MultiSampleCount;
    u8 m_IsTimeUpdated : 1;
    u8 m_IsUpdateRequested : 1;
    u8 m_Unk158Bits : 6;
    nn::gfx::Texture* m_pDepthStencilTexture;
    nn::gfx::TextureView* m_pDepthStencilTextureView;
    nn::gfx::DescriptorSlot* m_pDepthStencilTextureSlot;
    nn::gfx::DepthStencilView m_DepthStencilView;
    RenderTargetTextureInfo* m_pMultiSampleRenderTarget;
    nn::gfx::RasterizerState* m_pRasterizerState;
};

};  // namespace detail
};  // namespace ui2d
};  // namespace nn
