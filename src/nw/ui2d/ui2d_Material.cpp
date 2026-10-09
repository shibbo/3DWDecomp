#include <nn/ui2d/ui2d_Material.h>

#include <atomic>
#include <cstring>
#include <new>
#include <nn/font/font_GpuBuffer.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_Shader.h>
#include <nn/gfx/gfx_StateInfo.h>
#include <nn/ui2d/ui2d_AnimTransform.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_CaptureTexture.h>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_ExtUserData.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_RenderTargetTextureInfo.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_ShaderInfo.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_VectorApi.h>
#include <nvn/nvn_FuncPtrInline.h>

namespace nn {
namespace ui2d {

namespace detail {
const char* FindCaptureTextureBaseNameFromOverwriteString(const char* pName);
int CalcCaptureTextureNameOverwriteDepth(const char* pName);
}  // namespace detail

namespace {

/** @brief Signature of the archive shader used by the built-in material combinations. */
const u32 ArchiveShaderSignatureNormal = 0x4d524f4e;
/** @brief Signature of the archive shader used by the detailed combiner. */
const u32 ArchiveShaderSignatureDetailedCombiner = 0x42435444;
/** @brief Signature of the archive shader used by the combiner user shader. */
const u32 ArchiveShaderSignatureCombinerUserShader = 0x53554243;

/** @brief Number of shader variations generated for every shader id. */
const int ShaderVariationCountPerShaderId = 14;

/**
 * @param alphaTest Alpha test function of ui2d.
 * @return The matching NVN alpha function.
 */
NVNalphaFunc GetNvnAlphaFunc(int alphaTest) {
    static const NVNalphaFunc alphaFuncTable[AlphaTest_MaxAlphaTest] = {
        NVN_ALPHA_FUNC_NEVER,   NVN_ALPHA_FUNC_LESS,     NVN_ALPHA_FUNC_LEQUAL,
        NVN_ALPHA_FUNC_EQUAL,   NVN_ALPHA_FUNC_NOTEQUAL, NVN_ALPHA_FUNC_GEQUAL,
        NVN_ALPHA_FUNC_GREATER, NVN_ALPHA_FUNC_ALWAYS,
    };
    return alphaFuncTable[alphaTest];
}

/**
 * @brief Cast a runtime-typed object to a derived type.
 * @tparam TTo Pointer type to cast to.
 * @param pObject Object to cast.
 * @return pObject as TTo when it derives from it, otherwise nullptr.
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
 * @param pTextureList Texture name table of a layout resource.
 * @param index Index of the texture.
 * @return Name of the texture.
 */
const char* GetTextureName(const void* pTextureList, int index) {
    const u32* pOffsets = reinterpret_cast<const u32*>(static_cast<const u8*>(pTextureList) + 0xc);
    return reinterpret_cast<const char*>(pOffsets) + pOffsets[index];
}

/**
 * @brief Reserve a region of a constant buffer.
 * @param pBuffer Constant buffer to allocate from.
 * @param size Size of the region, already aligned.
 * @return Offset of the region within the buffer.
 */
u64 AllocateConstantBufferRegion(font::GpuBuffer* pBuffer, size_t size) {
    if (pBuffer->m_Flags & font::GpuBuffer::Flag_AtomicAllocation) {
        return reinterpret_cast<std::atomic<u64>*>(pBuffer->m_pAtomicAllocatedSize)->fetch_add(size);
    }

    u64 offset = pBuffer->m_AllocatedSize;
    pBuffer->m_AllocatedSize += size;
    return offset;
}

/**
 * @param degree Angle in degrees.
 * @return Angle index of degree.
 */
util::AngleIndex DegreeToAngleIndex(float degree) {
    return static_cast<int64_t>(
        degree * (static_cast<float>(util::detail::AngleIndexHalfRound) / util::detail::FloatDegree180));
}

/**
 * @brief Evaluate sine and cosine from the sample table.
 * @param pSin Receives the sine.
 * @param pCos Receives the cosine.
 * @param angleIndex Angle to evaluate.
 */
void SinCosTable(float* pSin, float* pCos, util::AngleIndex angleIndex) {
    const util::detail::SinCosSample* pSample =
        &util::detail::SinCosSampleTable[(angleIndex >> 24) & 0xff];
    float rest = static_cast<float>(angleIndex & 0xffffff) * (1.0f / 0x1000000);
    *pSin = pSample->sinValue + pSample->sinDelta * rest;
    *pCos = pSample->cosValue + pSample->cosDelta * rest;
}

/** @brief Type of the extended user data that holds system data. */
const u8 ExtUserDataType_SystemData = 3;
/** @brief Type of the system data describing a procedural shape. */
const u32 SystemDataType_ProceduralShape = 6;

/** @brief System data block of the extended user data. */
struct ResSystemExtUserData {
    u16 version;
    u16 count;
    u32 offsets[1];
};

/** @brief Procedural shape description stored in the system data. */
struct ResSystemDataProceduralShape {
    u32 type;
    u8 flags;
    u8 innerStrokeType;
    u8 reserve[2];
    u8 shadowType;
    u8 outerStrokeType;
};

/**
 * @brief Inverts an affine matrix.
 * @param pOut Receives the inverse; it is zero when rMtx is singular.
 * @param rMtx Matrix to invert.
 */
void MatrixInverse(util::MatrixT4x3fType* pOut, const util::MatrixT4x3fType& rMtx) {
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
 * @brief Multiplies two affine matrices.
 * @param pOut Receives rLhs * rRhs.
 * @param rLhs Left matrix.
 * @param rRhs Right matrix.
 */
void MatrixMultiply(util::MatrixT4x3fType* pOut, const util::MatrixT4x3fType& rLhs,
                    const util::MatrixT4x3fType& rRhs) {
    const float32x4_t zero = vdupq_n_f32(0.0f);
    for (int i = 0; i < 3; i++) {
        const float32x4_t row = rLhs._m.val[i];
        float32x4_t result = vmulq_n_f32(rRhs._m.val[0], vgetq_lane_f32(row, 0));
        result = vfmaq_n_f32(result, rRhs._m.val[1], vgetq_lane_f32(row, 1));
        result = vfmaq_n_f32(result, rRhs._m.val[2], vgetq_lane_f32(row, 2));
        pOut->_m.val[i] = vaddq_f32(vsetq_lane_f32(vgetq_lane_f32(row, 3), zero, 3), result);
    }
}

/**
 * @brief Stores the first three components of a vector.
 * @param pOut Receives the components.
 * @param rVector Vector to store.
 */
void VectorStore(float* pOut, const util::Vector3fType& rVector) {
    vst1_f32(pOut, vget_low_f32(rVector._v));
    vst1q_lane_f32(pOut + 2, rVector._v, 2);
}

/**
 * @brief Stores an affine matrix as three rows.
 * @param pOut Receives the rows.
 * @param rMtx Matrix to store.
 */
void StoreMatrix(float (*pOut)[4], const util::MatrixT4x3fType& rMtx) {
    const float32x4x3_t rows = rMtx._m;
    vst1q_f32(pOut[0], rows.val[0]);
    vst1q_f32(pOut[1], rows.val[1]);
    vst1q_f32(pOut[2], rows.val[2]);
}

}  // namespace

/** @brief Constructs an empty material. */
Material::Material() {
    Initialize();
}

/** @brief Resets the colours, the parameter memory and the shader to their defaults. */
void Material::Initialize() {
    mBlackColor = 0;
    mWhiteColor = 0xffffffff;
    InitializeMatMemCount(&m_MemCap);
    InitializeMatMemCount(&m_MemNum);
    m_pMem = nullptr;
    m_pShaderInfo = nullptr;
    mName = nullptr;
    mUserShader = nullptr;
    mUserShaderConstantBufferInformation = nullptr;
    m_pBlendState = nullptr;
    mFlags = 0xe0;
}

/**
 * @brief Builds the material from its resource.
 * @param pResult Receives the memory requirements of the material.
 * @param pDevice Device the material resources are created on.
 * @param pBaseRes Material resource of the layout.
 * @param pOverrideRes Material resource of the parts that overrides pBaseRes, or nullptr.
 * @param rBuildArgSet Build arguments.
 */
void Material::InitializeMaterialImpl(BuildResultInformation* pResult, gfx::Device* pDevice,
                                      const ResMaterial* pBaseRes, const ResMaterial* pOverrideRes,
                                      const BuildArgSet& rBuildArgSet) {
    Initialize();

    const BuildResSet* pBuildResSet;
    const ResMaterial* pColorRes = pBaseRes;
    const ResMaterial* pTextureRes = pBaseRes;
    if (pOverrideRes != nullptr) {
        if (rBuildArgSet.overrideUsageFlag != 0) {
            pBuildResSet = rBuildArgSet.pCurrentBuildResSet;
            pColorRes = (rBuildArgSet.overrideUsageFlag & 1) ? pOverrideRes : pBaseRes;
            if ((rBuildArgSet.overrideUsageFlag & 2) &&
                ((pBaseRes->flags ^ pOverrideRes->flags) & 3) == 0) {
                pBuildResSet = rBuildArgSet.pOverrideBuildResSet;
                pTextureRes = pOverrideRes;
            }
        } else if (rBuildArgSet.overridePaneUsageFlag == 0 ||
                   (rBuildArgSet.overridePaneUsageFlag & 4)) {
            pBuildResSet = rBuildArgSet.pOverrideBuildResSet;
            pBaseRes = pOverrideRes;
            pColorRes = pOverrideRes;
            pTextureRes = pOverrideRes;
        } else {
            pBuildResSet = rBuildArgSet.pCurrentBuildResSet;
        }
    } else {
        pBuildResSet = rBuildArgSet.pCurrentBuildResSet;
    }

    mName = pBaseRes->name;
    if (rBuildArgSet._DA) {
        m_IsDynamicTextureCreator = 1;
    }

    const ResMaterialColor* pColor = &pColorRes->color;
    if (pColor->byteColorFlags & 3) {
        m_pFloatColors = static_cast<util::Float4*>(
            Layout::AllocateMemory(sizeof(util::Float4) * MaterialColor_Max));
        m_IsFloatColorAllocated = 1;
        m_IsBlackColorFloat = 1;
        m_pFloatColors[MaterialColor_Black] = *pColor->GetFloatColor(MaterialColor_Black);
        m_IsWhiteColorFloat = 1;
        m_pFloatColors[MaterialColor_White] = *pColor->GetFloatColor(MaterialColor_White);
    } else {
        m_ByteColors[MaterialColor_Black] = *pColor->GetByteColor(MaterialColor_Black);
        m_ByteColors[MaterialColor_White] = *pColor->GetByteColor(MaterialColor_White);
    }

    size_t texMapOffset = offsetof(ResMaterial, color) + pColor->GetSize();
    size_t additionalInfoOffset = texMapOffset + sizeof(ResTexMap) * (pTextureRes->flags & 3);
    const u32 resFlags = pBaseRes->flags;
    size_t texSrtOffset =
        additionalInfoOffset + ((resFlags & ResMaterial::Flag_TexMapAdditionalInfo)
                                    ? sizeof(ResTexMapAdditionalInfo) * detail::GetBits(resFlags, 0, 2)
                                    : 0);
    const u8 texSrtNum = detail::GetBits<u8>(resFlags, 2, 2);
    const u8 texCoordGenNum = detail::GetBits<u8>(resFlags, 4, 2);
    const u8 resTevStageNum = detail::GetBits(resFlags, 6, 3);
    size_t texCoordGenOffset = texSrtOffset + sizeof(ResTexSrt) * texSrtNum;
    size_t tevStageOffset = texCoordGenOffset + sizeof(ResTexCoordGen) * texCoordGenNum;
    size_t alpCompOffset = tevStageOffset + sizeof(ResTevStage) * resTevStageNum;
    size_t blendModeOffset = alpCompOffset + ((resFlags & ResMaterial::Flag_AlphaCompare) >> 6);
    size_t blendModeAlphaOffset = blendModeOffset + ((resFlags & ResMaterial::Flag_BlendMode) >> 8);
    size_t indirectOffset =
        blendModeAlphaOffset + ((resFlags & ResMaterial::Flag_BlendModeAlpha) >> 10);
    size_t offset = indirectOffset + ((resFlags & ResMaterial::Flag_IndirectParameter) ? sizeof(ResIndirectParameter) : 0);

    const ResDetailedCombinerStageInfo* pResDetailedCombinerStageInfo;
    const ResDetailedCombinerStage* pResDetailedCombinerStages;
    size_t detailedCombinerStageSize;
    if (resFlags & ResMaterial::Flag_DetailedCombiner) {
        pResDetailedCombinerStageInfo = pBaseRes->GetPtr<ResDetailedCombinerStageInfo>(offset);
        offset += sizeof(ResDetailedCombinerStageInfo);
        pResDetailedCombinerStages = pBaseRes->GetPtr<ResDetailedCombinerStage>(offset);
        detailedCombinerStageSize = sizeof(ResDetailedCombinerStage) * resTevStageNum;
    } else {
        pResDetailedCombinerStages = nullptr;
        detailedCombinerStageSize = 0;
        pResDetailedCombinerStageInfo = nullptr;
    }

    const bool isAlpCompAllocated = (resFlags & ResMaterial::Flag_AlphaCompare) != 0;
    const u32 fontShadowFlag = resFlags & ResMaterial::Flag_FontShadowParameter;
    const bool isFontShadowAllocated = fontShadowFlag != 0;
    const size_t combinerUserShaderSize =
        (resFlags & ResMaterial::Flag_CombinerUserShader) ? sizeof(ResCombinerUserShader) : 0;
    const int vectorGraphicsTextureNum = (resFlags >> 22) & 3;
    const u32 vectorGraphicsTextureInfoSize =
        vectorGraphicsTextureNum != 0 ? sizeof(ResVectorGraphicsTextureInfo) * vectorGraphicsTextureNum : 0;
    const int tevStageNum = resTevStageNum == 7 ? TevStageMax : resTevStageNum;
    const int blendModeNum = (resFlags & ResMaterial::Flag_BlendModeAlpha) ? 2 : (resFlags & ResMaterial::Flag_BlendMode) >> 10;
    m_IsThresholdingAlphaInterpolation = (resFlags & ResMaterial::Flag_ThresholdingAlphaInterpolation) != 0;
    const int projectionTexGenNum = (resFlags >> 15) & 3;
    const u8 texMapNum = detail::GetBits<u8>(resFlags, 0, 2);
    const int brickRepeatNum = (resFlags >> 24) & 3;
    ReserveMem(texMapNum, texSrtNum, texCoordGenNum, tevStageNum, isAlpCompAllocated, blendModeNum,
               (resFlags >> 14) & 1, projectionTexGenNum, isFontShadowAllocated,
               (resFlags >> 19) & 1, (resFlags >> 20) & 1, vectorGraphicsTextureNum,
               brickRepeatNum);
    m_IsTextureOnly = (pBaseRes->flags & ResMaterial::Flag_TextureOnly) != 0;

    if (m_pMem != nullptr) {
        size_t projectionTexGenOffset = detailedCombinerStageSize + offset;
        size_t fontShadowOffset = projectionTexGenOffset + sizeof(ResProjectionTexGenParameters) * projectionTexGenNum;
        size_t combinerUserShaderOffset = fontShadowOffset + (fontShadowFlag >> 14);
        size_t vectorGraphicsOffset = combinerUserShaderOffset + combinerUserShaderSize;

        for (int i = 0; i < m_MemCap.texMap; i++) {
            new (&GetTexMapAry()[i]) TexMap();
        }

        m_MemNum.texMap = texMapNum;
        if (texMapNum != 0) {
            InitializeTexMap(pDevice, pBuildResSet, rBuildArgSet, pTextureRes,
                             pTextureRes->GetPtr<ResTexMap>(texMapOffset),
                             pTextureRes->GetPtr<ResTexMapAdditionalInfo>(additionalInfoOffset),
                             pBaseRes->GetPtr<ResVectorGraphicsTextureInfo>(vectorGraphicsOffset));
        }

        ResTexSrt* pTexSrts = GetTexSrtAry();
        for (int i = 0; i < m_MemCap.texSrt; i++) {
            new (&pTexSrts[i]) ResTexSrt();
        }

        m_MemNum.texSrt = texSrtNum;
        const ResTexSrt* pResTexSrts = pBaseRes->GetPtr<ResTexSrt>(texSrtOffset);
        for (int i = 0; i < texSrtNum; i++) {
            pTexSrts[i].translate = pResTexSrts[i].translate;
            pTexSrts[i].rotate = pResTexSrts[i].rotate;
            pTexSrts[i].scale = pResTexSrts[i].scale;
        }

        ResTexCoordGen* pTexCoordGens = GetTexCoordGenAry();
        for (int i = 0; i < m_MemCap.texCoordGen; i++) {
            new (&pTexCoordGens[i]) ResTexCoordGen();
        }

        m_MemNum.texCoordGen = texCoordGenNum;
        const ResTexCoordGen* pResTexCoordGens = pBaseRes->GetPtr<ResTexCoordGen>(texCoordGenOffset);
        int projectionTexGenIdx = 0;
        for (int i = 0; i < m_MemNum.texCoordGen; i++) {
            pTexCoordGens[i] = pResTexCoordGens[i];
            if (pTexCoordGens[i].IsProjection()) {
                pTexCoordGens[i].pProjectionTexGenParameters =
                    &GetProjectionTexGenAry()[projectionTexGenIdx];
                projectionTexGenIdx++;
            } else {
                pTexCoordGens[i].pProjectionTexGenParameters = nullptr;
            }
        }

        if (tevStageNum != 0) {
            ResTevStage* pTevStages = GetTevStageAry();
            for (int i = 0; i < m_MemCap.tevStage; i++) {
                new (&pTevStages[i]) ResTevStage();
            }

            m_MemNum.tevStage = tevStageNum;
            const ResTevStage* pResTevStages = pBaseRes->GetPtr<ResTevStage>(tevStageOffset);
            for (int i = 0; i < tevStageNum; i++) {
                pTevStages[i] = pResTevStages[i];
            }
        }

        if (resFlags & ResMaterial::Flag_AlphaCompare) {
            *GetAlphaComparePtr() = *pBaseRes->GetPtr<ResAlphaCompare>(alpCompOffset);
        }

        if (resFlags & ResMaterial::Flag_BlendMode) {
            GetBlendModeAry()[0] = *pBaseRes->GetPtr<ResBlendMode>(blendModeOffset);
        }

        if (resFlags & ResMaterial::Flag_BlendModeAlpha) {
            GetBlendModeAry()[1] = *pBaseRes->GetPtr<ResBlendMode>(blendModeAlphaOffset);
        }

        if (resFlags & ResMaterial::Flag_IndirectParameter) {
            *GetIndirectParameterPtr() = *pBaseRes->GetPtr<ResIndirectParameter>(indirectOffset);
        }

        if ((resFlags & ResMaterial::Flag_DetailedCombiner) &&
            pResDetailedCombinerStageInfo != nullptr) {
            *GetDetailedCombinerStageInfoPtr() = *pResDetailedCombinerStageInfo;
            for (int i = 0; i < tevStageNum; i++) {
                GetDetailedCombinerStageAry()[i] = pResDetailedCombinerStages[i];
            }
        }

        if (projectionTexGenNum != 0) {
            ResProjectionTexGenParameters* pParameters = GetProjectionTexGenAry();
            for (int i = 0; i < m_MemCap.projectionTexGen; i++) {
                new (&pParameters[i]) ResProjectionTexGenParameters();
            }

            m_MemNum.projectionTexGen = projectionTexGenNum;
            const ResProjectionTexGenParameters* pResParameters =
                pBaseRes->GetPtr<ResProjectionTexGenParameters>(projectionTexGenOffset);
            for (int i = 0; i < projectionTexGenNum; i++) {
                pParameters[i] = pResParameters[i];
            }
        }

        if (brickRepeatNum != 0) {
            ResBrickRepeatTexGenParameters* pParameters = GetBrickRepeatTexGenParametersAry();
            for (int i = 0; i < m_MemCap.brickRepeatParameter; i++) {
                new (&pParameters[i]) ResBrickRepeatTexGenParameters();
            }

            m_MemNum.brickRepeatParameter = brickRepeatNum;
            const ResBrickRepeatTexGenParameters* pResParameters =
                pBaseRes->GetPtr<ResBrickRepeatTexGenParameters>(vectorGraphicsOffset +
                                                                 vectorGraphicsTextureInfoSize);
            for (int i = 0; i < brickRepeatNum; i++) {
                pParameters[i] = pResParameters[i];
            }
        }

        if (fontShadowFlag != 0) {
            *GetFontShadowParameterPtr() = *pBaseRes->GetPtr<ResFontShadowParameter>(fontShadowOffset);
        }

        if (resFlags & ResMaterial::Flag_CombinerUserShader) {
            *GetCombinerUserShaderPtr() =
                *pBaseRes->GetPtr<ResCombinerUserShader>(combinerUserShaderOffset);
        }
    }

    SetupShader(pDevice, rBuildArgSet, pBuildResSet);
    CollectConstantBufferSize(pResult, pDevice);
    InitializeBlendInformationImpl(pDevice);
}

/**
 * @brief Allocates the parameter memory of the material.
 * @param texMapNum Number of texture maps.
 * @param texSrtNum Number of texture SRT parameters.
 * @param texCoordGenNum Number of texture coordinate generators.
 * @param tevStageNum Number of combiner stages.
 * @param allocateAlpComp Whether to allocate the alpha compare settings.
 * @param blendModeNum Number of blend modes.
 * @param allocateIndirectParameter Whether to allocate the indirect parameter.
 * @param projectionTexGenNum Number of projection texture generation parameters.
 * @param allocateFontShadowParameter Whether to allocate the font shadow parameter.
 * @param allocateDetailedCombiner Whether to allocate the detailed combiner parameters.
 * @param allocateCombinerUserShader Whether to allocate the combiner user shader settings.
 * @param vectorGraphicsTextureNum Number of vector graphics texture references.
 * @param brickRepeatNum Number of brick repeat texture generation parameters.
 * @return Whether the memory is available.
 */
bool Material::ReserveMem(int texMapNum, int texSrtNum, int texCoordGenNum, int tevStageNum,
                          bool allocateAlpComp, int blendModeNum, bool allocateIndirectParameter,
                          int projectionTexGenNum, bool allocateFontShadowParameter,
                          bool allocateDetailedCombiner, bool allocateCombinerUserShader,
                          int vectorGraphicsTextureNum, int brickRepeatNum) {
    if (m_MemCap.texMap < texMapNum || m_MemCap.texSrt < texSrtNum ||
        m_MemCap.texCoordGen < texCoordGenNum || m_MemCap.tevStage < tevStageNum ||
        m_MemCap.alpComp < static_cast<u32>(allocateAlpComp) || m_MemCap.blendMode < blendModeNum ||
        m_MemCap.indirectParameter < static_cast<u32>(allocateIndirectParameter) ||
        m_MemCap.projectionTexGen < projectionTexGenNum ||
        m_MemCap.fontShadowParameter < static_cast<u32>(allocateFontShadowParameter) ||
        m_MemCap.detailedCombinerParameter < static_cast<u32>(allocateDetailedCombiner) ||
        m_MemCap.combinerUserShaderParameter < static_cast<u32>(allocateCombinerUserShader) ||
        m_MemCap.vectorGraphicsTexture < vectorGraphicsTextureNum ||
        m_MemCap.brickRepeatParameter < brickRepeatNum) {
        if (m_pMem != nullptr) {
            Layout::FreeMemory(m_pMem);
            m_pMem = nullptr;
            InitializeMatMemCount(&m_MemCap);
            InitializeMatMemCount(&m_MemNum);
        }

        m_pMem = Layout::AllocateMemory(CalculateReserveMemSize(
            texMapNum, texSrtNum, texCoordGenNum, tevStageNum, allocateAlpComp, blendModeNum,
            allocateIndirectParameter, projectionTexGenNum, allocateFontShadowParameter,
            allocateDetailedCombiner, allocateCombinerUserShader, vectorGraphicsTextureNum,
            brickRepeatNum));
        if (m_pMem == nullptr) {
            return false;
        }

        ResParameterToMemory(texMapNum, texSrtNum, texCoordGenNum, tevStageNum, allocateAlpComp,
                             blendModeNum, allocateIndirectParameter, projectionTexGenNum,
                             allocateFontShadowParameter, allocateDetailedCombiner,
                             allocateCombinerUserShader, vectorGraphicsTextureNum,
                             brickRepeatNum);
    }

    return true;
}

/**
 * @brief Binds the textures of the material.
 * @param pDevice Device the textures are created on.
 * @param pBuildResSet Resources of the layout.
 * @param rBuildArgSet Build arguments.
 * @param pResMaterial Material resource holding the textures.
 * @param pResTexMap Texture resources.
 * @param pAdditionalInfo Runtime texture information for every texture.
 * @param pVectorGraphicsInfo Vector graphics texture resources.
 */
void Material::InitializeTexMap(gfx::Device* pDevice, const BuildResSet* pBuildResSet,
                                const BuildArgSet& rBuildArgSet, const ResMaterial* pResMaterial,
                                const ResTexMap* pResTexMap,
                                const ResTexMapAdditionalInfo* pAdditionalInfo,
                                const ResVectorGraphicsTextureInfo* pVectorGraphicsInfo) {
    const int texMapNum = m_MemNum.texMap;
    TexMap* pTexMaps = GetTexMapAry();
    u8 vectorGraphicsIdx = 0;
    for (int i = 0; i < texMapNum; i++) {
        const char* pName = GetTextureName(pBuildResSet->pTextureList, pResTexMap[i].texIdx);
        const TextureInfo* pTextureInfo;
        if ((pResMaterial->flags & ResMaterial::Flag_TexMapAdditionalInfo) &&
            pAdditionalInfo[i].info != 0) {
            if (pAdditionalInfo[i].info & ResTexMapAdditionalInfo::Info_CaptureTexture) {
                CaptureTexture* pTexture = InitializeCaptureTextureReference(
                    &pTexMaps[i], pBuildResSet, rBuildArgSet, pName);
                pTextureInfo = static_cast<const TextureInfo*>(pTexture->_08);
                pTexture->m_DrawPriority = 2;
            } else if (pAdditionalInfo[i].info & ResTexMapAdditionalInfo::Info_VectorGraphicsTexture) {
                if (rBuildArgSet._A8) {
                    pTextureInfo = new (Layout::AllocateMemory(sizeof(DummyRenderTargetTextureInfo), 16))
                        DummyRenderTargetTextureInfo(pName);
                    pTexMaps[i].mIsDummyTextureInfo = 1;
                    pTexMaps[i].mIsDynamicTextureOwner = 0;
                    pTexMaps[i].mShareInfoStackOffset = 0;
                } else {
                    detail::VectorGraphicsTexture* pTexture = InitializeVectorGraphicsTextureReference(
                        &pTexMaps[i], pBuildResSet, rBuildArgSet, pName);
                    SetVectorGraphicsTexturRefInfo(
                        &GetVectorGraphicsTextureRefInfoAry()[vectorGraphicsIdx],
                        pVectorGraphicsInfo[vectorGraphicsIdx], pTexture);
                    vectorGraphicsIdx++;
                    pTextureInfo = static_cast<const TextureInfo*>(pTexture->_08);
                }
            } else {
                pTextureInfo = nullptr;
            }
        } else {
            pTextureInfo = pBuildResSet->pResAccessor->AcquireTexture(pDevice, pName);
        }

        pTexMaps[i].m_pTextureInfo = pTextureInfo;
        pTexMaps[i].SetWrapMode(pResTexMap[i].GetWarpModeS(), pResTexMap[i].GetWarpModeT());
        pTexMaps[i].SetFilter(pResTexMap[i].GetMinFilter(), pResTexMap[i].GetMagFilter());
    }
}

/**
 * @brief Chooses the shader the material is drawn with.
 * @param pDevice Device the shader is created on.
 * @param rBuildArgSet Build arguments.
 * @param pBuildResSet Resources of the layout.
 */
void Material::SetupShader(gfx::Device* pDevice, const BuildArgSet& rBuildArgSet,
                           const BuildResSet* pBuildResSet) {
    m_pShaderInfo = nullptr;
    mShaderVariation = 0;
    m_ShaderId = ShaderId_Undefined;

    if (TryToSetupUserShader(pDevice, rBuildArgSet, pBuildResSet)) {
        return;
    }

    if (TryToSetupDetailedCombinerOrCombinerUserShader(pDevice, pBuildResSet)) {
        return;
    }

    size_t keyCount;
    u32 keys[ShaderKeyMax];
    if (TryToSetupSharedShader(&keyCount, keys, rBuildArgSet)) {
        return;
    }

    m_pShaderInfo = pBuildResSet->pLayout->AcquireArchiveShader(pDevice, ArchiveShaderSignatureNormal,
                                                                keyCount, keys);
    int variation = SearchShaderVariationIndexFromTable(m_pShaderInfo->m_pVariationTable,
                                                        ArchiveShaderSignatureNormal, keyCount, keys);
    m_ShaderId = ShaderId_Archive;
    mShaderVariation = variation;

    if (IsBrickRepeatTextureUsed()) {
        GetBrickRepeatShaderInfoPtr()->slot = m_pShaderInfo->GetPixelShader(variation & 0xffff)->GetInterfaceSlot(
            gfx::ShaderStage_Pixel, gfx::ShaderInterfaceType_ConstantBuffer,
            "uConstantBufferForBrickRepeatPixelShader");
    }
}

/**
 * @brief Adds the constant buffer sizes of the material to the build result.
 * @param pResult Receives the memory requirements of the material.
 * @param pDevice Device the constant buffers are created on.
 */
void Material::CollectConstantBufferSize(BuildResultInformation* pResult,
                                         gfx::Device* pDevice) const {
    if (pResult == nullptr) {
        return;
    }

    pResult->requiredUi2dConstantBufferSize +=
        GetAlignedBufferSize(pDevice, gfx::GpuAccess_ConstantBuffer, GetVertexShaderConstantBufferSize());

    size_t pixelSize;
    if (m_MemCap.detailedCombinerParameter) {
        pixelSize = GetPixelShaderDetailedCombinerConstantBufferSize();
    } else if (m_MemCap.combinerUserShaderParameter) {
        pixelSize = GetPixelShaderCombinerUserShaderConstantBufferSize();
    } else {
        pixelSize = GetPixelShaderConstantBufferSize();
    }

    pResult->requiredUi2dConstantBufferSize +=
        GetAlignedBufferSize(pDevice, gfx::GpuAccess_ConstantBuffer, pixelSize);

    if (IsBrickRepeatTextureUsed()) {
        pResult->requiredUi2dConstantBufferSize +=
            GetAlignedBufferSize(pDevice, gfx::GpuAccess_ConstantBuffer, BrickRepeatConstantBufferSize);
    }

    if (mUserShaderConstantBufferInformation != nullptr &&
        mUserShaderConstantBufferInformation->geometrySize != 0) {
        pResult->requiredUi2dConstantBufferSize += GetAlignedBufferSize(
            pDevice, gfx::GpuAccess_ConstantBuffer, mUserShaderConstantBufferInformation->geometrySize);
    }
}

/**
 * @brief Creates the blend state of the material when no preset matches.
 * @param pDevice Device the blend state is created on.
 */
void Material::InitializeBlendInformationImpl(gfx::Device* pDevice) {
    if (GetBlendStateId() != PresetBlendStateId_None) {
        m_pBlendState = nullptr;
        return;
    }

    m_pBlendState = new (Layout::AllocateMemory(sizeof(gfx::BlendState))) gfx::BlendState();

    gfx::BlendStateInfo blendStateInfo;
    gfx::BlendTargetStateInfo blendTargetStateInfo;
    memset(&blendStateInfo, 0, sizeof(blendStateInfo));
    memset(&blendTargetStateInfo, 0, sizeof(blendTargetStateInfo));
    size_t memorySize = GraphicsResource::SetupBlendStateInfo(
        &blendStateInfo, &blendTargetStateInfo, GetBlendMode(), GetBlendModeAlpha());
    m_pBlendState->SetMemory(Layout::AllocateMemory(memorySize, 8), memorySize);
    m_pBlendState->Initialize(pDevice, blendStateInfo);
}

/**
 * @brief Links a material texture to a vector graphics texture.
 * @param pRefInfo Receives the link.
 * @param rResInfo Vector graphics texture resource of the material.
 * @param pTexture Vector graphics texture.
 */
void Material::SetVectorGraphicsTexturRefInfo(detail::RefVectorGraphicsTextureInfo* pRefInfo,
                                              const ResVectorGraphicsTextureInfo& rResInfo,
                                              detail::VectorGraphicsTexture* pTexture) const {
    pRefInfo->time = rResInfo.time;
    pRefInfo->color.x = rResInfo.color[0] / 255.0f;
    pRefInfo->color.y = rResInfo.color[1] / 255.0f;
    pRefInfo->color.z = rResInfo.color[2] / 255.0f;
    pRefInfo->color.w = rResInfo.color[3] / 255.0f;
    pRefInfo->pTexture = pTexture;
    pTexture->SetColor(pRefInfo->color);
}

/**
 * @brief Finds or creates the capture texture a material texture refers to.
 * @param pTexMap Material texture.
 * @param pBuildResSet Resources of the layout.
 * @param rBuildArgSet Build arguments.
 * @param pName Name of the texture.
 * @return The capture texture.
 */
CaptureTexture* Material::InitializeCaptureTextureReference(TexMap* pTexMap,
                                                            const BuildResSet* pBuildResSet,
                                                            const BuildArgSet& rBuildArgSet,
                                                            const char* pName) {
    const BuildResSet* pCurrentBuildResSet = rBuildArgSet.pCurrentBuildResSet;
    int currentOffset = rBuildArgSet.pBuildPaneTreeContext->GetCurrentShareInfoStackOffset();
    const char* pBaseName = detail::FindCaptureTextureBaseNameFromOverwriteString(pName);
    int offset;
    if (pCurrentBuildResSet == pBuildResSet) {
        offset = currentOffset;
    } else {
        offset = rBuildArgSet.mAlternateDynamicTexturePrefixDepth +
                 detail::CalcCaptureTextureNameOverwriteDepth(pName);
    }

    detail::DynamicTextureShareInfo* pShareInfo =
        rBuildArgSet.pBuildPaneTreeContext->GetTextureShareInfoFromRootOffset(offset);
    CaptureTexture* pTexture = pShareInfo->FindCaptureTexture(pBaseName);
    if (pTexture == nullptr) {
        pTexture = pShareInfo->CreateCaptureTexture(pBaseName);
        pTexMap->mIsDynamicTextureOwner = 1;
    }

    pTexMap->mShareInfoStackOffset = currentOffset - offset;
    return pTexture;
}

/**
 * @brief Creates the vector graphics texture a material texture refers to.
 * @param pTexMap Material texture.
 * @param pBuildResSet Resources of the layout.
 * @param rBuildArgSet Build arguments.
 * @param pName Name of the texture.
 * @return The vector graphics texture.
 */
detail::VectorGraphicsTexture* Material::InitializeVectorGraphicsTextureReference(
    TexMap* pTexMap, const BuildResSet* pBuildResSet, const BuildArgSet& rBuildArgSet,
    const char* pName) {
    detail::VectorGraphicsTexture* pTexture;
    int stackOffset;
    if (rBuildArgSet.pCurrentBuildResSet == pBuildResSet) {
        pTexture = rBuildArgSet.pBuildPaneTreeContext->GetCurrentTextureShareInfo()
                       ->CreateVectorGraphicsTexture(pName);
        stackOffset = 0;
    } else {
        pTexture = rBuildArgSet.pBuildPaneTreeContext
                       ->GetTextureShareInfoFromRootOffset(
                           rBuildArgSet.mAlternateDynamicTexturePrefixDepth)
                       ->CreateVectorGraphicsTexture(pName);
        stackOffset = rBuildArgSet.pBuildPaneTreeContext->GetCurrentShareInfoStackOffset() -
                      rBuildArgSet.mAlternateDynamicTexturePrefixDepth;
    }

    pTexMap->mIsDummyTextureInfo = 0;
    pTexMap->mIsDynamicTextureOwner = 1;
    pTexMap->mShareInfoStackOffset = stackOffset;
    return pTexture;
}

/**
 * @brief Sets up the resources of the capture and vector graphics textures of the material.
 * @param pResult Receives the memory requirements of the textures.
 * @param pDevice Device the textures are created on.
 * @param rBuildArgSet Build arguments.
 */
void Material::InitializeDynamicRenderingTexture(BuildResultInformation* pResult,
                                                 gfx::Device* pDevice,
                                                 const BuildArgSet& rBuildArgSet) {
    const int texMapNum = GetTexMapNum();
    TexMap* pTexMaps = GetTexMapAry();
    for (int i = 0; i < texMapNum; i++) {
        TexMap* pTexMap = &pTexMaps[i];
        if (pTexMap->mIsDynamicTextureOwner) {
            const TextureInfo* pTextureInfo = pTexMap->m_pTextureInfo;
            CaptureTexture* pCaptureTexture =
                DynamicCast<CaptureTexture*>(static_cast<detail::DynamicRenderingTexture*>(
                    pTextureInfo->GetPrivateTextureInstancePtr()));
            detail::VectorGraphicsTexture* pVectorGraphicsTexture =
                DynamicCast<detail::VectorGraphicsTexture*>(
                    static_cast<detail::DynamicRenderingTexture*>(
                        pTextureInfo->GetPrivateTextureInstancePtr()));
            detail::DynamicTextureShareInfo* pShareInfo =
                rBuildArgSet.pBuildPaneTreeContext->GetUpperOffsetedShareInfo(
                    pTexMap->mShareInfoStackOffset);
            if (pCaptureTexture != nullptr) {
                pShareInfo->SetupCaptureTextureInitializeResource(pCaptureTexture->GetName());
            } else if (pVectorGraphicsTexture != nullptr) {
                pShareInfo->InitializeVectorGraphicsTexture(
                    pResult, pDevice, rBuildArgSet.m_pPartsLayout,
                    rBuildArgSet.m_pPartsLayout->GetResourceAccessor(),
                    pVectorGraphicsTexture->GetName());
            }
        }
    }
}

/**
 * @brief Uses the shader chosen by the application.
 * @param pDevice Device the shader is created on.
 * @param rBuildArgSet Build arguments.
 * @param pBuildResSet Resources of the layout.
 * @return Whether the application chose a shader.
 */
bool Material::TryToSetupUserShader(gfx::Device* pDevice, const BuildArgSet& rBuildArgSet,
                                    const BuildResSet* pBuildResSet) {
    UserShaderInformation info;
    info.SetDefault();

    GetUserShaderInformationFromUserDataCallback pCallback =
        reinterpret_cast<GetUserShaderInformationFromUserDataCallback>(rBuildArgSet._B0);
    if (pCallback == nullptr || !pCallback(info, rBuildArgSet.pExtUserDataList, rBuildArgSet._B8)) {
        return false;
    }

    SetupUserShaderConstantBufferInformation(info);

    char name[sizeof(info.userShaderName)];
    strcpy(name, info.userShaderName);
    m_pShaderInfo = pBuildResSet->pLayout->AcquireArchiveShader(pDevice, name);
    mShaderVariation = 0;
    m_ShaderId = ShaderId_Archive;
    return true;
}

/**
 * @brief Uses the archive shader of the detailed combiner or of the combiner user shader.
 * @param pDevice Device the shader is created on.
 * @param pBuildResSet Resources of the layout.
 * @return Whether the material uses one of the two.
 */
bool Material::TryToSetupDetailedCombinerOrCombinerUserShader(gfx::Device* pDevice,
                                                              const BuildResSet* pBuildResSet) {
    if (!m_MemNum.detailedCombinerParameter && !m_MemNum.combinerUserShaderParameter) {
        return false;
    }

    u32 keys[ShaderKeyMax] = {};
    u32 signature;
    if (m_MemNum.detailedCombinerParameter) {
        signature = ArchiveShaderSignatureDetailedCombiner;
        const int stageNum = m_MemNum.tevStage;
        const ResDetailedCombinerStage* pStages = GetDetailedCombinerStageAry();
        for (int i = 0; i < stageNum; i++) {
            keys[i * 4 + 0] = pStages[i].bits[0];
            keys[i * 4 + 1] = pStages[i].bits[1];
            keys[i * 4 + 2] = pStages[i].bits[2];
            keys[i * 4 + 3] = pStages[i].bits[3];
        }
    } else {
        signature = ArchiveShaderSignatureCombinerUserShader;
        memcpy(keys, GetCombinerUserShaderPtr()->keys, sizeof(keys));
    }

    m_pShaderInfo = pBuildResSet->pLayout->AcquireArchiveShader(pDevice, signature, ShaderKeyMax, keys);
    int variation =
        SearchShaderVariationIndexFromTable(m_pShaderInfo->m_pVariationTable, signature, ShaderKeyMax, keys);
    m_ShaderId = ShaderId_Archive;
    mShaderVariation = variation;
    return true;
}

/**
 * @brief Uses a built-in shader when the texture combination has one.
 * @param pKeyCount Receives the number of archive shader keys.
 * @param pKeys Receives the archive shader keys.
 * @param rBuildArgSet Build arguments.
 * @return Whether a built-in shader is used.
 */
bool Material::TryToSetupSharedShader(size_t* pKeyCount, u32* pKeys,
                                      const BuildArgSet& rBuildArgSet) {
    const int texMapNum = m_MemNum.texMap;
    bool isArchiveShaderRequired = false;
    u32 firstBlend = 0;
    u32 secondBlend = 0;
    u32 flags = 0;
    switch (texMapNum) {
    case 0:
        m_ShaderId = ShaderId_NoTexture;
        break;
    case 1:
        m_ShaderId = ShaderId_SingleTexture;
        break;
    case 2:
        if (IsIndirectBlendUsed()) {
            m_ShaderId = ShaderId_IndirectBlend;
        } else {
            m_ShaderId = ShaderId_DoubleTextureBase + GetTevStageAry()[0].combineRgb;
        }

        break;
    case 3: {
        const ResTevStage* pTevStages = GetTevStageAry();
        firstBlend = pTevStages[0].combineRgb;
        secondBlend = pTevStages[1].combineRgb;
        isArchiveShaderRequired = true;
        break;
    }
    }

    if (IsPerspectiveTextureProjectionUsed()) {
        isArchiveShaderRequired = true;
        flags |= 2;
    }

    if (IsBrickRepeatTextureUsed()) {
        isArchiveShaderRequired = true;
        flags |= 8;
    }

    const u32 proceduralShapeFlags = GetProceduralShapeVariationBitFlags(rBuildArgSet);
    if (proceduralShapeFlags != 0) {
        isArchiveShaderRequired = true;
        flags |= 4;
    }

    if (flags != 0 || proceduralShapeFlags != 0) {
        if (texMapNum == 3) {
            pKeys[0] = firstBlend;
            pKeys[1] = secondBlend;
            pKeys[2] = flags;
            *pKeyCount = 3;
        } else {
            pKeys[0] = m_ShaderId;
            pKeys[1] = 0;
            pKeys[2] = flags | 1;
            *pKeyCount = 3;
        }

        if (proceduralShapeFlags != 0) {
            pKeys[3] = proceduralShapeFlags;
            (*pKeyCount)++;
        }
    } else {
        pKeys[0] = firstBlend;
        pKeys[1] = secondBlend;
        *pKeyCount = 2;
    }

    return !isArchiveShaderRequired;
}

/**
 * @brief Uses an archive shader.
 * @param pDevice Device the shader is created on.
 * @param pBuildResSet Resources of the layout.
 * @param keyCount Number of keys in pKeys.
 * @param pKeys Keys of the shader variation.
 */
void Material::SetupArchiveShader(gfx::Device* pDevice, const BuildResSet* pBuildResSet,
                                  size_t keyCount, const u32* pKeys) {
    m_pShaderInfo = pBuildResSet->pLayout->AcquireArchiveShader(pDevice, ArchiveShaderSignatureNormal,
                                                                keyCount, pKeys);
    int variation = SearchShaderVariationIndexFromTable(m_pShaderInfo->m_pVariationTable,
                                                        ArchiveShaderSignatureNormal, keyCount, pKeys);
    m_ShaderId = ShaderId_Archive;
    mShaderVariation = variation;
}

/** @return Whether a texture uses brick repeat texture coordinates. */
bool Material::IsBrickRepeatTextureUsed() const {
    const int texMapNum = m_MemNum.texMap;
    const ResTexCoordGen* pTexCoordGens = GetTexCoordGenAry();
    for (int i = 0; i < texMapNum; i++) {
        if (pTexCoordGens[i].texGenSrc == TexGenSrc_BrickRepeat) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Copies the constant buffer extension sizes chosen by the application.
 * @param rInfo Shader information of the application.
 */
void Material::SetupUserShaderConstantBufferInformation(const UserShaderInformation& rInfo) {
    mUserShaderConstantBufferInformation = static_cast<UserShaderConstantBufferInformation*>(
        Layout::AllocateMemory(sizeof(UserShaderConstantBufferInformation)));
    memset(mUserShaderConstantBufferInformation, 0, sizeof(UserShaderConstantBufferInformation));
    mUserShaderConstantBufferInformation->vertexSize = rInfo.vertexShaderConstantBufferExtendSize;
    mUserShaderConstantBufferInformation->pixelSize = rInfo.pixelShaderConstantBufferExtendSize;
    mUserShaderConstantBufferInformation->geometrySize = rInfo.geometryShaderConstantBufferExtendSize;
}

/** @return Whether a combiner stage uses an indirect blend. */
bool Material::IsIndirectBlendUsed() const {
    const int tevStageNum = m_MemNum.tevStage;
    const ResTevStage* pTevStages = GetTevStageAry();
    for (int i = 0; i < tevStageNum; i++) {
        if (pTevStages[i].IsIndirectBlend()) {
            return true;
        }
    }

    return false;
}

/** @return Whether a texture uses perspective projection. */
bool Material::IsPerspectiveTextureProjectionUsed() const {
    const int texMapNum = m_MemNum.texMap;
    const ResTexCoordGen* pTexCoordGens = GetTexCoordGenAry();
    for (int i = 0; i < texMapNum; i++) {
        if (pTexCoordGens[i].IsPerspectiveProjection()) {
            return true;
        }
    }

    return false;
}

/**
 * @param rBuildArgSet Build arguments holding the extended user data.
 * @return Shader variation flags of the procedural shape of the pane.
 */
u32 Material::GetProceduralShapeVariationBitFlags(const BuildArgSet& rBuildArgSet) const {
    if (rBuildArgSet.pExtUserDataList == nullptr && rBuildArgSet.pOverrideExtUserDataList == nullptr) {
        return 0;
    }

    const ResExtUserDataList* pList = rBuildArgSet.pOverrideExtUserDataList != nullptr
                                          ? rBuildArgSet.pOverrideExtUserDataList
                                          : rBuildArgSet.pExtUserDataList;
    const ResExtUserData* pData = &pList->entries[0];
    if (pData->type != ExtUserDataType_SystemData) {
        return 0;
    }

    const ResSystemExtUserData* pSystemData =
        static_cast<const ResSystemExtUserData*>(pData->GetData());
    u32 flags = 0;
    for (int i = 0; i < pSystemData->count; i++) {
        const ResSystemDataProceduralShape* pShape =
            reinterpret_cast<const ResSystemDataProceduralShape*>(
                reinterpret_cast<const u8*>(pSystemData) + pSystemData->offsets[i]);
        if (pShape->type != SystemDataType_ProceduralShape) {
            continue;
        }

        const u8 shapeFlags = pShape->flags;
        u32 shapeVariation;
        if (shapeFlags & 1) {
            shapeVariation = flags | (pShape->innerStrokeType << 6) | 3;
        } else {
            shapeVariation = flags | 1;
        }

        if (shapeFlags & 8) {
            shapeVariation |= (pShape->outerStrokeType << 8) | 4;
        }

        if (shapeFlags & 4) {
            shapeVariation |= (pShape->shadowType << 10) | 8;
        }

        flags = shapeVariation | ((shapeFlags << 3) & 0x10) | (((shapeFlags >> 4) & 1) << 5);
    }

    return flags;
}

/**
 * @param variation Variation of the shader id.
 * @param baseVariation Base variation of the archive shader.
 * @return Variation of the archive shader.
 */
int Material::GetArchiveShaderVariation(int variation, int baseVariation) const {
    return variation / 3 * 3 + baseVariation;
}

/**
 * @brief Copies the constant buffer extension sizes of another material.
 * @param rSrc Material to copy from.
 */
void Material::CopyUserShaderConstantBufferInformation(const Material& rSrc) {
    if (rSrc.mUserShaderConstantBufferInformation != nullptr) {
        mUserShaderConstantBufferInformation = static_cast<UserShaderConstantBufferInformation*>(
            Layout::AllocateMemory(sizeof(UserShaderConstantBufferInformation)));
        *mUserShaderConstantBufferInformation = *rSrc.mUserShaderConstantBufferInformation;
        mUserShaderConstantBufferInformation->flags = 0;
    } else {
        mUserShaderConstantBufferInformation = nullptr;
    }
}

/** @return Size of the vertex shader constant buffer. */
size_t Material::GetVertexShaderConstantBufferSize() const {
    return mUserShaderConstantBufferInformation != nullptr
               ? sizeof(ConstantBufferForVertexShader) + mUserShaderConstantBufferInformation->vertexSize
               : sizeof(ConstantBufferForVertexShader);
}

/** @return Size of the pixel shader constant buffer. */
size_t Material::GetPixelShaderConstantBufferSize() const {
    return mUserShaderConstantBufferInformation != nullptr
               ? sizeof(ConstantBufferForPixelShader) + mUserShaderConstantBufferInformation->pixelSize
               : sizeof(ConstantBufferForPixelShader);
}

/** @return Size of the pixel shader constant buffer of the detailed combiner. */
size_t Material::GetPixelShaderDetailedCombinerConstantBufferSize() const {
    return sizeof(ConstantBufferForDetailedCombinerPixelShader);
}

/** @return Size of the pixel shader constant buffer of the combiner user shader. */
size_t Material::GetPixelShaderCombinerUserShaderConstantBufferSize() const {
    return sizeof(ConstantBufferForCombinerUserShaderPixelShader);
}

/** @return Size of the geometry shader constant buffer. */
size_t Material::GetGeometryShaderConstantBufferSize() const {
    return mUserShaderConstantBufferInformation != nullptr
               ? mUserShaderConstantBufferInformation->geometrySize
               : 0;
}

/** @return Preset blend state matching the blend modes of the material. */
int Material::GetBlendStateId() const {
    if (m_MemCap.blendMode == 0) {
        return GraphicsResource::DefalutPresetBlendStateId;
    }

    return GraphicsResource::GetPresetBlendStateId(GetBlendMode(), GetBlendModeAlpha());
}

/**
 * @brief Destroys the blend state created by the material.
 * @param pDevice Device the blend state was created on.
 */
void Material::FinalizeBlendInformationImpl(gfx::Device* pDevice) {
    if (m_pBlendState == nullptr) {
        return;
    }

    if (GetBlendStateId() == PresetBlendStateId_None) {
        void* pMemory = m_pBlendState->GetMemory();
        m_pBlendState->Finalize(pDevice);
        Layout::FreeMemory(pMemory);
        Layout::DeleteObj(m_pBlendState);
    }

    m_pBlendState = nullptr;
}

/**
 * @brief Uses a preset blend state when the material did not create one.
 * @param pDrawInfo Draw state holding the graphics resource.
 */
void Material::SetupBlendState(const DrawInfo* pDrawInfo) {
    if (m_pBlendState != nullptr) {
        return;
    }

    m_pBlendState = const_cast<GraphicsResource*>(pDrawInfo->m_pGraphicsResource)
                        ->GetPresetBlendState(static_cast<PresetBlendStateId>(GetBlendStateId()));
}

/**
 * @brief Copies a material.
 * @param rSrc Material to copy.
 * @param pDevice Device the resources of the copy are created on.
 */
Material::Material(const Material& rSrc, gfx::Device* pDevice) {
    MaterialCopyContext context;
    context.pDevice = pDevice;
    context.pResourceAccessor = nullptr;
    context.pBuildPaneTreeContext = nullptr;
    CopyMaterialImpl(rSrc, context);
}

/**
 * @brief Copies the parameters of another material into this one.
 * @param rSrc Material to copy.
 * @param rContext Objects needed to rebuild the resources of the copy.
 */
void Material::CopyMaterialImpl(const Material& rSrc, MaterialCopyContext& rContext) {
    Initialize();

    m_pShaderInfo = rSrc.m_pShaderInfo;
    m_DrawTextureNum = rSrc.m_DrawTextureNum;
    m_ShaderId = rSrc.m_ShaderId;
    mOwnershipFlags = rSrc.mOwnershipFlags;
    mShaderVariation = rSrc.mShaderVariation;
    mName = rSrc.mName;
    CopyUserShaderConstantBufferInformation(rSrc);

    if (rSrc.m_IsFloatColorAllocated) {
        m_pFloatColors = static_cast<util::Float4*>(
            Layout::AllocateMemory(sizeof(util::Float4) * MaterialColor_Max));
        m_pFloatColors[MaterialColor_Black] = rSrc.m_pFloatColors[MaterialColor_Black];
        m_pFloatColors[MaterialColor_White] = rSrc.m_pFloatColors[MaterialColor_White];
    } else {
        m_ByteColors[MaterialColor_Black] = rSrc.m_ByteColors[MaterialColor_Black];
        m_ByteColors[MaterialColor_White] = rSrc.m_ByteColors[MaterialColor_White];
    }

    InitializeMatMemCount(&m_MemCap);
    InitializeMatMemCount(&m_MemNum);
    ReserveMem(rSrc.m_MemCap.texMap, rSrc.m_MemCap.texSrt, rSrc.m_MemCap.texCoordGen,
               rSrc.m_MemCap.tevStage, rSrc.m_MemCap.alpComp, rSrc.m_MemCap.blendMode,
               rSrc.m_MemCap.indirectParameter, rSrc.m_MemCap.projectionTexGen,
               rSrc.m_MemCap.fontShadowParameter, rSrc.m_MemCap.detailedCombinerParameter,
               rSrc.m_MemCap.combinerUserShaderParameter, rSrc.m_MemCap.vectorGraphicsTexture,
               rSrc.m_MemCap.brickRepeatParameter);

    if (m_pMem != nullptr) {
        m_MemNum.texMap = rSrc.m_MemNum.texMap;
        m_MemNum.texSrt = rSrc.m_MemNum.texSrt;
        m_MemNum.texCoordGen = rSrc.m_MemNum.texCoordGen;
        m_MemNum.tevStage = rSrc.m_MemNum.tevStage;
        m_MemNum.projectionTexGen = rSrc.m_MemNum.projectionTexGen;
        m_MemNum.fontShadowParameter = rSrc.m_MemNum.fontShadowParameter;
        m_MemNum.vectorGraphicsTexture = rSrc.m_MemNum.vectorGraphicsTexture;
        m_MemNum.brickRepeatParameter = rSrc.m_MemNum.brickRepeatParameter;
        memcpy(m_pMem, rSrc.m_pMem,
               CalculateReserveMemSize(m_MemNum.texMap, m_MemNum.texSrt, m_MemNum.texCoordGen,
                                       m_MemNum.tevStage, m_MemNum.alpComp, m_MemNum.blendMode,
                                       m_MemNum.indirectParameter, m_MemNum.projectionTexGen,
                                       m_MemNum.fontShadowParameter,
                                       m_MemNum.detailedCombinerParameter,
                                       m_MemNum.combinerUserShaderParameter,
                                       m_MemNum.vectorGraphicsTexture,
                                       m_MemNum.brickRepeatParameter));
    }

    CopyDynamicRenderingTexture(rSrc, rContext);
    InitializeBlendInformationImpl(rContext.pDevice);
}

/**
 * @brief Copies a material.
 * @param rSrc Material to copy.
 * @param rContext Objects needed to rebuild the resources of the copy.
 */
Material::Material(const Material& rSrc, MaterialCopyContext& rContext) {
    CopyMaterialImpl(rSrc, rContext);
}

/**
 * @brief Clears a parameter count.
 * @param pCount Count to clear.
 */
void Material::InitializeMatMemCount(detail::MatMemCount* pCount) const {
    pCount->texMap = 0;
    pCount->texSrt = 0;
    pCount->texCoordGen = 0;
    pCount->tevStage = 0;
    pCount->alpComp = 0;
    pCount->blendMode = 0;
    pCount->indirectParameter = 0;
    pCount->projectionTexGen = 0;
    pCount->fontShadowParameter = 0;
    pCount->detailedCombinerParameter = 0;
    pCount->combinerUserShaderParameter = 0;
    pCount->vectorGraphicsTexture = 0;
    pCount->brickRepeatParameter = 0;
}

/**
 * @brief Copies the capture and vector graphics textures of another material.
 * @param rSrc Material to copy.
 * @param rContext Objects needed to rebuild the textures.
 */
void Material::CopyDynamicRenderingTexture(const Material& rSrc, MaterialCopyContext& rContext) {
    const int texMapNum = m_MemNum.texMap;
    const TexMap* pSrcTexMaps = rSrc.GetTexMapAry();
    TexMap* pTexMaps = GetTexMapAry();
    u8 vectorGraphicsIdx = 0;
    for (int i = 0; i < texMapNum; i++) {
        const TexMap* pSrcTexMap = &pSrcTexMaps[i];
        const TextureInfo* pTextureInfo = pSrcTexMap->m_pTextureInfo;
        if (DynamicCast<const DummyRenderTargetTextureInfo*>(pTextureInfo) != nullptr) {
            pTexMaps[i].mIsDummyTextureInfo = 1;
            pTexMaps[i].mIsDynamicTextureOwner = 0;
            pTexMaps[i].mShareInfoStackOffset = 0;
            pTexMaps[i].m_pTextureInfo = new (Layout::AllocateMemory(
                sizeof(DummyRenderTargetTextureInfo), 16))
                DummyRenderTargetTextureInfo(
                    static_cast<const DummyRenderTargetTextureInfo*>(pTextureInfo)->mName);
            continue;
        }

        if (pTextureInfo->GetPrivateTextureInstancePtr() == nullptr) {
            continue;
        }

        const CaptureTexture* pCaptureTexture = DynamicCast<const CaptureTexture*>(
            static_cast<const detail::DynamicRenderingTexture*>(
                pTextureInfo->GetPrivateTextureInstancePtr()));
        const detail::VectorGraphicsTexture* pVectorGraphicsTexture =
            DynamicCast<const detail::VectorGraphicsTexture*>(
                static_cast<const detail::DynamicRenderingTexture*>(
                    pTextureInfo->GetPrivateTextureInstancePtr()));
        detail::DynamicTextureShareInfo* pShareInfo =
            rContext.pBuildPaneTreeContext->GetUpperOffsetedShareInfo(
                pSrcTexMap->mShareInfoStackOffset);
        bool isCreated = false;
        detail::DynamicRenderingTexture* pTexture;
        if (pCaptureTexture != nullptr) {
            pTexture = pShareInfo->CopyCaptureTexture(&isCreated, pCaptureTexture);
        } else if (pVectorGraphicsTexture != nullptr) {
            detail::VectorGraphicsTexture* pCopied = pShareInfo->CopyVectorGraphicsTexture(
                rContext.pDevice, rContext.pLayout, pVectorGraphicsTexture);
            isCreated = true;
            detail::RefVectorGraphicsTextureInfo* pRefInfo =
                &GetVectorGraphicsTextureRefInfoAry()[vectorGraphicsIdx];
            *pRefInfo = rSrc.GetVectorGraphicsTextureRefInfoAry()[vectorGraphicsIdx];
            pRefInfo->pTexture = pCopied;
            pCopied->SetColor(pRefInfo->color);
            vectorGraphicsIdx++;
            pTexture = pCopied;
        } else {
            pTexture = nullptr;
        }

        pTexMaps[i].mIsDynamicTextureOwner = isCreated;
        pTexMaps[i].mShareInfoStackOffset = pSrcTexMap->mShareInfoStackOffset;
        pTexMaps[i].m_pTextureInfo = static_cast<const TextureInfo*>(pTexture->_08);
    }
}

/** @brief Destroys the material. */
Material::~Material() {}

/**
 * @brief Releases the textures of the material.
 * @param pDevice Device the textures were created on.
 */
void Material::FinalizeTexMap(gfx::Device* pDevice) {
    for (int i = 0; i < m_MemCap.texMap; i++) {
        TexMap* pTexMap = &GetTexMapAry()[i];
        if (pTexMap->mIsDynamicTextureOwner) {
            TextureInfo* pTextureInfo = static_cast<TextureInfo*>(
                pTexMap->m_pTextureInfo->GetPrivateTextureInstancePtr());
            if (pTextureInfo != nullptr) {
                pTextureInfo->Finalize(pDevice);
                pTextureInfo->~TextureInfo();
                Layout::FreeMemory(pTextureInfo);
            }
        } else if (pTexMap->mIsDummyTextureInfo) {
            DummyRenderTargetTextureInfo* pTextureInfo = DynamicCast<DummyRenderTargetTextureInfo*>(
                const_cast<TextureInfo*>(pTexMap->m_pTextureInfo));
            if (pTextureInfo != nullptr) {
                pTextureInfo->Finalize(pDevice);
                pTextureInfo->~DummyRenderTargetTextureInfo();
                Layout::FreeMemory(pTextureInfo);
            }
        }

        GetTexMapAry()[i].Finalize();
        GetTexMapAry()[i].~TexMap();
    }
}

/**
 * @brief Releases the resources of the material.
 * @param pDevice Device the resources were created on.
 */
void Material::Finalize(gfx::Device* pDevice) {
    FinalizeBlendInformationImpl(pDevice);

    mUserShader = nullptr;
    if (mUserShaderConstantBufferInformation != nullptr) {
        Layout::FreeMemory(mUserShaderConstantBufferInformation);
        mUserShaderConstantBufferInformation = nullptr;
    }

    FinalizeTexMap(pDevice);

    if (m_pMem != nullptr) {
        Layout::FreeMemory(m_pMem);
        m_pMem = nullptr;
    }

    if (m_IsFloatColorAllocated) {
        Layout::FreeMemory(m_pFloatColors);
        m_pFloatColors = nullptr;
    }
}

/**
 * @return Size of the parameter memory for the given counts.
 * @param texMapNum Number of texture maps.
 * @param texSrtNum Number of texture SRT parameters.
 * @param texCoordGenNum Number of texture coordinate generators.
 * @param tevStageNum Number of combiner stages.
 * @param alpCompNum Number of alpha compare settings.
 * @param blendModeNum Number of blend modes.
 * @param indirectParameterNum Number of indirect parameters.
 * @param projectionTexGenNum Number of projection texture generation parameters.
 * @param fontShadowParameterNum Number of font shadow parameters.
 * @param detailedCombinerNum Number of detailed combiner settings.
 * @param combinerUserShaderNum Number of combiner user shader settings.
 * @param vectorGraphicsTextureNum Number of vector graphics texture references.
 * @param brickRepeatNum Number of brick repeat texture generation parameters.
 */
size_t Material::CalculateReserveMemSize(int texMapNum, int texSrtNum, int texCoordGenNum,
                                         int tevStageNum, int alpCompNum, int blendModeNum,
                                         int indirectParameterNum, int projectionTexGenNum,
                                         int fontShadowParameterNum, int detailedCombinerNum,
                                         int combinerUserShaderNum, int vectorGraphicsTextureNum,
                                         int brickRepeatNum) {
    return sizeof(TexMap) * texMapNum + sizeof(ResTexSrt) * texSrtNum +
           sizeof(ResTexCoordGen) * texCoordGenNum + sizeof(ResTevStage) * tevStageNum +
           sizeof(ResAlphaCompare) * alpCompNum + sizeof(ResBlendMode) * blendModeNum +
           sizeof(ResIndirectParameter) * indirectParameterNum +
           sizeof(ResProjectionTexGenParameters) * projectionTexGenNum +
           sizeof(ResFontShadowParameter) * fontShadowParameterNum +
           sizeof(ResDetailedCombinerStageInfo) * detailedCombinerNum +
           sizeof(ResDetailedCombinerStage) * detailedCombinerNum * tevStageNum +
           sizeof(ResCombinerUserShader) * combinerUserShaderNum +
           sizeof(detail::RefVectorGraphicsTextureInfo) * vectorGraphicsTextureNum +
           sizeof(ResBrickRepeatTexGenParameters) * brickRepeatNum +
           (brickRepeatNum > 0 ? sizeof(BrickRepeatShaderInfo) : 0);
}

/**
 * @brief Records the counts of the parameter memory and fills it with defaults.
 * @param texMapNum Number of texture maps.
 * @param texSrtNum Number of texture SRT parameters.
 * @param texCoordGenNum Number of texture coordinate generators.
 * @param tevStageNum Number of combiner stages.
 * @param alpCompNum Number of alpha compare settings.
 * @param blendModeNum Number of blend modes.
 * @param indirectParameterNum Number of indirect parameters.
 * @param projectionTexGenNum Number of projection texture generation parameters.
 * @param fontShadowParameterNum Number of font shadow parameters.
 * @param detailedCombinerNum Number of detailed combiner settings.
 * @param combinerUserShaderNum Number of combiner user shader settings.
 * @param vectorGraphicsTextureNum Number of vector graphics texture references.
 * @param brickRepeatNum Number of brick repeat texture generation parameters.
 */
void Material::ResParameterToMemory(int texMapNum, int texSrtNum, int texCoordGenNum,
                                    int tevStageNum, int alpCompNum, int blendModeNum,
                                    int indirectParameterNum, int projectionTexGenNum,
                                    int fontShadowParameterNum, int detailedCombinerNum,
                                    int combinerUserShaderNum, int vectorGraphicsTextureNum,
                                    int brickRepeatNum) {
    m_MemCap.texMap = texMapNum;
    m_MemCap.texSrt = texSrtNum;
    m_MemCap.texCoordGen = texCoordGenNum;
    m_MemCap.tevStage = tevStageNum;
    m_MemCap.alpComp = alpCompNum;
    m_MemCap.blendMode = blendModeNum;
    m_MemCap.indirectParameter = indirectParameterNum;
    m_MemCap.projectionTexGen = projectionTexGenNum;
    m_MemCap.fontShadowParameter = fontShadowParameterNum;
    m_MemCap.detailedCombinerParameter = detailedCombinerNum;
    m_MemCap.combinerUserShaderParameter = combinerUserShaderNum;
    m_MemCap.vectorGraphicsTexture = vectorGraphicsTextureNum;
    m_MemCap.brickRepeatParameter = brickRepeatNum;

    m_MemNum.texSrt = texSrtNum;
    ResTexSrt* pTexSrts = GetTexSrtAry();
    const int num = GetTexSrtNum();
    for (int i = 0; i < num; i++) {
        pTexSrts[i].translate.x = 0.0f;
        pTexSrts[i].translate.y = 0.0f;
        pTexSrts[i].rotate = 0.0f;
        pTexSrts[i].scale.x = 1.0f;
        pTexSrts[i].scale.y = 1.0f;
    }

    m_MemNum.alpComp = m_MemCap.alpComp;
    if (m_MemCap.alpComp) {
        *GetAlphaComparePtr() = ResAlphaCompare(AlphaTest_Always, 0.0f);
    }

    m_MemNum.blendMode = m_MemCap.blendMode;
    if (m_MemCap.blendMode) {
        GetBlendModeAry()[0].Set(BlendOp_Disable, BlendFactor_SrcAlpha, BlendFactor_InvSrcAlpha,
                                 LogicOp_Disable);
        if (m_MemNum.blendMode == 2) {
            GetBlendModeAry()[1].Set(BlendOp_Disable, BlendFactor_SrcAlpha,
                                     BlendFactor_InvSrcAlpha, LogicOp_Disable);
        }
    }

    m_MemNum.indirectParameter = m_MemCap.indirectParameter;
    if (m_MemCap.indirectParameter) {
        ResIndirectParameter* pParameter = GetIndirectParameterPtr();
        pParameter->rotate = 0.0f;
        pParameter->scale.x = 1.0f;
        pParameter->scale.y = 1.0f;
    }

    m_MemNum.fontShadowParameter = m_MemCap.fontShadowParameter;
    if (m_MemCap.fontShadowParameter) {
        ResFontShadowParameter* pParameter = GetFontShadowParameterPtr();
        pParameter->blackInterporateColor[0] = 0;
        pParameter->blackInterporateColor[1] = 0;
        pParameter->blackInterporateColor[2] = 0;
        pParameter->whiteInterporateColor[0] = 0xff;
        pParameter->whiteInterporateColor[1] = 0xff;
        pParameter->whiteInterporateColor[2] = 0xff;
        pParameter->whiteInterporateColor[3] = 0xff;
        pParameter->reserve = 0;
    }

    m_MemNum.detailedCombinerParameter = m_MemCap.detailedCombinerParameter;
    if (m_MemCap.detailedCombinerParameter) {
        memset(GetDetailedCombinerStageInfoPtr(), 0, sizeof(ResDetailedCombinerStageInfo));
        for (int i = 0; i < tevStageNum; i++) {
            memset(&GetDetailedCombinerStageAry()[i], 0, sizeof(ResDetailedCombinerStage));
        }
    }

    m_MemNum.combinerUserShaderParameter = m_MemCap.combinerUserShaderParameter;
    if (m_MemCap.combinerUserShaderParameter) {
        memset(GetCombinerUserShaderPtr(), 0, sizeof(ResCombinerUserShader));
    }

    m_MemNum.vectorGraphicsTexture = m_MemCap.vectorGraphicsTexture;
    if (m_MemCap.vectorGraphicsTexture) {
        for (int i = 0; i < vectorGraphicsTextureNum; i++) {
            GetVectorGraphicsTextureRefInfoAry()[i].time = 0.0f;
            memset(&GetVectorGraphicsTextureRefInfoAry()[i].color, 0, sizeof(util::Float4));
            GetVectorGraphicsTextureRefInfoAry()[i].pTexture = nullptr;
        }
    }

    m_MemNum.brickRepeatParameter = m_MemCap.brickRepeatParameter;
    if (m_MemCap.brickRepeatParameter) {
        GetBrickRepeatShaderInfoPtr()->constantBufferOffset = 0;
        GetBrickRepeatShaderInfoPtr()->slot = 0;
        for (int i = 0; i < brickRepeatNum; i++) {
            memset(&GetBrickRepeatTexGenParametersAry()[i], 0,
                   sizeof(ResBrickRepeatTexGenParameters));
        }
    }
}

/**
 * @brief Binds this material to an animation.
 * @param pAnimTrans Animation that animates the material.
 */
void Material::BindAnimation(AnimTransform* pAnimTrans) {
    pAnimTrans->BindMaterial(this);
}

/**
 * @brief Unbinds this material from an animation.
 * @param pAnimTrans Animation that animated the material.
 */
void Material::UnbindAnimation(AnimTransform* pAnimTrans) {
    pAnimTrans->UnbindMaterial(this);
}

/**
 * @param texCoordGenIdx Index of a texture coordinate generator.
 * @return Index of the projection texture generation parameter it uses.
 */
int Material::GetProjectionTexGenParametersIdxFromTexCoordGenIdx(int texCoordGenIdx) const {
    const int num = m_MemNum.texCoordGen < m_MemNum.texMap ? m_MemNum.texCoordGen : m_MemNum.texMap;
    int idx = 0;
    const ResTexCoordGen* pTexCoordGens = GetTexCoordGenAry();
    for (int i = 0; i < num; i++) {
        if (pTexCoordGens[i].IsProjection()) {
            if (i == texCoordGenIdx) {
                return idx;
            }

            idx++;
        }
    }

    return 0;
}

/**
 * @param texCoordGenIdx Index of a texture coordinate generator.
 * @return Index of the brick repeat texture generation parameter it uses.
 */
int Material::GetBrickRepeatTexGenParametersIdxFromTexCoordGenIdx(int texCoordGenIdx) const {
    const int num = m_MemNum.texCoordGen < m_MemNum.texMap ? m_MemNum.texCoordGen : m_MemNum.texMap;
    int idx = 0;
    const ResTexCoordGen* pTexCoordGens = GetTexCoordGenAry();
    for (int i = 0; i < num; i++) {
        if (pTexCoordGens[i].texGenSrc == TexGenSrc_BrickRepeat) {
            if (i == texCoordGenIdx) {
                return idx;
            }

            idx++;
        }
    }

    return 0;
}

/**
 * @brief Calculates a texture matrix.
 * @param pMtx Receives the matrix.
 * @param rTexSrt Scale, rotation and translation of the texture.
 * @param rTexMap Texture.
 */
void Material::CalculateTextureMtx(float (*pMtx)[3], const ResTexSrt& rTexSrt,
                                   const TexMap& rTexMap) {
    float sinR;
    float cosR;
    SinCosTable(&sinR, &cosR, DegreeToAngleIndex(rTexSrt.rotate));

    const float a0 = rTexSrt.scale.x * cosR;
    const float a1 = rTexSrt.scale.y * sinR;
    pMtx[0][0] = a0;
    pMtx[0][1] = -a1;
    pMtx[0][2] = rTexSrt.translate.x + 0.5f - a0 * 0.5f + a1 * 0.5f;

    const float b0 = rTexSrt.scale.x * sinR;
    const float b1 = rTexSrt.scale.y * cosR;
    pMtx[1][0] = b0;
    pMtx[1][1] = b1;
    const float translateY = rTexSrt.translate.y + 0.5f - b0 * 0.5f;
    pMtx[1][0] = -b0;
    pMtx[1][1] = -b1;
    pMtx[1][2] = b1 * 0.5f - translateY + 1.0f;
}

/**
 * @brief Calculates an indirect texture matrix.
 * @param pMtx Receives the matrix.
 * @param rotate Rotation in degrees.
 * @param rScale Scale.
 */
void Material::CalculateIndirectMtx(float (*pMtx)[3], float rotate, const ResVec2& rScale) {
    float sinR;
    float cosR;
    SinCosTable(&sinR, &cosR, DegreeToAngleIndex(rotate));

    pMtx[0][0] = rScale.x * cosR * 2.0f;
    pMtx[0][1] = rScale.y * sinR * -2.0f;
    pMtx[0][2] = pMtx[0][1] * -0.5f - pMtx[0][0] * 0.5f;
    pMtx[1][0] = sinR * rScale.x * 2.0f;
    pMtx[1][1] = cosR * rScale.y * 2.0f;
    pMtx[1][2] = pMtx[1][1] * -0.5f - pMtx[1][0] * 0.5f;
}

/**
 * @brief Prepares the constant buffers of the material for drawing.
 * @param rDrawInfo Draw state.
 * @param alpha Alpha of the pane.
 * @param variation Shader variation.
 * @param bInitializeCDef Whether to reset the vertex colour flag.
 * @param rGlobalMtx Global matrix of the pane.
 * @param pSize Size of the pane.
 * @param pExtUserData Extended user data of the pane.
 * @param extUserDataCount Number of extended user data.
 */
void Material::SetupGraphics(DrawInfo& rDrawInfo, u8 alpha, ShaderVariation variation,
                             bool bInitializeCDef, const util::MatrixT4x3fType& rGlobalMtx,
                             const Size* pSize, const ResExtUserData* pExtUserData,
                             u16 extUserDataCount) {
    const bool isTextureOnly = m_IsTextureOnly;
    if (!isTextureOnly) {
        rDrawInfo.m_TexMapNum = m_MemNum.texMap;
    }

    rDrawInfo.mModelViewLoaded = false;

    if (m_ShaderId == ShaderId_Archive) {
        mShaderVariation = GetArchiveShaderVariation(mShaderVariation, variation);
    } else {
        if (m_pShaderInfo == nullptr) {
            m_pShaderInfo = &rDrawInfo.m_pGraphicsResource->m_CommonShaderInfo;
        }

        mShaderVariation = m_ShaderId + variation * ShaderVariationCountPerShaderId;
    }

    ConstantBufferForVertexShader* pVertexShaderConstantBuffer =
        static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo));
    if (variation == ShaderVariation_WithoutVertexColor) {
        ConstantBufferForVertexShader* pBuffer =
            static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo));
        for (int i = 0; i < 4; i++) {
            pBuffer->vertexColor[i][0] = 255.0f;
            pBuffer->vertexColor[i][1] = 255.0f;
            pBuffer->vertexColor[i][2] = 255.0f;
            pBuffer->vertexColor[i][3] = 255.0f;
        }
    }

