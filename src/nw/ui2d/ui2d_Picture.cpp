#include <nn/ui2d/ui2d_Picture.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include <nn/font/font_GpuBuffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/util/gfx_PrimitiveShape.h>
#include <nn/nn_SdkAssert.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_ShaderInfo.h>
#include <nn/ui2d/ui2d_TextureInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_MatrixApi.h>

namespace nn::ui2d {
static_assert(sizeof(Picture) == 0x100, "Picture size");
static_assert(sizeof(Picture::ShapeDrawInfo) == 0x20, "ShapeDrawInfo size");
static_assert(sizeof(Picture::ProceduralShapeConstantBuffer) == 0x120,
              "ProceduralShapeConstantBuffer size");
static_assert(sizeof(SystemDataProceduralShape) == 0xfc, "SystemDataProceduralShape size");
static_assert(sizeof(SystemDataProceduralShapeRuntimeInfo) == 0x14,
              "SystemDataProceduralShapeRuntimeInfo size");

namespace {
/** @brief Size of the pixel shader constants of a procedural shape. */
const size_t ProceduralShapeConstantBufferSize = sizeof(Picture::ProceduralShapeConstantBuffer);

/** @brief Size of the vertex shader constants of a material. */
const size_t VertexShaderConstantBufferSize = sizeof(Material::ConstantBufferForVertexShader);

/** @brief Shader variation of a material that draws a primitive shape. */
const ShaderVariation ShaderVariation_GfxPrimitive = static_cast<ShaderVariation>(2);

/** @brief Vertex format of the primitive shapes of pictures. */
const nn::gfx::util::PrimitiveShapeFormat ShapeVertexFormat =
    static_cast<nn::gfx::util::PrimitiveShapeFormat>(nn::gfx::util::PrimitiveShapeFormat_Pos |
                                                     nn::gfx::util::PrimitiveShapeFormat_Uv);

/** @brief Name of the procedural shape constants in the pixel shader. */
const char* const ProceduralShapeConstantBufferName = "uProceduralShape";

/**
 * @brief Reserves space at the back of the constant buffer.
 * @param pBuffer Constant buffer to allocate from.
 * @param size Byte count to reserve.
 * @return Offset of the reserved block from the start of the buffer.
 */
ptrdiff_t AllocateFromBufferBack(nn::font::GpuBuffer* pBuffer, size_t size) {
    uint64_t allocatedSize;

    if ((pBuffer->m_Flags & nn::font::GpuBuffer::Flag_AtomicAllocation) == 0) {
        allocatedSize = pBuffer->m_AllocatedSize2 += size;
    } else {
        allocatedSize =
            reinterpret_cast<std::atomic<uint64_t>*>(pBuffer->m_pAtomicAllocatedSize2)->fetch_add(
                size) +
            size;
    }

    return pBuffer->m_BufferSize - allocatedSize;
}

/**
 * @param color Colour channel stored as 0 to 255.
 * @return The channel normalized to [0, 1].
 */
float NormalizeColor(float color) {
    return color / 255.0f;
}

/**
 * @brief Estimates the sines and the cosines of four angles with polynomials.
 * @param pSin Receives the sines.
 * @param pCos Receives the cosines.
 * @param angles Angles in radians.
 */
inline void SinCosEst(float32x4_t* pSin, float32x4_t* pCos, float32x4_t angles) {
    using namespace nn::util::detail;

    float32x4_t turns = vmulq_n_f32(angles, Float1Divided2Pi);
    float32x4_t rounding = vbslq_f32(vcgezq_f32(turns), vdupq_n_f32(0.5f), vdupq_n_f32(-0.5f));
    turns = vcvtq_f32_s32(vcvtq_s32_f32(vaddq_f32(turns, rounding)));
    angles = vfmsq_n_f32(angles, turns, Float2Pi);
    uint32x4_t upper = vcgtq_f32(angles, vdupq_n_f32(FloatPiDivided2));
    angles = vbslq_f32(upper, vsubq_f32(vdupq_n_f32(FloatPi), angles), angles);
    uint32x4_t lower = vcltq_f32(angles, vdupq_n_f32(-FloatPiDivided2));
    angles = vbslq_f32(lower, vsubq_f32(vdupq_n_f32(-FloatPi), angles), angles);
    float32x4_t sign = vbslq_f32(vorrq_u32(upper, lower), vdupq_n_f32(-1.0f), vdupq_n_f32(1.0f));
    float32x4_t square = vmulq_f32(angles, angles);
    float32x4_t sine = vfmsq_n_f32(vdupq_n_f32(SinCoefficients[1]), square, SinCoefficients[0]);
    float32x4_t cosine = vfmsq_n_f32(vdupq_n_f32(CosCoefficients[1]), square, CosCoefficients[0]);
    sine = vfmaq_f32(vdupq_n_f32(-SinCoefficients[2]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(-CosCoefficients[2]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(SinCoefficients[3]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(CosCoefficients[3]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(-SinCoefficients[4]), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(-CosCoefficients[4]), square, cosine);
    sine = vfmaq_f32(vdupq_n_f32(1.0f), square, sine);
    cosine = vfmaq_f32(vdupq_n_f32(1.0f), square, cosine);
    *pSin = vmulq_f32(angles, sine);
    *pCos = vmulq_f32(sign, cosine);
}

/**
 * @brief Makes the rotation matrix of the angles around the x, y and z axes, in that order.
 * @param pOut Receives the matrix, without translation.
 * @param rotate Angles in radians.
 */
inline void MakeRotateXyzMatrix(nn::util::MatrixT4x3fType* pOut, float32x4_t rotate) {
    float32x4_t sin;
    float32x4_t cos;
    SinCosEst(&sin, &cos, rotate);

    float32x2_t sinZw = vget_high_f32(sin);
    float32x2_t cosZw = vget_high_f32(cos);
    float32x2_t cosSinZ = vzip1_f32(cosZw, sinZw);
    float32x2_t sinCosZ = vzip1_f32(sinZw, cosZw);
    const float32x2_t signX = {1.0f, 0.0f};
    const float32x2_t signY = {-1.0f, 0.0f};
    const float32x4_t signRow1 = {-1.0f, 1.0f, 0.0f, 0.0f};
    const float32x4_t signRow2 = {1.0f, -1.0f, 0.0f, 0.0f};

    float32x2_t sinY = vset_lane_f32(0.0f, vdup_laneq_f32(sin, 1), 1);
    float32x2_t cosY = vset_lane_f32(0.0f, vdup_laneq_f32(cos, 1), 1);
    float32x4_t row0 =
        vcombine_f32(vmul_f32(cosSinZ, vdup_laneq_f32(cos, 1)), vmul_f32(sinY, signY));
    float32x4_t rowY =
        vcombine_f32(vmul_f32(cosSinZ, vdup_laneq_f32(sin, 1)), vmul_f32(cosY, signX));
    float32x4_t rowZ = vcombine_f32(sinCosZ, vdup_n_f32(0.0f));

    float32x4x4_t rows;
    rows.val[0] = row0;
    rows.val[1] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(rowZ, cos, 0), signRow1),
                            vmulq_laneq_f32(rowY, sin, 0));
    rows.val[2] = vaddq_f32(vmulq_f32(vmulq_laneq_f32(rowZ, sin, 0), signRow2),
                            vmulq_laneq_f32(rowY, cos, 0));
    rows.val[3] = vdupq_n_f32(0.0f);

    const float32x4x4_t columns = nn::util::detail::Matrix4x4fTranspose(rows);
    pOut->_m.val[0] = columns.val[0];
    pOut->_m.val[1] = columns.val[1];
    pOut->_m.val[2] = columns.val[2];
}

/**
 * @brief Makes a matrix that moves points in the xy plane.
 * @param pOut Receives the matrix.
 * @param x Horizontal offset.
 * @param y Vertical offset.
 */
inline void MakeTranslateMatrix(nn::util::MatrixT4x3fType* pOut, float x, float y) {
    const float32x4_t axisX = {1.0f, 0.0f, 0.0f, 0.0f};
    const float32x4_t axisY = {0.0f, 1.0f, 0.0f, 0.0f};
    pOut->_m.val[0] = vsetq_lane_f32(x, axisX, 3);
    pOut->_m.val[1] = vsetq_lane_f32(y, axisY, 3);
    pOut->_m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
}

/**
 * @brief Multiplies a row of a 4x3 matrix by another 4x3 matrix.
 * @param row Row of the left matrix.
 * @param right0 First row of the right matrix.
 * @param right1 Second row of the right matrix.
 * @param right2 Third row of the right matrix.
 * @return Row of the product.
 */
inline float32x4_t MultiplyRow(float32x4_t row, float32x4_t right0, float32x4_t right1,
                               float32x4_t right2) {
    float32x4_t result = vmulq_laneq_f32(right0, row, 0);
    result = vfmaq_laneq_f32(result, right1, row, 1);
    result = vfmaq_laneq_f32(result, right2, row, 2);
    return vaddq_f32(vsetq_lane_f32(vgetq_lane_f32(row, 3), vdupq_n_f32(0.0f), 3), result);
}

/**
 * @brief Multiplies two 4x3 matrices.
 * @param pOut Receives rLeft * rRight.
 * @param rLeft Left matrix.
 * @param rRight Right matrix.
 */
inline void MatrixMultiply(nn::util::MatrixT4x3fType* pOut, const nn::util::MatrixT4x3fType& rLeft,
                           const nn::util::MatrixT4x3fType& rRight) {
    const float32x4_t left0 = rLeft._m.val[0];
    const float32x4_t left1 = rLeft._m.val[1];
    const float32x4_t left2 = rLeft._m.val[2];
    const float32x4_t right0 = rRight._m.val[0];
    const float32x4_t right1 = rRight._m.val[1];
    const float32x4_t right2 = rRight._m.val[2];
    pOut->_m.val[0] = MultiplyRow(left0, right0, right1, right2);
    pOut->_m.val[1] = MultiplyRow(left1, right0, right1, right2);
    pOut->_m.val[2] = MultiplyRow(left2, right0, right1, right2);
}
}  // namespace

/**
 * @brief Creates a picture with an empty material.
 * @param texCoordCount Number of texture coordinate sets to reserve.
 */
Picture::Picture(int texCoordCount) : m_SharedMemory() {
    Initialize(texCoordCount);
    InitializeMaterial(texCoordCount);
}

/**
 * @brief Resets the vertex colors and reserves the texture coordinates.
 * @param texCoordCount Number of texture coordinate sets to reserve.
 */
void Picture::Initialize(int texCoordCount) {
    m_pMaterial = nullptr;

    for (int i = 0; i < 4; ++i) {
        m_VertexColors[i] = {{0xff, 0xff, 0xff, 0xff}};
    }

    if (texCoordCount > 0) {
        m_SharedMemory.texCoordArray.Initialize();
        m_SharedMemory.texCoordArray.Reserve(texCoordCount);
    }
}

/**
 * @brief Creates the material of a picture that is not built from a resource.
 * @param texCoordCount Number of textures the material can hold.
 */
void Picture::InitializeMaterial(int texCoordCount) {
    m_pMaterial = Layout::NewObj<Material>();

    if (m_pMaterial != nullptr) {
        SetDefaultShaderId(m_pMaterial, texCoordCount);
        m_pMaterial->ReserveMem(texCoordCount, texCoordCount, texCoordCount, 0, false, 0, false, 0,
                                false, false, false, 0, 0);
    }
}

/**
 * @brief Creates a picture showing one texture.
 * @param rTextureInfo Texture to show.
 */
Picture::Picture(const TextureInfo& rTextureInfo) : m_SharedMemory() {
    const int texCoordCount = 1;
    Initialize(texCoordCount);
    InitializeMaterial(texCoordCount);

    if (m_pMaterial != nullptr) {
        Append(rTextureInfo);
    }
}

/**
 * @brief Builds a picture from its resource.
 * @param pResult Receives the constant buffer size the picture needs, or nullptr.
 * @param pDevice Graphics device.
 * @param pBaseBlock Picture resource.
 * @param pOverrideBlock Override resource of the enclosing parts pane, or nullptr.
 * @param rBuildArgSet Build arguments.
 */
Picture::Picture(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                 const ResPicture* pBaseBlock, const ResPicture* pOverrideBlock,
                 const BuildArgSet& rBuildArgSet)
    : Pane(pResult, pDevice, pBaseBlock, rBuildArgSet), m_SharedMemory() {
    const ResPicture* pBlock =
        pOverrideBlock != nullptr && rBuildArgSet.overrideUsageFlag == 0 ? pOverrideBlock
                                                                          : pBaseBlock;

    const int texCoordCount = std::min<int>(pBlock->texCoordCount, TexCoordCountMax);
    const bool isShape = pBlock->IsShape();
    Initialize(isShape ? 0 : texCoordCount);

    for (int i = 0; i < 4; ++i) {
        m_VertexColors[i] = pBlock->vtxCols[i];
    }

    if (texCoordCount != 0 && !isShape && m_SharedMemory.texCoordArray.GetCapacity() != 0) {
        m_SharedMemory.texCoordArray.Copy(pBlock->GetTexCoords(), texCoordCount);
    }

    if (isShape) {
        InitializeShape(pResult, pDevice, pBlock, rBuildArgSet,
                        pBlock->GetShapeBinaryIndex(texCoordCount));
    }

    const ResMaterial* pResMaterial =
        detail::GetResMaterial(rBuildArgSet.pCurrentBuildResSet, pBaseBlock->materialIdx);
    const ResMaterial* pOverrideResMaterial = nullptr;

    if (pOverrideBlock != nullptr) {
        pOverrideResMaterial =
            detail::GetResMaterial(rBuildArgSet.pOverrideBuildResSet, pOverrideBlock->materialIdx);
    }

    Material* pMaterial = static_cast<Material*>(Layout::AllocateMemory(sizeof(Material)));
    if (pMaterial != nullptr) {
        new (pMaterial)
            Material(pResult, pDevice, pResMaterial, pOverrideResMaterial, rBuildArgSet);
    }

    m_pMaterial = pMaterial;
    m_pMaterial->InitializeDynamicRenderingTexture(pResult, pDevice, rBuildArgSet);
    InitializeProceduralShape(pResult, pDevice);
}

/**
 * @brief Creates the primitive shape the picture is drawn with.
 * @param pResult Receives the buffer sizes the shape needs, or nullptr.
 * @param pDevice Graphics device.
 * @param pBlock Picture resource.
 * @param rBuildArgSet Build arguments.
 * @param shapeBinaryIndex Index of the shape resource.
 */
void Picture::InitializeShape(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                              const ResPicture* pBlock, const BuildArgSet& rBuildArgSet,
                              u32 shapeBinaryIndex) {
    const BuildResSet* pBuildResSet = rBuildArgSet.pCurrentBuildResSet;
    if (rBuildArgSet.pOverrideBuildResSet != nullptr &&
        rBuildArgSet.pOverrideBuildResSet->pShapeInfoList != nullptr) {
        pBuildResSet = rBuildArgSet.pOverrideBuildResSet;
    }

    const ResShapeInfo* pShapeInfo = detail::GetResShapeInfo(pBuildResSet, shapeBinaryIndex);

    switch (pShapeInfo->shapeType) {
    case ShapeType_GfxPrimitiveRoundRect: {
        const auto* pRoundRect = static_cast<const ResShapeInfoRoundRect*>(pShapeInfo);
        const float width = pBlock->size.x;
        const float height = pBlock->size.y;
        float cornerSizeX;
        float cornerSizeY;

        if (width > height) {
            const float aspect = std::abs(width / height);
            cornerSizeY = std::abs(static_cast<float>(pRoundRect->radius) / height);
            cornerSizeX = cornerSizeY / aspect;
        } else {
            const float aspect = std::abs(height / width);
            cornerSizeX = std::abs(static_cast<float>(pRoundRect->radius) / width);
            cornerSizeY = cornerSizeX / aspect;
        }

        m_SharedMemory.shape.pShapeDrawInfo = Layout::NewObj<ShapeDrawInfo>();
        m_SharedMemory.shape.pShapeDrawInfo->pShape = Layout::NewObj<RoundRectShape>(
            ShapeVertexFormat, nn::gfx::PrimitiveTopology_TriangleList, cornerSizeX, cornerSizeY,
            static_cast<uint32_t>(pRoundRect->sliceCount));
        break;
    }
    case ShapeType_GfxPrimitiveCircle: {
        const auto* pCircle = static_cast<const ResShapeInfoCircle*>(pShapeInfo);
        m_SharedMemory.shape.pShapeDrawInfo = Layout::NewObj<ShapeDrawInfo>();
        m_SharedMemory.shape.pShapeDrawInfo->pShape = Layout::NewObj<Ui2dCircleShape>(
            ShapeVertexFormat, nn::gfx::PrimitiveTopology_TriangleList,
            static_cast<int>(pCircle->sliceCount));
        break;
    }
    default:
        m_SharedMemory.shape.pShapeDrawInfo = nullptr;
        break;
    }

    if (m_SharedMemory.shape.pShapeDrawInfo == nullptr) {
        return;
    }

    m_SharedMemory.shape.kind = SharedMemoryKind_Shape;
    m_SharedMemory.shape.pShapeDrawInfo->shapeType = pShapeInfo->shapeType;

    const size_t vertexBufferSize =
        m_SharedMemory.shape.pShapeDrawInfo->pShape->GetVertexBufferSize();
    const size_t indexBufferSize = m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBufferSize();
    nn::gfx::util::PrimitiveShape* pShape = m_SharedMemory.shape.pShapeDrawInfo->pShape;
    void* pVertexBuffer = Layout::AllocateMemory(vertexBufferSize);
    void* pIndexBuffer = Layout::AllocateMemory(indexBufferSize);
    pShape->Calculate(pVertexBuffer, vertexBufferSize, pIndexBuffer, indexBufferSize);

    if (pResult != nullptr) {
        pResult->requiredUi2dConstantBufferSize +=
            GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_VertexBuffer, vertexBufferSize);
        pResult->requiredUi2dConstantBufferSize +=
            GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_IndexBuffer, indexBufferSize);
    }
}

/**
 * @brief Looks up the procedural shape constants of the pixel shader.
 * @param pResult Receives the constant buffer size the procedural shape needs, or nullptr.
 * @param pDevice Graphics device.
 */
void Picture::InitializeProceduralShape(BuildResultInformation* pResult,
                                        nn::gfx::Device* pDevice) {
    const auto* pShape = static_cast<const SystemDataProceduralShape*>(
        GetSystemExtDataByType(PaneSystemDataType_ProceduralShape));
    auto* pRuntimeInfo = static_cast<SystemDataProceduralShapeRuntimeInfo*>(
        GetSystemExtDataForModify(PaneSystemDataType_ProceduralShapeRuntimeInfo));
    if (pRuntimeInfo == nullptr) {
        return;
    }

    pRuntimeInfo->constantBufferSlot =
        GetMaterial()
            ->GetShaderInfo()
            ->GetPixelShader(GetMaterial()->GetShaderVariation())
            ->GetInterfaceSlot(nn::gfx::ShaderStage_Pixel,
                               nn::gfx::ShaderInterfaceType_ConstantBuffer,
                               ProceduralShapeConstantBufferName);

    if (pResult == nullptr) {
        return;
    }

    pResult->requiredUi2dConstantBufferSize += GetAlignedBufferSize(
        pDevice, nn::gfx::GpuAccess_ConstantBuffer, ProceduralShapeConstantBufferSize);

    if ((pShape->flags & SystemDataProceduralShape::Flag_DropShadow) != 0) {
        pResult->requiredUi2dConstantBufferSize += GetAlignedBufferSize(
            pDevice, nn::gfx::GpuAccess_ConstantBuffer, VertexShaderConstantBufferSize);
        pResult->requiredUi2dConstantBufferSize += GetAlignedBufferSize(
            pDevice, nn::gfx::GpuAccess_ConstantBuffer, ProceduralShapeConstantBufferSize);
    }
}

/**
 * @brief Creates a copy of a picture.
 * @param rOther Source picture.
 * @param pDevice Device that owns the copied material resources.
 * @param pLayout Layout that owns the copy.
 */
Picture::Picture(const Picture& rOther, nn::gfx::Device* pDevice, Layout* pLayout)
    : Pane(rOther, pDevice, pLayout), m_pMaterial(nullptr), m_SharedMemory() {
    void* pContextMemory =
        __builtin_alloca(detail::BuildPaneTreeContext::CalculateContextRequireMemorySize());
    detail::BuildPaneTreeContext context(
        pContextMemory, detail::BuildPaneTreeContext::CalculateContextRequireMemorySize());
    context.Initialize();
    context.PushCache(pLayout, nullptr);

    CopyImpl(rOther, pDevice, pLayout, &context);

    pLayout->AggregateDynamicTextureList(context.GetCurrentTextureShareInfo());
    context.PopCache();
    context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
    context.Finalize();
}

/**
 * @brief Copies the picture state and duplicates its material and shape.
 * @param rOther Source picture.
 * @param pDevice Device that owns the copied material resources.
 * @param pLayout Layout that owns the copy.
 * @param pContext Pane tree build state shared by the copy.
 */
void Picture::CopyImpl(const Picture& rOther, nn::gfx::Device* pDevice, const Layout* pLayout,
                       detail::BuildPaneTreeContext* pContext) {
    for (int i = 0; i < 4; ++i) {
        m_VertexColors[i] = rOther.m_VertexColors[i];
    }

    if (rOther.IsTexCoordArrayEnabled()) {
        const int texCoordCount = rOther.m_SharedMemory.texCoordArray.GetSize();
        if (texCoordCount != 0) {
            m_SharedMemory.texCoordArray.Initialize();
            m_SharedMemory.texCoordArray.Reserve(texCoordCount);
            m_SharedMemory.texCoordArray.SetSize(texCoordCount);

            for (int i = 0; i < texCoordCount; ++i) {
                m_SharedMemory.texCoordArray.SetCoord(
                    i, rOther.m_SharedMemory.texCoordArray.GetArray()[i]);
            }
        }
    } else {
        CopyShapeInfo(rOther);
    }

    MaterialCopyContext context = {pDevice, nullptr, pLayout, pContext};

    Material* pSrcMaterial = rOther.m_pMaterial;
    if (pSrcMaterial != nullptr && !pSrcMaterial->IsUserAllocated()) {
        Material* pMaterial = static_cast<Material*>(Layout::AllocateMemory(sizeof(Material)));
        if (pMaterial != nullptr) {
            new (pMaterial) Material(*pSrcMaterial, context);
        }

        m_pMaterial = pMaterial;
    } else {
        m_pMaterial = pSrcMaterial;
    }

    InitializeProceduralShape(nullptr, pDevice);
}

/**
 * @brief Copies the picture state and duplicates its material and shape.
 * @param rOther Source picture.
 * @param pDevice Device that owns the copied material resources.
 * @param pAccessor Unused resource accessor.
 * @param pNewRootName Unused name of the new root pane.
 * @param pLayout Layout that owns the copy.
 */
void Picture::CopyImpl(const Picture& rOther, nn::gfx::Device* pDevice,
                       ResourceAccessor* pAccessor, const char* pNewRootName,
                       const Layout* pLayout) {
    CopyImpl(rOther, pDevice, pLayout, nullptr);
}

/**
 * @brief Reserves texture coordinate sets.
 * @param count Number of texture coordinate sets.
 */
void Picture::ReserveTexCoord(int count) {
    m_SharedMemory.texCoordArray.Reserve(count);
}

/**
 * @brief Duplicates the primitive shape of another picture.
 * @param rOther Source picture.
 */
void Picture::CopyShapeInfo(const Picture& rOther) {
    const ShapeDrawInfo* pSrcShapeDrawInfo = rOther.m_SharedMemory.shape.pShapeDrawInfo;

    switch (pSrcShapeDrawInfo->shapeType) {
    case ShapeType_GfxPrimitiveRoundRect: {
        m_SharedMemory.shape.pShapeDrawInfo = Layout::NewObj<ShapeDrawInfo>();
        RoundRectShape* pShape = Layout::NewObj<RoundRectShape>(
            ShapeVertexFormat, nn::gfx::PrimitiveTopology_TriangleList, 0.0f, 0.0f, 1);
        pShape->CopyParams(*static_cast<const RoundRectShape*>(pSrcShapeDrawInfo->pShape));
        m_SharedMemory.shape.pShapeDrawInfo->pShape = pShape;
        break;
    }
    case ShapeType_GfxPrimitiveCircle: {
        m_SharedMemory.shape.pShapeDrawInfo = Layout::NewObj<ShapeDrawInfo>();
        const auto* pSrcShape = static_cast<const Ui2dCircleShape*>(pSrcShapeDrawInfo->pShape);
        Ui2dCircleShape* pShape = Layout::NewObj<Ui2dCircleShape>(
            ShapeVertexFormat, nn::gfx::PrimitiveTopology_TriangleList, pSrcShape->GetVertexCount() - 1);
        pShape->CopyParams(*pSrcShape);
        m_SharedMemory.shape.pShapeDrawInfo->pShape = pShape;
        break;
    }
    default:
        m_SharedMemory.shape.pShapeDrawInfo = nullptr;
        break;
    }

    if (m_SharedMemory.shape.pShapeDrawInfo != nullptr) {
        m_SharedMemory.shape.kind = SharedMemoryKind_Shape;
        m_SharedMemory.shape.pShapeDrawInfo->shapeType = pSrcShapeDrawInfo->shapeType;
    }
}

/** @brief Destroys the picture; resources are released by Finalize. */
Picture::~Picture() {
    NN_SDK_ASSERT(m_pMaterial == nullptr);
}

/**
 * @brief Releases the material, the texture coordinates and the shape of the picture.
 * @param pDevice Device that owns the material resources.
 */
void Picture::Finalize(nn::gfx::Device* pDevice) {
    Pane::Finalize(pDevice);

    if (m_pMaterial != nullptr && !m_pMaterial->IsUserAllocated()) {
        m_pMaterial->Finalize(pDevice);
        Layout::DeleteObj(m_pMaterial);
    }

    m_pMaterial = nullptr;

    if (IsTexCoordArrayEnabled()) {
        m_SharedMemory.texCoordArray.Free();
    } else {
        ShapeDrawInfo* pShapeDrawInfo = m_SharedMemory.shape.pShapeDrawInfo;
        if (pShapeDrawInfo != nullptr) {
            void* pVertexBuffer = pShapeDrawInfo->pShape->GetVertexBuffer();
            if (pVertexBuffer != nullptr) {
                Layout::FreeMemory(pVertexBuffer);
            }

            void* pIndexBuffer = m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBuffer();
            if (pIndexBuffer != nullptr) {
                Layout::FreeMemory(pIndexBuffer);
            }

            Layout::DeleteObj(m_SharedMemory.shape.pShapeDrawInfo->pShape);
            Layout::DeleteObj(m_SharedMemory.shape.pShapeDrawInfo);
            m_SharedMemory.shape.pShapeDrawInfo = nullptr;
        }
    }
}

/** @return The number of materials: one when the picture has a material. */
u32 Picture::GetMaterialCount() const {
    return m_pMaterial != nullptr;
}

/**
 * @param index Zero for the material of the picture.
 * @return The material, or nullptr when @p index is out of range.
 */
Material* Picture::GetMaterial(int index) const {
    GetMaterialCount();
    return index == 0 ? m_pMaterial : nullptr;
}

/**
 * @brief Replaces the material, deleting the old one unless its owner manages it.
 * @param pMaterial New material.
 */
void Picture::SetMaterial(Material* pMaterial) {
    if (m_pMaterial == pMaterial) {
        return;
    }

    if (m_pMaterial != nullptr && !m_pMaterial->IsUserAllocated()) {
        Layout::DeleteObj(m_pMaterial);
    }

    m_pMaterial = pMaterial;
}

/**
 * @brief Adds a texture to the material and sizes an empty picture to it.
 * @param rTextureInfo Texture to add.
 */
void Picture::Append(const TextureInfo& rTextureInfo) {
    Material* pMaterial = m_pMaterial;
    if (pMaterial->GetTexMapNum() >= pMaterial->GetTexMapCap() ||
        pMaterial->GetTexCoordGenNum() >= pMaterial->GetTexCoordGenCap()) {
        return;
    }

    const u8 texIdx = pMaterial->GetTexMapNum();
    new (&pMaterial->GetTexMapAry()[pMaterial->m_MemNum.texMap]) TexMap(&rTextureInfo);
    pMaterial->m_MemNum.texMap = pMaterial->m_MemNum.texMap + 1;
    m_pMaterial->m_MemNum.texCoordGen = m_pMaterial->m_MemNum.texMap;

    ResTexCoordGen* pTexCoordGens = reinterpret_cast<ResTexCoordGen*>(
        static_cast<u8*>(m_pMaterial->m_pMem) + m_pMaterial->GetTexCoordGenOffset());
    pTexCoordGens[texIdx] = ResTexCoordGen();

    SetTexCoordCount(m_pMaterial->GetTexMapNum());

    if (mSizeX == 0.0f && mSizeY == 0.0f && m_pMaterial->GetTexMapNum() == 1) {
        const TextureInfo* pTextureInfo = m_pMaterial->GetTexMapAry()[0].m_pTextureInfo;

        if (pTextureInfo != nullptr) {
            const TextureSize textureSize = pTextureInfo->GetSize();
            const Size size = {static_cast<float>(textureSize.width),
                               static_cast<float>(textureSize.height)};
            SetSize(size);
        } else {
            const Size size = {0.0f, 0.0f};
            SetSize(size);
        }
    }
}

/**
 * @brief Changes the number of texture coordinate sets in use.
 * @param count Number of texture coordinate sets.
 */
void Picture::SetTexCoordCount(int count) {
    m_SharedMemory.texCoordArray.SetSize(count);
}

/** @return The number of texture coordinate sets in use. */
int Picture::GetTexCoordCount() const {
    return m_SharedMemory.texCoordArray.GetSize();
}

/**
 * @param pCoords Receives the four corner coordinates.
 * @param index Texture coordinate set to read.
 */
void Picture::GetTexCoord(nn::util::Float2* pCoords, int index) const {
    m_SharedMemory.texCoordArray.GetCoord(pCoords, index);
}

/**
 * @param index Texture coordinate set to change.
 * @param pCoords New four corner coordinates.
 */
void Picture::SetTexCoord(int index, const nn::util::Float2* pCoords) {
    m_SharedMemory.texCoordArray.SetCoord(index, pCoords);
}

/**
 * @param index Corner to read.
 * @return The vertex color of that corner.
 */
nn::util::Unorm8x4 Picture::GetVertexColor(int index) const {
    return m_VertexColors[index];
}

/**
 * @param index Corner to change.
 * @param rColor New vertex color of that corner.
 */
void Picture::SetVertexColor(int index, const nn::util::Unorm8x4& rColor) {
    m_VertexColors[index] = rColor;
}

/**
 * @param index Channel index across the four corner colors.
 * @return The value of that channel.
 */
u8 Picture::GetVertexColorElement(int index) const {
    return reinterpret_cast<const u8*>(&m_VertexColors[index / 4])[index % 4];
}

/**
 * @param index Channel index across the four corner colors.
 * @param value New value of that channel.
 */
void Picture::SetVertexColorElement(int index, u8 value) {
    reinterpret_cast<u8*>(&m_VertexColors[index / 4])[index % 4] = value;
}

/**
 * @brief Loads the model view matrix, rotated around the collision box when the picture has one.
 * @param rDrawInfo Drawing state.
 */
void Picture::LoadMtx(DrawInfo& rDrawInfo) {
    if ((m_SystemExtDataFlag & (1 << PaneSystemDataType_SimpleOBBCollision)) == 0) {
        auto* pDst = reinterpret_cast<nn::util::Float4*>(&rDrawInfo.m_ModelViewMtx);
        const auto* pSrc = reinterpret_cast<const nn::util::Float4*>(mGlobalMtx);
        for (int i = 0; i < 3; ++i) {
            pDst[i] = pSrc[i];
        }
    } else {
        const float width = mSizeX;
        const float height = mSizeY;
        const auto* pCollision = static_cast<const SystemDataSimpleOBBCollision*>(
            GetSystemExtDataByType(PaneSystemDataType_SimpleOBBCollision));

        const float boxWidth = width * pCollision->size.x;
        const float boxHeight = height * pCollision->size.y;
        const float pivotX = width * 0.5f + (-(width * pCollision->offset.x) - boxWidth * 0.5f);
        const float pivotY = height * pCollision->offset.y + boxHeight * 0.5f - height * 0.5f;

        nn::util::MatrixT4x3fType rotateMtx;
        const float32x4_t rotate = {0.0f, 0.0f, pCollision->rotate, 0.0f};
        MakeRotateXyzMatrix(&rotateMtx, rotate);

        nn::util::MatrixT4x3fType toPivotMtx;
        MakeTranslateMatrix(&toPivotMtx, pivotX, pivotY);
        nn::util::MatrixT4x3fType fromPivotMtx;
        MakeTranslateMatrix(&fromPivotMtx, -pivotX, -pivotY);

        nn::util::MatrixT4x3fType mtx;
        MatrixMultiply(&mtx, rotateMtx, toPivotMtx);
        MatrixMultiply(&mtx, fromPivotMtx, mtx);
        MatrixMultiply(&rDrawInfo.m_ModelViewMtx, GetGlobalMatrix(), mtx);
    }

    rDrawInfo.mModelViewLoaded = false;
}

/**
 * @brief Updates the matrices and the constant buffers of the picture.
 * @param rDrawInfo Drawing state.
 * @param rContext Calculation state of the pane tree.
 * @param isDirtyParentMtx Whether the parent matrix changed.
 */
void Picture::Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext, bool isDirtyParentMtx) {
    Pane::Calculate(rDrawInfo, rContext, isDirtyParentMtx);

    if (m_pMaterial == nullptr) {
        mFlags &= ~PaneFlag_IsCalculationFinished;
        return;
    }

    LoadMtx(rDrawInfo);

    if (CheckInvisibleAndUpdateConstantBufferReady()) {
        return;
    }

    m_pMaterial->AllocateConstantBuffer(rDrawInfo);
    m_pMaterial->SetupBlendState(&rDrawInfo);

    const u32 systemExtDataFlag = m_SystemExtDataFlag;
    const bool isProceduralShapeEnabled =
        (systemExtDataFlag & (1 << PaneSystemDataType_ProceduralShapeRuntimeInfo)) != 0;

    if (isProceduralShapeEnabled) {
        const auto* pShape = static_cast<const SystemDataProceduralShape*>(
            GetSystemExtDataByType(PaneSystemDataType_ProceduralShape));
        auto* pRuntimeInfo = static_cast<SystemDataProceduralShapeRuntimeInfo*>(
            GetSystemExtDataForModify(PaneSystemDataType_ProceduralShapeRuntimeInfo));

        if (pRuntimeInfo != nullptr) {
            const size_t alignment = rDrawInfo.GetGraphicsResource()->m_ConstantBufferAlignment;
            const size_t size = nn::util::align_up(ProceduralShapeConstantBufferSize, alignment);
            pRuntimeInfo->constantBufferOffset =
                rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(size);

            if ((pShape->flags & SystemDataProceduralShape::Flag_DropShadow) != 0) {
                pRuntimeInfo->dropShadowVertexConstantBufferOffset =
                    rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(
                        nn::util::align_up(VertexShaderConstantBufferSize, alignment));
                pRuntimeInfo->dropShadowConstantBufferOffset =
                    rDrawInfo.GetUi2dConstantBuffer()->AllocateWithoutAlignment(size);
            }
        }
    }

    if (m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo) == nullptr ||
        m_pMaterial->GetConstantBufferForPixelShader(rDrawInfo) == nullptr) {
        return;
    }

    if (IsTexCoordArrayEnabled()) {
        if (!m_pMaterial->IsCombinerUserShaderCapable() && IsVertexColorWhite() &&
            GetGlobalAlpha() == 0xff) {
            m_pMaterial->SetupGraphics(rDrawInfo, 0xff, ShaderVariation_WithoutVertexColor, true,
                                       GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                       GetExtUserDataCount());
        } else {
            m_pMaterial->SetupGraphics(rDrawInfo, GetGlobalAlpha(), ShaderVariation_Standard, true,
                                       GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                       GetExtUserDataCount());

            auto* pConstantBuffer = static_cast<Material::ConstantBufferForVertexShader*>(
                m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo));
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    pConstantBuffer->vertexColor[i][j] = m_VertexColors[i].v[j];
                }
            }
        }

        if ((m_SystemExtDataFlag & ((1 << PaneSystemDataType_SimpleAABBCollision) |
                                    (1 << PaneSystemDataType_SimpleOBBCollision))) != 0) {
            const auto* pCollision = static_cast<const SystemDataSimpleAABBCollision*>(
                GetSystemExtDataByType(PaneSystemDataType_SimpleAABBCollision));
            if (pCollision == nullptr) {
                pCollision = static_cast<const SystemDataSimpleAABBCollision*>(
                    GetSystemExtDataByType(PaneSystemDataType_SimpleOBBCollision));
            }

            nn::util::Float2 basePos = GetVertexPos();
            Size size = GetSize();
            basePos.x += pCollision->offset.x * size.width;
            basePos.y -= pCollision->offset.y * size.height;
            size.width = pCollision->size.x * size.width;
            size.height = pCollision->size.y * size.height;

            detail::CalculateQuadWithTexCoords(
                rDrawInfo,
                static_cast<Material::ConstantBufferForVertexShader*>(
                    m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo)),
                basePos, size, m_SharedMemory.texCoordArray.GetSize(),
                m_SharedMemory.texCoordArray.GetArray());
        } else {
            auto* pConstantBuffer = static_cast<Material::ConstantBufferForVertexShader*>(
                m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo));
            const nn::util::Float2 basePos = GetVertexPos();
            detail::CalculateQuadWithTexCoords(rDrawInfo, pConstantBuffer, basePos, GetSize(),
                                               m_SharedMemory.texCoordArray.GetSize(),
                                               m_SharedMemory.texCoordArray.GetArray());
        }
    } else {
        m_pMaterial->SetupGraphics(rDrawInfo, GetGlobalAlpha(), ShaderVariation_GfxPrimitive, true,
                                   GetGlobalMatrix(), &GetSize(), GetExtUserDataArray(),
                                   GetExtUserDataCount());
        CopyShapeVertexData(rDrawInfo);

        auto* pConstantBuffer = static_cast<Material::ConstantBufferForVertexShader*>(
            m_pMaterial->GetConstantBufferForVertexShader(rDrawInfo));
        pConstantBuffer->halfSize[0] = mSizeX * 0.5f;
        pConstantBuffer->halfSize[1] = mSizeY * 0.5f;
        pConstantBuffer->reserve218[0] = 0.0f;
        pConstantBuffer->reserve218[1] = 0.0f;
        rDrawInfo.LoadMtxModelView(pConstantBuffer->modelView);
    }

    if (IsPaneEffectEnabled()) {
        UpdateMaterialConstantBufferForEffectCapture(rDrawInfo);
    }

    if (isProceduralShapeEnabled) {
        CalculateProceduralShape(rDrawInfo);
    }

    DrawInfo::PostCalculateCallback pCallback = rDrawInfo.GetPostCalculateCallback();
    if (pCallback != nullptr) {
        pCallback(rDrawInfo, this, rDrawInfo.GetPostCalculateCallbackUserData());
    }
}

