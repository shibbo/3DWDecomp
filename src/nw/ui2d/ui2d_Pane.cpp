#include <nn/ui2d/ui2d_Pane.h>

#include <attributes.h>
#include <arm_neon.h>
#include <cstring>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/font/font_TagProcessorBase.h>
#include <nn/ui2d/ui2d_AnimTransform.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_PaneEffect.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_Resources.h>
#include <nn/ui2d/ui2d_StateMachine.h>
#include <nn/util/util_Arithmetic.h>
#include <nn/util/util_StringUtil.h>

namespace nn::ui2d {
namespace {
/** @brief Signature of an extended user data block ("usd1"). */
const u32 ExtUserDataListSignature = 0x31647375;
/** @brief Type of the extended user data that holds system data. */
const u8 ExtUserDataType_SystemData = 3;
/** @brief Type of a string extended user data. */
const u8 ExtUserDataType_String = 0;
/** @brief Size in bytes of the header of an extended user data block. */
const u32 ExtUserDataListHeaderSize = 0xc;
/** @brief Size in bytes of an extended user data entry. */
const u32 ResExtUserDataSize = 0xc;
/** @brief Number of characters compared in a pane name. */
const int ResourceNameStrMax = 24;
/** @brief Number of characters compared in a material name. */
const int MaterialNameStrMax = 28;
/** @brief Animation target of the pane alpha in GetColorElement/SetColorElement. */
const int AnimTargetPaneColor_Alpha = 16;

using MatrixT4x3fType = nn::util::MatrixT4x3fType;
using MatrixT4x4fType = nn::util::MatrixT4x4fType;
using StateLayerList = StateMachine::StateLayerList;
using StoreList = FeatureParameterStoreSet::StoreList;

/** @brief Header of an extended user data block, without its entries. */
struct ResExtUserDataListHeader {
    u32 signature;
    u32 size;
    u16 count;
    u16 reserved;
};

/**
 * @brief Check whether an object derives from a runtime type.
 * @tparam T Requested type; must provide GetRuntimeTypeInfoStatic.
 * @tparam U Static type of the object.
 * @param rObject Object to check.
 * @return True when the object is a T.
 */
template <typename T, typename U>
inline bool IsDerivedFrom(const U& rObject) {
    const auto* pWanted = T::GetRuntimeTypeInfoStatic();
    for (const auto* pType = rObject.GetRuntimeTypeInfo(); pType != nullptr;
         pType = pType->m_ParentTypeInfo) {
        if (pType == pWanted) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Compares two pane names over at most ResourceNameStrMax characters.
 * @param pName1 First name.
 * @param pName2 Second name.
 * @return Whether the names are equal.
 */
inline bool EqualsResName(const char* pName1, const char* pName2) {
    for (int i = 0; i < ResourceNameStrMax; ++i) {
        if (pName1[i] != pName2[i]) {
            return false;
        }

        if (pName1[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Compares two material names over at most MaterialNameStrMax characters.
 * @param pName1 First name.
 * @param pName2 Second name.
 * @return Whether the names are equal.
 */
inline bool EqualsMaterialName(const char* pName1, const char* pName2) {
    for (int i = 0; i < MaterialNameStrMax; ++i) {
        if (pName1[i] != pName2[i]) {
            return false;
        }

        if (pName1[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @param pList Extended user data block.
 * @return The block as raw bytes.
 */
inline const u8* GetBytes(const ResExtUserDataList* pList) { return reinterpret_cast<const u8*>(pList); }

/**
 * @param pList Extended user data block, as raw bytes, whose first entry holds system data.
 * @return Header of the system data.
 */
inline const SystemDataHeader* GetSystemDataHeader(const u8* pList) {
    const u8* pBase = pList + ExtUserDataListHeaderSize;
    return reinterpret_cast<const SystemDataHeader*>(
        pBase + reinterpret_cast<const ResExtUserData*>(pBase)->dataOffset);
}

/**
 * @param pList Extended user data block, as raw bytes, whose first entry holds system data.
 * @param index Index of a system data.
 * @return The system data at index.
 */
inline const SystemDataBase* GetSystemData(const u8* pList, int index) {
    const u8* pBase = pList + ExtUserDataListHeaderSize;
    const u32 dataOffset = reinterpret_cast<const ResExtUserData*>(pBase)->dataOffset;
    const u32 offset =
        dataOffset + *reinterpret_cast<const u32*>(pBase + (dataOffset + sizeof(u32) + index * sizeof(u32)));
    return reinterpret_cast<const SystemDataBase*>(pBase + offset);
}

/**
 * @param pList Extended user data block, as raw bytes, whose first entry holds system data.
 * @param index Index of a system data.
 * @return The system data at index.
 */
inline const SystemDataBase* GetSystemDataByOffsetTable(const u8* pList, int index) {
    const u8* pBase = pList + ExtUserDataListHeaderSize;
    const u32 dataOffset = reinterpret_cast<const ResExtUserData*>(pBase)->dataOffset;
    const u32 offset =
        dataOffset + *reinterpret_cast<const u32*>(pBase + (dataOffset + sizeof(u32) * index) + sizeof(u32));
    return reinterpret_cast<const SystemDataBase*>(pBase + offset);
}

/**
 * @param degree Angle in degrees.
 * @return Angle index of degree.
 */
inline util::AngleIndex DegreeToAngleIndex(float degree) {
    return static_cast<int64_t>(
        degree * (static_cast<float>(util::detail::AngleIndexHalfRound) / util::detail::FloatDegree180));
}

/**
 * @brief Evaluate sine and cosine from the sample table.
 * @param pSin Receives the sine.
 * @param pCos Receives the cosine.
 * @param angleIndex Angle to evaluate.
 */
inline void SinCosTable(float* pSin, float* pCos, util::AngleIndex angleIndex) {
    const util::detail::SinCosSample* pSample =
        &util::detail::SinCosSampleTable[(angleIndex >> 24) & 0xff];
    const float rest = static_cast<float>(angleIndex & 0xffffff) * (1.0f / 0x1000000);
    *pSin = pSample->sinValue + pSample->sinDelta * rest;
    *pCos = pSample->cosValue + pSample->cosDelta * rest;
}

/**
 * @param row Row of a matrix.
 * @return A vector holding only the translation (fourth) element of row.
 */
inline float32x4_t GetTranslationVector(float32x4_t row) {
    return vsetq_lane_f32(vgetq_lane_f32(row, 3), vdupq_n_f32(0.0f), 3);
}

/**
 * @brief Multiply a row of a 4x3 matrix by another 4x3 matrix.
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
    return vaddq_f32(GetTranslationVector(row), result);
}

/**
 * @brief Multiply two 4x3 matrices.
 * @param pOut Receives rLeft * rRight.
 * @param rLeft Left matrix.
 * @param rRight Right matrix.
 */
inline void MatrixMultiply(MatrixT4x3fType* pOut, const MatrixT4x3fType& rLeft,
                           const MatrixT4x3fType& rRight) {
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

/**
 * @brief Build an orthographic projection matrix for the capture of a pane.
 * @param pOut Receives the matrix.
 * @param left Left edge.
 * @param right Right edge.
 * @param bottom Bottom edge.
 * @param top Top edge.
 */
inline void MatrixOrthographicOffCenter(MatrixT4x4fType* pOut, float left, float right, float bottom,
                                        float top) {
    const float rcpWidth = 1.0f / (right - left);
    const float rcpHeight = 1.0f / (top - bottom);
    const float32x2_t zero = vdup_n_f32(0.0f);

    float32x4x4_t mtx;
    mtx.val[0] = vcombine_f32(vset_lane_f32(rcpWidth + rcpWidth, zero, 0),
                              vset_lane_f32(-(left + right) * rcpWidth, zero, 1));
    mtx.val[1] = vcombine_f32(vset_lane_f32(rcpHeight + rcpHeight, zero, 1),
                              vset_lane_f32(-(top + bottom) * rcpHeight, zero, 1));
    mtx.val[2] = float32x4_t{0.0f, 0.0f, -0.002f, -0.0f};
    mtx.val[3] = float32x4_t{0.0f, 0.0f, 0.0f, 1.0f};
    pOut->_m = mtx;
}

/**
 * @brief Move a matrix vertically.
 * @param pMtx Matrix to translate.
 * @param y Vertical offset.
 */
inline void MatrixTranslateY(MatrixT4x3fType* pMtx, float y) {
    const float x = 0.0f;
    MatrixT4x3fType translation;
    const float32x4_t axisX = {1.0f, 0.0f, 0.0f, 0.0f};
    const float32x4_t axisY = {0.0f, 1.0f, 0.0f, 0.0f};
    translation._m.val[0] = vsetq_lane_f32(x, axisX, 3);
    translation._m.val[1] = vsetq_lane_f32(y, axisY, 3);
    translation._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
    MatrixMultiply(pMtx, translation, *pMtx);
}

/**
 * @brief Copy a 4x3 matrix element by element.
 * @param pDst Destination elements.
 * @param pSrc Source elements.
 */
inline void CopyMatrix(float* pDst, const float* pSrc) {
    auto* pDstRows = reinterpret_cast<nn::util::Float4*>(pDst);
    const auto* pSrcRows = reinterpret_cast<const nn::util::Float4*>(pSrc);
    for (int i = 0; i < 3; i++) {
        pDstRows[i] = pSrcRows[i];
    }
}

/**
 * @param pEffect Pane effect instance.
 * @return Whether the static mask cache of the effect must be rendered.
 */
inline bool IsMaskStaticCacheRenderingNeeded(const detail::PaneEffect* pEffect) {
    return pEffect->IsMaskStaticCacheEnabled() && !pEffect->m_Mask.isStaticCacheUpdated;
}

/**
 * @param pEffect Pane effect instance.
 * @return Whether the static drop shadow cache of the effect must be rendered.
 */
inline bool IsDropShadowStaticCacheRenderingNeeded(const detail::PaneEffect* pEffect) {
    return pEffect->IsDropShadowStaticCacheEnabled() && !pEffect->m_DropShadow.isStaticCacheUpdated;
}

/**
 * @param pEffect Pane effect instance.
 * @return Whether a static cache of the effect must be rendered.
 */
inline bool IsStaticCacheRenderingNeeded(const detail::PaneEffect* pEffect) {
    const bool isDropShadowNeeded = IsDropShadowStaticCacheRenderingNeeded(pEffect);
    const bool isMaskNeeded = IsMaskStaticCacheRenderingNeeded(pEffect);
    return isMaskNeeded | isDropShadowNeeded;
}

/**
 * @param pList Extended user data block.
 * @param index Index of an entry.
 * @return The entry at index.
 */
inline const ResExtUserData* GetEntry(const ResExtUserDataList* pList, int index) {
    return reinterpret_cast<const ResExtUserData*>(GetBytes(pList) + ExtUserDataListHeaderSize) + index;
}

/**
 * @brief Set or clear one bit of a flag byte.
 * @param pFlags Flags to modify.
 * @param bit Index of the bit.
 * @param isSet Whether the bit is set.
 */
inline void SetFlagBit(u8* pFlags, int bit, bool isSet) {
    *pFlags = (*pFlags & ~(1 << bit)) | (isSet << bit);
}

/**
 * @brief Release a block of layout memory if there is one.
 * @param pMemory Memory to release, or nullptr.
 */
inline void SafeFreeMemory(void* pMemory) {
    if (pMemory != nullptr) {
        Layout::FreeMemory(pMemory);
    }
}

/**
 * @brief Release the values of every store of a feature parameter store set.
 * @param rStores Stores to release.
 */
inline void FinalizeStores(StoreList& rStores) {
    for (StoreList::iterator it = rStores.begin(); it != rStores.end();) {
        StoreList::iterator current = it++;
        rStores.erase(current);
        FeatureParameterStore& rStore = *current;

        for (int i = 0; i < rStore.m_Count; i++) {
            SafeFreeMemory(rStore.m_pValues[i].pValues);
        }

        SafeFreeMemory(rStore.m_pValues);
        Layout::FreeMemory(&rStore);
    }
}
}  // namespace

/** @brief Constructs an unlinked pane base. */
detail::PaneBase::PaneBase() {}

/** @brief Destroys the pane base. */
detail::PaneBase::~PaneBase() {}

/** @brief Constructs an empty, visible pane. */
Pane::Pane() {
    InitializeParams();
    mOriginFlags = 0;
    std::memset(mPanelName, 0, sizeof(mPanelName));
    std::memset(mUserData, 0, sizeof(mUserData));
    mPositionX = 0.0f;
    mPositionY = 0.0f;
    mPositionZ = 0.0f;
    mRotationX = 0.0f;
    mRotationY = 0.0f;
    mRotationZ = 0.0f;
    mScaleX = 1.0f;
    mScaleY = 1.0f;
    mSizeX = 0.0f;
    mSizeY = 0.0f;
    mAlpha = 255;
    mAlphaInfluence = 255;
    SetVisible(true);
    SetGlobalMatrixDirty();
}

/** @brief Resets the hierarchy, flags, matrices and extended user data of the pane. */
void Pane::InitializeParams() {
    mParent = nullptr;
    mFlags = 0;
    mFlagEx = 0;
    m_SystemExtDataFlag = 0;
    m_pUserMtx = nullptr;
    m_pExtUserDataList = nullptr;
    auto* pGlobalMtx = reinterpret_cast<MatrixT4x3fType*>(mGlobalMtx);
    pGlobalMtx->_m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
    pGlobalMtx->_m.val[1] = float32x4_t{0.0f, 1.0f, 0.0f, 0.0f};
    pGlobalMtx->_m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
}

/**
 * @brief Constructs a pane from its resource block.
 * @param pResPane Pane block of a layout resource.
 * @param rBuildArgSet Build arguments.
 */
Pane::Pane(const ResPane* pResPane, const BuildArgSet& rBuildArgSet) {
    InitializeByResourceBlock(nullptr, nullptr, pResPane, rBuildArgSet);
}

/**
 * @brief Initializes the pane from its resource block.
 * @param pResult Receives the required constant buffer size, or nullptr.
 * @param pDevice Graphics device.
 * @param pResPane Pane block of a layout resource.
 * @param rBuildArgSet Build arguments.
 */
void Pane::InitializeByResourceBlock(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                     const ResPane* pResPane, const BuildArgSet& rBuildArgSet) {
    InitializeParams();

    const nn::util::Float3* pTranslate = &pResPane->translate;
    const nn::util::Float3* pRotate = &pResPane->rotate;
    const nn::util::Float2* pScale = &pResPane->scale;
    const nn::util::Float2* pSize = &pResPane->size;
    const u8* pAlpha = &pResPane->alpha;
    const char* pUserData = pResPane->userData;

    const u8* pBasicInfo = static_cast<const u8*>(rBuildArgSet.pOverridePartsPaneBasicInfo);
    if (pBasicInfo != nullptr) {
        const u16 usageFlag = rBuildArgSet.overrideBasicUsageFlag;
        if ((usageFlag & 8) != 0) {
            pTranslate = reinterpret_cast<const nn::util::Float3*>(pBasicInfo + 0x8);
        }

        if ((usageFlag & 0x40) != 0) {
            pRotate = reinterpret_cast<const nn::util::Float3*>(pBasicInfo + 0x14);
        }

        if ((usageFlag & 0x20) != 0) {
            pScale = reinterpret_cast<const nn::util::Float2*>(pBasicInfo + 0x20);
        }

        if ((usageFlag & 0x10) != 0) {
            pSize = reinterpret_cast<const nn::util::Float2*>(pBasicInfo + 0x28);
        }

        if ((usageFlag & 0x80) != 0) {
            pAlpha = pBasicInfo + 0x30;
        }

        if ((usageFlag & 4) != 0) {
            pUserData = reinterpret_cast<const char*>(pBasicInfo);
        }
    }

    mOriginFlags = pResPane->basePosition;
    SetName(pResPane->name);
    SetUserData(pUserData);
    mFlagEx = pResPane->flagEx;

    if ((mFlagEx & PaneFlagEx_ExtUserDataAnimationEnabled) != 0 &&
        rBuildArgSet.pExtUserDataList != nullptr) {
        AllocateAndCopyAnimatedExtUserData(rBuildArgSet.pExtUserDataList);
    } else {
        mFlagEx &= ~PaneFlagEx_ExtUserDataAnimationEnabled;
        SetExtUserDataList(rBuildArgSet.pExtUserDataList);
    }

    if (m_pExtUserDataList != nullptr &&
        m_pExtUserDataList->entries[0].type == ExtUserDataType_SystemData) {
        AddSystemExtUserDataReferenceTable();
    }

    mPositionX = pTranslate->x;
    mPositionY = pTranslate->y;
    mPositionZ = pTranslate->z;
    mRotationX = pRotate->x;
    mRotationY = pRotate->y;
    mRotationZ = pRotate->z;
    mScaleX = pScale->x;
    mScaleY = pScale->y;

    if (rBuildArgSet.pOverrideBuildResSet != nullptr &&
        (rBuildArgSet.magnify.x != 1.0f || rBuildArgSet.magnify.y != 1.0f) &&
        (pResPane->flagEx & PaneFlagEx_IgnorePartsMagnify) == 0) {
        if ((pResPane->flagEx & PaneFlagEx_PartsMagnifyAdjustToPartsBound) != 0) {
            const float rateX = GetBasePositionX() == 0 ? 1.0f : 0.5f;
            const float rateY = (mOriginFlags & 0xc) == 0 ? 1.0f : 0.5f;
            nn::util::Float2 parentScale;
            CalculateScaleFromPartsRoot(&parentScale, rBuildArgSet.pParentPane);
            const float diffY = rBuildArgSet.partsSize.y * rBuildArgSet.magnify.y - rBuildArgSet.partsSize.y;
            const float diffX = rBuildArgSet.partsSize.x * rBuildArgSet.magnify.x - rBuildArgSet.partsSize.x;
            mSizeX = pSize->x + rateX * diffX / (parentScale.x * pScale->x);
            mSizeY = pSize->y + rateY * diffY / (parentScale.y * pScale->y);
        } else {
            mSizeX = rBuildArgSet.magnify.x * pSize->x;
            mSizeY = pSize->y * rBuildArgSet.magnify.y;
        }
    } else {
        mSizeX = pSize->x;
        mSizeY = pSize->y;
    }

    mAlpha = *pAlpha;
    mAlphaInfluence = *pAlpha;
    mFlags = pResPane->flag;

    if (rBuildArgSet.pOverrideBuildResSet != nullptr && (rBuildArgSet.overrideBasicUsageFlag & 1) != 0) {
        SetFlagBit(&mFlags, 0, (rBuildArgSet.overrideBasicUsageFlag & 2) != 0);
    }

    ApplyProceduralShapeOverride(rBuildArgSet);

    if (FindSystemExtData(PaneSystemDataType_ProceduralShape) != nullptr) {
        SystemDataProceduralShapeRuntimeInfo info;
        info.type = PaneSystemDataType_ProceduralShapeRuntimeInfo;
        AddDynamicSystemExtUserData(PaneSystemDataType_ProceduralShapeRuntimeInfo, &info, sizeof(info));
    }

    if (rBuildArgSet._D8 && FindSystemExtData(PaneSystemDataType_DynamicInfo) == nullptr) {
        SystemDataDynamicInfo info;
        info.type = PaneSystemDataType_DynamicInfo;
        info.value = 0;
        AddDynamicSystemExtUserData(PaneSystemDataType_DynamicInfo, &info, sizeof(info));
    }

    InitializePaneEffects(pResult, pDevice, rBuildArgSet);
    SetGlobalMatrixDirty();
}

/**
 * @brief Constructs a pane from its resource block.
 * @param pResult Receives the required constant buffer size, or nullptr.
 * @param pDevice Graphics device.
 * @param pResPane Pane block of a layout resource.
 * @param rBuildArgSet Build arguments.
 */
Pane::Pane(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const ResPane* pResPane,
           const BuildArgSet& rBuildArgSet) {
    InitializeByResourceBlock(pResult, pDevice, pResPane, rBuildArgSet);
}

/**
 * @brief Sets the name of the pane.
 * @param name Name; at most 24 visible characters are kept.
 */
void Pane::SetName(const char* name) { nn::util::Strlcpy(mPanelName, name, sizeof(mPanelName)); }

/**
 * @brief Sets the user data string of the pane.
 * @param data User data; at most 8 visible characters are kept.
 */
void Pane::SetUserData(const char* data) { nn::util::Strlcpy(mUserData, data, sizeof(mUserData)); }

/**
 * @brief Uses a private copy of an extended user data block, so that it can be animated.
 * @param pList Extended user data block to copy.
 */
void Pane::AllocateAndCopyAnimatedExtUserData(const ResExtUserDataList* pList) {
    void* pCopy = Layout::AllocateMemory(pList->size);
    std::memcpy(pCopy, pList, pList->size);
    m_pExtUserDataList = static_cast<const ResExtUserDataList*>(pCopy);
    UpdateSystemExtDataFlag(m_pExtUserDataList);
}

/**
 * @brief Sets the extended user data block of the pane.
 * @param pList Extended user data block, or nullptr.
 */
void Pane::SetExtUserDataList(const ResExtUserDataList* pList) {
    m_pExtUserDataList = pList;
    m_SystemExtDataFlag = 0;
    UpdateSystemExtDataFlag(pList);
}

/** @brief Adds an empty system data reference table to the extended user data. */
void Pane::AddSystemExtUserDataReferenceTable() {
    SystemDataReferenceTable table;
    table.type = PaneSystemDataType_ReferenceTable;
    std::memset(table.indices, -1, sizeof(table.indices));
    AddDynamicSystemExtUserDataImpl(PaneSystemDataType_ReferenceTable, &table, sizeof(table), true);
}

/**
 * @brief Calculates the accumulated scale between a pane and the root of its parts layout.
 * @param pScale Receives the scale.
 * @param pPane First pane of the chain, or nullptr.
 */
void Pane::CalculateScaleFromPartsRoot(nn::util::Float2* pScale, Pane* pPane) const {
    pScale->x = 1.0f;
    pScale->y = 1.0f;

    for (; pPane != nullptr; pPane = pPane->mParent) {
        if (IsDerivedFrom<Parts>(*pPane)) {
            return;
        }

        pScale->x = pPane->mScaleX * pScale->x;
        pScale->y = pPane->mScaleY * pScale->y;
    }
}

/**
 * @brief Applies the procedural shapes of the override extended user data of a parts pane.
 * @param rBuildArgSet Build arguments.
 */
void Pane::ApplyProceduralShapeOverride(const BuildArgSet& rBuildArgSet) {
    const ResExtUserDataList* pList = rBuildArgSet.pOverrideExtUserDataList;
    if (pList == nullptr) {
        return;
    }

    const u8* pSystem = GetBytes(pList);
    const int count = GetSystemDataHeader(pSystem)->count;

    for (int i = 0; i < count; i++) {
        const SystemDataBase* pData = GetSystemData(pSystem, i);
        if (pData->type != PaneSystemDataType_ProceduralShape) {
            continue;
        }

        void* pTarget = GetSystemExtDataForModify(PaneSystemDataType_ProceduralShape);
        if (pTarget != nullptr) {
            std::memcpy(pTarget, pData, sizeof(SystemDataProceduralShape));
        } else {
            AddDynamicSystemExtUserData(PaneSystemDataType_ProceduralShape, pData,
                                        sizeof(SystemDataProceduralShape));
        }
    }
}

/**
 * @param type Type of the system data.
 * @return The system data of the type, or nullptr when the pane has none.
 */
const void* Pane::GetSystemExtDataByType(PaneSystemDataType type) const {
    if (m_SystemExtDataFlag == 0) {
        return nullptr;
    }

    const u8* pSystem = GetBytes(m_pExtUserDataList);
    const auto* pTable = static_cast<const SystemDataReferenceTable*>(GetSystemData(pSystem, 0));
    if (pTable->type != PaneSystemDataType_ReferenceTable) {
        return nullptr;
    }

    const int index = pTable->indices[ConvertSystemExtDataTypeToReferenceTableIndex(type)];
    if (index < 0) {
        return nullptr;
    }

    return GetSystemDataByOffsetTable(pSystem, index);
}

/**
 * @brief Adds a system data to the extended user data of the pane.
 * @param type Type of the system data.
 * @param pData System data to copy.
 * @param dataSize Size of the system data in bytes.
 */
void Pane::AddDynamicSystemExtUserData(PaneSystemDataType type, const void* pData, int dataSize) {
    if ((m_SystemExtDataFlag & (1 << PaneSystemDataType_ReferenceTable)) == 0) {
        AddSystemExtUserDataReferenceTable();
    }

    AddDynamicSystemExtUserDataImpl(type, pData, dataSize, false);
}

/**
 * @brief Creates the pane effect of a pane that has a mask or a drop shadow.
 * @param pResult Receives the required constant buffer size, or nullptr.
 * @param pDevice Graphics device.
 * @param rBuildArgSet Build arguments.
 */
void Pane::InitializePaneEffects(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                 const BuildArgSet& rBuildArgSet) {
    const auto* pMask =
        static_cast<const SystemDataMaskTexture*>(FindSystemExtData(PaneSystemDataType_Mask));
    const auto* pDropShadow =
        static_cast<const SystemDataDropShadow*>(FindSystemExtData(PaneSystemDataType_DropShadow));

    if (pMask != nullptr || pDropShadow != nullptr) {
        detail::PaneEffect* pEffect = Layout::NewObj<detail::PaneEffect>();
        SystemDataPaneEffectInstance data;
        data.type = PaneSystemDataType_PaneEffectInstance;
        data.pPaneEffect = pEffect;
        pEffect->Initialize(pResult, pDevice, this, rBuildArgSet);

        if (pResult != nullptr) {
            pResult->requiredUi2dConstantBufferSize +=
                detail::PaneEffect::GetRequiredConstantBufferSize(pDevice, pMask, pDropShadow);
        }

        AddDynamicSystemExtUserData(PaneSystemDataType_PaneEffectInstance, &data, sizeof(data));
    }
}

/**
 * @param type Type of the system data.
 * @return The system data of the type, or nullptr when the pane has none.
 */
void* Pane::GetSystemExtDataForModify(PaneSystemDataType type) {
    return const_cast<void*>(FindSystemExtData(type));
}

/**
 * @brief Copies another pane.
 * @param rOther Source pane.
 * @param pDevice Graphics device.
 * @param pLayout Layout that owns the copy.
 * @param pContext Pane tree build context.
 */
void Pane::CopyImpl(const Pane& rOther, nn::gfx::Device* pDevice, const Layout* pLayout,
                    detail::BuildPaneTreeContext* pContext) {
    mParent = nullptr;
    *reinterpret_cast<nn::util::Float3*>(&mPositionX) =
        *reinterpret_cast<const nn::util::Float3*>(&rOther.mPositionX);
    *reinterpret_cast<nn::util::Float3*>(&mRotationX) =
        *reinterpret_cast<const nn::util::Float3*>(&rOther.mRotationX);
    *reinterpret_cast<nn::util::Float2*>(&mScaleX) =
        *reinterpret_cast<const nn::util::Float2*>(&rOther.mScaleX);
    *reinterpret_cast<nn::util::Float2*>(&mSizeX) =
        *reinterpret_cast<const nn::util::Float2*>(&rOther.mSizeX);
    mFlags = rOther.mFlags;
    mAlpha = rOther.mAlpha;
    mAlphaInfluence = rOther.mAlphaInfluence;
    mOriginFlags = rOther.mOriginFlags;
    mFlagEx = rOther.mFlagEx;
    m_SystemExtDataFlag = rOther.m_SystemExtDataFlag;
    CopyMatrix(mGlobalMtx, rOther.mGlobalMtx);
    m_pUserMtx = nullptr;
    m_pExtUserDataList = nullptr;
    SetName(rOther.mPanelName);
    SetUserDataAsBinary(rOther.mUserData);
    mFlags &= ~PaneFlag_UserMatrix;
    SetGlobalMatrixDirty();

    if (IsExtUserDataMemoryDynamicallyAllocated()) {
        AllocateAndCopyAnimatedExtUserData(rOther.m_pExtUserDataList);
    } else {
        m_pExtUserDataList = rOther.m_pExtUserDataList;
    }

    const auto* pSource = static_cast<const SystemDataPaneEffectInstance*>(
        rOther.FindSystemExtData(PaneSystemDataType_PaneEffectInstance));
    if (pSource != nullptr) {
        detail::PaneEffect* pEffect =
            Layout::NewObj<detail::PaneEffect>(*pSource->pPaneEffect, this, pDevice, pLayout, pContext);
        const_cast<SystemDataPaneEffectInstance*>(static_cast<const SystemDataPaneEffectInstance*>(
            GetSystemExtDataByTypeUnchecked(PaneSystemDataType_PaneEffectInstance)))
            ->pPaneEffect = pEffect;
    }
}

/**
 * @brief Sets the user data of the pane from raw bytes.
 * @param pData Eight bytes of user data.
 */
void Pane::SetUserDataAsBinary(const void* pData) { std::memcpy(mUserData, pData, 8); }

/** @return Whether the extended user data of the pane is owned by the pane. */
bool Pane::IsExtUserDataMemoryDynamicallyAllocated() const {
    return (mFlagEx & (PaneFlagEx_ExtUserDataAnimationEnabled | PaneFlagEx_DynamicExtUserDataEnabled)) != 0;
}

/**
 * @brief Copies another pane.
 * @param rOther Source pane.
 * @param pDevice Graphics device.
 * @param pAccessor Resource accessor of the copy.
 * @param pNewRootName Name of the copied root pane.
 * @param pLayout Layout that owns the copy.
 */
void Pane::CopyImpl(const Pane& rOther, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                    const char* pNewRootName, const Layout* pLayout) {
    mParent = nullptr;
    *reinterpret_cast<nn::util::Float3*>(&mPositionX) =
        *reinterpret_cast<const nn::util::Float3*>(&rOther.mPositionX);
    *reinterpret_cast<nn::util::Float3*>(&mRotationX) =
        *reinterpret_cast<const nn::util::Float3*>(&rOther.mRotationX);
    *reinterpret_cast<nn::util::Float2*>(&mScaleX) =
        *reinterpret_cast<const nn::util::Float2*>(&rOther.mScaleX);
    *reinterpret_cast<nn::util::Float2*>(&mSizeX) =
        *reinterpret_cast<const nn::util::Float2*>(&rOther.mSizeX);
    mFlags = rOther.mFlags;
    mAlpha = rOther.mAlpha;
    mAlphaInfluence = rOther.mAlphaInfluence;
    mOriginFlags = rOther.mOriginFlags;
    mFlagEx = rOther.mFlagEx;
    m_SystemExtDataFlag = rOther.m_SystemExtDataFlag;
    CopyMatrix(mGlobalMtx, rOther.mGlobalMtx);
    m_pUserMtx = nullptr;
    m_pExtUserDataList = nullptr;
    SetName(rOther.mPanelName);
    SetUserDataAsBinary(rOther.mUserData);
    mFlags &= ~PaneFlag_UserMatrix;
    SetGlobalMatrixDirty();

    if (IsExtUserDataMemoryDynamicallyAllocated()) {
        AllocateAndCopyAnimatedExtUserData(rOther.m_pExtUserDataList);
    } else {
        m_pExtUserDataList = rOther.m_pExtUserDataList;
    }

    const auto* pSource = static_cast<const SystemDataPaneEffectInstance*>(
        rOther.FindSystemExtData(PaneSystemDataType_PaneEffectInstance));
    if (pSource != nullptr) {
        detail::PaneEffect* pEffect =
            Layout::NewObj<detail::PaneEffect>(*pSource->pPaneEffect, this, pDevice, pLayout, nullptr);
        const_cast<SystemDataPaneEffectInstance*>(static_cast<const SystemDataPaneEffectInstance*>(
            GetSystemExtDataByTypeUnchecked(PaneSystemDataType_PaneEffectInstance)))
            ->pPaneEffect = pEffect;
    }
}

/**
 * @brief Constructs a copy of a pane.
 * @param rOther Source pane.
 * @param pDevice Graphics device.
 * @param pLayout Layout that owns the copy.
 */
Pane::Pane(const Pane& rOther, nn::gfx::Device* pDevice, Layout* pLayout) {
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

/** @brief Destroys the pane; resources are released by Finalize. */
Pane::~Pane() {}

/**
 * @brief Releases the pane effect, the state machine, the children and the extended user data.
 * @param pDevice Graphics device.
 */
void Pane::Finalize(nn::gfx::Device* pDevice) {
    if ((m_SystemExtDataFlag & (1 << PaneSystemDataType_PaneEffectInstance)) != 0) {
        const auto* pData = static_cast<const SystemDataPaneEffectInstance*>(
            FindSystemExtData(PaneSystemDataType_PaneEffectInstance));
        pData->pPaneEffect->Finalize(pDevice);
        Layout::FreeMemory(pData->pPaneEffect);
    }

    const auto* pStateMachineData = static_cast<const SystemDataStateMachineInstance*>(
        FindSystemExtData(PaneSystemDataType_StateMachine));
    if (pStateMachineData != nullptr) {
        StateMachine* pStateMachine = pStateMachineData->pStateMachine;
        if (pStateMachine != nullptr) {
            pStateMachine->Finalize();
            Layout::FreeMemory(pStateMachine);
        }
    }

    PaneList& rChildren = GetChildList();
    for (PaneList::iterator it = rChildren.begin(); it != rChildren.end();) {
        PaneList::iterator current = it++;
        rChildren.erase(current);
        Pane* pChild = FromLink(current.GetNode());

        if (pChild->IsUserAllocated()) {
            continue;
        }

        if (IsDerivedFrom<Parts>(*pChild) && static_cast<Parts*>(pChild)->m_pLayout != nullptr) {
            Layout* pLayout = static_cast<Parts*>(pChild)->m_pLayout;
            pLayout->Finalize(pDevice);
            Layout::DeleteObj(pLayout);
        } else {
            pChild->Finalize(pDevice);
            Layout::DeleteObj(pChild);
        }
    }

    if (IsExtUserDataMemoryDynamicallyAllocated()) {
        Layout::FreeMemory(const_cast<ResExtUserDataList*>(m_pExtUserDataList));
    }
}

/** @brief Releases every layer, the event handler and the variables of the state machine. */
inline NOINLINE void StateMachine::Finalize() {
    for (StateLayerList::iterator it = m_StateLayers.begin(); it != m_StateLayers.end();) {
        StateLayerList::iterator current = it++;
        m_StateLayers.erase(current);
        current->Finalize();
        Layout::FreeMemory(&*current);
    }

    _C0 = nullptr;
    m_pLayout = nullptr;
    m_pName = nullptr;
    Layout::DeleteObj(m_pEventHandler);
    m_pEventHandler = nullptr;
    m_pListener = nullptr;
    SafeFreeMemory(m_EventQueue.m_pEvents);
    m_EventQueue.m_pEvents = nullptr;
    m_VariableManager.Finalize();
}

/**
 * @brief Inserts a child pane at the end of the child list.
 * @param child Child pane; its global matrix becomes dirty.
 */
void Pane::AppendChild(Pane* child) {
    m_Children.LinkPrev(&child->m_Link);
    child->mParent = this;
    child->SetGlobalMatrixDirty();
}

/**
 * @brief Inserts a child pane before another child.
 * @param next Position of the child that follows the new one.
 * @param pChild Child pane; its global matrix becomes dirty.
 */
void Pane::InsertChild(PaneList::iterator next, Pane* pChild) {
    next.GetNode()->LinkPrev(&pChild->m_Link);
    pChild->mParent = this;
    pChild->SetGlobalMatrixDirty();
}

/**
 * @brief Inserts a child pane at the beginning of the child list.
 * @param child Child pane; its global matrix becomes dirty.
 */
void Pane::PrependChild(Pane* child) {
    m_Children.GetNext()->LinkPrev(&child->m_Link);
    child->mParent = this;
    child->SetGlobalMatrixDirty();
}

/**
 * @brief Inserts a child pane before another child.
 * @param pNext Child that follows the new one.
 * @param pChild Child pane; its global matrix becomes dirty.
 */
void Pane::InsertChild(Pane* pNext, Pane* pChild) {
    pNext->m_Link.LinkPrev(&pChild->m_Link);
    pChild->mParent = this;
    pChild->SetGlobalMatrixDirty();
}

/**
 * @brief Detaches a child pane without destroying it.
 * @param child Child pane.
 */
void Pane::RemoveChild(Pane* child) {
    nn::util::IntrusiveListNode* pLink = &child->m_Link;
    if (&m_Children != pLink) {
        pLink->Unlink();
    }

    child->mParent = nullptr;
}

/** @return Rectangle covered by the pane in its local coordinates. */
const nn::font::Rectangle Pane::GetPaneRect() const {
    nn::font::Rectangle rect = {};
    const nn::util::Float2 basePos = GetVertexPos();
    rect.left = basePos.x;
    rect.top = basePos.y;
    rect.right = rect.left + mSizeX;
    rect.bottom = rect.top - mSizeY;
    return rect;
}

/** @return Position of the top-left vertex of the pane relative to its origin. */
nn::util::Float2 Pane::GetVertexPos() const {
    nn::util::Float2 pos;

    switch (GetBasePositionX()) {
    case 0:
        pos.x = mSizeX * -0.5f;
        break;
    case 2:
        pos.x = -mSizeX;
        break;
    default:
        pos.x = 0.0f;
        break;
    }

    switch (GetBasePositionY()) {
    case 0:
        pos.y = mSizeY * 0.5f;
        break;
    case 2:
        pos.y = mSizeY;
        break;
    default:
        pos.y = 0.0f;
        break;
    }

    return pos;
}

/**
 * @param index Vertex index.
 * @return Vertex color; a plain pane is white.
 */
nn::util::Unorm8x4 Pane::GetVertexColor(int index) const {
    nn::util::Unorm8x4 color = {{255, 255, 255, 255}};
    return color;
}

/**
 * @brief Sets a vertex color; a plain pane has none.
 * @param index Vertex index.
 * @param rColor Color.
 */
void Pane::SetVertexColor(int index, const nn::util::Unorm8x4& rColor) {}

/**
 * @param index Color element index.
 * @return Value of the color element.
 */
u8 Pane::GetColorElement(int index) const {
    if (index == AnimTargetPaneColor_Alpha) {
        return mAlpha;
    }

    return GetVertexColorElement(index);
}

/**
 * @brief Sets a color element.
 * @param index Color element index.
 * @param value Value of the color element.
 */
void Pane::SetColorElement(int index, u8 value) {
    if (index == AnimTargetPaneColor_Alpha) {
        mAlpha = value;
        return;
    }

    SetVertexColorElement(index, value);
}

/**
 * @param index Vertex color element index.
 * @return Value of the element; a plain pane is white.
 */
u8 Pane::GetVertexColorElement(int index) const { return 255; }

/**
 * @brief Sets a vertex color element; a plain pane has none.
 * @param index Vertex color element index.
 * @param value Value of the element.
 */
void Pane::SetVertexColorElement(int index, u8 value) {}

/**
 * @brief Finds a pane by name.
 * @param pName Name of the pane.
 * @param isRecursive Whether descendants are searched too.
 * @return The pane, or nullptr when there is none.
 */
Pane* Pane::FindPaneByName(const char* pName, bool isRecursive) {
    if (EqualsResName(mPanelName, pName)) {
        return this;
    }

    if (isRecursive) {
        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            Pane* pFound = FromLink(pNode)->FindPaneByNameRecursive(pName);
            if (pFound != nullptr) {
                return pFound;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Finds a pane by name.
 * @param pName Name of the pane.
 * @param isRecursive Whether descendants are searched too.
 * @return The pane, or nullptr when there is none.
 */
const Pane* Pane::FindPaneByName(const char* pName, bool isRecursive) const {
    return const_cast<Pane*>(this)->FindPaneByName(pName, isRecursive);
}

/**
 * @brief Finds a material by name.
 * @param pName Name of the material.
 * @param isRecursive Whether descendants are searched too.
 * @return The material, or nullptr when there is none.
 */
Material* Pane::FindMaterialByName(const char* pName, bool isRecursive) {
    const u8 count = GetMaterialCount();
    for (int i = 0; i < count; i++) {
        Material* pMaterial = GetMaterial(i);
        if (pMaterial != nullptr && EqualsMaterialName(pMaterial->GetName(), pName)) {
            return pMaterial;
        }
    }

    if (isRecursive) {
        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            Material* pFound = FromLink(pNode)->FindMaterialByNameRecursive(pName);
            if (pFound != nullptr) {
                return pFound;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Finds a material by name.
 * @param pName Name of the material.
 * @param isRecursive Whether descendants are searched too.
 * @return The material, or nullptr when there is none.
 */
const Material* Pane::FindMaterialByName(const char* pName, bool isRecursive) const {
    return const_cast<Pane*>(this)->FindMaterialByName(pName, isRecursive);
}

/** @return Whether the constant buffer of the pane must be built this frame. */
bool Pane::IsConstantBufferUpdateNeeded() const {
    const bool isDrawn = IsVisible() && GetGlobalAlpha() != 0;
    return IsPaneEffectStaticCacheRenderingNeeded() | isDrawn;
}

/** @return Whether a static cache of the pane effect must be rendered. */
bool Pane::IsPaneEffectStaticCacheRenderingNeeded() const {
    if (!IsPaneEffectEnabled()) {
        return false;
    }

    return IsStaticCacheRenderingNeeded(GetPaneEffectInstanceUnchecked());
}

/**
 * @brief Calculates the alpha, the global matrix and the pane effect of the pane and its children.
 * @param rDrawInfo Draw state.
 * @param rContext Calculation context.
 * @param isDirtyParentMtx Whether the parent matrix changed.
 */
void Pane::Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext, bool isDirtyParentMtx) {
    if (!IsVisible() && !rContext.isInvisiblePaneCalculateMtx) {
        if (isDirtyParentMtx) {
            SetGlobalMatrixDirty();
        }

        mFlags &= ~PaneFlag_IsCalculationFinished;
        return;
    }

    if (rContext.isInfluenceAlpha && mParent != nullptr) {
        mAlphaInfluence = static_cast<u8>(rContext.influenceAlpha * mAlpha);
    } else {
        mAlphaInfluence = mAlpha;
    }

    if (IsInfluencedAlpha() && mAlphaInfluence == 0 && !rContext.isAlphaZeroPaneCalculateMtx) {
        if (isDirtyParentMtx) {
            SetGlobalMatrixDirty();
        }

        mFlags &= ~PaneFlag_IsCalculationFinished;
        return;
    }

    bool isDirty = true;
    if (IsUserGlobalMatrix()) {
        // The global matrix is set by the user.
    } else if (IsUserMatrix()) {
        const MatrixT4x3fType& rParentMtx =
            mParent != nullptr ? mParent->GetGlobalMatrix() : *rContext.pViewMtx;
        MatrixMultiply(reinterpret_cast<MatrixT4x3fType*>(mGlobalMtx), rParentMtx, *m_pUserMtx);
    } else if (isDirtyParentMtx | IsGlobalMatrixDirty()) {
        mFlags &= ~PaneFlag_IsGlobalMatrixDirty;
        CalculateGlobalMatrixSelf(rContext);
    } else {
        isDirty = false;
    }

    if (IsInfluencedAlpha() && mAlpha != 255) {
        const float influenceAlpha = rContext.influenceAlpha;
        const bool isInfluenceAlpha = rContext.isInfluenceAlpha;
        rContext.isInfluenceAlpha = true;
        rContext.influenceAlpha = influenceAlpha * mAlpha * (1.0f / 255.0f);

        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            FromLink(pNode)->Calculate(rDrawInfo, rContext, isDirty);
        }

        rContext.influenceAlpha = influenceAlpha;
        rContext.isInfluenceAlpha = isInfluenceAlpha;
    } else {
        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            FromLink(pNode)->Calculate(rDrawInfo, rContext, isDirty);
        }
    }

    if (IsPaneEffectEnabled()) {
        GetPaneEffectInstanceUnchecked()->Calculate(rDrawInfo);
    }

    mFlags |= PaneFlag_IsCalculationFinished;
    mFlagEx |= PaneFlagEx_IsConstantBufferReady;
}

/**
 * @brief Calculates the global matrix of the pane from its parent matrix and its transform.
 * @param rContext Calculation context.
 */
void Pane::CalculateGlobalMatrixSelf(CalculateContext& rContext) {
    const MatrixT4x3fType& rParentMtx =
        mParent != nullptr ? mParent->GetGlobalMatrix() : *rContext.pViewMtx;

    float scaleX = mScaleX;
    float scaleY = mScaleY;
    if (rContext.isLocationAdjust && IsLocationAdjust()) {
        scaleX = rContext.locationAdjustScale.x * scaleX;
        scaleY = rContext.locationAdjustScale.y * scaleY;
    }

    float transX = mPositionX;
    float transY = mPositionY;
    const float transZ = mPositionZ;

    switch (GetParentRelativePositionX()) {
    case 1:
        transX += mParent->mSizeX * -0.5f;
        break;
    case 2:
        transX += mParent->mSizeX * 0.5f;
        break;
    }

    switch (GetParentRelativePositionY()) {
    case 1:
        transY += mParent->mSizeY * 0.5f;
        break;
    case 2:
        transY += mParent->mSizeY * -0.5f;
        break;
    }

    MatrixT4x3fType* pGlobalMtx = reinterpret_cast<MatrixT4x3fType*>(mGlobalMtx);

    if (mRotationX != 0.0f || mRotationY != 0.0f) {
        float sinX, cosX, sinY, cosY, sinZ, cosZ;
        SinCosTable(&sinX, &cosX, DegreeToAngleIndex(mRotationX));
        SinCosTable(&sinY, &cosY, DegreeToAngleIndex(mRotationY));
        SinCosTable(&sinZ, &cosZ, DegreeToAngleIndex(mRotationZ));

        const float opt1 = cosX * cosZ;
        const float opt2 = sinX * sinY;
        const float opt3 = cosX * sinZ;

        MatrixT4x3fType mtx;
        mtx._m.val[0] = float32x4_t{scaleX * (cosY * cosZ), scaleY * (opt2 * cosZ - opt3),
                                    opt1 * sinY + sinX * sinZ, transX};
        mtx._m.val[1] = float32x4_t{scaleX * (cosY * sinZ), scaleY * (opt2 * sinZ + opt1),
                                    opt3 * sinY - sinX * cosZ, transY};
        mtx._m.val[2] = float32x4_t{scaleX * -sinY, scaleY * (sinX * cosY), cosX * cosY, transZ};
        MatrixMultiply(pGlobalMtx, rParentMtx, mtx);
    } else if (mRotationZ != 0.0f) {
        float sinZ, cosZ;
        SinCosTable(&sinZ, &cosZ, DegreeToAngleIndex(mRotationZ));

        MatrixT4x3fType mtx;
        mtx._m.val[0] = float32x4_t{scaleX * cosZ, -sinZ * scaleY, 0.0f, transX};
        mtx._m.val[1] = float32x4_t{scaleX * sinZ, scaleY * cosZ, 0.0f, transY};
        mtx._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, transZ};
        MatrixMultiply(pGlobalMtx, rParentMtx, mtx);
    } else {
        for (int i = 0; i < 3; i++) {
            const float32x4_t row = rParentMtx._m.val[i];
            const float x = vgetq_lane_f32(row, 0);
            const float y = vgetq_lane_f32(row, 1);
            const float z = vgetq_lane_f32(row, 2);
            const float w = vgetq_lane_f32(row, 3);
            pGlobalMtx->_m.val[i] =
                float32x4_t{scaleX * x, scaleY * y, z, w + (transX * x + (transY * y + transZ * z))};
        }
    }
}

/**
 * @brief Updates the system data flags and the reference table from an extended user data block.
 * @param pList Extended user data block, or nullptr.
 */
void Pane::UpdateSystemExtDataFlag(const ResExtUserDataList* pList) {
    if (pList == nullptr || pList->entries[0].type != ExtUserDataType_SystemData) {
        return;
    }

    const u8* pSystem = GetBytes(pList);
    const int count = GetSystemDataHeader(pSystem)->count;
    auto* pTable = const_cast<SystemDataReferenceTable*>(
        static_cast<const SystemDataReferenceTable*>(GetSystemData(pSystem, 0)));

    for (int i = 0; i < count; i++) {
        const SystemDataBase* pData = GetSystemData(pSystem, i);
        m_SystemExtDataFlag |= 1 << pData->type;

        if (pTable->type == PaneSystemDataType_ReferenceTable) {
            pTable->indices[ConvertSystemExtDataTypeToReferenceTableIndex(
                static_cast<PaneSystemDataType>(pData->type))] = i;
        }
    }
}

/**
 * @brief Adds a system data to the extended user data of the pane.
 * @param type Type of the system data.
 * @param pData System data to copy.
 * @param dataSize Size of the system data in bytes.
 * @param isReferenceTable Whether the system data is the reference table.
 */
void Pane::AddDynamicSystemExtUserDataImpl(PaneSystemDataType type, const void* pData, int dataSize,
                                           bool isReferenceTable) {
    if (m_pExtUserDataList == nullptr) {
        AddDynamicSystemExtUserDataAllNewImpl(pData, dataSize);
    } else {
        const ResExtUserDataList* pOldList = m_pExtUserDataList;
        if (!IsExtUserDataMemoryDynamicallyAllocated()) {
            pOldList = nullptr;
        } else if ((mFlagEx & PaneFlagEx_DynamicExtUserDataEnabled) != 0 &&
                   GetSystemExtDataByType(type) != nullptr) {
            return;
        }

        if (m_SystemExtDataFlag == 0) {
            AddDynamicSystemExtUserDataNewSystemDataImpl(pData, dataSize);
        } else {
            AddDynamicSystemExtUserDataToSystemDataImpl(pData, dataSize, isReferenceTable);
        }

        if (pOldList != nullptr) {
            Layout::FreeMemory(const_cast<ResExtUserDataList*>(pOldList));
        }
    }

    mFlagEx |= PaneFlagEx_DynamicExtUserDataEnabled;
    UpdateSystemExtDataFlag(m_pExtUserDataList);
}

/**
 * @brief Creates an extended user data block holding only a system data.
 * @param pData System data to copy.
 * @param dataSize Size of the system data in bytes.
 */
void Pane::AddDynamicSystemExtUserDataAllNewImpl(const void* pData, int dataSize) {
    const u32 listSize = dataSize + (sizeof(ResExtUserDataListHeader) + sizeof(ResExtUserData) +
                                     sizeof(u16) * 2 + sizeof(u32));
    auto* pList = static_cast<ResExtUserDataList*>(Layout::AllocateMemory(listSize));
    pList->signature = ExtUserDataListSignature;
    pList->size = listSize;
    pList->count = 1;

    ResExtUserData& rSystem = pList->entries[0];
    rSystem.nameOffset = 0;
    rSystem.dataOffset = sizeof(ResExtUserData);
    rSystem.count = 1;
    rSystem.type = ExtUserDataType_SystemData;
    rSystem.reserved = 0;

    auto* pHeader = reinterpret_cast<SystemDataHeader*>(reinterpret_cast<u8*>(pList) + 0x18);
    pHeader->version = 0;
    pHeader->count = 1;
    pHeader->offsets[0] = 8;

    mFlagEx |= PaneFlagEx_DynamicExtUserDataEnabled;
    m_pExtUserDataList = pList;
    std::memcpy(reinterpret_cast<u8*>(pList) + 0x20, pData, dataSize);
}

/**
 * @brief Inserts a system data entry in front of the existing extended user data.
 * @param pData System data to copy.
 * @param dataSize Size of the system data in bytes.
 */
void Pane::AddDynamicSystemExtUserDataNewSystemDataImpl(const void* pData, int dataSize) {
    const u32 newSize = dataSize + (m_pExtUserDataList->size + sizeof(ResExtUserData) + 8);
    u8* pBase = static_cast<u8*>(Layout::AllocateMemory(newSize));
    auto* pNew = reinterpret_cast<ResExtUserDataList*>(pBase);

    *reinterpret_cast<ResExtUserDataListHeader*>(pNew) =
        *reinterpret_cast<const ResExtUserDataListHeader*>(m_pExtUserDataList);
    pNew->size = newSize;
    pNew->count = m_pExtUserDataList->count + 1;

    ResExtUserData& rSystem = pNew->entries[0];
    rSystem.nameOffset = 0;
    rSystem.dataOffset = pNew->count * sizeof(ResExtUserData);
    rSystem.count = 1;
    rSystem.type = ExtUserDataType_SystemData;
    rSystem.reserved = 0;

    u32 newPos = ExtUserDataListHeaderSize + sizeof(ResExtUserData);
    u32 oldPos = ExtUserDataListHeaderSize;

    for (int i = 0; i < m_pExtUserDataList->count; i++) {
        const ResExtUserData& rSource = m_pExtUserDataList->entries[i];
        ResExtUserData& rDest = pNew->entries[i + 1];
        rDest.nameOffset = rSource.nameOffset + dataSize + 8;
        rDest.dataOffset = rSource.dataOffset + dataSize + 8;
        rDest.count = rSource.count;
        rDest.type = rSource.type;
        rDest.reserved = 0;
        oldPos += sizeof(ResExtUserData);
        newPos += sizeof(ResExtUserData);
    }

    auto* pHeader = reinterpret_cast<SystemDataHeader*>(pBase + newPos);
    pHeader->version = 0;
    pHeader->count = 1;
    *reinterpret_cast<u32*>(pBase + (newPos + 4)) = 8;

    const u32 dataPos = newPos + 8;
    std::memcpy(pBase + dataPos, pData, dataSize);
    std::memcpy(pBase + (dataPos + dataSize),
                reinterpret_cast<const u8*>(m_pExtUserDataList) + oldPos,
                m_pExtUserDataList->size - m_pExtUserDataList->count * ResExtUserDataSize -
                    ExtUserDataListHeaderSize);
    m_pExtUserDataList = pNew;
}

/**
 * @brief Appends a system data to the existing system data entry.
 * @param pData System data to copy.
 * @param dataSize Size of the system data in bytes.
 * @param isReferenceTable Whether the system data is the reference table, which is placed first.
 */
void Pane::AddDynamicSystemExtUserDataToSystemDataImpl(const void* pData, int dataSize,
                                                       bool isReferenceTable) {
    const u32 addedSize = dataSize + sizeof(u32);
    const u32 newSize = m_pExtUserDataList->size + addedSize;
    u8* pBase = static_cast<u8*>(Layout::AllocateMemory(newSize));
    auto* pNew = reinterpret_cast<ResExtUserDataList*>(pBase);

    *reinterpret_cast<ResExtUserDataListHeader*>(pNew) =
        *reinterpret_cast<const ResExtUserDataListHeader*>(m_pExtUserDataList);
    pNew->size = newSize;
    pNew->count = m_pExtUserDataList->count;

    u32 oldPos = ExtUserDataListHeaderSize;
    u32 newPos = ExtUserDataListHeaderSize;

    for (int i = 0; i < m_pExtUserDataList->count; i++) {
        const ResExtUserData& rSource = m_pExtUserDataList->entries[i];
        ResExtUserData& rDest = pNew->entries[i];
        rDest.nameOffset = rSource.nameOffset + addedSize;
        rDest.dataOffset =
            rSource.dataOffset + (rSource.type == ExtUserDataType_SystemData ? 0 : addedSize);
        rDest.count = rSource.count;
        rDest.type = rSource.type;
        rDest.reserved = 0;
        oldPos += sizeof(ResExtUserData);
        newPos += sizeof(ResExtUserData);
    }

    const u8* pOldBase = reinterpret_cast<const u8*>(m_pExtUserDataList);
    const auto* pOldHeader = reinterpret_cast<const SystemDataHeader*>(pOldBase + oldPos);
    auto* pNewHeader = reinterpret_cast<SystemDataHeader*>(pBase + newPos);
    *reinterpret_cast<u32*>(pNewHeader) = *reinterpret_cast<const u32*>(pOldHeader);
    pNewHeader->count = pOldHeader->count + 1;

    u32 oldOffsetPos = oldPos + 4;
    u32 newOffsetPos = newPos + 4;

    u32 payloadSize;
    if (m_pExtUserDataList->count == 1) {
        payloadSize = m_pExtUserDataList->size - 0x1c - pOldHeader->count * sizeof(u32);
    } else {
        int nextIndex = 1;
        for (int i = 1; i < m_pExtUserDataList->count; i++) {
            if (m_pExtUserDataList->entries[i].type != ExtUserDataType_String) {
                nextIndex = i;
                break;
            }
        }

        payloadSize = nextIndex * sizeof(ResExtUserData) - sizeof(u32) -
                      m_pExtUserDataList->entries[0].dataOffset +
                      m_pExtUserDataList->entries[nextIndex].dataOffset -
                      pOldHeader->count * sizeof(u32);
    }

    u32 writePos;
    if (isReferenceTable) {
        *reinterpret_cast<u32*>(pBase + newOffsetPos) = pNewHeader->count * sizeof(u32) + sizeof(u32);
        newOffsetPos += sizeof(u32);

        for (int i = 0; i < pOldHeader->count; i++) {
            *reinterpret_cast<u32*>(pBase + newOffsetPos) =
                addedSize + *reinterpret_cast<const u32*>(pOldBase + oldOffsetPos);
            newOffsetPos += sizeof(u32);
            oldOffsetPos += sizeof(u32);
        }

        std::memcpy(pBase + newOffsetPos, pData, dataSize);
        std::memcpy(pBase + (newOffsetPos + dataSize), pOldBase + oldOffsetPos, payloadSize);
        writePos = newOffsetPos + dataSize + payloadSize;
    } else {
        for (int i = 0; i < pOldHeader->count; i++) {
            *reinterpret_cast<u32*>(pBase + newOffsetPos) =
                *reinterpret_cast<const u32*>(pOldBase + oldOffsetPos) + sizeof(u32);
            oldOffsetPos += sizeof(u32);
            newOffsetPos += sizeof(u32);
        }

        *reinterpret_cast<u32*>(pBase + newOffsetPos) =
            payloadSize + pNewHeader->count * sizeof(u32) + sizeof(u32);
        newOffsetPos += sizeof(u32);

        std::memcpy(pBase + newOffsetPos, pOldBase + oldOffsetPos, payloadSize);
        std::memcpy(pBase + (newOffsetPos + payloadSize), pData, dataSize);
        writePos = newOffsetPos + payloadSize + dataSize;
    }

    if (m_pExtUserDataList->count >= 2) {
        const u32 restPos = oldOffsetPos + payloadSize;
        std::memcpy(pBase + writePos, reinterpret_cast<const u8*>(m_pExtUserDataList) + restPos,
                    m_pExtUserDataList->size - restPos);
    }

    m_pExtUserDataList = pNew;
}

/**
 * @brief Draws the pane and its children.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer.
 */
void Pane::Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (!IsVisible() || !IsCalculationFinished()) {
        return;
    }

    if (IsConstantBufferReady() && GetGlobalAlpha() != 0) {
        if (IsPaneEffectEnabled()) {
            GetPaneEffectInstanceUnchecked()->Draw(rDrawInfo, rCommands);
        } else {
            DrawSelf(rDrawInfo, rCommands);
        }
    }

    DrawChildren(rDrawInfo, rCommands);
}

/**
 * @brief Draws the children of the pane.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer.
 */
void Pane::DrawChildren(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (IsInfluencedAlpha() && GetGlobalAlpha() == 0) {
        return;
    }

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        FromLink(pNode)->Draw(rDrawInfo, rCommands);
    }
}

/**
 * @brief Draws the pane itself; a plain pane draws nothing.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer.
 */
void Pane::DrawSelf(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {}

/**
 * @brief Binds an animation to the pane.
 * @param pAnimTrans Animation.
 * @param isRecursive Whether the children are bound too.
 * @param isEnabled Whether the animation is enabled.
 */
void Pane::BindAnimation(AnimTransform* pAnimTrans, bool isRecursive, bool isEnabled) {
    pAnimTrans->BindPane(this, isRecursive);
    pAnimTrans->SetEnabled(isEnabled);
}

/**
 * @brief Checks that a copied pane equals its source.
 * @param rOther Source pane.
 * @return Whether the copy is equal.
 */
bool Pane::CompareCopiedInstanceTest(const Pane& rOther) const {
    if (std::memcmp(&mPositionX, &rOther.mPositionX, sizeof(nn::util::Float3)) != 0) {
        return false;
    }

    if (std::memcmp(&mRotationX, &rOther.mRotationX, sizeof(nn::util::Float3)) != 0) {
        return false;
    }

    if (std::memcmp(&mScaleX, &rOther.mScaleX, sizeof(nn::util::Float2)) != 0) {
        return false;
    }

    if (std::memcmp(&mSizeX, &rOther.mSizeX, sizeof(nn::util::Float2)) != 0) {
        return false;
    }

    if (std::memcmp(mGlobalMtx, rOther.mGlobalMtx, sizeof(mGlobalMtx)) != 0) {
        return false;
    }

    const u8 flags = (mFlags & ~PaneFlag_UserMatrix) | PaneFlag_IsGlobalMatrixDirty;
    const u8 otherFlags = (rOther.mFlags & ~PaneFlag_UserMatrix) | PaneFlag_IsGlobalMatrixDirty;
    if (flags != otherFlags) {
        return false;
    }

    if (mAlpha != rOther.mAlpha) {
        return false;
    }

    if (mAlphaInfluence != rOther.mAlphaInfluence) {
        return false;
    }

    if (mOriginFlags != rOther.mOriginFlags) {
        return false;
    }

    if (mFlagEx != rOther.mFlagEx) {
        return false;
    }

    if (m_pUserMtx != rOther.m_pUserMtx) {
        return false;
    }

    if (m_pExtUserDataList != rOther.m_pExtUserDataList) {
        return false;
    }

    if (IsExtUserDataMemoryDynamicallyAllocated() && m_pExtUserDataList == nullptr) {
        return false;
    }

    if (std::strncmp(mPanelName, rOther.mPanelName, sizeof(mPanelName)) != 0) {
        return false;
    }

    return std::strncmp(mUserData, rOther.mUserData, sizeof(mPanelName)) == 0;
}

/**
 * @brief Unbinds an animation from the pane.
 * @param pAnimTrans Animation.
 * @param isRecursive Whether the children are unbound too.
 */
void Pane::UnbindAnimation(AnimTransform* pAnimTrans, bool isRecursive) {
    UnbindAnimationSelf(pAnimTrans);

    if (isRecursive) {
        for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
             pNode = pNode->GetNext()) {
            FromLink(pNode)->UnbindAnimation(pAnimTrans, true);
        }
    }
}

/**
 * @brief Unbinds an animation from the pane and its materials.
 * @param pAnimTrans Animation.
 */
void Pane::UnbindAnimationSelf(AnimTransform* pAnimTrans) {
    const u8 count = GetMaterialCount();
    for (int i = 0; i < count; i++) {
        Material* pMaterial = GetMaterial(i);
        if (pMaterial != nullptr) {
            pAnimTrans->UnbindMaterial(pMaterial);
        }
    }

    pAnimTrans->UnbindPane(this);
}

/**
 * @brief Loads the global matrix of the pane as the model view matrix.
 * @param rDrawInfo Draw state.
 */
void Pane::LoadMtx(DrawInfo& rDrawInfo) {
    CopyMatrix(reinterpret_cast<float*>(&rDrawInfo.m_ModelViewMtx), mGlobalMtx);
    rDrawInfo.mModelViewLoaded = false;
}

/**
 * @brief Calculates the global matrices of the pane and its children.
 * @param rContext Calculation context.
 * @param isDirtyParentMtx Whether the parent matrix changed.
 */
void Pane::CalculateGlobalMatrix(CalculateContext& rContext, bool isDirtyParentMtx) {
    const bool isDirty = isDirtyParentMtx || IsGlobalMatrixDirty();

    if (!IsUserGlobalMatrix() && isDirty) {
        mFlags &= ~PaneFlag_IsGlobalMatrixDirty;
        CalculateGlobalMatrixSelf(rContext);
    }

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        FromLink(pNode)->CalculateGlobalMatrix(rContext, isDirty);
    }
}

/** @return The pane effect of the pane, or nullptr when it has none. */
detail::PaneEffect* Pane::GetPaneEffectInstance() const {
    if (!IsPaneEffectEnabled()) {
        return nullptr;
    }

    return GetPaneEffectInstanceUnchecked();
}

/** @return Whether the pane has a mask. */
bool Pane::IsMaskEnabled() const {
    if (!IsPaneEffectEnabled()) {
        return false;
    }

    return GetPaneEffectInstanceUnchecked()->m_Mask.pData != nullptr;
}

/** @return Whether the pane has a drop shadow. */
bool Pane::IsDropShadowEnabled() const {
    if (!IsPaneEffectEnabled()) {
        return false;
    }

    return GetPaneEffectInstanceUnchecked()->m_DropShadow.pData != nullptr;
}

/** @return The first material of the pane, or nullptr when it has none. */
Material* Pane::GetMaterial() const {
    const u8 count = GetMaterialCount();
    if (count != 0) {
        return GetMaterial(0);
    }

    return nullptr;
}

/** @return Number of materials; a plain pane has none. */
u32 Pane::GetMaterialCount() const { return 0; }

/**
 * @param index Material index.
 * @return The material; a plain pane has none.
 */
Material* Pane::GetMaterial(int index) const {
    static_cast<void>(GetMaterialCount());
    return nullptr;
}

/**
 * @brief Gets the size of the pane including its capture effect.
 * @param pSize Receives the size.
 */
void Pane::GetSizeWithCaptureEffect(Size* pSize) const { *pSize = GetSize(); }

/**
 * @brief Gets the top-left vertex position of the pane including its capture effect.
 * @param pPos Receives the position.
 */
void Pane::GetVertexPosWithCaptureEffect(nn::util::Float2* pPos) const { *pPos = GetVertexPos(); }

/** @return Horizontal spread of italic text; a plain pane has none. */
float Pane::GetItalicSize() const { return 0.0f; }

/** @return Number of user-visible extended user data entries. */
int Pane::GetExtUserDataCount() const {
    if (m_pExtUserDataList == nullptr) {
        return 0;
    }

    return m_pExtUserDataList->count - (m_SystemExtDataFlag != 0 ? 1 : 0);
}

/** @return The user-visible extended user data entries, or nullptr when there are none. */
const ResExtUserData* Pane::GetExtUserDataArray() const {
    if (m_pExtUserDataList == nullptr) {
        return nullptr;
    }

    if (m_SystemExtDataFlag == 0) {
        return GetEntry(m_pExtUserDataList, 0);
    }

    if (m_pExtUserDataList->count == 1) {
        return nullptr;
    }

    return GetEntry(m_pExtUserDataList, 1);
}

/**
 * @param pName Name of the extended user data.
 * @return The extended user data, or nullptr when there is none.
 */
const ResExtUserData* Pane::FindExtUserDataByName(const char* pName) const {
    const ResExtUserData* pArray = GetExtUserDataArray();
    if (pArray == nullptr) {
        return nullptr;
    }

    const u16 count = GetExtUserDataCount();
    for (int i = 0; i < count; i++) {
        if (std::strcmp(pName, pArray[i].GetName()) == 0) {
            return &pArray[i];
        }
    }

    return nullptr;
}

/** @return The animatable extended user data entries, or nullptr when there are none. */
const ResExtUserData* Pane::GetExtUserDataArrayForAnimation() const {
    if ((mFlagEx & PaneFlagEx_ExtUserDataAnimationEnabled) == 0) {
        return nullptr;
    }

    return GetExtUserDataArray();
}

/**
 * @param pName Name of the extended user data.
 * @return The animatable extended user data, or nullptr when there is none.
 */
const ResExtUserData* Pane::FindExtUserDataByNameForAnimation(const char* pName) const {
    if ((mFlagEx & PaneFlagEx_ExtUserDataAnimationEnabled) == 0) {
        return nullptr;
    }

    return FindExtUserDataByName(pName);
}

/**
 * @brief Finds a pane by name in the pane and its descendants.
 * @param pName Name of the pane.
 * @return The pane, or nullptr when there is none.
 */
Pane* Pane::FindPaneByNameRecursive(const char* pName) {
    if (EqualsResName(mPanelName, pName)) {
        return this;
    }

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        Pane* pFound = FromLink(pNode)->FindPaneByNameRecursive(pName);
        if (pFound != nullptr) {
            return pFound;
        }
    }

    return nullptr;
}

/**
 * @brief Finds a pane by name in the pane and its descendants.
 * @param pName Name of the pane.
 * @return The pane, or nullptr when there is none.
 */
const Pane* Pane::FindPaneByNameRecursive(const char* pName) const {
    return const_cast<Pane*>(this)->FindPaneByNameRecursive(pName);
}

/**
 * @brief Finds a material by name in the pane and its descendants.
 * @param pName Name of the material.
 * @return The material, or nullptr when there is none.
 */
Material* Pane::FindMaterialByNameRecursive(const char* pName) {
    const u8 count = GetMaterialCount();
    for (int i = 0; i < count; i++) {
        Material* pMaterial = GetMaterial(i);
        if (pMaterial != nullptr && EqualsMaterialName(pMaterial->GetName(), pName)) {
            return pMaterial;
        }
    }

    for (nn::util::IntrusiveListNode* pNode = m_Children.GetNext(); pNode != &m_Children;
         pNode = pNode->GetNext()) {
        Material* pFound = FromLink(pNode)->FindMaterialByNameRecursive(pName);
        if (pFound != nullptr) {
            return pFound;
        }
    }

    return nullptr;
}

/**
 * @brief Finds a material by name in the pane and its descendants.
 * @param pName Name of the material.
 * @return The material, or nullptr when there is none.
 */
const Material* Pane::FindMaterialByNameRecursive(const char* pName) const {
    return const_cast<Pane*>(this)->FindMaterialByNameRecursive(pName);
}

/**
 * @param type Type of a system data.
 * @return Index of the type in the system data reference table.
 */
int Pane::ConvertSystemExtDataTypeToReferenceTableIndex(PaneSystemDataType type) const {
    return type > 15 ? type - 9 : type;
}

/**
 * @brief Marks the constant buffer as not ready when the pane does not need it this frame.
 * @return True when the pane is not drawn, so that its constant buffer need not be built.
 */
bool Pane::CheckInvisibleAndUpdateConstantBufferReady() {
    if (IsConstantBufferUpdateNeeded()) {
        return false;
    }

    if (IsInfluencedAlpha()) {
        mFlags &= ~PaneFlag_IsCalculationFinished;
    } else {
        ResetConstantBufferReady();
    }

    return true;
}

/**
 * @brief Calculates the root matrix used to capture the pane effect of the pane.
 * @param rMtx Receives the matrix.
 * @param rDrawInfo Draw state.
 */
void Pane::CalculateCaptureRootMatrix(MatrixT4x3fType& rMtx, const DrawInfo& rDrawInfo) const {
    detail::CalculateCaptureRootMatrix(rMtx, rDrawInfo);

    if ((rDrawInfo.mFlags & 8) == 0) {
        return;
    }

    Size size;
    GetSizeWithCaptureEffect(&size);

    float offsetY;
    switch (GetBasePositionY()) {
    case 0:
        offsetY = 0.0f;
        break;
    case 2:
        offsetY = size.height;
        break;
    default:
        offsetY = -size.height;
        break;
    }

    MatrixTranslateY(&rMtx, offsetY);
}

/**
 * @brief Calculates the projection matrix used to capture the pane effect of the pane.
 * @param rMtx Receives the matrix.
 */
void Pane::CalculateCaptureProjectionMatrix(MatrixT4x4fType& rMtx) const {
    Size size;
    GetSizeWithCaptureEffect(&size);
    nn::util::Float2 pos;
    GetVertexPosWithCaptureEffect(&pos);
    MatrixOrthographicOffCenter(&rMtx, pos.x, pos.x + size.width, pos.y, pos.y - size.height);
}

/**
 * @brief Updates the matrices of every material to capture the pane effect of the pane.
 * @param rDrawInfo Draw state.
 */
void Pane::UpdateMaterialConstantBufferForEffectCapture(const DrawInfo& rDrawInfo) {
    MatrixT4x4fType projectionMtx;
    CalculateCaptureProjectionMatrix(projectionMtx);
    MatrixT4x3fType rootMtx;
    CalculateCaptureRootMatrix(rootMtx, rDrawInfo);

    const u8 count = GetMaterialCount();
    for (int i = 0; i < count; i++) {
        auto* pConstantBuffer = static_cast<Material::ConstantBufferForVertexShader*>(
            GetMaterial(i)->GetConstantBufferForVertexShader(rDrawInfo));
        std::memcpy(pConstantBuffer->projection, &projectionMtx, sizeof(projectionMtx));
        std::memcpy(pConstantBuffer->modelView, &rootMtx, sizeof(rootMtx));

        if (IsPaneEffectStaticCacheRenderingNeeded()) {
            pConstantBuffer->color[3] = 1.0f / 255.0f;
        }
    }
}

/**
 * @brief Sets the blend state used to capture the pane effect of the pane.
 * @param rCommands Command buffer.
 * @param rDrawInfo Draw state.
 */
void Pane::UpdateRenderStateForPaneEffectCapture(nn::gfx::CommandBuffer& rCommands,
                                                 const DrawInfo& rDrawInfo) {
    rCommands.SetBlendState(const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource())
                                ->GetPresetBlendState(PresetBlendStateId_OpaqueOrAlphaTest));
}

/** @brief Resets the context to draw without a layout. */
void Pane::CalculateContext::SetDefault() {
    pRectDrawer = nullptr;
    pViewMtx = nullptr;
    locationAdjustScale.x = 1.0f;
    locationAdjustScale.y = 1.0f;
    influenceAlpha = 1.0f;
    isLocationAdjust = false;
    isInvisiblePaneCalculateMtx = false;
    isAlphaZeroPaneCalculateMtx = false;
    isInfluenceAlpha = false;
    pLayoutInformation = nullptr;
}

/**
 * @brief Sets up the context from a draw state.
 * @param rDrawInfo Draw state.
 * @param pLayout Layout being calculated.
 */
void Pane::CalculateContext::Set(const DrawInfo& rDrawInfo, const Layout* pLayout) {
    pRectDrawer = rDrawInfo.GetGraphicsResource()->m_pFontDrawer;
    pViewMtx = &rDrawInfo.m_ViewMtx;
    locationAdjustScale = rDrawInfo.m_LocationAdjustScale;
    influenceAlpha = 1.0f;
    isLocationAdjust = (rDrawInfo.mFlags & 1) != 0;
    isInvisiblePaneCalculateMtx = (rDrawInfo.mFlags & 2) != 0;
    isAlphaZeroPaneCalculateMtx = (rDrawInfo.mFlags & 4) != 0;
    isInfluenceAlpha = false;
    pLayoutInformation = reinterpret_cast<const LayoutInformation*>(pLayout);
}

/** @return Extended alignment settings of the pane, or nullptr when it has none. */
const SystemDataAlignmentExInfo* Pane::FindAlignmentExInfo() const {
    if ((m_SystemExtDataFlag & (1 << PaneSystemDataType_AlignmentExInfo)) == 0) {
        return nullptr;
    }

    return static_cast<const SystemDataAlignmentExInfo*>(
        FindSystemExtData(PaneSystemDataType_AlignmentExInfo));
}

/** @return Whether alignment panes ignore this pane. */
bool Pane::IsAlignmentIgnore() {
    const SystemDataAlignmentExInfo* pInfo = FindAlignmentExInfo();
    if (pInfo != nullptr && (pInfo->flags & 2) != 0) {
        return true;
    }

    return false;
}

/** @return Whether the alignment margin of the pane is enabled. */
bool Pane::IsAlignmentMarginEnabled() {
    const SystemDataAlignmentExInfo* pInfo = FindAlignmentExInfo();
    if (pInfo != nullptr && (pInfo->flags & 1) != 0) {
        return true;
    }

    return false;
}

/** @return Whether alignment panes treat this pane as an empty pane. */
bool Pane::IsAlignmentNullPane() {
    const SystemDataAlignmentExInfo* pInfo = FindAlignmentExInfo();
    if (pInfo != nullptr && (pInfo->flags & 4) != 0) {
        return true;
    }

    return false;
}

/** @return Alignment margin of the pane. */
float Pane::GetAlignmentMargin() {
    const SystemDataAlignmentExInfo* pInfo = FindAlignmentExInfo();
    if (pInfo != nullptr) {
        return pInfo->margin;
    }

    return 0.0f;
}

/**
 * @param type Type of a system data the pane is known to have.
 * @return The system data of the type.
 */
inline const void* Pane::FindSystemExtData(PaneSystemDataType type) const {
    if (m_SystemExtDataFlag == 0) {
        return nullptr;
    }

    const u8* pSystem = GetBytes(m_pExtUserDataList);
    const auto* pTable = static_cast<const SystemDataReferenceTable*>(GetSystemData(pSystem, 0));
    if (pTable->type != PaneSystemDataType_ReferenceTable) {
        return nullptr;
    }

    const int index = pTable->indices[ConvertSystemExtDataTypeToReferenceTableIndex(type)];
    if (index < 0) {
        return nullptr;
    }

    return GetSystemData(pSystem, index);
}

/**
 * @param type Type of a system data the pane is known to have.
 * @return The system data of the type.
 */
inline const void* Pane::GetSystemExtDataByTypeUnchecked(PaneSystemDataType type) const {
    const u8* pSystem = GetBytes(m_pExtUserDataList);
    const auto* pTable = static_cast<const SystemDataReferenceTable*>(GetSystemData(pSystem, 0));
    return GetSystemData(pSystem, pTable->indices[ConvertSystemExtDataTypeToReferenceTableIndex(type)]);
}

/** @return The pane effect of a pane known to have one. */
inline detail::PaneEffect* Pane::GetPaneEffectInstanceUnchecked() const {
    return static_cast<const SystemDataPaneEffectInstance*>(
               GetSystemExtDataByTypeUnchecked(PaneSystemDataType_PaneEffectInstance))
        ->pPaneEffect;
}

/** @brief Releases the states, feature parameters, transitions and animation of the layer. */
inline NOINLINE void StateLayer::Finalize() {
    Layout::FreeMemory(const_cast<char*>(m_pName));
    m_IsPlaying = false;
    m_IsPaused = false;
    m_pName = nullptr;
    m_pCurrentState = nullptr;

    FinalizeStores(m_StoreSet.m_Stores);

    for (FeatureParameterList::iterator it = m_FeatureParameters.begin();
         it != m_FeatureParameters.end();) {
        FeatureParameterList::iterator current = it++;
        FeatureParameter& rParameter = *current;
        Layout::FreeMemory(const_cast<char*>(rParameter.m_pName));
        rParameter.m_pName = nullptr;

        for (int i = 0; i < rParameter.m_AnimInfoCount; i++) {
            SafeFreeMemory(rParameter.m_pAnimInfos[i].pTargets);
            rParameter.m_pAnimInfos[i].pTargets = nullptr;
        }

        SafeFreeMemory(rParameter.m_pAnimInfos);
        rParameter.m_pAnimInfos = nullptr;
        m_FeatureParameters.erase(current);
        Layout::FreeMemory(&rParameter);
    }

    for (TransitionList::iterator it = m_Transitions.begin(); it != m_Transitions.end();) {
        TransitionList::iterator current = it++;
        current->Finalize();
        m_Transitions.erase(current);
        Layout::FreeMemory(&*current);
    }

    for (StateList::iterator it = m_States.begin(); it != m_States.end();) {
        StateList::iterator current = it++;
        current->Finalize();
        m_States.erase(current);
        Layout::FreeMemory(&*current);
    }

    void* pAnimation = m_AnimatorSlot.Unbind();
    if (pAnimation != nullptr && !m_HasPartsStateLayer) {
        Layout::FreeMemory(pAnimation);
    }

    m_pCurrentTransition = nullptr;
    m_AnimatorSlot.Finalzie();
}

/** @brief Releases the names, the condition and the timeline of the transition. */
inline NOINLINE void Transition::Finalize() {
    Layout::FreeMemory(const_cast<char*>(m_pName));
    m_pName = nullptr;
    Layout::FreeMemory(const_cast<char*>(m_pSourceStateName));
    m_pSourceStateName = nullptr;
    Layout::FreeMemory(const_cast<char*>(m_pDestinationStateName));
    m_pDestinationStateName = nullptr;
    Layout::DeleteObj(m_pCondition);
    m_pCondition = nullptr;

    TransitionTimeline* pTimeline = m_pTimeline;
    if (pTimeline->pTracks != nullptr) {
        for (int i = 0; i < pTimeline->trackCount; i++) {
            TransitionTimelineTrack& rTrack = pTimeline->pTracks[i];
            SafeFreeMemory(rTrack.pKeys);
            rTrack.pKeys = nullptr;
            rTrack.keyCount = 0;
        }

        SafeFreeMemory(pTimeline->pTracks);
    }

    pTimeline->pTracks = nullptr;
    pTimeline->trackCount = 0;
    SafeFreeMemory(m_pTimeline);
    m_pTimeline = nullptr;
}

/** @brief Releases the name and the stored values of the state. */
inline NOINLINE void State::Finalize() {
    Layout::FreeMemory(const_cast<char*>(m_pName));
    m_pName = nullptr;
    FinalizeStores(m_StoreSet.m_Stores);
}
}  // namespace nn::ui2d