    pVertexShaderConstantBuffer->color[0] = 1.0f / 255.0f;
    pVertexShaderConstantBuffer->color[1] = 1.0f / 255.0f;
    pVertexShaderConstantBuffer->color[2] = 1.0f / 255.0f;
    pVertexShaderConstantBuffer->color[3] = alpha * (1.0f / 255.0f / 255.0f);
    rDrawInfo.LoadProjectionMtx(pVertexShaderConstantBuffer->projection);

    if (bInitializeCDef) {
        pVertexShaderConstantBuffer->frameSpec = 0;
    }

    SetupSubmaterialOf_TextureMatrix(rDrawInfo, rGlobalMtx, pSize);
    SetRcpTexSize(rDrawInfo);

    if (!isTextureOnly) {
        SetupSubmaterialOf_Tev(rDrawInfo, pExtUserData, extUserDataCount, pSize);
    }

    if (m_MemNum.vectorGraphicsTexture != 0) {
        detail::RefVectorGraphicsTextureInfo* pRefInfos = GetVectorGraphicsTextureRefInfoAry();
        for (int i = 0; i < m_MemNum.vectorGraphicsTexture; i++) {
            if (pRefInfos[i].pTexture != nullptr) {
                pRefInfos[i].pTexture->SetTime(GetVectorGraphicsTextureRefInfoAry()[i].time);
                pRefInfos[i].pTexture->SetUpdateRequested();
            }
        }
    }