/**
 * @brief Copies the vertices and the indices of the shape into the constant buffer.
 * @param rDrawInfo Drawing state.
 */
void Picture::CopyShapeVertexData(DrawInfo& rDrawInfo) {
    const GraphicsResource* pGraphicsResource = rDrawInfo.GetGraphicsResource();
    const size_t vertexBufferAlignment = pGraphicsResource->m_VertexBufferAlignment;
    const size_t indexBufferAlignment = pGraphicsResource->m_IndexBufferAlignment;

    ShapeDrawInfo* pShapeDrawInfo = m_SharedMemory.shape.pShapeDrawInfo;
    nn::font::GpuBuffer* pBuffer = rDrawInfo.GetUi2dConstantBuffer();
    ptrdiff_t offset = AllocateFromBufferBack(
        pBuffer, nn::util::align_up(pShapeDrawInfo->pShape->GetVertexBufferSize(),
                                    vertexBufferAlignment));
    m_SharedMemory.shape.pShapeDrawInfo->vertexBufferGpuMemoryOffset = offset;

    void* pMappedPointer = rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer();
    if (pMappedPointer != nullptr) {
        void* pDst = static_cast<u8*>(pMappedPointer) + offset;
        std::memcpy(pDst, m_SharedMemory.shape.pShapeDrawInfo->pShape->GetVertexBuffer(),
                    m_SharedMemory.shape.pShapeDrawInfo->pShape->GetVertexBufferSize());
    }

    pBuffer = rDrawInfo.GetUi2dConstantBuffer();
    offset = AllocateFromBufferBack(
        pBuffer, nn::util::align_up(m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBufferSize(),
                                    indexBufferAlignment));
    m_SharedMemory.shape.pShapeDrawInfo->indexBufferGpuMemoryOffset = offset;

    pMappedPointer = rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer();
    if (pMappedPointer != nullptr) {
        void* pDst = static_cast<u8*>(pMappedPointer) + offset;
        std::memcpy(pDst, m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBuffer(),
                    m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBufferSize());
    }
}

/**
 * @brief Fills the procedural shape constants of the picture.
 * @param rDrawInfo Drawing state.
 */
void Picture::CalculateProceduralShape(DrawInfo& rDrawInfo) {
    const auto* pShape = static_cast<const SystemDataProceduralShape*>(
        GetSystemExtDataByType(PaneSystemDataType_ProceduralShape));
    auto* pRuntimeInfo = static_cast<SystemDataProceduralShapeRuntimeInfo*>(
        GetSystemExtDataForModify(PaneSystemDataType_ProceduralShapeRuntimeInfo));
    if (pRuntimeInfo == nullptr) {
        return;
    }

    void* pMappedPointer = rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer();
    if (pMappedPointer != nullptr) {
        auto* pConstantBuffer = reinterpret_cast<ProceduralShapeConstantBuffer*>(
            static_cast<u8*>(pMappedPointer) + pRuntimeInfo->constantBufferOffset);
        const float radius = CalculateProceduralShapeBasicParams(pConstantBuffer, pShape);

        if ((pShape->flags & SystemDataProceduralShape::Flag_InnerStroke) != 0) {
            pConstantBuffer->innerStrokeSize = pShape->innerStrokeSize;

            const nn::util::MatrixT4x3fType& rGlobalMtx = GetGlobalMatrix();
            if (vgetq_lane_f32(rGlobalMtx._m.val[1], 2) == 0.0f &&
                vgetq_lane_f32(rGlobalMtx._m.val[0], 2) == 0.0f &&
                vgetq_lane_f32(rGlobalMtx._m.val[2], 1) == 0.0f &&
                vgetq_lane_f32(rGlobalMtx._m.val[0], 1) == 0.0f &&
                vgetq_lane_f32(rGlobalMtx._m.val[1], 0) == 0.0f &&
                vgetq_lane_f32(rGlobalMtx._m.val[2], 0) == 0.0f) {
                pConstantBuffer->innerStrokeAntiAliasScale = 1000000.0f;
            } else {
                pConstantBuffer->innerStrokeAntiAliasScale =
                    1.0f / (0.7f / pShape->innerStrokeSize);
            }

            pConstantBuffer->innerStrokeBlendType = pShape->innerStrokeBlendMode;
            for (int i = 0; i < 4; ++i) {
                pConstantBuffer->innerStrokeColor.v[i] = NormalizeColor(pShape->innerStrokeColor[i]);
            }
        } else {
            pConstantBuffer->innerStrokeColor.v[3] = 0.0f;
        }

        CalculateProceduralShapeShadow(
            pConstantBuffer, radius,
            (pShape->flags & SystemDataProceduralShape::Flag_InnerShadow) != 0, false,
            pShape->innerShadowSize, pShape->innerShadowBlendMode, pShape->innerShadowAngle,
            pShape->innerShadowDistance, pShape->innerShadowColor, pShape->innerShadowType, false);

        if ((pShape->flags & SystemDataProceduralShape::Flag_ColorOverlay) != 0) {
            pConstantBuffer->colorOverlayBlendType = pShape->colorOverlayBlendMode;
            for (int i = 0; i < 4; ++i) {
                pConstantBuffer->colorOverlayColor.v[i] = NormalizeColor(pShape->colorOverlayColor[i]);
            }
        } else {
            pConstantBuffer->colorOverlayColor.w = 0.0f;
        }

        CalculateProceduralShapeGradationOverlay(pConstantBuffer, pShape);
    }

    if ((pShape->flags & SystemDataProceduralShape::Flag_DropShadow) != 0) {
        CalculateProceduralShapeDropShadow(rDrawInfo, pShape, pRuntimeInfo);
    }
}