    m_DrawTextureNum = rDrawInfo.m_TexMapNum;
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The vertex shader constants of the material.
 */
void* Material::GetConstantBufferForVertexShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr) {
        return nullptr;
    }

    return pBuffer + m_VertexShaderConstantBufferOffset;
}

/**
 * @brief Sets up the texture matrices of the material.
 * @param rDrawInfo Draw state.
 * @param rGlobalMtx Global matrix of the pane.
 * @param pSize Size of the pane.
 */
void Material::SetupSubmaterialOf_TextureMatrix(DrawInfo& rDrawInfo,
                                                const util::MatrixT4x3fType& rGlobalMtx,
                                                const Size* pSize) {
    if (m_MemNum.texSrt != 0) {
        u32 num = GetTexMapNum() < GetTexSrtNum() ? GetTexMapNum() : GetTexSrtNum();
        if (static_cast<int>(num) > rDrawInfo.m_TexMapNum) {
            num = rDrawInfo.m_TexMapNum;
        }

        for (int i = 0; i < num; i++) {
            float* pRow0;
            float* pRow1;
            if (GetTexCoordGenAry()[i].IsPerspectiveProjection()) {
                ConstantBufferForVertexShader* pBuffer = static_cast<ConstantBufferForVertexShader*>(
                    GetConstantBufferForVertexShader(rDrawInfo));
                switch (i) {
                case 0:
                    pRow0 = pBuffer->texMtx0[0];
                    pRow1 = pBuffer->texMtx0[1];
                    break;
                case 1:
                    pRow0 = pBuffer->texMtx1[0];
                    pRow1 = pBuffer->texMtx1[1];
                    break;
                case 2:
                    pRow0 = pBuffer->texMtx2[0];
                    pRow1 = pBuffer->texMtx2[1];
                    break;
                default:
                    continue;
                }

                pRow0[0] = 1.0f;
                pRow0[1] = 0.0f;
                pRow0[2] = 0.0f;
                pRow0[3] = 0.0f;
                pRow1[0] = 0.0f;
                pRow1[1] = -1.0f;
                pRow1[2] = 0.0f;
                pRow1[3] = 1.0f;
            } else {
                float mtx[2][3];
                CalculateTextureMtx(mtx, GetTexSrtAry()[i], GetTexMapAry()[i]);
                ConstantBufferForVertexShader* pBuffer = static_cast<ConstantBufferForVertexShader*>(
                    GetConstantBufferForVertexShader(rDrawInfo));
                switch (i) {
                case 0:
                    pRow0 = pBuffer->texMtx0[0];
                    pRow1 = pBuffer->texMtx0[1];
                    break;
                case 1:
                    pRow0 = pBuffer->texMtx1[0];
                    pRow1 = pBuffer->texMtx1[1];
                    break;
                case 2:
                    pRow0 = pBuffer->texMtx2[0];
                    pRow1 = pBuffer->texMtx2[1];
                    break;
                default:
                    continue;
                }

                pRow0[0] = mtx[0][0];
                pRow0[1] = mtx[0][1];
                pRow0[2] = 0.0f;
                pRow0[3] = mtx[0][2];
                pRow1[0] = mtx[1][0];
                pRow1[1] = mtx[1][1];
                pRow1[2] = 0.0f;
                pRow1[3] = mtx[1][2];
            }
        }
    }

    int i = 0;
    if (m_MemNum.texCoordGen != 0) {
        for (; i < m_MemNum.texCoordGen; i++) {
            rDrawInfo.m_TexCoordSrc[i] = GetTexCoordGenAry()[i].texGenSrc;
        }

        if (i != TexMapMax) {
            memset(&rDrawInfo.m_TexCoordSrc[i], 0xff, TexMapMax - i);
        }
    } else {
        rDrawInfo.m_TexCoordSrc[0] = 0xff;
    }

    SetupSubmaterialOf_TextureCoordGenerateParams(rDrawInfo, rGlobalMtx, pSize);
}