/**
 * @brief Sets the blend state of the material for the pane effect source image.
 * @param rCommands Command buffer.
 */
void Picture::SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer& rCommands) const {
    m_pMaterial->SetCommandBufferOnlyBlend(rCommands);
}

/**
 * @brief Fills the size and the corner constants of a procedural shape.
 * @param pConstantBuffer Constants to fill.
 * @param pShape Procedural shape settings.
 * @return The clamped radius of the first corner.
 */
float Picture::CalculateProceduralShapeBasicParams(ProceduralShapeConstantBuffer* pConstantBuffer,
                                                   const SystemDataProceduralShape* pShape) {
    const float halfWidth = mSizeX * 0.5f;
    const float halfHeight = mSizeY * 0.5f;
    const float rcpHalfWidth = 1.0f / halfWidth;
    const float rcpHalfHeight = 1.0f / halfHeight;
    float firstRadius;

    for (int i = 0; i < 4; ++i) {
        const int index =
            (pShape->flags & SystemDataProceduralShape::Flag_IndividualCorner) != 0 ? i : 0;
        float radius = pShape->radius[index];
        detail::ClampValue(radius, 0.0f, halfWidth);
        detail::ClampValue(radius, 0.0f, halfHeight);

        pConstantBuffer->rcpHalfSize[0] = rcpHalfWidth;
        pConstantBuffer->rcpHalfSize[1] = rcpHalfHeight;
        pConstantBuffer->isEffectOnly =
            (pShape->flags & SystemDataProceduralShape::Flag_EffectOnly) != 0 ? 1.0f : 0.0f;

        ProceduralShapeConstantBuffer::Corner& rCorner = pConstantBuffer->corners[i];
        rCorner.radius = radius;
        rCorner.exp = pShape->exp[index];
        detail::ClampValue(rCorner.exp, 0.5f, 3.0f);

        const float ratioX = radius / halfWidth;
        rCorner.innerSize[0] = ratioX == 0.0f ? 0.99999f : 1.0f - ratioX;
        detail::ClampValue(rCorner.innerSize[0], 0.0f, 1.0f);

        const float ratioY = radius / halfHeight;
        rCorner.innerSize[1] = ratioY == 0.0f ? 0.99999f : 1.0f - ratioY;
        detail::ClampValue(rCorner.innerSize[1], 0.0f, 1.0f);

        if (i == 0) {
            firstRadius = radius;
        }
    }

    return firstRadius;
}

/**
 * @brief Fills the shadow constants of a procedural shape.
 * @param pConstantBuffer Constants to fill.
 * @param radius Radius of the first corner of the shape.
 * @param isEnabled Whether the shadow is drawn.
 * @param isKnockout Whether the shape cuts the drop shadow out.
 * @param size Blur size of the shadow.
 * @param blendType Blend mode of the shadow.
 * @param angle Direction of the shadow in degrees.
 * @param distance Distance of the shadow.
 * @param pColor Colour of the shadow.
 * @param shadowType Channel the shadow is drawn with.
 * @param isDropShadow Whether the shadow is a drop shadow.
 */
void Picture::CalculateProceduralShapeShadow(ProceduralShapeConstantBuffer* pConstantBuffer,
                                             float radius, bool isEnabled, bool isKnockout,
                                             float size, int blendType, float angle,
                                             float distance, const float* pColor, int shadowType,
                                             bool isDropShadow) {
    if (!isEnabled) {
        pConstantBuffer->shadowColor.w = 0.0f;
        return;
    }

    detail::ClampValue(size, 0.0f, 100.0f);
    const float softness = size / radius + 0.00001f;
    pConstantBuffer->shadowBlendType = blendType;
    pConstantBuffer->shadowSoftness = softness;

    detail::ClampValue(distance, 0.0f, 100.0f);
    const float radian = nn::util::DegreeToRadian(angle);
    pConstantBuffer->shadowOffset[0] = std::cos(radian) * (distance / mSizeX) * 2.0f;
    pConstantBuffer->shadowOffset[1] = std::sin(radian) * (distance / mSizeY) * -2.0f;

    for (int i = 0; i < 4; ++i) {
        pConstantBuffer->shadowColor.v[i] = NormalizeColor(pColor[i]);
        pConstantBuffer->shadowChannelWeight[i] = 0.0f;
    }

    pConstantBuffer->shadowChannelWeight[shadowType] = 1.0f;
    pConstantBuffer->shadowType = 0.0f;

    if (isDropShadow) {
        pConstantBuffer->shadowType = isKnockout ? 2.0f : 1.0f;
        pConstantBuffer->shadowColor.w *= NormalizeColor(GetGlobalAlpha());
    }
}