/**
 * @brief Sets up the reciprocal size of the first texture.
 * @param rDrawInfo Draw state.
 */
void Material::SetRcpTexSize(const DrawInfo& rDrawInfo) const {
    if (m_MemNum.texMap == 0 || rDrawInfo.m_TexMapNum < 1) {
        return;
    }

    ConstantBufferForVertexShader* pBuffer =
        static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo));
    const TextureInfo* pTextureInfo = GetTexMapAry()[0].m_pTextureInfo;
    if (pTextureInfo == nullptr) {
        return;
    }

    const TextureSize size = pTextureInfo->GetSize();
    pBuffer->rcpTexSize0[0] = 1.0f / size.width;
    pBuffer->rcpTexSize0[1] = 1.0f / size.height;
    pBuffer->rcpTexSize0[2] = 0.0f;
    pBuffer->rcpTexSize0[3] = 0.0f;
}

/**
 * @brief Sets up the colour combiner constants of the material.
 * @param rDrawInfo Draw state.
 * @param pExtUserData Extended user data of the pane.
 * @param extUserDataCount Number of extended user data.
 * @param pSize Size of the pane.
 */
void Material::SetupSubmaterialOf_Tev(DrawInfo& rDrawInfo, const ResExtUserData* pExtUserData,
                                      u16 extUserDataCount, const Size* pSize) const {
    if (m_MemNum.detailedCombinerParameter || m_MemNum.combinerUserShaderParameter) {
        if (m_MemNum.detailedCombinerParameter) {
            SetupSubmaterialOf_DetailedCombiner(rDrawInfo);
        } else {
            SetupSubmaterialOf_CombinerUserShader(rDrawInfo, pExtUserData, extUserDataCount,
                                                  pSize);
        }

        return;
    }

    ConstantBufferForPixelShader* pBuffer =
        static_cast<ConstantBufferForPixelShader*>(GetConstantBufferForPixelShader(rDrawInfo));
    if (GetTexMapNum() != 0) {
        const util::Float4 white = GetColorFloat(MaterialColor_White);
        const util::Float4 black = GetColorFloat(MaterialColor_Black);
        pBuffer->interpolateWidth.x = white.x - black.x;
        pBuffer->interpolateWidth.y = white.y - black.y;
        pBuffer->interpolateWidth.z = white.z - black.z;
        pBuffer->interpolateOffset.x = black.x;
        pBuffer->interpolateOffset.y = black.y;
        pBuffer->interpolateOffset.z = black.z;
        const float alphaWidth = white.w - black.w;
        if (m_IsThresholdingAlphaInterpolation) {
            const float width = 1.0f / (alphaWidth + 0.0001f);
            pBuffer->interpolateWidth.w = width;
            pBuffer->interpolateOffset.w = -black.w * width;
        } else {
            pBuffer->interpolateWidth.w = alphaWidth;
            pBuffer->interpolateOffset.w = black.w;
        }
    } else {
        pBuffer->interpolateWidth = GetColorFloat(MaterialColor_White);
        pBuffer->interpolateOffset = util::MakeFloat4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    if (GetTevStageNum() == 0) {
        return;
    }

    const u32 texMapNum = GetTexMapNum();
    if (texMapNum < 2) {
        pBuffer->tevAlphaFlags = texMapNum;
        return;
    }

    u32 alphaFlags = 0;
    const int tevStageNum = GetTevStageNum();
    const ResTevStage* pTevStages = GetTevStageAry();
    for (int i = 0; i < tevStageNum; i++) {
        if (pTevStages[i].combineAlpha == 1) {
            alphaFlags |= 0x80 << (i * 8);
        }
    }

    pBuffer->tevAlphaFlags = alphaFlags;

    if (m_MemCap.indirectParameter) {
        const ResIndirectParameter* pParameter = GetIndirectParameterPtr();
        float mtx[2][3];
        CalculateIndirectMtx(mtx, pParameter->rotate, pParameter->scale);
        pBuffer->indirectMtx0[0] = mtx[0][0];
        pBuffer->indirectMtx0[1] = mtx[0][1];
        pBuffer->indirectMtx0[2] = 0.0f;
        pBuffer->indirectMtx0[3] = mtx[0][2];
        pBuffer->indirectMtx1[0] = mtx[1][0];
        pBuffer->indirectMtx1[1] = mtx[1][1];
        pBuffer->indirectMtx1[2] = 0.0f;
        pBuffer->indirectMtx1[3] = mtx[1][2];
    }
}

/**
 * @brief Sets up the texture coordinate generation constants of the material.
 * @param rDrawInfo Draw state.
 * @param rGlobalMtx Global matrix of the pane.
 * @param pSize Size of the pane.
 */
void Material::SetupSubmaterialOf_TextureCoordGenerateParams(DrawInfo& rDrawInfo,
                                                             const util::MatrixT4x3fType& rGlobalMtx,
                                                             const Size* pSize) const {
    if (m_MemCap.texMap == 0) {
        return;
    }

    const int num = m_MemNum.texCoordGen < m_MemNum.texMap ? m_MemNum.texCoordGen : m_MemNum.texMap;
    const int loopNum = num < TexMapMax ? num : TexMapMax;

    ConstantBufferForPixelShader* pPixelBuffer =
        static_cast<ConstantBufferForPixelShader*>(GetConstantBufferForPixelShader(rDrawInfo));
    pPixelBuffer->reserve84 = 0;

    if (IsBrickRepeatTextureUsed()) {
        memset(GetConstantBufferForBrickRepeatPixelShader(rDrawInfo), 0, BrickRepeatConstantBufferSize);
    }

    int generatingTexCoord[TexMapMax];
    int projectionIdx = 0;
    int brickRepeatIdx = 0;
    for (int i = 0; i < loopNum; i++) {
        const u8 src = GetTexCoordGenAry()[i].texGenSrc;
        generatingTexCoord[i] = src == TexGenSrc_PerspectiveProjection || (src >= TexGenSrc_OrthogonalProjection && src < TexGenSrc_PerspectiveProjection);
        if (generatingTexCoord[i]) {
            SetupSubmaterialOf_TextureProjectionMatrix(rDrawInfo, rGlobalMtx, pSize,
                                                       GetProjectionTexGenAry()[projectionIdx], i);
            projectionIdx++;
        } else if (src == TexGenSrc_BrickRepeat) {
            SetupSubmaterialOf_TextureBrickRepeatParams(
                rDrawInfo, GetBrickRepeatTexGenParametersAry()[brickRepeatIdx], i);
            brickRepeatIdx++;
        }
    }

    if (loopNum < TexMapMax) {
        memset(&generatingTexCoord[num], 0, sizeof(int) * (TexMapMax - num));
    }

    ConstantBufferForVertexShader* pVertexBuffer =
        static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo));
    pVertexBuffer->generatingTexCoord[0] = generatingTexCoord[0];
    pVertexBuffer->generatingTexCoord[1] = generatingTexCoord[1];
    pVertexBuffer->generatingTexCoord[2] = generatingTexCoord[2];
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The pixel shader constants of the material.
 */
void* Material::GetConstantBufferForPixelShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr) {
        return nullptr;
    }

    return pBuffer + m_PixelShaderConstantBufferOffset;
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The pixel shader constants of the brick repeated textures.
 */
void* Material::GetConstantBufferForBrickRepeatPixelShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr) {
        return nullptr;
    }

    return pBuffer + GetBrickRepeatShaderInfoPtr()->constantBufferOffset;
}

/**
 * @brief Sets up the projection matrix of a projected texture.
 * @param rDrawInfo Draw state.
 * @param rGlobalMtx Global matrix of the pane.
 * @param pSize Size of the pane.
 * @param rParameters Projection parameters.
 * @param texCoordGenIdx Index of the texture coordinate generator.
 */
void Material::SetupSubmaterialOf_TextureProjectionMatrix(
    DrawInfo& rDrawInfo, const util::MatrixT4x3fType& rGlobalMtx, const Size* pSize,
    const ResProjectionTexGenParameters& rParameters, int texCoordGenIdx) const {
    float width;
    float height;
    if (rParameters.flag & 1) {
        width = rDrawInfo.m_pLayoutInformation->size.width;
        height = rDrawInfo.m_pLayoutInformation->size.height;
    } else if (rParameters.flag & 2) {
        width = pSize->width;
        height = pSize->height;
    } else {
        const TextureInfo* pTextureInfo = GetTexMapAry()[texCoordGenIdx].m_pTextureInfo;
        if (pTextureInfo != nullptr) {
            const TextureSize size = pTextureInfo->GetSize();
            width = size.width;
            height = size.height;
        } else {
            width = 0.0f;
            height = 0.0f;
        }
    }

    ConstantBufferForVertexShader* pBuffer =
        static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo));
    float (*pMtx)[4];
    switch (texCoordGenIdx) {
    case 0:
        pMtx = pBuffer->texMtx0;
        break;
    case 1:
        pMtx = pBuffer->texMtx1;
        break;
    case 2:
        pMtx = pBuffer->texMtx2;
        break;
    default:
        return;
    }

    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;
    const float scaleX = 0.5f / rParameters.scale.x;
    const float scaleY = 0.5f / rParameters.scale.y;
    pMtx[0][0] = scaleX / halfWidth;
    pMtx[0][1] = 0.0f;
    pMtx[0][2] = 0.0f;
    pMtx[0][3] = 0.5f - rParameters.translate.x / rParameters.scale.x / width;
    pMtx[1][0] = 0.0f;
    pMtx[1][1] = -scaleY / halfHeight;
    pMtx[1][2] = 0.0f;
    pMtx[1][3] = rParameters.translate.y / rParameters.scale.y / height + 0.5f;
    (void)rGlobalMtx;
}