/**
 * @brief Fills the gradation overlay constants of a procedural shape.
 * @param pConstantBuffer Constants to fill.
 * @param pShape Procedural shape settings.
 */
void Picture::CalculateProceduralShapeGradationOverlay(
    ProceduralShapeConstantBuffer* pConstantBuffer, const SystemDataProceduralShape* pShape) {
    if ((pShape->flags & SystemDataProceduralShape::Flag_GradationOverlay) == 0) {
        for (int i = 0; i < 4; ++i) {
            pConstantBuffer->gradationControlPoint[i] = -1.0f;
        }

        return;
    }

    pConstantBuffer->gradationOverlayBlendType = pShape->gradationOverlayBlendMode;

    float prevControlPoint = 0.0f;
    int endIndex = -1;

    for (int i = 0; i < 4; ++i) {
        float controlPoint = pShape->gradationOverlayControlPoint[i];
        detail::ClampValue(controlPoint, 0.0f, 1.0f);

        if (endIndex >= 0) {
            pConstantBuffer->gradationControlPoint[i] = 1.0f;
            for (int j = 0; j < 4; ++j) {
                pConstantBuffer->gradationColor[i].v[j] =
                    NormalizeColor(pShape->gradationOverlayColor[endIndex][j]);
            }
        } else {
            pConstantBuffer->gradationControlPoint[i] = std::max(controlPoint, prevControlPoint);
            for (int j = 0; j < 4; ++j) {
                pConstantBuffer->gradationColor[i].v[j] =
                    NormalizeColor(pShape->gradationOverlayColor[i][j]);
            }

            prevControlPoint = controlPoint;
            if (controlPoint >= 1.0f) {
                endIndex = i;
            }
        }
    }

    const float angle = pShape->gradationOverlayAngle;
    const float quadrant = angle - static_cast<int>(angle / 90.0f) * 90.0f;
    const float diagonalRate = 1.0f - std::abs(quadrant / 90.0f - 0.5f) * 2.0f;
    const float diagonalScale = diagonalRate * 0.41421356f + 1.0f;
    const float aspectX = std::max(mSizeX / mSizeY, 1.0f);
    const float aspectY = std::max(mSizeY / mSizeX, 1.0f);
    const float degreeToRadian = nn::util::detail::FloatPi / nn::util::detail::FloatDegree180;

    pConstantBuffer->gradationDirection[0] =
        std::cos(angle * degreeToRadian) /
        (diagonalScale / ((aspectX - 1.0f) * diagonalRate + 1.0f));
    pConstantBuffer->gradationDirection[1] =
        -std::sin(degreeToRadian * pShape->gradationOverlayAngle) /
        (diagonalScale / ((aspectY - 1.0f) * diagonalRate + 1.0f));
}

/**
 * @brief Fills the vertex and pixel shader constants of the drop shadow of a procedural shape.
 * @param rDrawInfo Drawing state.
 * @param pShape Procedural shape settings.
 * @param pRuntimeInfo Constant buffer offsets of the procedural shape.
 */
void Picture::CalculateProceduralShapeDropShadow(DrawInfo& rDrawInfo,
                                                 const SystemDataProceduralShape* pShape,
                                                 SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo) {
    auto* pVertexConstantBuffer = reinterpret_cast<Material::ConstantBufferForVertexShader*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
        pRuntimeInfo->dropShadowVertexConstantBufferOffset);
    std::memcpy(pVertexConstantBuffer,
                GetMaterial()->GetConstantBufferForVertexShader(rDrawInfo),
                VertexShaderConstantBufferSize);

    nn::util::Float2 basePos = GetVertexPos();
    float distance = pShape->dropShadowDistance;
    detail::ClampValue(distance, 0.0f, 100.0f);

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (pShape->dropShadowDistance > 0.0f) {
        const float radian = nn::util::DegreeToRadian(pShape->dropShadowAngle);
        const float cosValue = std::cos(radian);
        offsetX = -(cosValue * distance);
        basePos.x -= cosValue * distance;
        const float sinValue = std::sin(radian);
        offsetY = -(sinValue * distance);
        basePos.y -= sinValue * distance;
    }

    detail::CalculateQuadWithTexCoords(
        rDrawInfo, pVertexConstantBuffer, basePos, GetSize(), GetTexCoordCount(),
        m_SharedMemory.texCoordArray.GetArray(), offsetX / mSizeX, -offsetY / mSizeY);

    for (int i = 0; i < 4; ++i) {
        pVertexConstantBuffer->color[i] = 1.0f;
    }

    auto* pConstantBuffer = reinterpret_cast<ProceduralShapeConstantBuffer*>(
        static_cast<u8*>(rDrawInfo.GetUi2dConstantBuffer()->GetMappedPointer()) +
        pRuntimeInfo->dropShadowConstantBufferOffset);
    const float radius = CalculateProceduralShapeBasicParams(pConstantBuffer, pShape);
    pConstantBuffer->innerStrokeColor.w = 0.0f;
    CalculateProceduralShapeShadow(
        pConstantBuffer, radius, (pShape->flags & SystemDataProceduralShape::Flag_DropShadow) != 0,
        (pShape->flags & SystemDataProceduralShape::Flag_DropShadowKnockout) != 0,
        pShape->dropShadowSize, pShape->dropShadowBlendMode, pShape->dropShadowAngle,
        pShape->dropShadowDistance, pShape->dropShadowColor, pShape->dropShadowType, true);
    pConstantBuffer->colorOverlayColor.w = 0.0f;
    pConstantBuffer->gradationControlPoint[0] = -1.0f;
}

/**
 * @param blendMode Blend mode of a procedural shape effect.
 * @return The preset blend state that draws the effect.
 */
PresetBlendStateId Picture::ConvertProceduralShapeEffectBlendType(
    detail::ProceduralShapeEffectBlendMode blendMode) const {
    switch (blendMode) {
    case detail::ProceduralShapeEffectBlendMode_Multiply:
        return PresetBlendStateId_Multiplication;
    case detail::ProceduralShapeEffectBlendMode_Add:
        return PresetBlendStateId_Addition;
    case detail::ProceduralShapeEffectBlendMode_Subtract:
        return PresetBlendStateId_Subtraction;
    default:
        return PresetBlendStateId_Default;
    }
}

/**
 * @brief Draws the drop shadow of a procedural shape.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer.
 * @param pShape Procedural shape settings.
 * @param pRuntimeInfo Constant buffer offsets of the procedural shape.
 */