/**
 * @brief Sets up the constants of a brick repeated texture.
 * @param rDrawInfo Draw state.
 * @param rParameters Brick repeat parameters.
 * @param texCoordGenIdx Index of the texture coordinate generator.
 */
void Material::SetupSubmaterialOf_TextureBrickRepeatParams(
    DrawInfo& rDrawInfo, const ResBrickRepeatTexGenParameters& rParameters,
    int texCoordGenIdx) const {
    BrickRepeatConstantBuffer* pBrickBuffer =
        static_cast<BrickRepeatConstantBuffer*>(GetConstantBufferForBrickRepeatPixelShader(rDrawInfo));
    if (pBrickBuffer != nullptr) {
        pBrickBuffer->params0[texCoordGenIdx][0] = rParameters.params[0];
        pBrickBuffer->params0[texCoordGenIdx][1] = rParameters.params[1];
        pBrickBuffer->params0[texCoordGenIdx][2] = rParameters.params[2];
        pBrickBuffer->params0[texCoordGenIdx][3] = rParameters.params[3];
        pBrickBuffer->params1[texCoordGenIdx][0] = rParameters.params[4];
        pBrickBuffer->params1[texCoordGenIdx][1] = rParameters.params[5];
        const float degreeToRadian = util::detail::FloatPi / util::detail::FloatDegree180;
        pBrickBuffer->params1[texCoordGenIdx][3] = 0.0f;
        pBrickBuffer->flags[texCoordGenIdx][0] = 1.0f;
        pBrickBuffer->params1[texCoordGenIdx][2] = rParameters.angle * degreeToRadian;
        pBrickBuffer->flags[texCoordGenIdx][1] = (rParameters.flag & 4) ? 1.0f : 0.0f;
        pBrickBuffer->ranges[texCoordGenIdx][0] = 0.0f;
        pBrickBuffer->ranges[texCoordGenIdx][1] = 0.0f;
        pBrickBuffer->ranges[texCoordGenIdx][2] = 0.0f;
        pBrickBuffer->ranges[texCoordGenIdx][3] = 0.0f;
        if (rParameters.flag & 1) {
            pBrickBuffer->ranges[texCoordGenIdx][0] = rParameters.offset[0];
            pBrickBuffer->ranges[texCoordGenIdx][1] = rParameters.offset[1] - rParameters.offset[0];
        }

        if (rParameters.flag & 2) {
            pBrickBuffer->ranges[texCoordGenIdx][2] = degreeToRadian * rParameters.range[0];
            pBrickBuffer->ranges[texCoordGenIdx][3] =
                degreeToRadian * (rParameters.range[1] - rParameters.range[0]);
        }
    }

    float (*pMtx)[4];
    switch (texCoordGenIdx) {
    case 0:
        pMtx = static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo))
                   ->brickRepeatMtx0;
        break;
    case 1:
        pMtx = static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo))
                   ->brickRepeatMtx1;
        break;
    case 2:
        pMtx = static_cast<ConstantBufferForVertexShader*>(GetConstantBufferForVertexShader(rDrawInfo))
                   ->brickRepeatMtx2;
        break;
    default:
        return;
    }

    pMtx[0][0] = 0.0f;
    pMtx[0][1] = 0.0f;
    pMtx[0][2] = 0.0f;
    pMtx[0][3] = 1.0f;
    pMtx[1][0] = 1.0f;
    pMtx[1][1] = 0.0f;
    pMtx[1][2] = 0.0f;
    pMtx[1][3] = 1.0f;
    pMtx[2][0] = 0.0f;
    pMtx[2][1] = 1.0f;
    pMtx[2][2] = 0.0f;
    pMtx[2][3] = 1.0f;
    pMtx[3][0] = 1.0f;
    pMtx[3][1] = 1.0f;
    pMtx[3][2] = 0.0f;
    pMtx[3][3] = 1.0f;
}

/**
 * @brief Binds the textures and samplers of the material.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record to.
 */
void Material::SetupSubmaterialOf_Texture(DrawInfo& rDrawInfo,
                                          gfx::CommandBuffer& rCommands) const {
    if (m_MemNum.texMap != 0) {
        int num = m_MemNum.texMap;
        if (num > m_DrawTextureNum) {
            num = m_DrawTextureNum;
        }

        for (int i = 0; i < num; i++) {
            const TexMap* pTexMap = &GetTexMapAry()[i];
            const int* pSlots = m_pShaderInfo->m_pTextureSlots;
            const int slot =
                pSlots[m_pShaderInfo->GetTextureSlotCount() * mShaderVariation + i];
            const gfx::DescriptorSlot sampler = rDrawInfo.m_pGraphicsResource->GetSamplerDescriptorSlot(
                static_cast<TexWrap>(pTexMap->mWrapS), static_cast<TexWrap>(pTexMap->mWrapT),
                static_cast<TexFilter>(pTexMap->mMinFilter),
                static_cast<TexFilter>(pTexMap->mMagFilter));
            rCommands.SetTextureAndSampler(slot, gfx::ShaderStage_Pixel,
                                           pTexMap->m_pTextureInfo->mDescriptor, sampler);
        }
    }

    if (m_MemNum.combinerUserShaderParameter) {
        const int slot = m_pShaderInfo->GetPixelShader(mShaderVariation)
                             ->GetInterfaceSlot(gfx::ShaderStage_Pixel,
                                                gfx::ShaderInterfaceType_Sampler, "uTexture3");
        if (slot >= 0) {
            rCommands.SetTextureAndSampler(slot, gfx::ShaderStage_Pixel,
                                           *rDrawInfo.GetFramebufferTextureDescriptorSlot(),
                                           *rDrawInfo.GetFramebufferSamplerDescriptorSlot());
        }
    }
}

/** @return Whether the material samples the framebuffer. */
bool Material::IsUseFramebufferTexture() const {
    return m_MemNum.combinerUserShaderParameter;
}

/**
 * @brief Binds the framebuffer texture to the shader of the material.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record to.
 */
void Material::SetupSubmaterialOf_FramebufferTexture(DrawInfo& rDrawInfo,
                                                     gfx::CommandBuffer& rCommands) const {
    const int slot = m_pShaderInfo->GetPixelShader(mShaderVariation)
                         ->GetInterfaceSlot(gfx::ShaderStage_Pixel,
                                            gfx::ShaderInterfaceType_Sampler, "uTexture3");
    if (slot < 0) {
        return;
    }

    rCommands.SetTextureAndSampler(slot, gfx::ShaderStage_Pixel,
                                   *rDrawInfo.GetFramebufferTextureDescriptorSlot(),
                                   *rDrawInfo.GetFramebufferSamplerDescriptorSlot());
}

namespace {

/**
 * @brief Converts a byte colour to a float colour.
 * @param pOut Destination colour.
 * @param color Source colour.
 */
void SetConstantColor(util::Float4* pOut, util::Unorm8x4 color) {
    pOut->x = static_cast<float>(static_cast<u32>(color.v[0])) / 255.0f;
    pOut->y = static_cast<float>(static_cast<u32>(color.v[1])) / 255.0f;
    pOut->z = static_cast<float>(static_cast<u32>(color.v[2])) / 255.0f;
    pOut->w = static_cast<float>(static_cast<u32>(color.v[3])) / 255.0f;
}

}  // namespace

/**
 * @brief Sets up the constants of the detailed combiner.
 * @param rDrawInfo Draw state.
 */
void Material::SetupSubmaterialOf_DetailedCombiner(DrawInfo& rDrawInfo) const {
    ConstantBufferForDetailedCombinerPixelShader* pBuffer =
        static_cast<ConstantBufferForDetailedCombinerPixelShader*>(
            GetConstantBufferForDetailedCombinerPixelShader(rDrawInfo));
    const int stageNum = m_MemNum.tevStage;
    pBuffer->stageCount = stageNum;
    for (int i = 0; i < stageNum; i++) {
        pBuffer->stageBits[i][0] = GetDetailedCombinerStageAry()[i].bits[0];
        pBuffer->stageBits[i][1] = GetDetailedCombinerStageAry()[i].bits[1];
        pBuffer->stageBits[i][2] = GetDetailedCombinerStageAry()[i].bits[2];
        pBuffer->stageBits[i][3] = GetDetailedCombinerStageAry()[i].bits[3];
    }

    pBuffer->reserve = 0;

    const util::Float4 black = GetColorFloat(MaterialColor_Black);
    const util::Float4 white = GetColorFloat(MaterialColor_White);
    pBuffer->colors[MaterialColor_Black] = black;
    pBuffer->colors[MaterialColor_White] = white;
    SetConstantColor(&pBuffer->constantColors[0], GetDetailedCombinerStageInfoPtr()->constantColor[0]);
    SetConstantColor(&pBuffer->constantColors[1], GetDetailedCombinerStageInfoPtr()->constantColor[1]);
    SetConstantColor(&pBuffer->constantColors[2], GetDetailedCombinerStageInfoPtr()->constantColor[2]);
    SetConstantColor(&pBuffer->constantColors[3], GetDetailedCombinerStageInfoPtr()->constantColor[3]);
    SetConstantColor(&pBuffer->constantColors[4], GetDetailedCombinerStageInfoPtr()->constantColor[4]);
}

/**
 * @brief Sets up the constants of the combiner user shader.
 * @param rDrawInfo Draw state.
 * @param pExtUserData Extended user data of the pane.
 * @param extUserDataCount Number of extended user data.
 * @param pSize Size of the pane.
 */
void Material::SetupSubmaterialOf_CombinerUserShader(DrawInfo& rDrawInfo,
                                                     const ResExtUserData* pExtUserData,
                                                     u16 extUserDataCount, const Size* pSize) const {
    ConstantBufferForCombinerUserShaderPixelShader* pBuffer =
        static_cast<ConstantBufferForCombinerUserShaderPixelShader*>(
            GetConstantBufferForCombinerUserShaderPixelShader(rDrawInfo));
    memset(pBuffer, 0, sizeof(ConstantBufferForCombinerUserShaderPixelShader));
    pBuffer->frameCount = 0;
    pBuffer->paneSize[0] = pSize->width;
    pBuffer->paneSize[1] = pSize->height;
    SetupConstantBufferColor_for_CombinerUserShader(pBuffer);
    SetupConstantBufferPosture_for_CombinerUserShader(pBuffer, rDrawInfo);
    SetupConstantBufferExData_for_CombinerUserShader(pBuffer, pExtUserData, extUserDataCount);
    SetupConstantBufferTextureData_for_CombinerUserShader(pBuffer);
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The pixel shader constants of the detailed combiner.
 */
void* Material::GetConstantBufferForDetailedCombinerPixelShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr) {
        return nullptr;
    }

    return pBuffer + m_PixelShaderConstantBufferOffset;
}

/**
 * @brief Sets up the colours of the combiner user shader.
 * @param pConstantBuffer Constants to set up.
 */
void Material::SetupConstantBufferColor_for_CombinerUserShader(
    ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer) const {
    const util::Float4 black = GetColorFloat(MaterialColor_Black);
    const util::Float4 white = GetColorFloat(MaterialColor_White);
    pConstantBuffer->colors[MaterialColor_Black][0] = black.x;
    pConstantBuffer->colors[MaterialColor_Black][1] = black.y;
    pConstantBuffer->colors[MaterialColor_Black][2] = black.z;
    pConstantBuffer->colors[MaterialColor_Black][3] = black.w;
    pConstantBuffer->colors[MaterialColor_White][0] = white.x;
    pConstantBuffer->colors[MaterialColor_White][1] = white.y;
    pConstantBuffer->colors[MaterialColor_White][2] = white.z;
    pConstantBuffer->colors[MaterialColor_White][3] = white.w;
    for (int i = 0; i < detail::CombinerUserShaderConstantColorMax; i++) {
        const u8* pColor =
            GetCombinerUserShaderPtr()->constantColor[static_cast<u8>(i) %
                                                      detail::CombinerUserShaderConstantColorMax];
        pConstantBuffer->constantColors[i][0] = pColor[0] / 255.0f;
        pConstantBuffer->constantColors[i][1] = pColor[1] / 255.0f;
        pConstantBuffer->constantColors[i][2] = pColor[2] / 255.0f;
        pConstantBuffer->constantColors[i][3] = pColor[3] / 255.0f;
    }
}

/**
 * @brief Sets up the pane matrices of the combiner user shader.
 * @param pConstantBuffer Constants to set up.
 * @param rDrawInfo Draw state holding the matrices.
 */
void Material::SetupConstantBufferPosture_for_CombinerUserShader(
    ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer, DrawInfo& rDrawInfo) const {
    util::MatrixT4x3fType invViewMtx;
    MatrixInverse(&invViewMtx, rDrawInfo.m_ViewMtx);

    util::MatrixT4x3fType globalMtx;
    MatrixMultiply(&globalMtx, invViewMtx, rDrawInfo.m_ModelViewMtx);

    StoreMatrix(pConstantBuffer->paneMtx, rDrawInfo.m_ModelViewMtx);
    StoreMatrix(pConstantBuffer->globalMtx, globalMtx);
    StoreMatrix(pConstantBuffer->viewMtx, rDrawInfo.m_ViewMtx);

    util::Vector3fType cameraPosition;
    cameraPosition._v = float32x4_t{vgetq_lane_f32(invViewMtx._m.val[0], 3),
                                    vgetq_lane_f32(invViewMtx._m.val[1], 3),
                                    vgetq_lane_f32(invViewMtx._m.val[2], 3), 0.0f};
    VectorStore(pConstantBuffer->cameraPosition, cameraPosition);
}

/**
 * @brief Sets up the extended user data of the combiner user shader.
 * @param pConstantBuffer Constants to set up.
 * @param pExtUserData Extended user data of the pane.
 * @param extUserDataCount Number of extended user data.
 */