void Picture::DrawProceduralShapeDropShadow(DrawInfo& rDrawInfo,
                                            nn::gfx::CommandBuffer& rCommands,
                                            const SystemDataProceduralShape* pShape,
                                            SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo) {
    const PresetBlendStateId blendStateId = ConvertProceduralShapeEffectBlendType(
        static_cast<detail::ProceduralShapeEffectBlendMode>(pShape->dropShadowBlendMode));
    rCommands.SetBlendState(const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                                ->GetPresetBlendState(blendStateId));

    nn::gfx::GpuAddress address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
    address.Offset(pRuntimeInfo->dropShadowVertexConstantBufferOffset);
    const ShaderInfo* pShaderInfo = GetMaterial()->GetShaderInfo();
    rCommands.SetConstantBuffer(
        pShaderInfo->GetVertexShaderSlot(GetMaterial()->GetShaderVariation()),
        nn::gfx::ShaderStage_Vertex, address, VertexShaderConstantBufferSize);

    address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
    address.Offset(pRuntimeInfo->dropShadowConstantBufferOffset);
    rCommands.SetConstantBuffer(pRuntimeInfo->constantBufferSlot, nn::gfx::ShaderStage_Pixel,
                                address, ProceduralShapeConstantBufferSize);

    detail::DrawQuad(rCommands, rDrawInfo);
    GetMaterial()->SetCommandBufferOnlyBlend(rCommands);
    GetMaterial()->ApplyVertexShaderConstantBuffer(rCommands, rDrawInfo);
}

/**
 * @brief Binds the procedural shape constants to the pixel shader.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer.
 * @param pRuntimeInfo Constant buffer offsets of the procedural shape.
 */
void Picture::SetupProceduralShapeConstantBuffer(
    DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands,
    const SystemDataProceduralShapeRuntimeInfo* pRuntimeInfo) const {
    nn::gfx::GpuAddress address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
    address.Offset(pRuntimeInfo->constantBufferOffset);

    if (pRuntimeInfo->constantBufferSlot >= 0) {
        rCommands.SetConstantBuffer(pRuntimeInfo->constantBufferSlot, nn::gfx::ShaderStage_Pixel,
                                    address, ProceduralShapeConstantBufferSize);
    }
}

/**
 * @param pShape Procedural shape settings.
 * @return Whether the pane itself is drawn, which it is not when it only casts a drop shadow.
 */
bool Picture::CheckDrawOriginalPaneByProceduralShapeState(
    const SystemDataProceduralShape* pShape) const {
    const u8 flags = pShape->flags & ~(SystemDataProceduralShape::Flag_DropShadowKnockout |
                                       SystemDataProceduralShape::Flag_IndividualCorner);
    return flags != (SystemDataProceduralShape::Flag_DropShadow |
                     SystemDataProceduralShape::Flag_EffectOnly);
}

/**
 * @brief Draws the picture.
 * @param rDrawInfo Drawing state.
 * @param rCommands Command buffer.
 */
void Picture::DrawSelf(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (m_pMaterial == nullptr || rDrawInfo.GetUi2dConstantBuffer()->IsUnallocated()) {
        return;
    }

    m_pMaterial->SetupSubmaterialOf_Texture(rDrawInfo, rCommands);

    if (IsTexCoordArrayEnabled()) {
        detail::SetupMaterialRenderState(rCommands, rDrawInfo, *m_pMaterial);

        if (IsSystemExtDataEnabled(PaneSystemDataType_ProceduralShapeRuntimeInfo)) {
            auto* pRuntimeInfo = static_cast<SystemDataProceduralShapeRuntimeInfo*>(
                GetSystemExtDataForModify(PaneSystemDataType_ProceduralShapeRuntimeInfo));

            if (pRuntimeInfo != nullptr) {
                const auto* pShape = static_cast<const SystemDataProceduralShape*>(
                    GetSystemExtDataByType(PaneSystemDataType_ProceduralShape));

                if (!IsPaneEffectEnabled() &&
                    (pShape->flags & SystemDataProceduralShape::Flag_DropShadow) != 0 &&
                    pShape->dropShadowColor[3] > 0.0f) {
                    DrawProceduralShapeDropShadow(rDrawInfo, rCommands, pShape, pRuntimeInfo);
                }

                SetupProceduralShapeConstantBuffer(rDrawInfo, rCommands, pRuntimeInfo);

                if (IsPaneEffectEnabled()) {
                    UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
                }

                if (CheckDrawOriginalPaneByProceduralShapeState(pShape)) {
                    detail::DrawQuad(rCommands, rDrawInfo);
                }

                return;
            }
        }

        if (IsSystemExtDataEnabled(PaneSystemDataType_PaneEffectInstance)) {
            UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
        }

        detail::DrawQuad(rCommands, rDrawInfo);
    } else {
        if (rDrawInfo.RecordCurrentShader(m_pMaterial->GetShaderInfo(),
                                          m_pMaterial->GetShaderVariation())) {
            m_pMaterial->SetShader(rCommands);
        }

        nn::gfx::GpuAddress address;
        address.ToData()->value = 0;
        address.ToData()->impl = 0;
        address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
        address.Offset(m_SharedMemory.shape.pShapeDrawInfo->vertexBufferGpuMemoryOffset);
        rCommands.SetVertexBuffer(0, address, m_SharedMemory.shape.pShapeDrawInfo->pShape->GetStride(),
                                  m_SharedMemory.shape.pShapeDrawInfo->pShape->GetVertexBufferSize());
        rDrawInfo.mVertexBufferDirty = true;

        m_pMaterial->SetCommandBuffer(rCommands, rDrawInfo);

        if (IsPaneEffectEnabled()) {
            UpdateRenderStateForPaneEffectCapture(rCommands, rDrawInfo);
        }

        address = rDrawInfo.GetUi2dConstantBuffer()->GetGpuAddress();
        address.Offset(m_SharedMemory.shape.pShapeDrawInfo->indexBufferGpuMemoryOffset);
        rCommands.DrawIndexed(nn::gfx::PrimitiveTopology_TriangleList,
                              m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexBufferFormat(),
                              address, m_SharedMemory.shape.pShapeDrawInfo->pShape->GetIndexCount(),
                              0);
    }
}

/**
 * @brief Checks that a copied picture equals its source.
 * @param rOther Picture copied from this one.
 * @return Whether every copied member matches.
 */
bool Picture::CompareCopiedInstanceTest(const Picture& rOther) const {
    if (!Pane::CompareCopiedInstanceTest(rOther)) {
        return false;
    }

    if (!m_pMaterial->CompareCopiedInstanceTest(*rOther.m_pMaterial)) {
        return false;
    }

    if (IsTexCoordArrayEnabled()) {
        if (m_SharedMemory.texCoordArray.GetSize() !=
            rOther.m_SharedMemory.texCoordArray.GetSize()) {
            return false;
        }

        if (!m_SharedMemory.texCoordArray.CompareCopiedInstanceTest(
                rOther.m_SharedMemory.texCoordArray)) {
            return false;
        }
    } else {
        if (m_SharedMemory.shape.kind != rOther.m_SharedMemory.shape.kind) {
            return false;
        }

        const ShapeDrawInfo* pShapeDrawInfo = m_SharedMemory.shape.pShapeDrawInfo;
        const ShapeDrawInfo* pOtherShapeDrawInfo = rOther.m_SharedMemory.shape.pShapeDrawInfo;
        if (pShapeDrawInfo->shapeType != pOtherShapeDrawInfo->shapeType) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetPrimitiveTopology() !=
            pOtherShapeDrawInfo->pShape->GetPrimitiveTopology()) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetVertexFormat() !=
            pOtherShapeDrawInfo->pShape->GetVertexFormat()) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetVertexCount() !=
            pOtherShapeDrawInfo->pShape->GetVertexCount()) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetVertexBufferSize() !=
            pOtherShapeDrawInfo->pShape->GetVertexBufferSize()) {
            return false;
        }

        if (std::memcpy(pShapeDrawInfo->pShape->GetVertexBuffer(),
                        pOtherShapeDrawInfo->pShape->GetVertexBuffer(),
                        pShapeDrawInfo->pShape->GetVertexBufferSize()) != nullptr) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetIndexCount() !=
            pOtherShapeDrawInfo->pShape->GetIndexCount()) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetIndexBufferFormat() !=
            pOtherShapeDrawInfo->pShape->GetIndexBufferFormat()) {
            return false;
        }

        if (pShapeDrawInfo->pShape->GetIndexBufferSize() !=
            pOtherShapeDrawInfo->pShape->GetIndexBufferSize()) {
            return false;
        }

        if (std::memcpy(pShapeDrawInfo->pShape->GetIndexBuffer(),
                        pOtherShapeDrawInfo->pShape->GetIndexBuffer(),
                        pShapeDrawInfo->pShape->GetIndexBufferSize()) != nullptr) {
            return false;
        }
    }

    return true;
}
}  // namespace nn::ui2d