void Material::SetupConstantBufferExData_for_CombinerUserShader(
    ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer,
    const ResExtUserData* pExtUserData, u16 extUserDataCount) const {
    if (pExtUserData == nullptr || extUserDataCount == 0) {
        return;
    }

    for (int i = 0; i < extUserDataCount; i++) {
        const ResExtUserData* pData = &pExtUserData[i];
        if (strcmp("__CUS_Vec2_0", pData->GetName()) == 0) {
            pConstantBuffer->extVec2[0][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec2[0][1] = pData->GetFloatArray()[1];
        }

        if (strcmp("__CUS_Vec2_1", pData->GetName()) == 0) {
            pConstantBuffer->extVec2[1][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec2[1][1] = pData->GetFloatArray()[1];
        }

        if (strcmp("__CUS_Vec2_2", pData->GetName()) == 0) {
            pConstantBuffer->extVec2[2][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec2[2][1] = pData->GetFloatArray()[1];
        }

        if (strcmp("__CUS_Vec2_3", pData->GetName()) == 0) {
            pConstantBuffer->extVec2[3][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec2[3][1] = pData->GetFloatArray()[1];
        }

        if (strcmp("__CUS_Vec3_0", pData->GetName()) == 0) {
            pConstantBuffer->extVec3[0][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec3[0][1] = pData->GetFloatArray()[1];
            pConstantBuffer->extVec3[0][2] = pData->GetFloatArray()[2];
        }

        if (strcmp("__CUS_Vec3_1", pData->GetName()) == 0) {
            pConstantBuffer->extVec3[1][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec3[1][1] = pData->GetFloatArray()[1];
            pConstantBuffer->extVec3[1][2] = pData->GetFloatArray()[2];
        }

        if (strcmp("__CUS_Vec3_2", pData->GetName()) == 0) {
            pConstantBuffer->extVec3[2][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec3[2][1] = pData->GetFloatArray()[1];
            pConstantBuffer->extVec3[2][2] = pData->GetFloatArray()[2];
        }

        if (strcmp("__CUS_Vec3_3", pData->GetName()) == 0) {
            pConstantBuffer->extVec3[3][0] = pData->GetFloatArray()[0];
            pConstantBuffer->extVec3[3][1] = pData->GetFloatArray()[1];
            pConstantBuffer->extVec3[3][2] = pData->GetFloatArray()[2];
        }

        if (strcmp("__CUS_Rgba_0", pData->GetName()) == 0) {
            pConstantBuffer->extRgba[0][0] = pData->GetIntArray()[0] / 255.0f;
            pConstantBuffer->extRgba[0][1] = pData->GetIntArray()[1] / 255.0f;
            pConstantBuffer->extRgba[0][2] = pData->GetIntArray()[2] / 255.0f;
            pConstantBuffer->extRgba[0][3] = pData->GetIntArray()[3] / 255.0f;
        }

        if (strcmp("__CUS_Rgba_1", pData->GetName()) == 0) {
            pConstantBuffer->extRgba[1][0] = pData->GetIntArray()[0] / 255.0f;
            pConstantBuffer->extRgba[1][1] = pData->GetIntArray()[1] / 255.0f;
            pConstantBuffer->extRgba[1][2] = pData->GetIntArray()[2] / 255.0f;
            pConstantBuffer->extRgba[1][3] = pData->GetIntArray()[3] / 255.0f;
        }

        if (strcmp("__CUS_Rgba_2", pData->GetName()) == 0) {
            pConstantBuffer->extRgba[2][0] = pData->GetIntArray()[0] / 255.0f;
            pConstantBuffer->extRgba[2][1] = pData->GetIntArray()[1] / 255.0f;
            pConstantBuffer->extRgba[2][2] = pData->GetIntArray()[2] / 255.0f;
            pConstantBuffer->extRgba[2][3] = pData->GetIntArray()[3] / 255.0f;
        }

        if (strcmp("__CUS_Rgba_3", pData->GetName()) == 0) {
            pConstantBuffer->extRgba[3][0] = pData->GetIntArray()[0] / 255.0f;
            pConstantBuffer->extRgba[3][1] = pData->GetIntArray()[1] / 255.0f;
            pConstantBuffer->extRgba[3][2] = pData->GetIntArray()[2] / 255.0f;
            pConstantBuffer->extRgba[3][3] = pData->GetIntArray()[3] / 255.0f;
        }

        if (strcmp("__CUS_Float_0", pData->GetName()) == 0) {
            pConstantBuffer->extFloat[0] = pData->GetFloatArray()[0];
        }

        if (strcmp("__CUS_Float_1", pData->GetName()) == 0) {
            pConstantBuffer->extFloat[1] = pData->GetFloatArray()[0];
        }

        if (strcmp("__CUS_Float_2", pData->GetName()) == 0) {
            pConstantBuffer->extFloat[2] = pData->GetFloatArray()[0];
        }

        if (strcmp("__CUS_Float_3", pData->GetName()) == 0) {
            pConstantBuffer->extFloat[3] = pData->GetFloatArray()[0];
        }
    }
}

/**
 * @brief Sets up the texture sizes of the combiner user shader.
 * @param pConstantBuffer Constants to set up.
 */
void Material::SetupConstantBufferTextureData_for_CombinerUserShader(
    ConstantBufferForCombinerUserShaderPixelShader* pConstantBuffer) const {
    for (int i = 0; i < m_MemNum.texMap; i++) {
        const TextureInfo* pTextureInfo = GetTexMapAry()[i].m_pTextureInfo;
        if (pTextureInfo != nullptr) {
            const TextureSize size = pTextureInfo->GetSize();
            pConstantBuffer->textureSize[i][0] = size.width;
            pConstantBuffer->textureSize[i][1] = size.height;
        }
    }
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The pixel shader constants of the combiner user shader.
 */
void* Material::GetConstantBufferForCombinerUserShaderPixelShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr) {
        return nullptr;
    }

    return pBuffer + m_PixelShaderConstantBufferOffset;
}

/**
 * @brief Reserves the constant buffer regions of the material.
 * @param rDrawInfo Draw state holding the constant buffer.
 */
void Material::AllocateConstantBuffer(DrawInfo& rDrawInfo) {
    const size_t alignment = rDrawInfo.m_pGraphicsResource->m_ConstantBufferAlignment;
    m_VertexShaderConstantBufferOffset = AllocateConstantBufferRegion(
        rDrawInfo.m_pConstantBuffer, util::align_up(GetVertexShaderConstantBufferSize(), alignment));

    size_t pixelSize;
    if (m_MemCap.detailedCombinerParameter) {
        pixelSize = GetPixelShaderDetailedCombinerConstantBufferSize();
    } else if (m_MemCap.combinerUserShaderParameter) {
        pixelSize = GetPixelShaderCombinerUserShaderConstantBufferSize();
    } else {
        pixelSize = GetPixelShaderConstantBufferSize();
    }

    m_PixelShaderConstantBufferOffset =
        AllocateConstantBufferRegion(rDrawInfo.m_pConstantBuffer, util::align_up(pixelSize, alignment));

    if (mUserShaderConstantBufferInformation != nullptr &&
        mUserShaderConstantBufferInformation->geometrySize != 0) {
        mUserShaderConstantBufferInformation->flags = AllocateConstantBufferRegion(
            rDrawInfo.m_pConstantBuffer,
            util::align_up(mUserShaderConstantBufferInformation->geometrySize, alignment));
    }

    if (IsBrickRepeatTextureUsed()) {
        GetBrickRepeatShaderInfoPtr()->constantBufferOffset = AllocateConstantBufferRegion(
            rDrawInfo.m_pConstantBuffer, util::align_up(BrickRepeatConstantBufferSize, alignment));
    }
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The vertex shader constants added by the application, or nullptr.
 */
void* Material::GetConstantBufferForUserVertexShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr || mUserShaderConstantBufferInformation == nullptr ||
        mUserShaderConstantBufferInformation->vertexSize == 0) {
        return nullptr;
    }

    return pBuffer + m_VertexShaderConstantBufferOffset + sizeof(ConstantBufferForVertexShader);
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The pixel shader constants added by the application, or nullptr.
 */
void* Material::GetConstantBufferForUserPixelShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr || mUserShaderConstantBufferInformation == nullptr ||
        mUserShaderConstantBufferInformation->pixelSize == 0) {
        return nullptr;
    }

    return pBuffer + m_PixelShaderConstantBufferOffset + sizeof(ConstantBufferForPixelShader);
}

/**
 * @param rDrawInfo Draw state holding the constant buffer.
 * @return The geometry shader constants added by the application, or nullptr.
 */
void* Material::GetConstantBufferForUserGeometryShader(const DrawInfo& rDrawInfo) const {
    u8* pBuffer = static_cast<u8*>(rDrawInfo.m_pConstantBuffer->GetMappedPointer());
    if (pBuffer == nullptr || mUserShaderConstantBufferInformation == nullptr ||
        mUserShaderConstantBufferInformation->geometrySize == 0) {
        return nullptr;
    }

    return pBuffer + mUserShaderConstantBufferInformation->flags;
}

/**
 * @brief Records the blend state and the constant buffers of the material.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::SetCommandBuffer(gfx::CommandBuffer& rCommands, DrawInfo& rDrawInfo) const {
    if (m_MemCap.alpComp) {
        EnableAlphaTest(rCommands);
        rCommands.SetBlendState(m_pBlendState);
        DisableAlphaTest();
    } else {
        rCommands.SetBlendState(m_pBlendState);
    }

    ApplyVertexShaderConstantBuffer(rCommands, rDrawInfo);
    ApplyGeometryShaderConstantBuffer(rCommands, rDrawInfo);
    ApplyPixelShaderConstantBuffer(rCommands, rDrawInfo);
}

/**
 * @brief Enables the alpha test of the blend state.
 * @param rCommands Command buffer to record the alpha reference to.
 */
void Material::EnableAlphaTest(gfx::CommandBuffer& rCommands) const {
    const ResAlphaCompare* pAlphaCompare = GetAlphaComparePtr();
    NVNcolorState* pColorState =
        reinterpret_cast<NVNcolorState*>(&m_pBlendState->ToData()->nvnColorState);
    pfnc_nvnColorStateSetAlphaTest(pColorState, GetNvnAlphaFunc(pAlphaCompare->func));
    pfnc_nvnCommandBufferSetAlphaRef(
        static_cast<NVNcommandBuffer*>(rCommands.ToData()->pNvnCommandBuffer), pAlphaCompare->ref);
}

/** @brief Disables the alpha test of the blend state. */
void Material::DisableAlphaTest() const {
    pfnc_nvnColorStateSetAlphaTest(
        reinterpret_cast<NVNcolorState*>(&m_pBlendState->ToData()->nvnColorState),
        NVN_ALPHA_FUNC_ALWAYS);
}

/**
 * @brief Binds the vertex shader constant buffer.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyVertexShaderConstantBuffer(gfx::CommandBuffer& rCommands,
                                               DrawInfo& rDrawInfo) const {
    gfx::GpuAddress address;
    address.ToData()->value = 0;
    address.ToData()->impl = 0;
    address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
    address.Offset(m_VertexShaderConstantBufferOffset);
    const size_t size = GetVertexShaderConstantBufferSize();
    rCommands.SetConstantBuffer(m_pShaderInfo->GetVertexShaderSlot(mShaderVariation),
                                gfx::ShaderStage_Vertex, address, size);
}

/**
 * @brief Binds the geometry shader constant buffer added by the application.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyGeometryShaderConstantBuffer(gfx::CommandBuffer& rCommands,
                                                 DrawInfo& rDrawInfo) const {
    if (mUserShaderConstantBufferInformation == nullptr ||
        mUserShaderConstantBufferInformation->flags == 0) {
        return;
    }

    gfx::GpuAddress address;

    address.ToData()->value = 0;

    address.ToData()->impl = 0;

    address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();

    address.Offset(mUserShaderConstantBufferInformation->flags);
    rCommands.SetConstantBuffer(m_pShaderInfo->GetGeometryShaderSlot(mShaderVariation),
                                gfx::ShaderStage_Geometry, address,
                                mUserShaderConstantBufferInformation->geometrySize);
}

/**
 * @brief Binds the pixel shader constant buffer.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyPixelShaderConstantBuffer(gfx::CommandBuffer& rCommands,
                                              DrawInfo& rDrawInfo) const {
    if (m_MemCap.detailedCombinerParameter) {
        ApplyPixelShaderDetailedCombinerConstantBuffer(rCommands, rDrawInfo);
    } else if (m_MemCap.combinerUserShaderParameter) {
        ApplyPixelShaderCombinerUserShaderConstantBuffer(rCommands, rDrawInfo);
    } else {
        ApplyPixelShaderConstantBufferDefault(rCommands, rDrawInfo);
    }
}

/**
 * @brief Binds the pixel shader constant buffer of the built-in combiners.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyPixelShaderConstantBufferDefault(gfx::CommandBuffer& rCommands,
                                                     DrawInfo& rDrawInfo) const {
    gfx::GpuAddress address;
    address.ToData()->value = 0;
    address.ToData()->impl = 0;
    address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
    address.Offset(m_PixelShaderConstantBufferOffset);
    const size_t size = GetPixelShaderConstantBufferSize();
    rCommands.SetConstantBuffer(m_pShaderInfo->GetPixelShaderSlot(mShaderVariation),
                                gfx::ShaderStage_Pixel, address, size);

    if (IsBrickRepeatTextureUsed()) {
        const BrickRepeatShaderInfo* pInfo = GetBrickRepeatShaderInfoPtr();
        address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
        address.Offset(pInfo->constantBufferOffset);
        rCommands.SetConstantBuffer(pInfo->slot, gfx::ShaderStage_Pixel, address, size);
    }
}

/**
 * @brief Binds the pixel shader constant buffer of the detailed combiner.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyPixelShaderDetailedCombinerConstantBuffer(gfx::CommandBuffer& rCommands,
                                                              DrawInfo& rDrawInfo) const {
    gfx::GpuAddress address;
    address.ToData()->value = 0;
    address.ToData()->impl = 0;
    address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
    address.Offset(m_PixelShaderConstantBufferOffset);
    rCommands.SetConstantBuffer(m_pShaderInfo->GetPixelShaderSlot(mShaderVariation),
                                gfx::ShaderStage_Pixel, address,
                                GetPixelShaderDetailedCombinerConstantBufferSize());
}

/**
 * @brief Binds the pixel shader constant buffer of the combiner user shader.
 * @param rCommands Command buffer to record to.
 * @param rDrawInfo Draw state.
 */
void Material::ApplyPixelShaderCombinerUserShaderConstantBuffer(gfx::CommandBuffer& rCommands,
                                                                DrawInfo& rDrawInfo) const {
    gfx::GpuAddress address;
    address.ToData()->value = 0;
    address.ToData()->impl = 0;
    address = rDrawInfo.m_pConstantBuffer->GetGpuAddress();
    address.Offset(m_PixelShaderConstantBufferOffset);
    rCommands.SetConstantBuffer(m_pShaderInfo->GetPixelShaderSlot(mShaderVariation),
                                gfx::ShaderStage_Pixel, address,
                                GetPixelShaderCombinerUserShaderConstantBufferSize());
}

/**
 * @brief Records only the blend state of the material.
 * @param rCommands Command buffer to record to.
 */
void Material::SetCommandBufferOnlyBlend(gfx::CommandBuffer& rCommands) const {
    if (m_MemCap.alpComp) {
        EnableAlphaTest(rCommands);
        rCommands.SetBlendState(m_pBlendState);
        DisableAlphaTest();
    } else {
        rCommands.SetBlendState(m_pBlendState);
    }
}

/**
 * @brief Records the shader of the material.
 * @param rCommands Command buffer to record to.
 */
void Material::SetShader(gfx::CommandBuffer& rCommands) const {
    m_pShaderInfo->SetShader(rCommands, mShaderVariation);
}

/**
 * @brief Checks that a copied material equals this one.
 * @param rTarget Copied material.
 * @return Whether the materials are equal.
 */
bool Material::CompareCopiedInstanceTest(const Material& rTarget) const {
    if (mResourceCapacity != rTarget.mResourceCapacity) {
        return false;
    }

    if (mResourceCounts != rTarget.mResourceCounts) {
        return false;
    }

    if (rTarget.m_pMem != nullptr) {
        if (m_pMem == nullptr) {
            return false;
        }

        if (memcmp(m_pMem, rTarget.m_pMem, GetVectorGraphicsTextureRefInfoOffset()) != 0) {
            return false;
        }
    }

    if (m_pShaderInfo != rTarget.m_pShaderInfo) {
        return false;
    }

    if (mName != rTarget.mName) {
        return false;
    }

    if (rTarget.mUserShaderConstantBufferInformation != nullptr) {
        if (mUserShaderConstantBufferInformation == nullptr) {
            return false;
        }

        if (memcmp(mUserShaderConstantBufferInformation, rTarget.mUserShaderConstantBufferInformation,
                   sizeof(UserShaderConstantBufferInformation)) != 0) {
            return false;
        }
    }

    if (GetBlendStateId() != rTarget.GetBlendStateId()) {
        return false;
    }

    if (mTextureCount != rTarget.mTextureCount) {
        return false;
    }

    if (mOwnershipFlags != rTarget.mOwnershipFlags) {
        return false;
    }

    if (mShaderVariation != rTarget.mShaderVariation) {
        return false;
    }

    if (m_IsFloatColorAllocated) {
        if (memcmp(m_pFloatColors, rTarget.m_pFloatColors, sizeof(util::Float4)) != 0) {
            return false;
        }
    } else if (mBlackColor != rTarget.mBlackColor) {
        return false;
    }

    return true;
}

}  // namespace ui2d
}  // namespace nn
