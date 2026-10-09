#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_BuildPaneTreeContext.h>
#include <nn/ui2d/ui2d_Capture.h>
#include <nn/ui2d/ui2d_CaptureTexture.h>
#include <nn/ui2d/ui2d_ControlCreator.h>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_Group.h>
#include <nn/ui2d/ui2d_LayoutPaneFactory.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_PaneEffect.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_StateMachine.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/ui2d/ui2d_VectorGraphics.h>
#include <nn/font/font_TagProcessorBase.h>
#include <nn/nn_SdkAssert.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/util/util_BinaryFormat.h>
#include <nn/util/util_StringUtil.h>

#include <attributes.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace nn::ui2d {
namespace {
/**
 * @brief Build a four-character block signature as it is read from a little-endian resource.
 * @param a First character of the signature.
 * @param b Second character of the signature.
 * @param c Third character of the signature.
 * @param d Fourth character of the signature.
 * @return Signature value.
 */
constexpr u32 MakeSignature(char a, char b, char c, char d) {
    return static_cast<u32>(a) | static_cast<u32>(b) << 8 | static_cast<u32>(c) << 16 |
           static_cast<u32>(d) << 24;
}

/** @brief Kinds of the data blocks stored in a layout resource. */
enum DataBlockKind : u32 {
    DataBlockKindLayout = MakeSignature('l', 'y', 't', '1'),
    DataBlockKindControl = MakeSignature('c', 'n', 't', '1'),
    DataBlockKindTextureList = MakeSignature('t', 'x', 'l', '1'),
    DataBlockKindFontList = MakeSignature('f', 'n', 'l', '1'),
    DataBlockKindMaterialList = MakeSignature('m', 'a', 't', '1'),
    DataBlockKindShapeInfoList = MakeSignature('s', 'p', 'i', '1'),
    DataBlockKindCaptureTextureList = MakeSignature('c', 't', 'l', '1'),
    DataBlockKindVectorGraphicsTextureList = MakeSignature('v', 'g', 'l', '1'),
    DataBlockKindPane = MakeSignature('p', 'a', 'n', '1'),
    DataBlockKindPicture = MakeSignature('p', 'i', 'c', '1'),
    DataBlockKindTextBox = MakeSignature('t', 'x', 't', '1'),
    DataBlockKindWindow = MakeSignature('w', 'n', 'd', '1'),
    DataBlockKindBounding = MakeSignature('b', 'n', 'd', '1'),
    DataBlockKindCapture = MakeSignature('c', 'p', 't', '1'),
    DataBlockKindAlignment = MakeSignature('a', 'l', 'i', '1'),
    DataBlockKindScissor = MakeSignature('s', 'c', 'r', '1'),
    DataBlockKindParts = MakeSignature('p', 'r', 't', '1'),
    DataBlockKindPaneChildStart = MakeSignature('p', 'a', 's', '1'),
    DataBlockKindPaneChildEnd = MakeSignature('p', 'a', 'e', '1'),
    DataBlockKindGroup = MakeSignature('g', 'r', 'p', '1'),
    DataBlockKindGroupChildStart = MakeSignature('g', 'r', 's', '1'),
    DataBlockKindGroupChildEnd = MakeSignature('g', 'r', 'e', '1'),
    DataBlockKindUserDataList = MakeSignature('u', 's', 'd', '1'),
    DataBlockKindStateMachine = MakeSignature('s', 't', 'm', '1'),
};

/** @brief Resource type of layout files in the resource accessor. */
const u32 ResourceTypeLayout = MakeSignature('t', 'y', 'l', 'b');
/** @brief Resource type of animation files in the resource accessor. */
const u32 ResourceTypeAnimation = MakeSignature('m', 'i', 'n', 'a');
/** @brief Resource type of vector graphics files in the resource accessor. */
const u32 ResourceTypeVectorGraphics = MakeSignature('g', 'v', 'n', 'b');

/** @brief Header at the start of a layout resource file. */
struct ResBinaryFileHeader {
    u32 signature;
    u16 byteOrder;
    u16 headerSize;
    u32 version;
    u32 fileSize;
    u16 dataBlocks;
    u16 reserved;
};

/** @brief Header shared by every data block of a layout resource file. */
struct ResDataBlockHeader {
    u32 kind;
    u32 size;
};

/** @brief System extended user data listing the animation tags of a layout. */
struct ResSystemDataAnimTagNames {
    u32 type;
    u32 count;
    u32 nameOffsets[1];
};

/**
 * @brief Cast an object to a type by walking its runtime type information.
 * @tparam T Requested type; must provide GetRuntimeTypeInfoStatic.
 * @tparam U Static type of the object.
 * @param pObject Object to cast; may be nullptr.
 * @return Object as T, or nullptr for a null or incompatible object.
 */
template <typename T, typename U>
inline T* DynamicCast(U* pObject) {
    const auto* pWanted = T::GetRuntimeTypeInfoStatic();
    if (pObject == nullptr) {
        return nullptr;
    }

    const auto* pType = pObject->GetRuntimeTypeInfo();
    while (pType != nullptr) {
        if (pType == pWanted) {
            return static_cast<T*>(pObject);
        }

        pType = pType->m_ParentTypeInfo;
    }

    return nullptr;
}

/**
 * @brief Allocate and construct an object through the layout allocator.
 * @tparam T Object type to construct.
 * @tparam TArgs Constructor argument types, passed by value.
 * @param args Constructor arguments.
 * @return New object, or nullptr when allocation fails.
 */
template <typename T, typename... TArgs>
inline T* NewLayoutObject(TArgs... args) {
    void* pMemory = Layout::AllocateMemory(sizeof(T));
    return pMemory != nullptr ? new (pMemory) T(args...) : nullptr;
}

/**
 * @brief Allocate and construct an object with an explicit alignment through the layout allocator.
 * @tparam T Object type to construct.
 * @tparam TArgs Constructor argument types, passed by value.
 * @param alignment Byte alignment of the allocation.
 * @param args Constructor arguments.
 * @return New object, or nullptr when allocation fails.
 */
template <typename T, typename... TArgs>
inline T* NewAlignedLayoutObject(size_t alignment, TArgs... args) {
    void* pMemory = Layout::AllocateMemory(sizeof(T), alignment);
    return pMemory != nullptr ? new (pMemory) T(args...) : nullptr;
}

/**
 * @brief Find a system extended user data entry of the animation-tag type.
 * @param pList Extended user data list of the layout; may be nullptr.
 * @return Animation tag table, or nullptr when the layout has none.
 */
inline const ResSystemDataAnimTagNames* FindAnimTagNames(const ResExtUserDataList* pList) {
    if (pList == nullptr) {
        return nullptr;
    }

    const ResExtUserData& rSystemData = pList->entries[0];
    if (rSystemData.type != 3) {
        return nullptr;
    }

    const u8* pBase = reinterpret_cast<const u8*>(&rSystemData);
    const u32 dataOffset = rSystemData.dataOffset;
    const u16 count = reinterpret_cast<const u16*>(pBase + dataOffset)[1];
    const u32* pOffsets = reinterpret_cast<const u32*>(pBase + dataOffset + 4);
    for (u32 i = 0; i < count; ++i) {
        const auto* pEntry =
            reinterpret_cast<const ResSystemDataAnimTagNames*>(pBase + (pOffsets[i] + dataOffset));
        if (pEntry->type == 0) {
            return pEntry;
        }
    }

    return nullptr;
}

/**
 * @brief Access the capture texture resource stored after the pane part of a capture block.
 * @param pBlock Pane block whose signature has been checked to be a capture pane.
 * @return Capture texture resource of the block.
 */
inline const ResCaptureTexture* GetCaptureTextureResource(const ResPane* pBlock) {
    return reinterpret_cast<const ResCaptureTexture*>(pBlock + 1);
}

/** @brief Mask parameters of a pane effect. */
struct PaneEffectMaskParameters {
    u32 _00;
    u8 flags;
};

/** @brief Drop shadow parameters of a pane effect. */
struct PaneEffectDropShadowParameters {
    u64 _00;
    u8 flags;
};

/**
 * @brief Access the mask parameters of a pane effect.
 * @param pEffect Pane effect instance.
 * @return Mask parameters, or nullptr when the effect has no mask.
 */
inline const PaneEffectMaskParameters* GetMaskParameters(const detail::PaneEffect* pEffect) {
    return *reinterpret_cast<const PaneEffectMaskParameters* const*>(reinterpret_cast<const u8*>(pEffect) + 8);
}

/**
 * @brief Access the drop shadow parameters of a pane effect.
 * @param pEffect Pane effect instance.
 * @return Drop shadow parameters, or nullptr when the effect has no drop shadow.
 */
inline const PaneEffectDropShadowParameters* GetDropShadowParameters(const detail::PaneEffect* pEffect) {
    return *reinterpret_cast<const PaneEffectDropShadowParameters* const*>(reinterpret_cast<const u8*>(pEffect) +
                                                                           88);
}

/**
 * @brief Check whether the mask of a pane effect is rendered into a static cache.
 * @param pEffect Pane effect instance.
 * @return True when the mask uses a static cache.
 */
inline bool IsMaskStaticCacheEnabled(const detail::PaneEffect* pEffect) {
    const auto* pMask = GetMaskParameters(pEffect);
    return pMask != nullptr && (pMask->flags & 2) != 0;
}

/**
 * @brief Check whether the drop shadow of a pane effect is rendered into a static cache.
 * @param pEffect Pane effect instance.
 * @return True when the drop shadow uses a static cache.
 */
inline bool IsDropShadowStaticCacheEnabled(const detail::PaneEffect* pEffect) {
    const auto* pShadow = GetDropShadowParameters(pEffect);
    return pShadow != nullptr && (pShadow->flags & 0x20) != 0;
}

/**
 * @brief Access the flag recording whether the static mask cache holds valid contents.
 * @param pEffect Pane effect instance.
 * @return Reference to the flag.
 */
inline bool& MaskStaticCacheUpdated(detail::PaneEffect* pEffect) {
    return *reinterpret_cast<bool*>(reinterpret_cast<u8*>(pEffect) + 84);
}

/**
 * @brief Access the flag recording whether the static drop shadow cache holds valid contents.
 * @param pEffect Pane effect instance.
 * @return Reference to the flag.
 */
inline bool& DropShadowStaticCacheUpdated(detail::PaneEffect* pEffect) {
    return *reinterpret_cast<bool*>(reinterpret_cast<u8*>(pEffect) + 152);
}
}  // namespace

/**
 * @brief Install allocation callbacks shared by all UI layouts.
 * @param pAllocate Allocation callback receiving byte size, alignment and pArgument.
 * @param pFree Deallocation callback receiving an allocation and pArgument.
 * @param pArgument Opaque context forwarded to both callbacks; may be nullptr.
 */
void Layout::SetAllocator(AllocateFunction pAllocate, FreeFunction pFree, void* pArgument) {
    g_pAllocateFunction = pAllocate;
    g_pFreeFunction = pFree;
    g_pUserDataForAllocator = pArgument;
}

/**
 * @brief Allocate layout storage using the configured callback.
 * @param size Requested byte count.
 * @param alignment Required byte alignment passed to the allocator.
 * @return Allocator result, possibly nullptr on failure.
 */
void* Layout::AllocateMemory(size_t size, size_t alignment) {
    return g_pAllocateFunction(size, alignment, g_pUserDataForAllocator);
}

/**
 * @brief Allocate layout storage with the default four-byte alignment.
 * @param size Requested byte count.
 * @return Allocator result, possibly nullptr on failure.
 */
void* Layout::AllocateMemory(size_t size) {
    return AllocateMemory(size, 4);
}

/**
 * @brief Release layout storage using the configured callback.
 * @param pMemory Allocation returned by the layout allocator; null handling is allocator-defined.
 */
void Layout::FreeMemory(void* pMemory) {
    g_pFreeFunction(pMemory, g_pUserDataForAllocator);
}

/**
 * @brief Configure shared texture-record capacities for subsequent layout builds.
 * @param captureCount Maximum number of capture-texture sharing records.
 * @param vectorCount Maximum number of vector-graphics texture sharing records.
 * @param dynamicCount Maximum number of dynamic-texture sharing records.
 * @param stackCount Maximum nested parts-layout stack depth during texture initialization.
 */
void Layout::SetDynamicTextureInitializationMemoryInfo(int captureCount, int vectorCount, int dynamicCount,
                                                       int stackCount) {
    g_CaptureTextureShareInfoCountMax = captureCount;
    g_VectorGraphicsTextureShareInfoCountMax = vectorCount;
    g_DynamicTextureShareInfoCountMax = dynamicCount;
    g_DynamicTextureShareInfoPartsStackMax = stackCount;
}

/** @brief Construct an empty layout with initialized lists and zero dimensions. */
Layout::Layout()
    : mRootPane(nullptr), _20(nullptr), _30(nullptr), _38(nullptr), mResourceAccessor(nullptr),
      mDynamicTextureList(nullptr) {
    mLayoutSize = {};
}

/** @brief Destroy the layout shell; owned resources must first be released through Finalize. */
Layout::~Layout() {
    NN_SDK_ASSERT(mRootPane == nullptr);
}

namespace {
/**
 * @brief Destroy an object and release its storage through the layout allocator.
 * @tparam T Allocated object type; its destructor may dispatch virtually.
 * @param pObject Object to destroy, or nullptr to do nothing.
 */
template <class T>
inline void DeleteLayoutObject(T* pObject) {
    if (pObject != nullptr) {
        pObject->~T();
        Layout::FreeMemory(pObject);
    }
}
}  // namespace

/**
 * @brief Release owned groups, panes, animations and the dynamic-texture pointer array.
 * @param pDevice Graphics device used to finalize the owned root pane's resources.
 */
void Layout::Finalize(nn::gfx::Device* pDevice) {
    if (mDynamicTextureList != nullptr) {
        if (mDynamicTextureList->pTextures != nullptr) {
            FreeMemory(mDynamicTextureList->pTextures);
        }

        DeleteLayoutObject(mDynamicTextureList);
        mDynamicTextureList = nullptr;
    }

    GetPartsList().clear();
    DeleteLayoutObject(GetGroupContainer());
    _20 = nullptr;
    if (mRootPane != nullptr && !(mRootPane->mFlags & 8)) {
        mRootPane->Finalize(pDevice);
        DeleteLayoutObject(mRootPane);
        mRootPane = nullptr;
    }

    auto& rAnimations = GetAnimTransformList();
    for (auto iter = rAnimations.begin(); iter != rAnimations.end();) {
        Layout::DeleteAnimTransform(&*iter++);
    }

    mLayoutSize = {};
    _30 = nullptr;
    _38 = nullptr;
    mResourceAccessor = nullptr;
}

/**
 * @brief Build the layout from a layout resource and initialize its shared capture textures.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for layout resources.
 * @param pAccessor Resource accessor used to resolve textures, fonts and parts layouts.
 * @param pControlCreator Optional factory for the controls described by the layout.
 * @param pTextSearcher Optional text resolver for text boxes.
 * @param pData Layout resource file.
 * @param rOption Build options.
 * @param isUtf8 Whether the layout's text resources use UTF-8.
 * @return Result of BuildImpl.
 */
bool Layout::Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                   ControlCreator* pControlCreator, TextSearcher* pTextSearcher, const void* pData,
                   const BuildOption& rOption, bool isUtf8) {
    BuildArgSet argSet;
    argSet.magnify.x = 1.0f;
    argSet.magnify.y = 1.0f;
    argSet.partsSize.x = 0.0f;
    argSet.partsSize.y = 0.0f;
    argSet.pControlCreator = pControlCreator;
    argSet.pTextSearcher = pTextSearcher;
    argSet.m_pPartsLayout = nullptr;
    argSet.m_pLayout = this;
    for (int i = 0; i < 8; ++i) {
        argSet.pDynamicTexturePrefixNames[i] = nullptr;
    }

    argSet.mDynamicTexturePrefixDepth = 0;
    argSet.mAlternateDynamicTexturePrefixDepth = -1;

    const size_t contextMemorySize = detail::BuildPaneTreeContext::CalculateContextRequireMemorySize();
    detail::BuildPaneTreeContext context(__builtin_alloca(contextMemorySize), contextMemorySize);
    argSet.pBuildPaneTreeContext = &context;
    argSet._A8 = rOption.GetFlag(25);
    argSet.isRootPaneParts = rOption.GetFlag(0);
    argSet.isUtf8 = isUtf8;
    argSet._D2 = rOption.GetFlag(24);
    argSet._B0 = reinterpret_cast<void*>(rOption._8);
    argSet._B8 = reinterpret_cast<void*>(rOption._10);
    argSet.resourceVersion = static_cast<const ResBinaryFileHeader*>(pData)->version;
    argSet._D8 = rOption.GetFlag(26);
    argSet.isCaptureTextureAllAllocateInitialized = rOption.GetFlag(27);
    argSet._DA = rOption.GetFlag(28);
    argSet.isIgnoreCaptureEffectFirstFrameOnly = rOption.GetFlag(29);
    argSet._DC = rOption.GetFlag(30);
    argSet.captureNestDepth = 0;

    const bool result = BuildImpl(pResult, pDevice, pData, pAccessor, argSet, nullptr);
    if (context.IsInitialized()) {
        context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
        context.Finalize();
    }

    return result;
}

namespace detail {
/**
 * @brief Calculate the scratch memory needed by a build context and its instance pool.
 * @return Byte size covering the share-info stack, list head, pool and all pooled records.
 */
size_t BuildPaneTreeContext::CalculateContextRequireMemorySize() {
    size_t size = sizeof(DynamicTextureShareInfo*) * Layout::g_DynamicTextureShareInfoPartsStackMax;
    size += sizeof(DynamicTextureShareInfoList);
    size += sizeof(BuildPaneTreeContextInstancePool);
    size += sizeof(CaptureTextureShareInfo) * Layout::g_CaptureTextureShareInfoCountMax;
    size += sizeof(VectorGraphicsTextureShareInfo) * Layout::g_VectorGraphicsTextureShareInfoCountMax;
    size += sizeof(DynamicTextureShareInfo) * Layout::g_DynamicTextureShareInfoCountMax;
    return size;
}

/**
 * @brief Check whether Initialize created the instance pool.
 * @return True once the context can record shared textures.
 */
bool BuildPaneTreeContext::IsInitialized() const {
    return m_pInstancePool != nullptr;
}

/**
 * @brief Initialize every capture texture recorded during the build once all panes exist.
 * @param pDevice Graphics device used to create the render targets.
 */
void BuildPaneTreeContext::InitializeCaptureTexturesAfterPaneTreeBuilt(nn::gfx::Device* pDevice) {
    for (auto iter = m_pShareInfoList->begin(); iter != m_pShareInfoList->end();) {
        DynamicTextureShareInfo& rShareInfo = *iter++;
        const int count = rShareInfo.GetCaptureTextureShareInfoCount();
        Layout* pLayout = rShareInfo.GetLayout();
        for (int i = 0; i < count; ++i) {
            const CaptureTextureShareInfo* pInfo = rShareInfo.GetCaptureTextureShareInfoByIndex(i);
            if (pInfo->pCaptureTexture->IsInitialized()) {
                continue;
            }

            Pane* pTargetPane = pLayout->GetRootPane()->FindPaneByName(pInfo->pPaneName, true);
            if (pInfo->pResCaptureTexture != nullptr) {
                pInfo->pCaptureTexture->Initialize(pDevice, pLayout, pInfo->pResCaptureTexture, pTargetPane,
                                                   rShareInfo.IsCaptureTextureAllAllocateInitialized());
                if (rShareInfo.IsIgnoreCaptureEffectFirstFrameOnlyFlag()) {
                    pInfo->pCaptureTexture->m_Flags &= ~4;
                    pInfo->pCaptureTexture->m_Flags &= ~2;
                }
            } else if (pInfo->pCopySource != nullptr) {
                pInfo->pCaptureTexture->Initialize(pDevice, pLayout, *pInfo->pCopySource, pTargetPane);
            }
        }
    }
}

/** @brief Release the instance pool and forget the scratch memory. */
void BuildPaneTreeContext::Finalize() {
    if (m_pInstancePool != nullptr) {
        m_pInstancePool->~BuildPaneTreeContextInstancePool();
        m_pInstancePool = nullptr;
    }

    m_pShareInfoList = nullptr;
    m_pShareInfoStack = nullptr;
    m_pMemory = nullptr;
}
}  // namespace detail

/**
 * @brief Resolve a layout file and build it when the resource accessor finds it.
 * @param pResult Optional build-result storage forwarded to Build.
 * @param pDevice Graphics device used for layout resources.
 * @param pAccessor Resource accessor used to resolve the file and build dependencies.
 * @param pControlCreator Control factory forwarded to Build.
 * @param pTextSearcher Text resolver forwarded to Build.
 * @param rOption Build options forwarded without modification.
 * @param pName Complete layout resource filename to resolve.
 * @param isUtf8 Whether the layout's text resources use UTF-8.
 * @return Build's result, or false when the named resource is absent.
 */
bool Layout::BuildWithName(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                           ResourceAccessor* pAccessor, ControlCreator* pControlCreator,
                           TextSearcher* pTextSearcher, const BuildOption& rOption, const char* pName,
                           bool isUtf8) {
    const void* pData = pAccessor->FindResourceByName(ResourceTypeLayout, pName);
    if (pData == nullptr) {
        return false;
    }

    return Build(pResult, pDevice, pAccessor, pControlCreator, pTextSearcher, pData, rOption, isUtf8);
}

/**
 * @brief Count the animation tags listed in the layout's system user data.
 * @return Number of animation tags, or zero when the layout lists none.
 */
int Layout::AcquireAnimTagNameCount() const {
    const auto* pTagNames = FindAnimTagNames(static_cast<const ResExtUserDataList*>(_38));
    return pTagNames != nullptr ? pTagNames->count : 0;
}

/**
 * @brief Access an animation tag name listed in the layout's system user data.
 * @param index Index of the tag; must be below AcquireAnimTagNameCount.
 * @return Null-terminated tag name.
 */
const char* Layout::AcquireAnimTagNameByIndex(int index) const {
    const uintptr_t tagNames =
        reinterpret_cast<uintptr_t>(FindAnimTagNames(static_cast<const ResExtUserDataList*>(_38)));
    const u32* pNameOffsets = reinterpret_cast<const u32*>(tagNames + offsetof(ResSystemDataAnimTagNames, nameOffsets));
    return reinterpret_cast<const char*>(tagNames + pNameOffsets[index]);
}

/**
 * @brief Build one pane block, applying the overrides of the enclosing parts pane.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for pane resources.
 * @param pData Pane block to build.
 * @param pPartsBuildDataSet Overrides of the enclosing parts pane, or nullptr at the layout root.
 * @param rBuildArgSet Build arguments; override and prefix fields are updated for the pane.
 * @param rBuildResSet Resource blocks of the layout being built.
 * @param kind Data block kind of pData.
 * @return Built pane, or nullptr when the factory did not create one.
 */
Pane* Layout::BuildPartsImpl(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const void* pData,
                             const PartsBuildDataSet* pPartsBuildDataSet, BuildArgSet& rBuildArgSet,
                             BuildResSet& rBuildResSet, u32 kind) {
    const auto* pResPane = static_cast<const ResPane*>(pData);
    rBuildArgSet.overridePaneUsageFlag = 0;
    rBuildArgSet.overrideBasicUsageFlag = 0;
    rBuildArgSet.overrideUsageFlag = 0;
    rBuildArgSet.pOverridePartsPaneBasicInfo = nullptr;
    rBuildArgSet.pOverrideExtUserDataList = nullptr;

    const void* pOverrideBlock = nullptr;
    Pane* pPane = nullptr;
    if (pPartsBuildDataSet != nullptr) {
        if (mRootPane == nullptr) {
            pPane = pPartsBuildDataSet->m_pPartsPane;
        } else {
            const ResPartsProperty* pProperty = pPartsBuildDataSet->FindPartsPropertyFromName(pResPane->name);
            if (pProperty != nullptr) {
                pOverrideBlock = pPartsBuildDataSet->GetPropertyResBlock(pProperty);
                rBuildArgSet.overridePaneUsageFlag = pProperty->usageFlag;
                rBuildArgSet.overrideBasicUsageFlag = pProperty->basicUsageFlag;
                rBuildArgSet.overrideUsageFlag = pProperty->materialUsageFlag;
                rBuildArgSet.pOverridePartsPaneBasicInfo =
                    pPartsBuildDataSet->GetPartsPaneBasicInfoResBlock(pProperty);

                bool isOverride;
                const ResExtUserDataList* pExtUserDataList =
                    pPartsBuildDataSet->GetExtUserDataListResBlock(&isOverride, pProperty);
                if (isOverride) {
                    rBuildArgSet.pExtUserDataList = pExtUserDataList;
                }

                if (pProperty->systemExtUserDataOverrideFlag != 0) {
                    rBuildArgSet.pOverrideExtUserDataList =
                        pPartsBuildDataSet->GetExtUserDataListResBlockRaw(pProperty);
                }
            }
        }
    } else if (mRootPane == nullptr && rBuildArgSet.isRootPaneParts) {
        ResParts resParts;
        static_cast<ResPane&>(resParts) = *pResPane;
        resParts.signature = DataBlockKindParts;
        resParts.propertyCount = 0;
        resParts.magnify.x = 1.0f;
        resParts.magnify.y = 1.0f;
        pPane = BuildPaneObj(pResult, pDevice, DataBlockKindParts, &resParts, nullptr, rBuildArgSet);
        static_cast<Parts*>(pPane)->m_pLayout = this;
    }

    if (pPane == nullptr) {
        pPane = BuildPaneObj(pResult, pDevice, kind, pData, pOverrideBlock, rBuildArgSet);
        if (pPane == nullptr) {
            return pPane;
        }
    }

    if (mRootPane == nullptr) {
        mRootPane = pPane;
    }

    if (rBuildArgSet.pParentPane != nullptr) {
        rBuildArgSet.pParentPane->AppendChild(pPane);
    }

    if (kind == DataBlockKindParts) {
        Parts* pParts = static_cast<Parts*>(pPane);
        const auto* pResParts =
            static_cast<const ResParts*>(pOverrideBlock == nullptr ? pData : pOverrideBlock);
        PartsBuildDataSet partsBuildDataSet(
            pParts, pResParts, pOverrideBlock == nullptr ? &rBuildResSet : rBuildArgSet.pOverrideBuildResSet,
            reinterpret_cast<const ResVec2*>(&pResPane->size));
        if (rBuildArgSet.mAlternateDynamicTexturePrefixDepth < 0 && partsBuildDataSet.IsOverwriting()) {
            rBuildArgSet.mAlternateDynamicTexturePrefixDepth = rBuildArgSet.mDynamicTexturePrefixDepth;
        }

        rBuildArgSet.pDynamicTexturePrefixNames[rBuildArgSet.mDynamicTexturePrefixDepth] = pPane->GetName();
        rBuildArgSet.mDynamicTexturePrefixDepth++;

        const auto* pLayoutName = reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(pResParts) +
            (sizeof(ResParts) + sizeof(ResPartsProperty) * pResParts->propertyCount));
        Layout* pPartsLayout = BuildPartsLayout(pResult, pDevice, pLayoutName, partsBuildDataSet, rBuildArgSet);
        if (pPartsLayout != nullptr) {
            pParts->m_pLayout = pPartsLayout;
            GetPartsList().push_back(*pParts);
        }

        rBuildArgSet.mDynamicTexturePrefixDepth--;
        if (rBuildArgSet.mAlternateDynamicTexturePrefixDepth == 0) {
            rBuildArgSet.mAlternateDynamicTexturePrefixDepth = -1;
        }
    }

    return pPane;
}

/**
 * @brief Inherit the build arguments that stay constant from a parent layout's build.
 * @param rArgSet Arguments of the nested build to fill.
 * @param rParentArgSet Arguments of the enclosing build.
 */
void SetBuildArgSetFromParent(BuildArgSet& rArgSet, const BuildArgSet& rParentArgSet) {
    rArgSet.pControlCreator = rParentArgSet.pControlCreator;
    rArgSet.pTextSearcher = rParentArgSet.pTextSearcher;
    rArgSet.m_pLayout = rParentArgSet.m_pLayout;
    rArgSet.isRootPaneParts = rParentArgSet.isRootPaneParts;
    rArgSet.isUtf8 = rParentArgSet.isUtf8;
    rArgSet._D2 = rParentArgSet._D2;
    std::memcpy(rArgSet.pDynamicTexturePrefixNames, rParentArgSet.pDynamicTexturePrefixNames,
                sizeof(rArgSet.pDynamicTexturePrefixNames));

    rArgSet.mDynamicTexturePrefixDepth = rParentArgSet.mDynamicTexturePrefixDepth;
    rArgSet.mAlternateDynamicTexturePrefixDepth = rParentArgSet.mAlternateDynamicTexturePrefixDepth;
    rArgSet._B0 = rParentArgSet._B0;
    rArgSet._B8 = rParentArgSet._B8;
    rArgSet.pBuildPaneTreeContext = rParentArgSet.pBuildPaneTreeContext;
    rArgSet._A8 = rParentArgSet._A8;
    rArgSet._D8 = rParentArgSet._D8;
    rArgSet.isCaptureTextureAllAllocateInitialized = rParentArgSet.isCaptureTextureAllAllocateInitialized;
    rArgSet._DA = rParentArgSet._DA;
    rArgSet.isIgnoreCaptureEffectFirstFrameOnly = rParentArgSet.isIgnoreCaptureEffectFirstFrameOnly;
    rArgSet._DC = rParentArgSet._DC;
    rArgSet.captureNestDepth = rParentArgSet.captureNestDepth;
}

namespace {
/**
 * @brief Attach the user data block that may follow a pane, control or layout block.
 * @param rContext Build cursor receiving the user data list.
 * @param pFileHeader Header of the layout resource.
 * @param ppBlock Current block; advanced to the user data block when one follows.
 * @param pBlockIndex Index of the current block; advanced with ppBlock.
 */
inline void ReadExtUserDataBlock(Layout::LayoutBuildContext& rContext, const ResBinaryFileHeader* pFileHeader,
                                 const ResDataBlockHeader** ppBlock, int* pBlockIndex) {
    if (*pBlockIndex + 1 < pFileHeader->dataBlocks) {
        const auto* pNextBlock = reinterpret_cast<const ResDataBlockHeader*>(
            reinterpret_cast<const u8*>(*ppBlock) + (*ppBlock)->size);
        if (pNextBlock->kind == DataBlockKindUserDataList) {
            *ppBlock = pNextBlock;
            (*pBlockIndex)++;
            rContext.pExtUserDataList = reinterpret_cast<const ResExtUserDataList*>(pNextBlock);
            return;
        }
    }

    rContext.pExtUserDataList = nullptr;
}
}  // namespace

/**
 * @brief Interpret the current block of a layout resource and locate the following block.
 * @param rContext Build cursor; its current block, user data and nesting state are updated.
 * @param pData Layout resource file containing the block.
 */
void ReadBlock(Layout::LayoutBuildContext& rContext, const void* pData) {
    const auto* pFileHeader = static_cast<const ResBinaryFileHeader*>(pData);
    const auto* pBlock = static_cast<const ResDataBlockHeader*>(rContext.pBlock);
    int blockIndex = rContext.blockIndex;
    rContext.blockKind = pBlock->kind;
    switch (rContext.blockKind) {
    case DataBlockKindLayout: {
        const auto* pResLayout = reinterpret_cast<const ResLayout*>(pBlock);
        rContext.buildArgSet.partsSize.x = pResLayout->partsSize.x;
        rContext.buildArgSet.partsSize.y = pResLayout->partsSize.y;
        ReadExtUserDataBlock(rContext, pFileHeader, &pBlock, &blockIndex);
        break;
    }
    case DataBlockKindPane:
    case DataBlockKindPicture:
    case DataBlockKindTextBox:
    case DataBlockKindWindow:
    case DataBlockKindBounding:
    case DataBlockKindCapture:
    case DataBlockKindAlignment:
    case DataBlockKindScissor:
    case DataBlockKindParts:
    case DataBlockKindControl:
        ReadExtUserDataBlock(rContext, pFileHeader, &pBlock, &blockIndex);
        break;
    case DataBlockKindTextureList:
        rContext.buildResSet.pTextureList = pBlock;
        break;
    case DataBlockKindFontList:
        rContext.buildResSet.pFontList = reinterpret_cast<const ResFontList*>(pBlock);
        break;
    case DataBlockKindMaterialList:
        rContext.buildResSet.pMaterialList = pBlock;
        break;
    case DataBlockKindShapeInfoList:
        rContext.buildResSet.pShapeInfoList = pBlock;
        break;
    case DataBlockKindCaptureTextureList:
        rContext.buildResSet.pCaptureTextureList = pBlock;
        break;
    case DataBlockKindVectorGraphicsTextureList:
        rContext.buildResSet.pVectorGraphicsTextureList = pBlock;
        break;
    case DataBlockKindPaneChildStart:
        rContext.buildArgSet.pParentPane = rContext.pLastBuiltPane;
        if (DynamicCast<Capture>(rContext.pLastBuiltPane) != nullptr ||
            rContext.buildArgSet.captureNestDepth > 0) {
            rContext.buildArgSet.captureNestDepth++;
        }

        break;
    case DataBlockKindPaneChildEnd:
        if (rContext.buildArgSet.captureNestDepth > 0) {
            rContext.buildArgSet.captureNestDepth--;
        }

        rContext.pLastBuiltPane = rContext.buildArgSet.pParentPane;
        if (rContext.pLastBuiltPane != nullptr) {
            rContext.buildArgSet.pParentPane = rContext.pLastBuiltPane->GetParent();
        }

        break;
    case DataBlockKindGroupChildStart:
        rContext.groupNestLevel++;
        break;
    case DataBlockKindGroupChildEnd:
        rContext.groupNestLevel--;
        break;
    default:
        break;
    }

    rContext.nextBlockIndex = blockIndex + 1;
    rContext.pNextBlock = blockIndex + 1 < pFileHeader->dataBlocks ?
                              reinterpret_cast<const u8*>(pBlock) + pBlock->size :
                              nullptr;
}

/**
 * @brief Start reading a layout resource at its first data block.
 * @param rContext Build cursor to position on the first block.
 * @param pData Layout resource file.
 */
void ReadFirstBlock(Layout::LayoutBuildContext& rContext, const void* pData) {
    const auto* pFileHeader = static_cast<const ResBinaryFileHeader*>(pData);
    rContext.pBlock = static_cast<const u8*>(pData) + pFileHeader->headerSize;
    ReadBlock(rContext, pData);
}

/**
 * @brief Advance the build cursor to the next data block.
 * @param rContext Build cursor positioned on the current block.
 * @param pData Layout resource file.
 * @return False when the current block was the last one.
 */
bool ReadNextBlock(Layout::LayoutBuildContext& rContext, const void* pData) {
    if (rContext.pNextBlock == nullptr) {
        return false;
    }

    rContext.pBlock = rContext.pNextBlock;
    rContext.blockIndex = rContext.nextBlockIndex;
    ReadBlock(rContext, pData);
    return true;
}

/**
 * @brief Find a pane block of a layout resource by its name.
 * @param ppResPane Receives the matching pane block, or nullptr.
 * @param ppExtUserDataList Receives the user data following the pane block, or nullptr.
 * @param pLayoutResource Layout resource file to search.
 * @param pName Pane name to find.
 * @param pSkipUntil Pane block after which the search starts, or nullptr to search every pane.
 */
void Layout::FindResPaneByName(const ResPane** ppResPane, const ResExtUserDataList** ppExtUserDataList,
                               const void* pLayoutResource, const char* pName, const ResPane* pSkipUntil) {
    *ppResPane = nullptr;
    *ppExtUserDataList = nullptr;

    LayoutBuildContext context = {};
    context.buildArgSet.resourceVersion = static_cast<const ResBinaryFileHeader*>(pLayoutResource)->version;
    context.buildArgSet.pCurrentBuildResSet = &context.buildResSet;
    ReadFirstBlock(context, pLayoutResource);
    do {
        switch (context.blockKind) {
        case DataBlockKindPane:
        case DataBlockKindPicture:
        case DataBlockKindTextBox:
        case DataBlockKindWindow:
        case DataBlockKindBounding:
        case DataBlockKindCapture:
        case DataBlockKindAlignment:
        case DataBlockKindScissor:
        case DataBlockKindParts: {
            const auto* pResPane = static_cast<const ResPane*>(context.pBlock);
            if (pSkipUntil != nullptr) {
                if (pSkipUntil == pResPane) {
                    pSkipUntil = nullptr;
                }
            } else if (std::strcmp(pResPane->name, pName) == 0) {
                *ppExtUserDataList = context.pExtUserDataList;
                *ppResPane = pResPane;
                return;
            }

            break;
        }
        default:
            break;
        }
    } while (ReadNextBlock(context, pLayoutResource));
}

/**
 * @brief Prepare the build arguments of a layout from its parent build and parts pane.
 * @param rArgSet Arguments to fill.
 * @param rParentArgSet Arguments of the enclosing build.
 * @param pPartsBuildDataSet Parts pane being built, or nullptr for a top-level layout.
 */
void Layout::PrepareBuildArgSet(BuildArgSet& rArgSet, const BuildArgSet& rParentArgSet,
                                const PartsBuildDataSet* pPartsBuildDataSet) {
    if (pPartsBuildDataSet != nullptr) {
        rArgSet.magnify = pPartsBuildDataSet->m_Magnify;
        rArgSet.pOverrideBuildResSet = pPartsBuildDataSet->m_pPropertyBuildResSet;
    } else {
        rArgSet.magnify.x = 1.0f;
        rArgSet.magnify.y = 1.0f;
        rArgSet.pOverrideBuildResSet = nullptr;
    }

    SetBuildArgSetFromParent(rArgSet, rParentArgSet);
    rArgSet.overridePaneUsageFlag = 0;
    rArgSet.overrideUsageFlag = 0;
    rArgSet.pParentPane = nullptr;
    rArgSet.partsSize.x = 0.0f;
    rArgSet.partsSize.y = 0.0f;
}

/**
 * @brief Apply the layout block that is the current block of the build cursor.
 * @param rContext Build cursor positioned on a layout block.
 */
void Layout::SetByResLayout(LayoutBuildContext& rContext) {
    const auto* pResLayout = static_cast<const ResLayout*>(rContext.pBlock);
    mLayoutSize.x = pResLayout->layoutSize.x;
    mLayoutSize.y = pResLayout->layoutSize.y;
    _30 = const_cast<char*>(pResLayout->GetName());
    _38 = const_cast<ResExtUserDataList*>(rContext.pExtUserDataList);
}

/**
 * @brief Create the control described by the current block through the control creator.
 * @param rContext Build cursor positioned on a control block.
 * @param pDevice Graphics device forwarded to the control creator.
 */
void Layout::BuildControl(LayoutBuildContext& rContext, nn::gfx::Device* pDevice) {
    if (rContext.buildArgSet.pControlCreator != nullptr) {
        ControlSrc controlSrc(rContext.pBlock, rContext.pExtUserDataList);
        rContext.buildArgSet.pControlCreator->CreateControl(pDevice, this, controlSrc);
    }
}

/**
 * @brief Build the pane described by the current block and remember it as the last built pane.
 * @param rContext Build cursor positioned on a pane block.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for pane resources.
 * @param pPartsBuildDataSet Overrides of the enclosing parts pane, or nullptr.
 */
void Layout::BuildPaneByResPane(LayoutBuildContext& rContext, BuildResultInformation* pResult,
                                nn::gfx::Device* pDevice, const PartsBuildDataSet* pPartsBuildDataSet) {
    rContext.buildArgSet.pExtUserDataList = rContext.pExtUserDataList;
    Pane* pPane = BuildPartsImpl(pResult, pDevice, rContext.pBlock, pPartsBuildDataSet, rContext.buildArgSet,
                                 rContext.buildResSet, rContext.blockKind);
    if (pPane != nullptr) {
        rContext.pLastBuiltPane = pPane;
    }
}

namespace {
/** @brief System extended user data attaching a state machine to the root pane. */
struct SystemDataStateMachine {
    PaneSystemDataType type;
    StateMachine* pStateMachine;
};
}  // namespace

/**
 * @brief Build the state machine block and apply the variable overrides of the enclosing parts pane.
 * @param rContext Build cursor positioned on a state machine block.
 * @param pDevice Graphics device used by the state machine.
 * @param pPartsBuildDataSet Overrides of the enclosing parts pane, or nullptr.
 */
void Layout::BuildStateMachine(LayoutBuildContext& rContext, nn::gfx::Device* pDevice,
                               const PartsBuildDataSet* pPartsBuildDataSet) {
    const auto* pSystemData = static_cast<const SystemDataStateMachine*>(
        mRootPane->GetSystemExtDataByType(PaneSystemDataType_StateMachine));
    if (pSystemData != nullptr && pSystemData->pStateMachine != nullptr) {
        return;
    }

    StateMachine* pStateMachine = Layout::NewObj<StateMachine>();
    StateMachineFactory factory(pDevice, this);
    factory.Build(pStateMachine, const_cast<void*>(rContext.pBlock));

    SystemDataStateMachine systemData;
    systemData.type = PaneSystemDataType_StateMachine;
    systemData.pStateMachine = pStateMachine;
    mRootPane->AddDynamicSystemExtUserData(PaneSystemDataType_StateMachine, &systemData, sizeof(systemData));
    if (pPartsBuildDataSet == nullptr) {
        return;
    }

    for (auto& rStateLayer : pStateMachine->m_StateLayers) {
        const char* pName = rStateLayer.m_pName;
        const ResPartsProperty* pProperty = pPartsBuildDataSet->FindPartsPropertyFromName(pName);
        if (pProperty == nullptr) {
            continue;
        }

        const float overrideValue = *reinterpret_cast<const float*>(
            reinterpret_cast<const u8*>(pPartsBuildDataSet->m_pResParts) + pProperty->propertyOffset);
        StateMachineVariableManager& rVariableManager = pStateMachine->m_VariableManager;
        if (rVariableManager.FindRefOnlyByName_(pName) == nullptr) {
            continue;
        }

        const float previousValue = rVariableManager.FindRefOnlyByName_(pName)->value;
        float value = overrideValue;
        StateMachineVariable* pVariable = rVariableManager.FindByName_(pName);
        detail::ClampValue(value, pVariable->minimum, pVariable->maximum);
        const float oldValue = pVariable->value;
        if (value != oldValue) {
            pVariable->value = value;
            rVariableManager.PushModifyEvent_(pVariable->name, pVariable);
            for (auto* pNode = pVariable->calculatedVariables.GetNext(); pNode != &pVariable->calculatedVariables;
                 pNode = pNode->GetNext()) {
                rVariableManager.DoUpdateCalcVarOnValueChanged_(
                    reinterpret_cast<StateMachineCalclatedVariable*>(pNode), oldValue, value);
            }
        }

        const StateMachineVariable* pCurrent = rVariableManager.FindRefOnlyByName_(pName);
        if (pStateMachine->m_pListener != nullptr) {
            pStateMachine->m_pListener->OnVariableChanged(pStateMachine->m_pName, pName, previousValue,
                                                          pCurrent->value);
        }
    }
}

/**
 * @brief Build a state machine from its resource block.
 * @param pStateMachine State machine to fill.
 * @param pResource State machine block; relocated in place on first use.
 */
inline void StateMachineFactory::Build(StateMachine* pStateMachine, void* pResource) {
    auto* pResStateMachine = static_cast<ResStateMachine*>(pResource);
    DoRelocate_(pResStateMachine, pResource);
    pStateMachine->m_pLayout = m_pLayout;
    pStateMachine->m_pName = pResStateMachine->name;
    pStateMachine->m_EventQueue.Initialize();
    pStateMachine->m_VariableManager.m_pStateMachine = pStateMachine;
    pStateMachine->m_VariableManager.m_pEventQueue = &pStateMachine->m_EventQueue;
    pStateMachine->m_pEventHandler = Layout::NewObj<StateMachineEventHandler>(pStateMachine);
    for (u32 i = 0; i < pResStateMachine->layerCount; ++i) {
        pStateMachine->m_StateLayers.push_back(*BuildStateLayer_(&pResStateMachine->pLayers[i]));
    }

    for (u32 i = 0; i < pResStateMachine->variableCount; ++i) {
        pStateMachine->m_VariableManager.RegisterNewVariableByResource(&pResStateMachine->pVariables[i]);
    }

    const void* pUserData = pResStateMachine->_38;
    const int userDataCount = pResStateMachine->_44;
    pStateMachine->_C0 = pUserData;
    pStateMachine->_C8 = userDataCount;
}

/**
 * @brief Create the group container or a top-level group from the current block.
 * @param rContext Build cursor positioned on a group block.
 */
void Layout::BuildGroup(LayoutBuildContext& rContext) {
    GroupContainer* pGroupContainer = GetGroupContainer();
    if (pGroupContainer == nullptr) {
        _20 = NewLayoutObject<GroupContainer>();
        return;
    }

    if (rContext.groupNestLevel == 1) {
        pGroupContainer->AppendGroup(
            NewLayoutObject<Group>(static_cast<const ResGroup*>(rContext.pBlock), mRootPane));
    }
}

/**
 * @brief Build the pane tree, groups and controls of a layout resource.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for layout resources.
 * @param pData Layout resource file.
 * @param pResourceAccessor Resource accessor used for the layout's dependencies.
 * @param rParentBuildArgSet Build arguments of the enclosing build.
 * @param pPartsBuildDataSet Parts pane being built, or nullptr for a top-level layout.
 * @return True; failures of individual panes are skipped.
 */
bool Layout::BuildImpl(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const void* pData,
                       ResourceAccessor* pResourceAccessor, const BuildArgSet& rParentBuildArgSet,
                       const PartsBuildDataSet* pPartsBuildDataSet) {
    mResourceAccessor = pResourceAccessor;

    LayoutBuildContext context = {};
    context.buildResSet.pResAccessor = pResourceAccessor;
    context.buildResSet.pLayout = this;
    PrepareBuildArgSet(context.buildArgSet, rParentBuildArgSet, pPartsBuildDataSet);
    context.buildArgSet.m_pPartsLayout = this;
    context.buildArgSet.resourceVersion = static_cast<const ResBinaryFileHeader*>(pData)->version;
    context.buildArgSet.pCurrentBuildResSet = &context.buildResSet;

    detail::BuildPaneTreeContext* pBuildPaneTreeContext = context.buildArgSet.pBuildPaneTreeContext;
    pBuildPaneTreeContext->PushCache(this, pPartsBuildDataSet);

    ReadFirstBlock(context, pData);
    do {
        switch (context.blockKind) {
        case DataBlockKindLayout:
            SetByResLayout(context);
            break;
        case DataBlockKindControl:
            BuildControl(context, pDevice);
            break;
        case DataBlockKindPane:
        case DataBlockKindPicture:
        case DataBlockKindTextBox:
        case DataBlockKindWindow:
        case DataBlockKindBounding:
        case DataBlockKindCapture:
        case DataBlockKindAlignment:
        case DataBlockKindScissor:
        case DataBlockKindParts:
            BuildPaneByResPane(context, pResult, pDevice, pPartsBuildDataSet);
            break;
        case DataBlockKindStateMachine:
            BuildStateMachine(context, pDevice, pPartsBuildDataSet);
            break;
        case DataBlockKindCaptureTextureList:
        case DataBlockKindVectorGraphicsTextureList: {
            if (!context.buildArgSet.pBuildPaneTreeContext->IsInitialized()) {
                context.buildArgSet.pBuildPaneTreeContext->Initialize();
                context.buildArgSet.pBuildPaneTreeContext->CreateCahceInfoToCurrentStackIndex(this,
                                                                                            pPartsBuildDataSet);
            }

            if (context.blockKind == DataBlockKindCaptureTextureList) {
                detail::BuildPaneTreeContext* pCurrent = context.buildArgSet.pBuildPaneTreeContext;
                pCurrent->GetTextureShareInfoFromRootOffset(pCurrent->GetCurrentShareInfoStackOffset())
                    ->SetResCaptureTextureList(
                        static_cast<const ResCaptureTextureList*>(context.buildResSet.pCaptureTextureList));
                pCurrent = context.buildArgSet.pBuildPaneTreeContext;
                pCurrent->GetTextureShareInfoFromRootOffset(pCurrent->GetCurrentShareInfoStackOffset())
                    ->SetCaptureTextureAllAllocateInitialized(
                        rParentBuildArgSet.isCaptureTextureAllAllocateInitialized);
                pCurrent = context.buildArgSet.pBuildPaneTreeContext;
                pCurrent->GetTextureShareInfoFromRootOffset(pCurrent->GetCurrentShareInfoStackOffset())
                    ->SetIgnoreCaptureEffectFirstFrameOnlyFlag(
                        rParentBuildArgSet.isIgnoreCaptureEffectFirstFrameOnly);
            }

            if (context.blockKind == DataBlockKindVectorGraphicsTextureList) {
                detail::BuildPaneTreeContext* pCurrent = context.buildArgSet.pBuildPaneTreeContext;
                pCurrent->GetTextureShareInfoFromRootOffset(pCurrent->GetCurrentShareInfoStackOffset())
                    ->SetResVectorGraphicsTextureList(static_cast<const ResVectorGraphicsTextureList*>(
                    context.buildResSet.pVectorGraphicsTextureList));
            }

            break;
        }
        case DataBlockKindGroup:
            BuildGroup(context);
            break;
        default:
            break;
        }
    } while (ReadNextBlock(context, pData));

    detail::BuildPaneTreeContext* pContext = context.buildArgSet.pBuildPaneTreeContext;
    if (pContext->IsInitialized()) {
        AggregateDynamicTextureList(pContext->GetCurrentTextureShareInfo());
    }

    context.buildArgSet.pBuildPaneTreeContext->PopCache();
    return true;
}

namespace detail {
/**
 * @brief Enter a nested layout build and record a texture-sharing entry for it.
 * @param pLayout Layout whose build starts.
 * @param pAccessor Overrides of the enclosing parts pane, or nullptr.
 */
void BuildPaneTreeContext::PushCache(Layout* pLayout, const Layout::PartsBuildDataAccessor* pAccessor) {
    ++m_ShareInfoStackDepth;
    CreateCahceInfoToCurrentStackIndex(pLayout, pAccessor);
}

/** @brief Carve the share-info stack, list head and instance pool out of the scratch memory. */
void BuildPaneTreeContext::Initialize() {
    DynamicTextureShareInfo** pShareInfoStack = static_cast<DynamicTextureShareInfo**>(m_pMemory);
    m_ShareInfoStackMax = Layout::g_DynamicTextureShareInfoPartsStackMax;
    m_pShareInfoStack = pShareInfoStack;
    const size_t stackSize = sizeof(DynamicTextureShareInfo*) * m_ShareInfoStackMax;
    void* pListMemory = reinterpret_cast<u8*>(m_pShareInfoStack) + stackSize;
    std::memset(m_pShareInfoStack, 0, stackSize);
    m_pShareInfoList = new (pListMemory) DynamicTextureShareInfoList();
    void* pPoolMemory = m_pShareInfoList + 1;
    m_pInstancePool = new (pPoolMemory)
        BuildPaneTreeContextInstancePool(static_cast<BuildPaneTreeContextInstancePool*>(pPoolMemory) + 1);
}

/**
 * @brief Record a texture-sharing entry for the layout at the current stack depth.
 * @param pLayout Layout whose dynamic textures the entry collects.
 * @param pAccessor Overrides of the enclosing parts pane, or nullptr.
 */
void BuildPaneTreeContext::CreateCahceInfoToCurrentStackIndex(Layout* pLayout,
                                                              const Layout::PartsBuildDataAccessor* pAccessor) {
    if (m_pInstancePool != nullptr) {
        DynamicTextureShareInfo* pInfo = m_pInstancePool->AllocateDynamicTextureShareInfo();
        ++m_ShareInfoInstanceUsedCount;
        pInfo->Initialize(pLayout, pAccessor, m_pInstancePool);
        m_pShareInfoList->push_back(*pInfo);
        m_pShareInfoStack[m_ShareInfoStackDepth - 1] = pInfo;
    }
}

/**
 * @brief Access the texture-sharing entry of the layout currently being built.
 * @return Entry at the top of the stack, or nullptr outside a valid stack depth.
 */
DynamicTextureShareInfo* BuildPaneTreeContext::GetCurrentTextureShareInfo() const {
    const int offset = m_ShareInfoStackDepth - 1;
    if (offset < 0 || m_ShareInfoStackDepth > Layout::g_DynamicTextureShareInfoPartsStackMax) {
        return nullptr;
    }

    return m_pShareInfoStack[offset];
}

/**
 * @brief Set the capture texture list block of the layout.
 * @param pList Capture texture list block.
 */
void DynamicTextureShareInfo::SetResCaptureTextureList(const ResCaptureTextureList* pList) {
    m_pResCaptureTextureList = pList;
}

/**
 * @brief Choose whether capture textures allocate their render targets when initialized.
 * @param isInitialized New flag value.
 */
void DynamicTextureShareInfo::SetCaptureTextureAllAllocateInitialized(bool isInitialized) {
    m_IsCaptureTextureAllAllocateInitialized = isInitialized;
}

/**
 * @brief Choose whether capture effects ignore their first-frame-only setting.
 * @param isIgnored New flag value.
 */
void DynamicTextureShareInfo::SetIgnoreCaptureEffectFirstFrameOnlyFlag(bool isIgnored) {
    m_IsIgnoreCaptureEffectFirstFrameOnly = isIgnored;
}

/**
 * @brief Set the vector-graphics texture list block of the layout.
 * @param pList Vector-graphics texture list block.
 */
void DynamicTextureShareInfo::SetResVectorGraphicsTextureList(const ResVectorGraphicsTextureList* pList) {
    m_pResVectorGraphicsTextureList = pList;
}
}  // namespace detail

namespace {
/**
 * @brief Order dynamic textures by their draw priority for qsort.
 * @param pLhs Pointer to the first texture pointer.
 * @param pRhs Pointer to the second texture pointer.
 * @return Difference of the draw priorities.
 */
int CompareDynamicRenderingTexturePriority(const void* pLhs, const void* pRhs);
}  // namespace

/**
 * @brief Append the dynamic textures of a texture-sharing entry to the layout's texture list.
 * @param pShareInfo Entry collected while the layout was built; may be nullptr.
 */
void Layout::AggregateDynamicTextureList(detail::DynamicTextureShareInfo* pShareInfo) {
    using TexturePtr = detail::DynamicRenderingTexture*;
    if (pShareInfo == nullptr) {
        return;
    }

    const int captureCount = pShareInfo->GetCaptureTextureShareInfoCount();
    const int vectorGraphicsCount = pShareInfo->GetVectorGraphicsTextureShareInfoCount();
    if (captureCount + vectorGraphicsCount == 0) {
        return;
    }

    if (mDynamicTextureList == nullptr) {
        mDynamicTextureList = NewLayoutObject<DynamicTextureList>();
    }

    int vectorGraphicsStart;
    int captureStart;
    if (mDynamicTextureList->pTextures == nullptr) {
        mDynamicTextureList->captureCount = captureCount;
        mDynamicTextureList->vectorGraphicsCount = vectorGraphicsCount;
        mDynamicTextureList->pTextures = NewArray<TexturePtr>(captureCount + vectorGraphicsCount);
        vectorGraphicsStart = 0;
        captureStart = vectorGraphicsCount;
    } else {
        const int newCaptureCount = mDynamicTextureList->captureCount + captureCount;
        const int newVectorGraphicsCount = mDynamicTextureList->vectorGraphicsCount + vectorGraphicsCount;
        TexturePtr* pTextures = NewArray<TexturePtr>(newCaptureCount + newVectorGraphicsCount);
        std::memcpy(pTextures, mDynamicTextureList->pTextures,
                    sizeof(TexturePtr) * mDynamicTextureList->vectorGraphicsCount);
        std::memcpy(pTextures + newVectorGraphicsCount,
                    mDynamicTextureList->pTextures + mDynamicTextureList->vectorGraphicsCount,
                    sizeof(TexturePtr) * mDynamicTextureList->captureCount);
        if (mDynamicTextureList->pTextures != nullptr) {
            FreeMemory(mDynamicTextureList->pTextures);
        }

        vectorGraphicsStart = mDynamicTextureList->vectorGraphicsCount;
        captureStart = mDynamicTextureList->captureCount + newVectorGraphicsCount;
        mDynamicTextureList->captureCount = newCaptureCount;
        mDynamicTextureList->vectorGraphicsCount = newVectorGraphicsCount;
        mDynamicTextureList->pTextures = pTextures;
    }

    for (int i = 0; i < captureCount; ++i) {
        auto iter = pShareInfo->m_CaptureTextureList.begin();
        for (int j = 0; j < i; ++j) {
            ++iter;
        }

        mDynamicTextureList->pTextures[captureStart + i] = iter->pCaptureTexture;
    }

    for (int i = 0; i < vectorGraphicsCount; ++i) {
        mDynamicTextureList->pTextures[vectorGraphicsStart + i] =
            pShareInfo->GetVectorGraphicsTextureShareInfoByIndex(i);
    }

    std::qsort(mDynamicTextureList->pTextures + mDynamicTextureList->vectorGraphicsCount,
               mDynamicTextureList->captureCount, sizeof(TexturePtr), CompareDynamicRenderingTexturePriority);
}

namespace detail {
/** @brief Leave the layout build at the top of the stack. */
void BuildPaneTreeContext::PopCache() {
    m_ShareInfoStackDepth--;
    if (m_ShareInfoStackDepth < 0) {
        m_ShareInfoStackDepth = 0;
    }
}
}  // namespace detail

/**
 * @brief Leave vector-graphics texture-list creation to derived layouts.
 * @param pResult Unused build-result storage.
 * @param pDevice Unused graphics device.
 * @param pResources Unused serialized vector-graphics texture list.
 * @param pName Unused layout resource name.
 */
void Layout::BuildVectorGraphicsTextureList(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                            const ResVectorGraphicsTextureList* pResources,
                                            const char* pName) {}

/**
 * @brief Update the vector-graphics textures of this layout and its parts layouts.
 * @param rDrawInfo Drawing state used to evaluate the vector graphics.
 */
void Layout::CalculateVectorGraphicsTexture(DrawInfo& rDrawInfo) {
    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->CalculateVectorGraphicsTexture(rDrawInfo);
    }

    if (mDynamicTextureList != nullptr && mDynamicTextureList->pTextures != nullptr) {
        for (int i = 0; i < mDynamicTextureList->vectorGraphicsCount; ++i) {
            mDynamicTextureList->pTextures[i]->Calculate(rDrawInfo);
        }
    }
}

namespace detail {
/**
 * @brief Count the capture textures recorded for the layout.
 * @return Number of capture textures.
 */
int DynamicTextureShareInfo::GetCaptureTextureShareInfoCount() const {
    return m_CaptureTextureCount;
}

/**
 * @brief Count the vector-graphics textures recorded for the layout.
 * @return Number of vector-graphics textures.
 */
int DynamicTextureShareInfo::GetVectorGraphicsTextureShareInfoCount() const {
    return m_VectorGraphicsTextureCount;
}

/**
 * @brief Access a recorded capture texture entry by its position.
 * @param index Position in registration order.
 * @return Entry, or nullptr when index is out of range.
 */
const CaptureTextureShareInfo* DynamicTextureShareInfo::GetCaptureTextureShareInfoByIndex(int index) const {
    int i = 0;
    for (auto iter = m_CaptureTextureList.begin(); m_CaptureTextureList.end() != iter; ++iter, ++i) {
        if (i == index) {
            return &*iter;
        }
    }

    return nullptr;
}

/**
 * @brief Access a recorded vector-graphics texture by its position.
 * @param index Position in registration order.
 * @return Texture, or nullptr when index is out of range.
 */
VectorGraphicsTexture* DynamicTextureShareInfo::GetVectorGraphicsTextureShareInfoByIndex(int index) const {
    int i = 0;
    for (auto iter = m_VectorGraphicsTextureList.begin(); m_VectorGraphicsTextureList.end() != iter;
         ++iter, ++i) {
        if (i == index) {
            return iter->pVectorGraphicsTexture;
        }
    }

    return nullptr;
}
}  // namespace detail

namespace {
int CompareDynamicRenderingTexturePriority(const void* pLhs, const void* pRhs) {
    const auto* pLhsTexture = *static_cast<detail::DynamicRenderingTexture* const*>(pLhs);
    const auto* pRhsTexture = *static_cast<detail::DynamicRenderingTexture* const*>(pRhs);
    return pLhsTexture->GetDrawPriority() - pRhsTexture->GetDrawPriority();
}
}  // namespace

/**
 * @brief Copy another layout's pane tree and groups using a temporary build context.
 * @param pDevice Graphics device used for the copied resources.
 * @param rSource Layout to copy.
 * @param pPartsLayout Layout that receives the copy as a parts layout, or nullptr.
 * @param pRootPaneName New name of the copied root pane, or nullptr to keep it.
 */
void Layout::CopyLayoutInstanceImpl(nn::gfx::Device* pDevice, const Layout& rSource, Layout* pPartsLayout,
                                    const char* pRootPaneName) {
    const size_t contextMemorySize = detail::BuildPaneTreeContext::CalculateContextRequireMemorySize();
    detail::BuildPaneTreeContext context(__builtin_alloca(contextMemorySize), contextMemorySize);
    CopyLayoutInstanceImpl(pDevice, rSource, pPartsLayout, pRootPaneName, &context);
    if (context.IsInitialized()) {
        context.InitializeCaptureTexturesAfterPaneTreeBuilt(pDevice);
        context.Finalize();
    }
}

/**
 * @brief Copy another layout's pane tree and groups.
 * @param pDevice Graphics device used for the copied resources.
 * @param rSource Layout to copy.
 * @param pPartsLayout Layout that receives the copy as a parts layout, or nullptr.
 * @param pRootPaneName New name of the copied root pane, or nullptr to keep it.
 * @param pContext Build context collecting the copied dynamic textures.
 */
void Layout::CopyLayoutInstanceImpl(nn::gfx::Device* pDevice, const Layout& rSource, Layout* pPartsLayout,
                                    const char* pRootPaneName, detail::BuildPaneTreeContext* pContext) {
    mRootPane = nullptr;
    _20 = nullptr;
    mLayoutSize = rSource.mLayoutSize;
    _30 = rSource._30;
    _38 = rSource._38;
    mResourceAccessor = rSource.mResourceAccessor;
    mDynamicTextureList = nullptr;
    if (rSource.mDynamicTextureList != nullptr && !pContext->IsInitialized()) {
        pContext->Initialize();
    }

    pContext->PushCache(this, nullptr);
    Pane* pRootPane = detail::ClonePaneTreeWithPartsLayoutImpl_(
        rSource.mRootPane, rSource.mRootPane->GetParent() != nullptr ? this : nullptr, pDevice, this, pContext);
    mRootPane = pRootPane;
    if (pRootPaneName != nullptr) {
        pRootPane->SetName(pRootPaneName);
    }

    _20 = NewLayoutObject<GroupContainer>();
    for (auto& rGroup : rSource.GetGroupContainer()->mGroups) {
        GetGroupContainer()->AppendGroup(NewLayoutObject<Group, const Group&, Pane*>(rGroup, mRootPane));
    }

    if (pPartsLayout != nullptr) {
        Parts* pParts = DynamicCast<Parts>(mRootPane);
        if (pParts != nullptr) {
            pPartsLayout->GetPartsList().push_back(*pParts);
        }
    }

    if (pContext->IsInitialized()) {
        AggregateDynamicTextureList(pContext->GetCurrentTextureShareInfo());
    }

    pContext->PopCache();
}

/**
 * @brief Calculate global matrices from the root pane using this layout's drawing context.
 * @param rDrawInfo View and layout state used to initialize pane calculation.
 * @param forceDirty Whether to force global matrices to be recalculated.
 */
void Layout::CalculateGlobalMatrix(DrawInfo& rDrawInfo, bool forceDirty) {
    if (mRootPane != nullptr) {
        Pane::CalculateContext context;
        context.Set(rDrawInfo, this);
        mRootPane->CalculateGlobalMatrix(context, forceDirty);
    }
}

/**
 * @brief Allocate and register a default animation transform owned by this layout.
 * @tparam T Concrete animation transform type to construct.
 * @return New transform, or nullptr when allocation fails.
 */
template <class T>
T* Layout::CreateAnimTransform() {
    void* pMemory = AllocateMemory(sizeof(T));
    T* pTransform = pMemory != nullptr ? new (pMemory) T : nullptr;
    if (pTransform != nullptr) {
        mAnimTransformList.LinkPrev(&pTransform->m_Link);
    }

    return pTransform;
}

/**
 * @brief Construct an animation transform and initialize it from a parsed resource.
 * @tparam T Concrete animation transform type to construct.
 * @param pDevice Graphics device used for the animation's resources.
 * @param rResource Parsed animation data; must contain an animation block to create a transform.
 * @return New transform, or nullptr when the animation block is absent or allocation fails.
 */
template <class T>
T* Layout::CreateAnimTransform(nn::gfx::Device* pDevice, const AnimResource& rResource) {
    const auto* pBlock = rResource.mAnimation;
    if (pBlock == nullptr) {
        return nullptr;
    }

    T* pTransform = CreateAnimTransform<T>();
    if (pTransform != nullptr) {
        pTransform->SetResource(pDevice, mResourceAccessor, pBlock);
    }

    return pTransform;
}

/**
 * @brief Resolve a named animation and construct an initialized transform.
 * @tparam T Concrete animation transform type to construct.
 * @param pDevice Graphics device used for the animation's resources.
 * @param pName Animation tag appended to the layout name to resolve its resource.
 * @return New transform, or nullptr when the resource lacks animation data or allocation fails.
 */
template <class T>
T* Layout::CreateAnimTransform(nn::gfx::Device* pDevice, const char* pName) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    return CreateAnimTransform<T>(pDevice, resource);
}

/**
 * @brief Allocate a basic transform and register it with this layout.
 * @return New transform, or nullptr when allocation fails.
 */
AnimTransformBasic* Layout::CreateAnimTransformBasic() {
    return CreateAnimTransform<AnimTransformBasic>();
}

/**
 * @brief Unlink, destroy and free a transform allocated by this layout.
 * @param pTransform Transform belonging to the animation list; must not be nullptr.
 */
void Layout::DeleteAnimTransform(AnimTransform* pTransform) {
    using Iterator = AnimTransformList::iterator;
    GetAnimTransformList().erase(Iterator(&pTransform->m_Link));
    if (pTransform != nullptr) {
        pTransform->~AnimTransform();
        FreeMemory(pTransform);
    }
}

/**
 * @brief Parse animation data and create its basic transform.
 * @param pDevice Graphics device used for animation resources.
 * @param pData Serialized animation file read by AnimResource::Set.
 * @return New transform, or nullptr when its animation block is absent or allocation fails.
 */
AnimTransformBasic* Layout::CreateAnimTransformBasic(nn::gfx::Device* pDevice, const void* pData) {
    AnimResource resource;
    resource.Set(pData);
    return CreateAnimTransform<AnimTransformBasic>(pDevice, resource);
}

/**
 * @brief Create a basic transform from parsed animation data.
 * @param pDevice Graphics device used for animation resources.
 * @param rResource Parsed animation resource containing the animation block.
 * @return New transform, or nullptr when its animation block is absent or allocation fails.
 */
AnimTransformBasic* Layout::CreateAnimTransformBasic(nn::gfx::Device* pDevice,
                                                     const AnimResource& rResource) {
    return CreateAnimTransform<AnimTransformBasic>(pDevice, rResource);
}

/**
 * @brief Create a basic transform from a named animation resource.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Animation tag appended to the layout name.
 * @return New transform, or nullptr when its animation block is absent or allocation fails.
 */
AnimTransformBasic* Layout::CreateAnimTransformBasic(nn::gfx::Device* pDevice, const char* pName) {
    return CreateAnimTransform<AnimTransformBasic>(pDevice, pName);
}

/**
 * @brief Construct an animator for a single pane.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Tag identifying a valid animation resource; allocation must succeed.
 * @param pPane Pane to bind without recursively binding children.
 * @param enabled Initial animation activation state.
 * @return Newly constructed and bound pane animator.
 */
PaneAnimator* Layout::CreatePaneAnimator(nn::gfx::Device* pDevice, const char* pName, Pane* pPane,
                                         bool enabled) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    auto* pAnimator = CreateAnimTransform<PaneAnimator>(pDevice, resource);
    pAnimator->Setup(pPane, enabled);
    return pAnimator;
}

/**
 * @brief Look up the animation file associated with this layout and a tag name.
 * @param pName Animation tag appended to the layout name.
 * @return Resource data returned by the layout's resource accessor, possibly nullptr.
 */
const void* Layout::GetAnimResourceData(const char* pName) const {
    char path[136];
    nn::util::SNPrintf(path, sizeof(path), "%s_%s.bflan", GetName(), pName);
    return mResourceAccessor->FindResourceByName(ResourceTypeAnimation, path);
}

/**
 * @brief Construct an animator for the panes in a group.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Tag identifying a valid animation resource; allocation must succeed.
 * @param pGroup Group containing the animation targets.
 * @param enabled Initial animation activation state.
 * @return Newly constructed and bound group animator.
 */
GroupAnimator* Layout::CreateGroupAnimator(nn::gfx::Device* pDevice, const char* pName, Group* pGroup,
                                           bool enabled) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    auto* pAnimator = CreateAnimTransform<GroupAnimator>(pDevice, resource);
    pAnimator->Setup(pGroup, enabled);
    return pAnimator;
}

/**
 * @brief Construct an animator bound to one group named by its animation resource.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Tag identifying a valid animation resource; allocation must succeed.
 * @param index Group index passed to GroupAnimator::Setup.
 * @param enabled Initial animation activation state.
 * @return Newly constructed group animator.
 */
GroupAnimator* Layout::CreateGroupAnimatorWithIndex(nn::gfx::Device* pDevice, const char* pName, int index,
                                                    bool enabled) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    auto* pAnimator = CreateAnimTransform<GroupAnimator>(pDevice, resource);
    pAnimator->Setup(resource, GetGroupContainer(), index, enabled);
    return pAnimator;
}

/**
 * @brief Construct an animator with inline storage for every group in the resource.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Unused tag name; rResource already identifies the animation.
 * @param rResource Parsed resource supplying animation data and group names.
 * @param enabled Initial animation activation state.
 * @return New group-array animator, or nullptr if animation data or allocation is unavailable.
 */
GroupArrayAnimator* Layout::DoCreateAndSetupGroupArrayAnimator_(nn::gfx::Device* pDevice, const char* pName,
                                                                const AnimResource& rResource, bool enabled) {
    const auto* pBlock = rResource.mAnimation;
    if (pBlock == nullptr) {
        return nullptr;
    }

    void* pMemory = AllocateMemory(sizeof(GroupArrayAnimator) + rResource.GetGroupCount() * sizeof(Group*));
    auto* pAnimator = pMemory != nullptr ? new (pMemory) GroupArrayAnimator : nullptr;
    if (pAnimator != nullptr) {
        pAnimator->SetResource(pDevice, mResourceAccessor, pBlock);
        mAnimTransformList.LinkPrev(&pAnimator->m_Link);
        pAnimator->Setup(rResource, GetGroupContainer(), reinterpret_cast<Group**>(pAnimator + 1), enabled);
    }

    return pAnimator;
}

/**
 * @brief Create a group-array animator through the layout's factory hook.
 * @param pDevice Graphics device used for animation resources.
 * @param rResource Parsed resource supplying animation data and group names.
 * @param enabled Initial animation activation state.
 * @return Animator returned by the factory hook, possibly nullptr.
 */
GroupArrayAnimator* Layout::CreateGroupArrayAnimator(nn::gfx::Device* pDevice, const AnimResource& rResource,
                                                     bool enabled) {
    return DoCreateAndSetupGroupArrayAnimator_(pDevice, nullptr, rResource, enabled);
}

/**
 * @brief Resolve a named resource and create an animator for all its groups.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Animation tag appended to the layout name.
 * @param enabled Initial animation activation state.
 * @return Animator returned by the factory hook, possibly nullptr.
 */
GroupArrayAnimator* Layout::CreateGroupArrayAnimator(nn::gfx::Device* pDevice, const char* pName,
                                                     bool enabled) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    return DoCreateAndSetupGroupArrayAnimator_(pDevice, pName, resource, enabled);
}

/**
 * @brief Construct an animator for the first group in a parsed resource.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Unused tag name; rResource already identifies the animation.
 * @param rResource Valid animation resource; allocation must succeed.
 * @param enabled Initial animation activation state.
 * @return Newly constructed group animator.
 */
GroupAnimator* Layout::DoCreateAndSetupGroupAnimator_(nn::gfx::Device* pDevice, const char* pName,
                                                      const AnimResource& rResource, bool enabled) {
    auto* pAnimator = CreateAnimTransform<GroupAnimator>(pDevice, rResource);
    pAnimator->Setup(rResource, GetGroupContainer(), 0, enabled);
    return pAnimator;
}

/**
 * @brief Select a single-group or group-array animator based on the resource's group count.
 * @param pDevice Graphics device used for animation resources.
 * @param pName Animation tag appended to the layout name.
 * @param enabled Initial animation activation state.
 * @return Animator created by the corresponding factory hook.
 */
Animator* Layout::CreateGroupAnimatorAuto(nn::gfx::Device* pDevice, const char* pName, bool enabled) {
    AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    if (resource.GetGroupCount() > 1) {
        return DoCreateAndSetupGroupArrayAnimator_(pDevice, pName, resource, enabled);
    }

    return DoCreateAndSetupGroupAnimator_(pDevice, pName, resource, enabled);
}

/**
 * @brief Bind an animation recursively to the root pane when one exists.
 * @param pTransform Animation transform to bind; must be non-null when the layout has a root pane.
 */
void Layout::BindAnimation(AnimTransform* pTransform) {
    if (mRootPane != nullptr) {
        pTransform->BindPane(mRootPane, true);
    }
}

/**
 * @brief Remove every binding from a transform.
 * @param pTransform Non-null animation transform to unbind.
 */
void Layout::UnbindAnimation(AnimTransform* pTransform) {
    pTransform->UnbindAll();
}

/**
 * @brief Remove a pane's bindings from all transforms owned by the layout.
 * @param pPane Pane whose animation bindings should be removed.
 */
void Layout::UnbindAnimation(Pane* pPane) {
    for (auto& rTransform : GetAnimTransformList()) {
        rTransform.UnbindPane(pPane);
    }
}

/** @brief Remove every binding from all transforms owned by the layout. */
void Layout::UnbindAllAnimation() {
    for (auto& rTransform : GetAnimTransformList()) {
        rTransform.UnbindAll();
    }
}

namespace {
/**
 * @brief Check whether a pane belongs to one of the groups an animation is bound to.
 * @param pGroupContainer Groups of the layout.
 * @param pGroups Groups the animation is bound to.
 * @param groupCount Number of entries in pGroups.
 * @param isDescendingBind Whether panes below a group member also count as members.
 * @param rTarget Link of the pane to look for.
 * @return True when the pane is part of a bound group.
 */
inline bool IsIncludeAnimationGroup(GroupContainer* pGroupContainer, const ResAnimationGroup* pGroups,
                                    u16 groupCount, bool isDescendingBind, const PaneLink& rTarget) {
    for (u32 i = 0; i < groupCount; ++i) {
        Group* pGroup = pGroupContainer->FindGroupByName(pGroups[i].name);
        const Pane* pTargetPane = rTarget.pane;
        for (auto& rLink : pGroup->mPanes) {
            if (isDescendingBind) {
                const Pane* pPane = pTargetPane;
                do {
                    if (rLink.pane == pPane) {
                        return true;
                    }

                    pPane = pPane->GetParent();
                } while (pPane != nullptr);
            } else if (rLink.pane == pTargetPane) {
                return true;
            }
        }
    }

    return false;
}
}  // namespace

/**
 * @brief Create a basic transform for an animation and bind it to its groups and shared panes.
 * @param pDevice Graphics device used for animation resources.
 * @param rResource Parsed animation resource.
 * @return True when every group and shared pane named by the resource was found.
 */
bool Layout::BindAnimationAuto(nn::gfx::Device* pDevice, const AnimResource& rResource) {
    if (mRootPane == nullptr) {
        return false;
    }

    if (rResource.mAnimation == nullptr) {
        return false;
    }

    AnimTransformBasic* pTransform = CreateAnimTransformBasic();
    if (pTransform == nullptr) {
        return false;
    }

    const u16 groupCount = rResource.GetGroupCount();
    bool result = true;
    if (groupCount == 0) {
        pTransform->SetResource(pDevice, mResourceAccessor, rResource.mAnimation,
                                rResource.mAnimation->contentCount);
        mRootPane->BindAnimation(pTransform, true, false);
    } else {
        const ResAnimationGroup* pGroups = rResource.GetGroupArray();
        int animationCount = 0;
        for (int i = 0; i < groupCount; ++i) {
            Group* pGroup = GetGroupContainer()->FindGroupByName(pGroups[i].name);
            if (pGroup == nullptr) {
                result = false;
                continue;
            }

            animationCount += rResource.CalculateAnimationCount(pGroup, rResource.IsDescendingBind());
        }

        pTransform->SetResource(pDevice, mResourceAccessor, rResource.mAnimation, animationCount);
        for (int i = 0; i < groupCount; ++i) {
            Group* pGroup = GetGroupContainer()->FindGroupByName(pGroups[i].name);
            if (pGroup != nullptr) {
                nn::ui2d::BindAnimation(pTransform, pGroup, false);
            }
        }
    }

    const u16 shareInfoCount = rResource.GetAnimationShareInfoCount();
    if (shareInfoCount != 0) {
        const ResAnimationShareInfo* pShareInfos = rResource.GetAnimationShareInfoArray();
        for (int i = 0; i < shareInfoCount; ++i) {
            Pane* pSourcePane = mRootPane->FindPaneByName(pShareInfos[i].srcPaneName, true);
            if (pSourcePane == nullptr) {
                result = false;
                continue;
            }

            detail::AnimPaneTree animPaneTree(pSourcePane, rResource);
            if (!animPaneTree.IsEnabled()) {
                continue;
            }

            Group* pGroup = GetGroupContainer()->FindGroupByName(pShareInfos[i].targetGroupName);
            if (pGroup == nullptr) {
                result = false;
                continue;
            }

            for (auto& rLink : pGroup->mPanes) {
                if (rLink.pane == pSourcePane) {
                    continue;
                }

                if (groupCount != 0 &&
                    !IsIncludeAnimationGroup(GetGroupContainer(), rResource.GetGroupArray(), groupCount,
                                             rResource.IsDescendingBind(), rLink)) {
                    continue;
                }

                animPaneTree.Bind(pDevice, this, rLink.pane, mResourceAccessor);
            }
        }
    }

    return result;
}

/**
 * @brief Calculate the pane tree and vector graphics within this layout's drawing context.
 * @param rDrawInfo Drawing state temporarily associated with this layout.
 * @param forceDirty Whether pane calculations must refresh global matrices.
 */
void Layout::CalculateImpl(DrawInfo& rDrawInfo, bool forceDirty) {
    if (mRootPane != nullptr) {
        Pane::CalculateContext context;
        context.Set(rDrawInfo, this);
        rDrawInfo.m_pLayoutInformation =
            reinterpret_cast<const Pane::CalculateContext::LayoutInformation*>(this);
        mRootPane->Calculate(rDrawInfo, context, forceDirty);
        CalculateVectorGraphicsTexture(rDrawInfo);
        rDrawInfo.m_pLayoutInformation = nullptr;
    }
}

/**
 * @brief Render the dynamic textures of this layout and its parts layouts.
 * @param pDevice Graphics device used by the textures.
 * @param rDrawInfo Drawing state configured for this layout while rendering.
 * @param rCommands Command buffer receiving the rendering commands.
 */
void Layout::DrawCaptureTexture(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                nn::gfx::CommandBuffer& rCommands) {
    if (mRootPane == nullptr) {
        return;
    }

    rDrawInfo.ConfigureBeforeDrawing(this);
    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->DrawCaptureTexture(pDevice, rDrawInfo, rCommands);
    }

    if (mDynamicTextureList != nullptr && mDynamicTextureList->pTextures != nullptr) {
        const int count = mDynamicTextureList->GetTotalCount();
        if (count > 0) {
            int priority = mDynamicTextureList->pTextures[0]->GetDrawPriority();
            for (int i = 0; i < count; ++i) {
                const u8 nextPriority = mDynamicTextureList->pTextures[i]->GetDrawPriority();
                if (priority != nextPriority) {
                    rCommands.FlushMemory(nn::gfx::GpuAccess_ColorBuffer);
                    rCommands.InvalidateMemory(nn::gfx::GpuAccess_Texture);
                }

                priority = nextPriority;
                mDynamicTextureList->pTextures[i]->Draw(pDevice, rDrawInfo, rCommands);
            }
        }

        rCommands.FlushMemory(nn::gfx::GpuAccess_ColorBuffer);
        rCommands.InvalidateMemory(nn::gfx::GpuAccess_Texture);
    }

    rDrawInfo.ConfigureAfterDrawing();
}

/**
 * @brief Discard the static drop shadow cache of a pane.
 * @param pPaneName Name of the pane whose cache is discarded.
 */
void Layout::DiscardDropShadowStaticCachedTexture(const char* pPaneName) {
    DiscardPaneEffectStaticCachedTexture(pPaneName);
}

/**
 * @brief Discard the static pane effect caches of the pane with the given name.
 * @param pPaneName Name of the pane whose caches are discarded.
 */
void Layout::DiscardPaneEffectStaticCachedTexture(const char* pPaneName) {
    if (mRootPane == nullptr) {
        return;
    }

    if (mDynamicTextureList == nullptr || mDynamicTextureList->pTextures == nullptr) {
        return;
    }

    const int count = mDynamicTextureList->GetTotalCount();
    for (int i = 0; i < count; ++i) {
        const Pane* pPane = mDynamicTextureList->pTextures[i]->GetTargetPane();
        if (pPane != nullptr && std::strcmp(pPane->GetName(), pPaneName) == 0) {
            DiscardPaneEffectStaticCachedTextureImpl(mDynamicTextureList->pTextures[i]);
            return;
        }
    }
}

/**
 * @brief Invalidate the static pane effect caches rendered for a dynamic texture.
 * @param pTexture Texture whose target pane owns the caches.
 */
void Layout::DiscardPaneEffectStaticCachedTextureImpl(detail::DynamicRenderingTexture* pTexture) const {
    Pane* pPane = pTexture->GetTargetPane();
    if (pPane == nullptr) {
        return;
    }

    detail::PaneEffect* pEffect = pPane->GetPaneEffectInstance();
    if (pEffect == nullptr) {
        return;
    }

    if (IsMaskStaticCacheEnabled(pEffect)) {
        pTexture->ResetFirstFrameCaptureUpdatedFlag();
        if (IsMaskStaticCacheEnabled(pEffect)) {
            MaskStaticCacheUpdated(pEffect) = false;
        }
    }

    if (IsDropShadowStaticCacheEnabled(pEffect)) {
        pTexture->ResetFirstFrameCaptureUpdatedFlag();
        if (IsDropShadowStaticCacheEnabled(pEffect)) {
            DropShadowStaticCacheUpdated(pEffect) = false;
        }
    }
}

/** @brief Discard every static pane effect cache of this layout and its parts layouts. */
void Layout::DiscardPaneEffectStaticCachedTexture() {
    if (mRootPane == nullptr) {
        return;
    }

    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->DiscardPaneEffectStaticCachedTexture();
    }

    if (mDynamicTextureList == nullptr || mDynamicTextureList->pTextures == nullptr) {
        return;
    }

    const int count = mDynamicTextureList->GetTotalCount();
    for (int i = 0; i < count; ++i) {
        DiscardPaneEffectStaticCachedTextureImpl(mDynamicTextureList->pTextures[i]);
    }
}

/**
 * @brief Render the static drop shadow caches of this layout.
 * @param pDevice Graphics device used by the caches.
 * @param rDrawInfo Drawing state configured for this layout while rendering.
 * @param rCommands Command buffer receiving the rendering commands.
 */
void Layout::DrawDropShadowStaticCache(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                       nn::gfx::CommandBuffer& rCommands) {
    DrawPaneEffectStaticCache(pDevice, rDrawInfo, rCommands);
}

/**
 * @brief Render the static pane effect caches of this layout and its parts layouts.
 * @param pDevice Graphics device used by the caches.
 * @param rDrawInfo Drawing state configured for this layout while rendering.
 * @param rCommands Command buffer receiving the rendering commands.
 */
void Layout::DrawPaneEffectStaticCache(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                       nn::gfx::CommandBuffer& rCommands) {
    if (mRootPane == nullptr) {
        return;
    }

    rDrawInfo.ConfigureBeforeDrawing(this);
    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->DrawPaneEffectStaticCache(pDevice, rDrawInfo, rCommands);
    }

    if (mDynamicTextureList != nullptr && mDynamicTextureList->pTextures != nullptr) {
        const int count = mDynamicTextureList->GetTotalCount();
        for (int i = 0; i < count; ++i) {
            Pane* pPane = mDynamicTextureList->pTextures[i]->GetTargetPane();
            if (pPane == nullptr) {
                continue;
            }

            detail::PaneEffect* pEffect = pPane->GetPaneEffectInstance();
            if (pEffect == nullptr) {
                continue;
            }

            if (IsMaskStaticCacheEnabled(pEffect) && !MaskStaticCacheUpdated(pEffect)) {
                pEffect->CaptureMaskedImage(rDrawInfo, rCommands);
            }

            if (IsDropShadowStaticCacheEnabled(pEffect) && !DropShadowStaticCacheUpdated(pEffect)) {
                pEffect->DrawDropShadow(rDrawInfo, rCommands);
            }
        }
    }

    rCommands.FlushMemory(nn::gfx::GpuAccess_ColorBuffer);
    rCommands.InvalidateMemory(nn::gfx::GpuAccess_Texture);

    rDrawInfo.ConfigureAfterDrawing();
}

/**
 * @brief Draw the root pane between draw-info setup and cleanup calls.
 * @param rDrawInfo Rendering state configured for this layout during drawing.
 * @param rCommands Command buffer receiving the pane draw commands.
 */
void Layout::Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (mRootPane != nullptr) {
        rDrawInfo.ConfigureBeforeDrawing(this);
        mRootPane->Draw(rDrawInfo, rCommands);
        rDrawInfo.ConfigureAfterDrawing();
    }
}

/** @brief Apply owned animations, then recursively animate all nested parts layouts. */
void Layout::Animate() {
    for (auto& rTransform : GetAnimTransformList()) {
        rTransform.Animate();
    }

    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->Animate();
    }
}

/**
 * @brief Advance owned animation frames and recursively update nested parts layouts.
 * @param frame Elapsed animation step passed to each transform and nested layout.
 */
void Layout::UpdateAnimFrame(float frame) {
    for (auto& rTransform : GetAnimTransformList()) {
        rTransform.UpdateFrame(frame);
    }

    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->UpdateAnimFrame(frame);
    }
}

/**
 * @brief Apply each animation before advancing its frame, then recurse through parts layouts.
 * @param frame Elapsed animation step passed to each transform and nested layout.
 */
void Layout::AnimateAndUpdateAnimFrame(float frame) {
    for (auto& rTransform : GetAnimTransformList()) {
        rTransform.Animate();
        rTransform.UpdateFrame(frame);
    }

    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->AnimateAndUpdateAnimFrame(frame);
    }
}

/**
 * @brief Return the layout bounds centered around the origin, with positive Y pointing upward.
 * @return Rectangle spanning half the layout's width and height on each side of the origin.
 */
nn::font::Rectangle Layout::GetLayoutRect() const {
    const float width = mLayoutSize.x;
    const float height = mLayoutSize.y;
    const float left = width * -0.5f;
    const float top = height * 0.5f;
    const float right = width * 0.5f;
    const float bottom = height * -0.5f;
    nn::font::Rectangle rectangle;
    rectangle.top = top;
    rectangle.left = left;
    rectangle.right = right;
    rectangle.bottom = bottom;
    return rectangle;
}

namespace {
/**
 * @brief Compare a supplied name against the fixed-width pane name field.
 * @param pName Null-terminated requested pane name.
 * @param pPaneName Pane name occupying at most 24 bytes.
 * @return True when the strings agree through a terminator or all 24 bytes.
 */
inline bool MatchesPaneName(const char* pName, const char* pPaneName) {
    for (size_t i = 0; i < 24; ++i) {
        if (pName[i] != pPaneName[i]) {
            return false;
        }

        if (pName[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Find a directly registered parts pane while preserving list constness.
 * @tparam TList Mutable or const parts-list type.
 * @param rParts List of parts panes belonging to one layout.
 * @param pName Requested pane name, compared through at most 24 bytes.
 * @return Matching parts pane, or nullptr when no registered pane has that name.
 */
template <class TList>
inline auto FindNamedParts(TList& rParts, const char* pName) -> decltype(&*rParts.begin()) {
    for (auto& rPane : rParts) {
        if (MatchesPaneName(pName, rPane.GetName())) {
            return &rPane;
        }
    }

    return nullptr;
}
}  // namespace

void SetTagProcessorRecursive(Pane* pPane, nn::font::TagProcessorBase<u16>* pProcessor);

/**
 * @brief Assign a tag processor to text boxes throughout a pane tree.
 * @param pPane Non-null root of the pane subtree to traverse.
 * @param pProcessor Tag processor to assign; nullptr selects the default text handling.
 */
void SetTagProcessorRecursive(Pane* pPane, nn::font::TagProcessorBase<u16>* pProcessor) {
    auto* pTextBox = DynamicCast<TextBox>(pPane);
    if (pTextBox != nullptr) {
        pTextBox->SetTagProcessor(pProcessor);
    }

    using PaneNodeTraits =
        nn::util::IntrusiveListMemberNodeTraits<detail::PaneBase, &detail::PaneBase::m_Link>;
    for (auto* pNode = pPane->m_Children.GetNext(); pNode != &pPane->m_Children; pNode = pNode->GetNext()) {
        auto* pChild = static_cast<Pane*>(&PaneNodeTraits::GetItem(*pNode));
        SetTagProcessorRecursive(pChild, pProcessor);
    }
}

/**
 * @brief Assign a tag processor to every text box beneath the layout's root pane.
 * @param pProcessor Tag processor to assign; the layout must have a root pane.
 */
void Layout::SetTagProcessor(nn::font::TagProcessorBase<u16>* pProcessor) {
    SetTagProcessorRecursive(mRootPane, pProcessor);
}

/**
 * @brief Find a parts pane directly registered with this layout.
 * @param pName Requested pane name, compared through at most 24 bytes.
 * @return Matching parts pane, or nullptr when no registered pane has that name.
 */
Parts* Layout::FindPartsPaneByName(const char* pName) {
    return FindNamedParts(GetPartsList(), pName);
}

/**
 * @brief Find the capture texture that renders a pane, searching parts layouts too.
 * @param pPane Pane rendered by the capture texture.
 * @return Capture texture, or nullptr when no capture texture renders the pane.
 */
CaptureTexture* Layout::FindCaptureTextureByPanePtr(const Pane* pPane) {
    if (mDynamicTextureList != nullptr && mDynamicTextureList->pTextures != nullptr) {
        const int count = mDynamicTextureList->GetTotalCount();
        for (int i = 0; i < count; ++i) {
            if (mDynamicTextureList->pTextures[i]->GetTargetPane() == pPane) {
                return DynamicCast<CaptureTexture>(mDynamicTextureList->pTextures[i]);
            }
        }
    }

    for (auto& rParts : GetPartsList()) {
        if (rParts.m_pLayout != nullptr) {
            CaptureTexture* pTexture = rParts.m_pLayout->FindCaptureTextureByPanePtr(pPane);
            if (pTexture != nullptr) {
                return pTexture;
            }
        }
    }

    return nullptr;
}

/** @brief Make every dynamic texture of this layout and its parts layouts capture its first frame again. */
void Layout::ResetFirstFrameCaptureUpdatedFlag() {
    if (mRootPane == nullptr) {
        return;
    }

    for (auto& rParts : GetPartsList()) {
        rParts.m_pLayout->ResetFirstFrameCaptureUpdatedFlag();
    }

    if (mDynamicTextureList == nullptr || mDynamicTextureList->pTextures == nullptr) {
        return;
    }

    const int count = mDynamicTextureList->GetTotalCount();
    for (int i = 0; i < count; ++i) {
        mDynamicTextureList->pTextures[i]->ResetFirstFrameCaptureUpdatedFlag();
    }
}

/**
 * @brief Find a parts pane without modifying this layout.
 * @param pName Requested pane name, compared through at most 24 bytes.
 * @return Matching parts pane, or nullptr when no registered pane has that name.
 */
const Parts* Layout::FindPartsPaneByName(const char* pName) const {
    return FindNamedParts(GetPartsList(), pName);
}

/**
 * @brief Check that a copied layout matches its source.
 * @param rOther Layout this one was copied from.
 * @return True when sizes, resources, pane trees and groups agree.
 */
bool Layout::CompareCopiedInstanceTest(const Layout& rOther) const {
    if (mLayoutSize.x != rOther.mLayoutSize.x) {
        return false;
    }

    if (mLayoutSize.y != rOther.mLayoutSize.y) {
        return false;
    }

    if (_30 != rOther._30) {
        return false;
    }

    if (mResourceAccessor != rOther.mResourceAccessor) {
        return false;
    }

    if (!ComparePaneTreeTest(mRootPane, rOther.mRootPane)) {
        return false;
    }

    const GroupContainer* pGroups = GetGroupContainer();
    const int groupCount = pGroups->mGroups.size();
    const GroupContainer* pOtherGroups = rOther.GetGroupContainer();
    if (groupCount != pOtherGroups->mGroups.size()) {
        return false;
    }

    auto otherIter = pOtherGroups->mGroups.begin();
    for (auto iter = pGroups->mGroups.begin(); iter != pGroups->mGroups.end(); ++iter, ++otherIter) {
        if (!iter->CompareCopiedInstanceTest(*otherIter)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Acquire a named shader through the layout's resource accessor.
 * @param pDevice Graphics device used to initialize shader resources.
 * @param pName Shader name forwarded to the resource accessor.
 * @return Shader supplied by the accessor, possibly nullptr.
 */
ShaderInfo* Layout::AcquireArchiveShader(nn::gfx::Device* pDevice, const char* pName) const {
    return mResourceAccessor->AcquireShader(pDevice, pName);
}

/**
 * @brief Acquire an archive shader selected by its signature and variation keys.
 * @param pDevice Graphics device used to initialize shader resources.
 * @param signature Shader archive signature forwarded to the accessor.
 * @param keyCount Number of variation keys in pKeys.
 * @param pKeys Array of keyCount variation keys.
 * @return Shader supplied by the accessor, possibly nullptr.
 */
ShaderInfo* Layout::AcquireArchiveShader(nn::gfx::Device* pDevice, u32 signature, size_t keyCount,
                                         const u32* pKeys) const {
    return mResourceAccessor->AcquireArchiveShader(pDevice, signature, keyCount, pKeys);
}

/**
 * @brief Count capture textures in this layout and all nested parts layouts.
 * @return Combined capture-texture count.
 */
int Layout::CalculateCaptureTextureCountRecursive() const {
    int count = 0;
    for (const auto& rParts : GetPartsList()) {
        count += rParts.m_pLayout->CalculateCaptureTextureCountRecursive();
    }

    return count + (mDynamicTextureList != nullptr ? mDynamicTextureList->captureCount : 0);
}

/**
 * @brief Count vector-graphics textures in this layout and all nested parts layouts.
 * @return Combined vector-graphics texture count.
 */
int Layout::CalculateVectorGraphicsTextureCountRecursive() const {
    int count = 0;
    for (const auto& rParts : GetPartsList()) {
        count += rParts.m_pLayout->CalculateVectorGraphicsTextureCountRecursive();
    }

    return count + (mDynamicTextureList != nullptr ? mDynamicTextureList->vectorGraphicsCount : 0);
}

/**
 * @brief Create a pane through the global pane factory.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for pane resources.
 * @param kind Data block kind of pBlock.
 * @param pBlock Pane block to build.
 * @param pOverrideBlock Override block of the enclosing parts pane, or nullptr.
 * @param rBuildArgSet Build arguments.
 * @return Created pane, or nullptr for an unknown block kind.
 */
Pane* Layout::BuildPaneObj(BuildResultInformation* pResult, nn::gfx::Device* pDevice, u32 kind,
                           const void* pBlock, const void* pOverrideBlock, const BuildArgSet& rBuildArgSet) {
    return g_pLayoutPaneFactory->BuildPaneObj(pResult, pDevice, kind, pBlock, pOverrideBlock, rBuildArgSet);
}

/**
 * @brief Allocate the layout object that a parts pane builds its layout into.
 * @param pName Name of the parts layout.
 * @param rPartsBuildDataSet Parts pane being built.
 * @param rBuildArgSet Build arguments of the parts layout.
 * @return New empty layout, or nullptr when allocation fails.
 */
Layout* Layout::DoCreatePartsLayout_(const char* pName, const PartsBuildDataSet& rPartsBuildDataSet,
                                     const BuildArgSet& rBuildArgSet) {
    return NewLayoutObject<Layout>();
}

/**
 * @brief Build the layout of a parts pane from its layout resource.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for layout resources.
 * @param pName Name of the parts layout resource, without extension.
 * @param rPartsBuildDataSet Parts pane being built.
 * @param rBuildArgSet Build arguments of the enclosing layout.
 * @return Built parts layout.
 */
Layout* Layout::BuildPartsLayout(BuildResultInformation* pResult, nn::gfx::Device* pDevice, const char* pName,
                                 const PartsBuildDataSet& rPartsBuildDataSet, const BuildArgSet& rBuildArgSet) {
    const void* pData = GetLayoutResourceData(pName);
    Layout* pPartsLayout = DoCreatePartsLayout_(pName, rPartsBuildDataSet, rBuildArgSet);
    pPartsLayout->BuildImpl(pResult, pDevice, pData, mResourceAccessor, rBuildArgSet, &rPartsBuildDataSet);
    return pPartsLayout;
}

/**
 * @brief Look up a layout resource after appending its file extension.
 * @param pName Layout resource basename, without the .bflyt extension.
 * @return Resource data returned by the accessor, possibly nullptr.
 */
const void* Layout::GetLayoutResourceData(const char* pName) const {
    char path[72];
    nn::util::SNPrintf(path, sizeof(path), "%s.bflyt", pName);
    return mResourceAccessor->FindResourceByName(ResourceTypeLayout, path);
}

/**
 * @brief Create a view of the property overrides of a parts pane.
 * @param pResParts Parts pane block.
 */
Layout::PartsBuildDataAccessor::PartsBuildDataAccessor(const ResParts* pResParts) {
    m_PropertyCount = pResParts->propertyCount;
    m_pProperties = reinterpret_cast<const ResPartsProperty*>(pResParts + 1);
    m_pResParts = pResParts;
}

/**
 * @brief Find the property override of a pane.
 * @param pName Pane name, compared through at most 24 bytes.
 * @return Property, or nullptr when the pane is not overridden.
 */
const ResPartsProperty* Layout::PartsBuildDataAccessor::FindPartsPropertyFromName(const char* pName) const {
    for (int i = 0; i < m_PropertyCount; ++i) {
        if (MatchesPaneName(pName, m_pProperties[i].name)) {
            return &m_pProperties[i];
        }
    }

    return nullptr;
}

/**
 * @brief Check whether the parts pane overrides any of its panes.
 * @return True when at least one property names a pane.
 */
bool Layout::PartsBuildDataAccessor::IsOverwriting() const {
    for (int i = 0; i < m_PropertyCount; ++i) {
        if (m_pProperties[i].name[0] != '\0') {
            return true;
        }
    }

    return false;
}

/**
 * @brief Access the override block of a property.
 * @param pProperty Property of this parts pane.
 * @return Override pane block, or nullptr when the property has none.
 */
const void* Layout::PartsBuildDataAccessor::GetPropertyResBlock(const ResPartsProperty* pProperty) const {
    if (pProperty->propertyOffset == 0) {
        return nullptr;
    }

    return reinterpret_cast<const u8*>(m_pResParts) + pProperty->propertyOffset;
}

/**
 * @brief Access the extended user data that overrides a pane's own user data.
 * @param pIsOverride Receives whether the pane's user data is replaced.
 * @param pProperty Property of this parts pane.
 * @return Replacement user data, or nullptr when the user data is removed or kept.
 */
const ResExtUserDataList*
Layout::PartsBuildDataAccessor::GetExtUserDataListResBlock(bool* pIsOverride,
                                                            const ResPartsProperty* pProperty) const {
    if (pProperty->extUserDataOffset == 0) {
        *pIsOverride = false;
        return nullptr;
    }

    if (pProperty->extUserDataOffset == 1) {
        *pIsOverride = true;
        return nullptr;
    }

    const u8 overrideFlag = pProperty->systemExtUserDataOverrideFlag;
    if (overrideFlag == 1) {
        *pIsOverride = false;
        return nullptr;
    }

    *pIsOverride = true;
    if (overrideFlag == 2) {
        return nullptr;
    }

    return reinterpret_cast<const ResExtUserDataList*>(reinterpret_cast<const u8*>(m_pResParts) +
                                                       pProperty->extUserDataOffset);
}

/**
 * @brief Access the extended user data block of a property without interpreting override flags.
 * @param pProperty Property of this parts pane with a user data block.
 * @return User data block.
 */
const ResExtUserDataList*
Layout::PartsBuildDataAccessor::GetExtUserDataListResBlockRaw(const ResPartsProperty* pProperty) const {
    return reinterpret_cast<const ResExtUserDataList*>(reinterpret_cast<const u8*>(m_pResParts) +
                                                       pProperty->extUserDataOffset);
}

/**
 * @brief Access the basic pane information overriding a parts pane's own values.
 * @param pProperty Property of this parts pane.
 * @return Basic information block, or nullptr when the property has none.
 */
const void* Layout::PartsBuildDataAccessor::GetPartsPaneBasicInfoResBlock(const ResPartsProperty* pProperty) const {
    if (pProperty->paneBasicInfoOffset == 0) {
        return nullptr;
    }

    return reinterpret_cast<const u8*>(m_pResParts) + pProperty->paneBasicInfoOffset;
}

/**
 * @brief Describe a parts pane whose layout is about to be built.
 * @param pPartsPane Parts pane receiving the layout.
 * @param pResParts Parts block supplying the property overrides.
 * @param pBuildResSet Resource blocks the property overrides refer to.
 * @param pOriginalSize Size of the parts pane in its resource, used to derive the magnification.
 */
Layout::PartsBuildDataSet::PartsBuildDataSet(Parts* pPartsPane, const ResParts* pResParts,
                                             const BuildResSet* pBuildResSet, const ResVec2* pOriginalSize)
    : PartsBuildDataAccessor(pResParts), m_pPartsPane(pPartsPane), m_pPropertyBuildResSet(pBuildResSet) {
    m_Magnify.x = pPartsPane->mSizeX / pOriginalSize->x * pResParts->magnify.x;
    m_Magnify.y = pPartsPane->mSizeY / pOriginalSize->y * pResParts->magnify.y;
}

namespace detail {
/**
 * @brief Construct the pooled texture-sharing records in caller-provided memory.
 * @param pMemory Storage sized for the configured record counts.
 */
BuildPaneTreeContextInstancePool::BuildPaneTreeContextInstancePool(void* pMemory) {
    u8* pCurrent = static_cast<u8*>(pMemory);
    m_CaptureTextureShareInfoCount = Layout::g_CaptureTextureShareInfoCountMax;
    m_pCaptureTextureShareInfos = reinterpret_cast<CaptureTextureShareInfo*>(pCurrent);
    for (int i = 0; i < m_CaptureTextureShareInfoCount; ++i) {
        new (&m_pCaptureTextureShareInfos[i]) CaptureTextureShareInfo();
    }

    pCurrent += sizeof(CaptureTextureShareInfo) * m_CaptureTextureShareInfoCount;
    m_VectorGraphicsTextureShareInfoCount = Layout::g_VectorGraphicsTextureShareInfoCountMax;
    m_pVectorGraphicsTextureShareInfos = reinterpret_cast<VectorGraphicsTextureShareInfo*>(pCurrent);
    for (int i = 0; i < m_VectorGraphicsTextureShareInfoCount; ++i) {
        new (&m_pVectorGraphicsTextureShareInfos[i]) VectorGraphicsTextureShareInfo();
    }

    pCurrent += sizeof(VectorGraphicsTextureShareInfo) * m_VectorGraphicsTextureShareInfoCount;
    m_DynamicTextureShareInfoCount = Layout::g_DynamicTextureShareInfoCountMax;
    m_pDynamicTextureShareInfos = reinterpret_cast<DynamicTextureShareInfo*>(pCurrent);
    for (int i = 0; i < m_DynamicTextureShareInfoCount; ++i) {
        new (&m_pDynamicTextureShareInfos[i]) DynamicTextureShareInfo();
    }

    m_CaptureTextureShareInfoUsedCount = 0;
    m_VectorGraphicsTextureShareInfoUsedCount = 0;
    m_DynamicTextureShareInfoUsedCount = 0;
}

/** @brief Forget the pooled records; the caller owns their memory. */
BuildPaneTreeContextInstancePool::~BuildPaneTreeContextInstancePool() {
    m_pCaptureTextureShareInfos = nullptr;
    m_pVectorGraphicsTextureShareInfos = nullptr;
    m_pDynamicTextureShareInfos = nullptr;
}

/**
 * @brief Take the next unused capture texture record.
 * @return Unused record.
 */
CaptureTextureShareInfo* BuildPaneTreeContextInstancePool::AllocateCaptureTextureShareInfo() {
    CaptureTextureShareInfo* pInfo = &m_pCaptureTextureShareInfos[m_CaptureTextureShareInfoUsedCount];
    m_CaptureTextureShareInfoUsedCount++;
    return pInfo;
}

/**
 * @brief Take the next unused vector-graphics texture record.
 * @return Unused record.
 */
VectorGraphicsTextureShareInfo* BuildPaneTreeContextInstancePool::AllocateVectorGraphicsTextureShareInfo() {
    VectorGraphicsTextureShareInfo* pInfo = &m_pVectorGraphicsTextureShareInfos[m_VectorGraphicsTextureShareInfoUsedCount];
    m_VectorGraphicsTextureShareInfoUsedCount++;
    return pInfo;
}

/**
 * @brief Take the next unused layout texture-sharing entry.
 * @return Unused entry.
 */
DynamicTextureShareInfo* BuildPaneTreeContextInstancePool::AllocateDynamicTextureShareInfo() {
    DynamicTextureShareInfo* pInfo = &m_pDynamicTextureShareInfos[m_DynamicTextureShareInfoUsedCount];
    m_DynamicTextureShareInfoUsedCount++;
    return pInfo;
}

/** @brief Construct an empty, unlinked entry. */
DynamicTextureShareInfo::DynamicTextureShareInfo()
    : m_pLayout(nullptr), m_pResCaptureTextureList(nullptr), m_pResVectorGraphicsTextureList(nullptr),
      m_pPartsBuildDataAccessor(nullptr), m_pInstancePool(nullptr), m_CaptureTextureCount(0),
      m_VectorGraphicsTextureCount(0), m_IsCaptureTextureAllAllocateInitialized(false),
      m_IsIgnoreCaptureEffectFirstFrameOnly(false) {}

/** @brief Destroy the entry; the recorded textures are owned by their layouts. */
DynamicTextureShareInfo::~DynamicTextureShareInfo() {}

/**
 * @brief Attach the entry to a layout being built.
 * @param pLayout Layout whose dynamic textures the entry collects.
 * @param pAccessor Overrides of the enclosing parts pane, or nullptr.
 * @param pInstancePool Pool providing the texture records.
 */
void DynamicTextureShareInfo::Initialize(Layout* pLayout, const Layout::PartsBuildDataAccessor* pAccessor,
                                         BuildPaneTreeContextInstancePool* pInstancePool) {
    m_pPartsBuildDataAccessor = pAccessor;
    m_pInstancePool = pInstancePool;
    m_CaptureTextureCount = 0;
    m_VectorGraphicsTextureCount = 0;
    m_pLayout = pLayout;
}

/** @brief Detach the entry; nothing is released. */
void DynamicTextureShareInfo::Finalize() {}

/**
 * @brief Record a capture texture that is initialized from its resource.
 * @param pTexture Capture texture to record.
 * @return New record.
 */
CaptureTextureShareInfo* DynamicTextureShareInfo::AddCaptureTexture(CaptureTexture* pTexture) {
    CaptureTextureShareInfo* pInfo = m_pInstancePool->AllocateCaptureTextureShareInfo();
    pInfo->pCaptureTexture = pTexture;
    pInfo->pResCaptureTexture = nullptr;
    pInfo->pCopySource = nullptr;
    pInfo->pPaneName = nullptr;
    m_CaptureTextureList.push_back(*pInfo);
    m_CaptureTextureCount++;
    return pInfo;
}

/**
 * @brief Record a capture texture that is initialized as a copy of another one.
 * @param pTexture Capture texture to record.
 * @param pSource Capture texture to copy.
 * @param pPaneName Name of the pane rendered by the texture.
 * @return New record.
 */
CaptureTextureShareInfo* DynamicTextureShareInfo::AddCaptureTexture(CaptureTexture* pTexture,
                                                                    const CaptureTexture* pSource,
                                                                    const char* pPaneName) {
    CaptureTextureShareInfo* pInfo = m_pInstancePool->AllocateCaptureTextureShareInfo();
    pInfo->pCaptureTexture = pTexture;
    pInfo->pResCaptureTexture = nullptr;
    pInfo->pCopySource = pSource;
    pInfo->pPaneName = pPaneName;
    m_CaptureTextureList.push_back(*pInfo);
    m_CaptureTextureCount++;
    return pInfo;
}

/**
 * @brief Record a vector-graphics texture.
 * @param pTexture Vector-graphics texture to record.
 * @return New record.
 */
VectorGraphicsTextureShareInfo* DynamicTextureShareInfo::AddVectorGraphicsTexture(VectorGraphicsTexture* pTexture) {
    VectorGraphicsTextureShareInfo* pInfo = m_pInstancePool->AllocateVectorGraphicsTextureShareInfo();
    pInfo->pVectorGraphicsTexture = pTexture;
    m_VectorGraphicsTextureList.push_back(*pInfo);
    m_VectorGraphicsTextureCount++;
    return pInfo;
}

/**
 * @brief Create and record a capture texture.
 * @param pName Name of the capture texture.
 * @return New capture texture.
 */
CaptureTexture* DynamicTextureShareInfo::CreateCaptureTexture(const char* pName) {
    CaptureTexture* pTexture = NewAlignedLayoutObject<CaptureTexture>(16, pName);
    AddCaptureTexture(pTexture);
    return pTexture;
}

/**
 * @brief Create and record a vector-graphics texture.
 * @param pName Name of the vector-graphics texture.
 * @return New vector-graphics texture.
 */
VectorGraphicsTexture* DynamicTextureShareInfo::CreateVectorGraphicsTexture(const char* pName) {
    VectorGraphicsTexture* pTexture = NewAlignedLayoutObject<VectorGraphicsTexture>(16, pName);
    AddVectorGraphicsTexture(pTexture);
    return pTexture;
}

/**
 * @brief Create and record a capture texture and resolve its initialization resource.
 * @param pName Name of the capture texture.
 * @return New capture texture.
 */
CaptureTexture* DynamicTextureShareInfo::CreateAndSetupCaptureTexture(const char* pName) {
    CaptureTexture* pTexture = NewAlignedLayoutObject<CaptureTexture>(16, pName);
    SetupCaptureTextureInitializeResource(AddCaptureTexture(pTexture));
    return pTexture;
}

/**
 * @brief Resolve the resource a recorded capture texture is initialized from.
 * @param pInfo Record of the capture texture.
 */
void DynamicTextureShareInfo::SetupCaptureTextureInitializeResource(CaptureTextureShareInfo* pInfo) {
    const ResCaptureTexture* pResource =
        FindCaptureTextureResource(m_pResCaptureTextureList, pInfo->pCaptureTexture->GetName());
    const char* pPaneName = reinterpret_cast<const char*>(m_pResCaptureTextureList) +
                            reinterpret_cast<const u32*>(pResource)[1];
    const ResCaptureTexture* pOverride = FindCapturePaneOverrideResource(pPaneName);
    pInfo->pResCaptureTexture = pOverride != nullptr ? pOverride : pResource;
    pInfo->pPaneName = pPaneName;
}

/**
 * @brief Find or create the capture texture that copies another layout's capture texture.
 * @param pIsCreated Receives whether a new capture texture was created; may be nullptr.
 * @param pSource Capture texture to copy.
 * @return Existing or new capture texture.
 */
CaptureTexture* DynamicTextureShareInfo::CopyCaptureTexture(bool* pIsCreated, const CaptureTexture* pSource) {
    if (pIsCreated != nullptr) {
        *pIsCreated = false;
    }

    const char* pName = pSource->GetName();
    CaptureTexture* pTexture = FindCaptureTexture(pName);
    if (pTexture != nullptr) {
        return pTexture;
    }

    pTexture = NewAlignedLayoutObject<CaptureTexture>(16, pName);
    AddCaptureTexture(pTexture, pSource, pSource->GetTargetPane()->GetName());
    pTexture->m_DrawPriority = pSource->m_DrawPriority;
    if (pIsCreated != nullptr) {
        *pIsCreated = true;
    }

    return pTexture;
}

/**
 * @brief Find a recorded capture texture by its name.
 * @param pName Name of the capture texture.
 * @return Capture texture, or nullptr when none is recorded.
 */
CaptureTexture* DynamicTextureShareInfo::FindCaptureTexture(const char* pName) const {
    if (m_CaptureTextureCount != 0) {
        for (auto iter = m_CaptureTextureList.begin(); iter != m_CaptureTextureList.end();) {
            CaptureTexture* pTexture = (iter++)->pCaptureTexture;
            if (std::strcmp(pName, pTexture->GetName()) == 0) {
                return pTexture;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Create and record a copy of another layout's vector-graphics texture.
 * @param pDevice Graphics device used for the copy.
 * @param pLayout Layout that owns the copy.
 * @param pSource Vector-graphics texture to copy.
 * @return New vector-graphics texture.
 */
VectorGraphicsTexture* DynamicTextureShareInfo::CopyVectorGraphicsTexture(nn::gfx::Device* pDevice,
                                                                         const Layout* pLayout,
                                                                         const VectorGraphicsTexture* pSource) {
    void* pMemory = Layout::AllocateMemory(sizeof(VectorGraphicsTexture), 16);
    VectorGraphicsTexture* pTexture =
        pMemory != nullptr ? new (pMemory) VectorGraphicsTexture(*pSource, pDevice, pLayout) : nullptr;
    AddVectorGraphicsTexture(pTexture);
    return pTexture;
}

/**
 * @brief Resolve the initialization resource of a recorded capture texture found by name.
 * @param pName Name of the capture texture.
 */
void DynamicTextureShareInfo::SetupCaptureTextureInitializeResource(const char* pName) {
    SetupCaptureTextureInitializeResource(FindCaptureTextureShareInfo(pName));
}

/**
 * @brief Find the record of a capture texture by its name.
 * @param pName Name of the capture texture.
 * @return Record, or nullptr when none is recorded.
 */
CaptureTextureShareInfo* DynamicTextureShareInfo::FindCaptureTextureShareInfo(const char* pName) const {
    if (m_CaptureTextureCount != 0) {
        for (auto iter = m_CaptureTextureList.begin(); iter != m_CaptureTextureList.end();) {
            const CaptureTextureShareInfo& rInfo = *iter++;
            if (std::strcmp(pName, rInfo.pCaptureTexture->GetName()) == 0) {
                return const_cast<CaptureTextureShareInfo*>(&rInfo);
            }
        }
    }

    return nullptr;
}

/**
 * @brief Find the capture settings of the enclosing parts pane that override a capture pane.
 * @param pPaneName Name of the capture pane.
 * @return Override capture texture resource, or nullptr when the pane is not overridden.
 */
const ResCaptureTexture* DynamicTextureShareInfo::FindCapturePaneOverrideResource(const char* pPaneName) const {
    if (m_pPartsBuildDataAccessor == nullptr) {
        return nullptr;
    }

    const ResPartsProperty* pProperty = m_pPartsBuildDataAccessor->FindPartsPropertyFromName(pPaneName);
    if (pProperty == nullptr) {
        return nullptr;
    }

    const auto* pBlock = static_cast<const ResPane*>(m_pPartsBuildDataAccessor->GetPropertyResBlock(pProperty));
    if (pBlock == nullptr || pBlock->signature != DataBlockKindCapture) {
        return nullptr;
    }

    return GetCaptureTextureResource(pBlock);
}

/**
 * @brief Find a recorded vector-graphics texture by its name.
 * @param pName Name of the vector-graphics texture.
 * @return Vector-graphics texture, or nullptr when none is recorded.
 */
VectorGraphicsTexture* DynamicTextureShareInfo::FindVectorGraphicsTexture(const char* pName) const {
    if (m_VectorGraphicsTextureCount != 0) {
        for (auto iter = m_VectorGraphicsTextureList.begin(); iter != m_VectorGraphicsTextureList.end();) {
            VectorGraphicsTexture* pTexture = (iter++)->pVectorGraphicsTexture;
            if (std::strcmp(pName, pTexture->GetName()) == 0) {
                return pTexture;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Load the bnvg file of a recorded vector-graphics texture and initialize the texture.
 * @param pResult Optional build-result storage.
 * @param pDevice Graphics device used for the texture.
 * @param pLayout Layout that owns the texture.
 * @param pResourceAccessor Resource accessor providing the bnvg file.
 * @param pName Name of the vector-graphics texture.
 */
void DynamicTextureShareInfo::InitializeVectorGraphicsTexture(BuildResultInformation* pResult,
                                                              nn::gfx::Device* pDevice, Layout* pLayout,
                                                              ResourceAccessor* pResourceAccessor,
                                                              const char* pName) {
    VectorGraphicsTexture* pTexture = FindVectorGraphicsTexture(pName);
    const ResVectorGraphicsTexture* pResource =
        FindVectorGraphicsTextureResource(m_pResVectorGraphicsTextureList, pName);
    const char* pFileName = reinterpret_cast<const char*>(m_pResVectorGraphicsTextureList) +
                            reinterpret_cast<const u32*>(pResource)[0] + 12;

    char path[72];
    nn::util::SNPrintf(path, sizeof(path), "%s.bnvg", pFileName);
    auto* pFileHeader = static_cast<nn::util::BinaryFileHeader*>(
        pResourceAccessor->FindResourceByName(ResourceTypeVectorGraphics, path));
    if (!pFileHeader->IsRelocated()) {
        pFileHeader->GetRelocationTable()->Relocate();
    }

    pTexture->Initialize(pResult, pDevice, pResourceAccessor, pLayout, pResource, pFileName,
                         reinterpret_cast<const BnvgFileHeader*>(pFileHeader));
}

/**
 * @brief Access the layout whose dynamic textures the entry collects.
 * @return Layout.
 */
Layout* DynamicTextureShareInfo::GetLayout() const {
    return m_pLayout;
}

/**
 * @brief Check whether capture textures allocate their render targets when initialized.
 * @return Flag value.
 */
bool DynamicTextureShareInfo::IsCaptureTextureAllAllocateInitialized() const {
    return m_IsCaptureTextureAllAllocateInitialized;
}

/**
 * @brief Check whether capture effects ignore their first-frame-only setting.
 * @return Flag value.
 */
bool DynamicTextureShareInfo::IsIgnoreCaptureEffectFirstFrameOnlyFlag() const {
    return m_IsIgnoreCaptureEffectFirstFrameOnly;
}

/**
 * @brief Create a context that carves its state from caller-provided memory on Initialize.
 * @param pMemory Scratch memory of CalculateContextRequireMemorySize bytes.
 * @param memorySize Size of pMemory.
 */
BuildPaneTreeContext::BuildPaneTreeContext(void* pMemory, size_t memorySize)
    : m_pShareInfoStack(nullptr), m_ShareInfoStackDepth(0), m_ShareInfoStackMax(0), m_pShareInfoList(nullptr),
      m_ShareInfoInstanceUsedCount(0), m_pInstancePool(nullptr), m_pMemory(pMemory) {}

/** @brief Destroy the context; Finalize releases its state. */
BuildPaneTreeContext::~BuildPaneTreeContext() {}

/**
 * @brief Access a stack entry counted from the outermost layout.
 * @param offset Stack index.
 * @return Texture-sharing entry.
 */
DynamicTextureShareInfo* BuildPaneTreeContext::GetTextureShareInfoFromRootOffset(int offset) const {
    return m_pShareInfoStack[offset];
}

/**
 * @brief Access a texture-sharing entry in creation order.
 * @param index Creation index.
 * @return Entry, or nullptr when index is out of range.
 */
DynamicTextureShareInfo* BuildPaneTreeContext::GetTextureShareInfoInstanceIndex(int index) const {
    int i = 0;
    for (auto iter = m_pShareInfoList->begin(); iter != m_pShareInfoList->end(); ++iter, ++i) {
        if (i == index) {
            return &*iter;
        }
    }

    return nullptr;
}

/**
 * @brief Access the number of nested layouts being built.
 * @return Stack depth.
 */
int BuildPaneTreeContext::GetCurrentShareInfoStackDepth() const {
    return m_ShareInfoStackDepth;
}

/**
 * @brief Access the stack index of the layout currently being built.
 * @return Stack index.
 */
int BuildPaneTreeContext::GetCurrentShareInfoStackOffset() const {
    return m_ShareInfoStackDepth - 1;
}

/**
 * @brief Count the texture-sharing entries created so far.
 * @return Number of entries.
 */
int BuildPaneTreeContext::GetTextureShareInfoInstanceUsedCount() const {
    return m_ShareInfoInstanceUsedCount;
}

/**
 * @brief Access an entry of an enclosing layout.
 * @param offset Number of layouts above the current one.
 * @return Texture-sharing entry.
 */
DynamicTextureShareInfo* BuildPaneTreeContext::GetUpperOffsetedShareInfo(int offset) {
    const int index = m_ShareInfoStackDepth - 1 - offset;
    return m_pShareInfoStack[index];
}
}  // namespace detail

namespace {
/**
 * @brief Convert a resource offset stored in a pointer field into a pointer.
 * @tparam T Pointed-to type.
 * @param rPointer Field holding an offset from pBase, or zero for a null pointer.
 * @param pBase Base address of the offsets.
 */
template <typename T>
inline void RelocatePointer(T*& rPointer, void* pBase) {
    const u64 offset = reinterpret_cast<u64>(rPointer);
    rPointer = offset == 0 ? nullptr : reinterpret_cast<T*>(static_cast<u8*>(pBase) + offset);
}

/**
 * @brief Duplicate a resource name through the layout allocator.
 * @param pName Name of at most 32 characters to copy.
 * @return Null-terminated copy.
 */
ALWAYS_INLINE inline char* DuplicateName(const char* pName) {
    const int length = nn::util::Strnlen(pName, 32);
    char* pCopy = static_cast<char*>(Layout::AllocateMemory(length + 1));
    nn::util::Strlcpy(pCopy, pName, length + 1);
    return pCopy;
}

/**
 * @brief Select which of the three texture SRT targets a feature parameter kind animates.
 * @param kind Feature parameter kind.
 * @return Texture index, or -1 for kinds that do not animate a texture SRT.
 */
inline int GetTextureSrtIndex(int kind) {
    switch (kind) {
    case 14:
    case 15:
    case 16:
        return 0;
    case 17:
    case 18:
    case 19:
        return 1;
    case 20:
    case 21:
    case 22:
        return 2;
    default:
        return -1;
    }
}
}  // namespace

namespace {
/**
 * @brief Check whether a layer animates the state layers of a parts pane.
 * @param rFeatureParameters Feature parameters of the layer.
 * @return True when a feature parameter drives a parts state layer.
 */
inline bool HasPartsStateLayerParameter(const FeatureParameterList& rFeatureParameters) {
    for (auto& rParameter : rFeatureParameters) {
        if (rParameter.m_Kind == 27) {
            return true;
        }
    }

    return false;
}
}  // namespace

/** @brief Allocate the event pool and put every event into the free list. */
inline void StateMachineEventQueue::Initialize() {
    m_pEvents = Layout::NewArray<StateMachineEvent>(20);
    for (int i = 0; i < 20; ++i) {
        m_FreeEvents.push_back(m_pEvents[i]);
    }

    for (auto iter = m_QueuedEvents.begin(); iter != m_QueuedEvents.end();) {
        m_FreeEvents.push_back(*iter++);
    }
}

/**
 * @brief Convert the offsets of a state machine resource into pointers.
 * @param pResource State machine block.
 * @param pBase Base address of the offsets.
 */
inline void StateMachineFactory::DoRelocate_(ResStateMachine* pResource, void* pBase) {
    if (pResource->isRelocated != 0) {
        return;
    }

    pResource->isRelocated = 1;
    RelocatePointer(pResource->pLayers, pBase);
    RelocatePointer(pResource->pVariables, pBase);
    RelocatePointer(pResource->_38, pBase);
    for (u32 i = 0; i < pResource->variableCount; ++i) {
        ResStateVariableDescriptions& rVariable = pResource->pVariables[i];
        RelocatePointer(rVariable.pCalculatedVariables, pBase);
        for (u32 j = 0; j < rVariable.calculatedVariableCount; ++j) {
            RelocatePointer(rVariable.pCalculatedVariables[j].pCalculation, pBase);
        }
    }

    for (u32 i = 0; i < pResource->layerCount; ++i) {
        ResStateLayer& rLayer = pResource->pLayers[i];
        RelocatePointer(rLayer.pStates, pBase);
        for (u32 j = 0; j < rLayer.stateCount; ++j) {
            RelocatePointer(rLayer.pStates[j].pFeatureParameters, pBase);
            RelocatePointer(rLayer.pStates[j].pPartsStateLayers, pBase);
        }

        RelocatePointer(rLayer.pTransitions, pBase);
        for (u32 j = 0; j < rLayer.transitionCount; ++j) {
            ResStateTransition& rTransition = rLayer.pTransitions[j];
            RelocatePointer(rTransition.pTracks, pBase);
            for (u32 k = 0; k < rTransition.trackCount; ++k) {
                RelocatePointer(rTransition.pTracks[k].pKeys, pBase);
            }

            RelocatePointer(rTransition.pTriggerData, pBase);
        }
    }
}

/**
 * @brief Build a state layer with its feature parameters, states and transitions.
 * @param pResource State layer resource.
 * @return New state layer in its initial state.
 */
inline StateLayer* StateMachineFactory::BuildStateLayer_(ResStateLayer* pResource) {
    StateLayer* pStateLayer = Layout::NewObj<StateLayer>();
    BuildFeatureParameters_(pStateLayer, pResource);
    pStateLayer->Initialize(m_pDevice, m_pLayout, pResource->name);
    BuildStates_(pStateLayer, pResource);
    BuildTransitions_(pStateLayer, pResource);
    pStateLayer->m_InitialStateIndex = pResource->initialStateIndex;
    if (pResource->targetPaneName[0] != '\0') {
        pStateLayer->m_pTargetPane = m_pLayout->GetRootPane()->FindPaneByName(pResource->targetPaneName, true);
    }

    pStateLayer->RestoreToInitialState();
    return pStateLayer;
}

/**
 * @brief Create the feature parameters described by the first state of a layer.
 * @param pStateLayer Layer receiving the feature parameters.
 * @param pResource State layer resource.
 */
inline void StateMachineFactory::BuildFeatureParameters_(StateLayer* pStateLayer, ResStateLayer* pResource) {
    const ResState* pState = pResource->pStates;
    for (u32 i = 0; i < pState->featureParameterCount; ++i) {
        FeatureParameter* pParameter = Layout::NewObj<FeatureParameter>();
        const ResStateFeatureParameter& rResParameter = pState->pFeatureParameters[i];
        const auto kind = static_cast<StateMachineFeatureParameterKind>(rResParameter.kind);
        switch (rResParameter.kind) {
        case 0: {
            const u8 targets[] = {0};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 1, targets);
            break;
        }
        case 1: {
            const u8 targets[] = {0, 1, 2};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 3, targets);
            break;
        }
        case 2: {
            const u8 targets[] = {6, 7};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 2, targets);
            break;
        }
        case 3: {
            const u8 targets[] = {3, 4, 5};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 3, targets);
            break;
        }
        case 4: {
            const u8 targets[] = {8, 9};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 2, targets);
            break;
        }
        case 5: {
            const u8 targets[] = {0};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'V', 'I'), 1, targets);
            break;
        }
        case 6: {
            const u8 targets[] = {16};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'V', 'C'), 1, targets);
            break;
        }
        case 8: {
            const u8 targets[] = {3, 4};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'M', 'T'), 2, targets);
            break;
        }
        case 9: {
            const u8 targets[] = {2};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'M', 'T'), 1, targets);
            break;
        }
        case 10: {
            const u8 targets[] = {0, 0};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'M', 'T'), 2, targets);
            break;
        }
        case 11: {
            const u8 targets[] = {0, 4};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'S'), 2, targets);
            break;
        }
        case 12: {
            const u8 targets[] = {4, 5, 6, 7};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(1),
                                   MakeSignature('F', 'L', 'M', 'C'), 4, targets);
            break;
        }
        case 13: {
            const u8 targets[] = {0, 1, 2, 3};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(1),
                                   MakeSignature('F', 'L', 'M', 'C'), 4, targets);
            break;
        }
        case 14:
        case 17:
        case 20: {
            const u8 targets[] = {3, 4};
            pParameter->Initialzie(rResParameter.name, kind, GetTextureSrtIndex(rResParameter.kind),
                                   AnimContentType(1), MakeSignature('F', 'L', 'T', 'S'), 2, targets);
            break;
        }
        case 15:
        case 18:
        case 21: {
            const u8 targets[] = {2};
            pParameter->Initialzie(rResParameter.name, kind, GetTextureSrtIndex(rResParameter.kind),
                                   AnimContentType(1), MakeSignature('F', 'L', 'T', 'S'), 1, targets);
            break;
        }
        case 16:
        case 19:
        case 22: {
            const u8 targets[] = {0, 1};
            pParameter->Initialzie(rResParameter.name, kind, GetTextureSrtIndex(rResParameter.kind),
                                   AnimContentType(1), MakeSignature('F', 'L', 'T', 'S'), 2, targets);
            break;
        }
        case 25: {
            const u8 targets[] = {0};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(3),
                                   MakeSignature('F', 'S', 'M', 'A'), 1, targets);
            break;
        }
        case 26: {
            const u8 targets[] = {0, 0, 0, 0};
            const ResStatePartsStateLayer* pPartsStateLayer = &pState->pPartsStateLayers[i];
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(4),
                                   MakeSignature('F', 'P', 'S', 'M'), 4, targets);
            pParameter->m_pExtraResource = pPartsStateLayer;
            break;
        }
        case 27: {
            const u8 targets[] = {0};
            pParameter->Initialzie(rResParameter.name, kind, 0, AnimContentType(0),
                                   MakeSignature('F', 'L', 'P', 'A'), 0, targets);
            break;
        }
        default:
            return;
        }

        pStateLayer->m_FeatureParameters.push_back(*pParameter);
    }
}

/**
 * @brief Prepare a state layer for use after its feature parameters were built.
 * @param pDevice Unused graphics device.
 * @param pLayout Layout whose panes the layer animates.
 * @param pName Name of the layer.
 */
inline void StateLayer::Initialize(nn::gfx::Device* pDevice, Layout* pLayout, const char* pName) {
    for (auto& rParameter : m_FeatureParameters) {
        if (pLayout->GetRootPane()->FindPaneByName(rParameter.m_pName, true) == nullptr) {
            return;
        }
    }

    m_HasPartsStateLayer = HasPartsStateLayerParameter(m_FeatureParameters);
    m_pLayout = pLayout;
    m_pName = DuplicateName(pName);
    m_AnimatorSlot.Initialize(pLayout, m_FeatureParameters);
    m_pCurrentState = nullptr;
    m_IsPlaying = false;
    m_IsPaused = false;
    m_StoreSet.Initialzie(m_FeatureParameters);
}

/**
 * @brief Create the states of a layer and fill in their feature parameter values.
 * @param pStateLayer Layer receiving the states.
 * @param pResource State layer resource.
 */
inline void StateMachineFactory::BuildStates_(StateLayer* pStateLayer, ResStateLayer* pResource) {
    for (u32 i = 0; i < pResource->stateCount; ++i) {
        const ResState* pResStates = pResource->pStates;
        State* pState = Layout::NewObj<State>();
        const ResState& rResState = pResStates[i];
        pState->Initialzie(rResState.name, pStateLayer->m_FeatureParameters);
        for (u32 j = 0; j < rResState.featureParameterCount; ++j) {
            const ResStateFeatureParameter& rResParameter = rResState.pFeatureParameters[j];
            float values[4];
            values[0] = rResParameter.values[0];
            values[1] = rResParameter.values[1];
            values[2] = rResParameter.values[2];
            values[3] = rResParameter.values[3];

            u32 index = 0;
            for (auto& rStore : pState->m_StoreSet.m_Stores) {
                if (j == index) {
                    for (int k = 0; k < rStore.m_Count; ++k) {
                        FeatureParameterValue& rValue = rStore.m_pValues[k];
                        for (int m = 0; m < rValue.count; ++m) {
                            rValue.pValues[m] = m < 4 ? values[m] : 0.0f;
                        }
                    }
                }

                ++index;
            }
        }

        pStateLayer->m_States.push_back(*pState);
    }
}

/**
 * @brief Create the transitions of a layer with their timelines and trigger conditions.
 * @param pStateLayer Layer receiving the transitions.
 * @param pResource State layer resource.
 */
inline void StateMachineFactory::BuildTransitions_(StateLayer* pStateLayer, ResStateLayer* pResource) {
    for (u32 i = 0; i < pResource->transitionCount; ++i) {
        ResStateTransition* pResTransitions = pResource->pTransitions;
        Transition* pTransition = Layout::NewObj<Transition>();
        ResStateTransition& rResTransition = pResTransitions[i];
        pTransition->Initialzie(rResTransition.name, rResTransition.sourceStateName,
                                rResTransition.destinationStateName, rResTransition.isCancelable == 1,
                                rResTransition.isLoop == 1, rResTransition.isEnabled == 1);

        TransitionTimeline* pTimeline = Layout::NewObj<TransitionTimeline>();
        pTransition->m_pTimeline = pTimeline;
        pTimeline->duration = rResTransition.duration;
        pTimeline->trackCount = rResTransition.trackCount;
        pTimeline->pTracks = Layout::NewArray<TransitionTimelineTrack>(rResTransition.trackCount);
        for (u32 j = 0; j < rResTransition.trackCount; ++j) {
            const ResStateTransitionTrack& rResTrack = rResTransition.pTracks[j];
            TransitionTimelineTrack& rTrack = pTimeline->pTracks[j];
            rTrack.offset = rResTrack.offset;
            rTrack.duration = rResTrack.duration;
            rTrack.easingType = rResTrack.easingType;
            rTrack.easingExtra = rResTrack.easingExtra;
            rTrack.keyCount = rResTrack.keyCount;
            rTrack.pKeys = Layout::NewArray<TransitionTimelineKey>(rResTrack.keyCount);
            for (int k = 0; k < rResTrack.keyCount; ++k) {
                TransitionTimelineKey* pKey = k < rTrack.keyCount ? &rTrack.pKeys[k] : nullptr;
                const ResStateTransitionKey& rResKey = rResTrack.pKeys[k];
                pKey->time = rResKey.time;
                pKey->scale = 1.0f;
                pKey->easingType = rResKey.easingType;
                pKey->easingExtra = rResKey.easingExtra;
                pKey->isCurve = rResKey.curveType != 0;
                if (rResKey.curveType != 0) {
                    pKey->parameter0 = rResKey.curveParameter;
                    pKey->parameter1 = rResKey.curveType;
                    pKey->pCurve0 = rResKey.curve0;
                    pKey->pCurve1 = rResKey.curve1;
                    pKey->pCurve2 = rResKey.curve2;
                } else {
                    pKey->parameter0 = rResKey.parameters[0];
                    pKey->parameter1 = rResKey.parameters[1];
                    pKey->parameter2 = rResKey.parameters[2];
                    pKey->parameter3 = rResKey.parameters[3];
                }
            }
        }

        pStateLayer->m_Transitions.push_back(*pTransition);
        BuildTransitionTrigger_(pTransition, pResource, &rResTransition);
    }
}

/** @brief Return the layer to its initial state and release the animation it generated. */
inline NOINLINE void StateLayer::RestoreToInitialState() {
    State* pInitialState = FindStateByName("In") != nullptr ? FindStateByName("In") : &m_States.front();
    m_pCurrentState = pInitialState;
    ApplyFeatureParameterToTargetAll_(pInitialState);
    m_IsPlaying = false;
    m_IsPaused = false;
    void* pAnimation = m_AnimatorSlot.Unbind();
    if (pAnimation != nullptr && !m_HasPartsStateLayer) {
        Layout::FreeMemory(pAnimation);
    }

    m_TransitionPlayers[0].m_pTransition = nullptr;
}

/**
 * @brief Set up a feature parameter and allocate the storage for its animation targets.
 * @param pName Name of the pane or material the parameter animates.
 * @param kind Kind of the animated feature.
 * @param targetIndex Texture index for texture SRT kinds.
 * @param animContentType Type of the animation content.
 * @param signature Signature of the animation content.
 * @param targetCount Number of animation targets.
 * @param pTargets Animation target indices.
 */
inline void FeatureParameter::Initialzie(const char* pName, StateMachineFeatureParameterKind kind, int targetIndex,
                                  AnimContentType animContentType, u32 signature, int targetCount,
                                  const u8* pTargets) {
    m_pName = DuplicateName(pName);
    m_Kind = kind;
    m_TargetIndex = targetIndex;
    m_AnimContentType = animContentType;
    m_AnimInfoCount = 1;
    m_pAnimInfos = Layout::NewObj<FeatureParameterAnimInfo>();
    m_pAnimInfos->signature = signature;
    m_pAnimInfos->targetCount = targetCount;
    m_pAnimInfos->pTargets = Layout::NewArray<u8>(targetCount);
    for (int i = 0; i < targetCount; ++i) {
        m_pAnimInfos->pTargets[i] = pTargets[i];
    }
}

/**
 * @brief Allocate value storage for every feature parameter.
 * @param rFeatureParameters Feature parameters of the layer.
 */
inline void FeatureParameterStoreSet::Initialzie(const FeatureParameterList& rFeatureParameters) {
    for (auto& rParameter : rFeatureParameters) {
        FeatureParameterStore* pStore = Layout::NewObj<FeatureParameterStore>();
        pStore->m_Count = rParameter.m_AnimInfoCount;
        pStore->m_pValues = Layout::NewArray<FeatureParameterValue>(pStore->m_Count);
        for (int i = 0; i < pStore->m_Count; ++i) {
            const int count = rParameter.m_pAnimInfos[i].targetCount;
            pStore->m_pValues[i].count = count;
            pStore->m_pValues[i].pValues = Layout::NewArray<float>(count);
        }

        m_Stores.push_back(*pStore);
    }
}

/**
 * @brief Name the state and allocate storage for its feature parameter values.
 * @param pName Name of the state.
 * @param rFeatureParameters Feature parameters of the layer.
 */
inline void State::Initialzie(const char* pName, const FeatureParameterList& rFeatureParameters) {
    m_pName = DuplicateName(pName);
    m_StoreSet.Initialzie(rFeatureParameters);
}

/**
 * @brief Name the transition and its states.
 * @param pName Name of the transition.
 * @param pSourceStateName Name of the state the transition starts from.
 * @param pDestinationStateName Name of the state the transition ends in.
 * @param isCancelable Whether another transition may interrupt this one.
 * @param isLoop Whether the transition repeats.
 * @param isEnabled Whether the transition can be triggered.
 */
inline void Transition::Initialzie(const char* pName, const char* pSourceStateName, const char* pDestinationStateName,
                            bool isCancelable, bool isLoop, bool isEnabled) {
    m_pName = DuplicateName(pName);
    m_pSourceStateName = DuplicateName(pSourceStateName);
    m_pDestinationStateName = DuplicateName(pDestinationStateName);
    m_IsCancelable = isCancelable;
    m_IsLoop = isLoop;
    m_IsEnabled = isEnabled;
}

/**
 * @brief Create the condition that triggers a transition.
 * @param pTransition Transition receiving the condition.
 * @param pResource Layer resource whose name identifies the layer.
 * @param pTransitionResource Transition resource describing the trigger.
 */
inline void StateMachineFactory::BuildTransitionTrigger_(Transition* pTransition, ResStateLayer* pResource,
                                                         ResStateTransition* pTransitionResource) {
    switch (pTransitionResource->triggerKind) {
    case 0:
        pTransition->m_pCondition = Layout::NewObj<TransitionConditionNone>();
        break;
    case 1: {
        auto* pIsHit = Layout::NewObj<TransitionConditionIsHit>();
        pIsHit->m_pTargetName = pResource->name;
        pTransition->m_pCondition = pIsHit;
        break;
    }
    case 2: {
        auto* pIsNoHit = Layout::NewObj<TransitionConditionIsNoHit>();
        pIsNoHit->m_pTargetName = pResource->name;
        pTransition->m_pCondition = pIsNoHit;
        break;
    }
    case 3:
        pTransition->m_pCondition = Layout::NewObj<TransitionConditionIsDecided>();
        break;
    case 4: {
        auto* pCompleted = Layout::NewObj<TransitionConditionIsStateTransitionCompleted>();
        pCompleted->m_Type = 5;
        pCompleted->m_pLayerName = pResource->name;
        pCompleted->m_pTransitionName = pTransitionResource->name;
        pTransition->m_pCondition = pCompleted;
        break;
    }
    case 5: {
        auto* pCompleted = Layout::NewObj<TransitionConditionIsStateTransitionCompleted>();
        const ResStateTransitionTriggerData* pData = pTransitionResource->pTriggerData;
        pCompleted->m_Type = 5;
        pCompleted->m_pStateName = pData->name;
        pCompleted->m_pLayerName = pData->layerName;
        pCompleted->m_pTransitionName = pData->transitionName;
        pTransition->m_pCondition = pCompleted;
        break;
    }
    case 6: {
        auto* pChanged = Layout::NewObj<TransitionConditionVariableChanged>();
        const ResStateTransitionTriggerData* pData = pTransitionResource->pTriggerData;
        nn::util::Strlcpy(pChanged->m_VariableName, pData->name, sizeof(pChanged->m_VariableName));
        pChanged->m_CompareOp = pTransitionResource->compareOp;
        pChanged->SetFloatValue(pData->value);
        pTransition->m_pCondition = pChanged;
        break;
    }
    case 7: {
        auto* pRequested = Layout::NewObj<TransitionConditionStateChangeRequested>();
        pRequested->m_pLayerName = pResource->name;
        pRequested->m_pStateName = pTransitionResource->sourceStateName;
        pTransition->m_pCondition = pRequested;
        break;
    }
    case 8:
        pTransition->m_pCondition = Layout::NewObj<TransitionConditionIsButtonDecided>();
        break;
    default:
        break;
    }
}

/**
 * @brief Check a variable change against the comparison.
 * @param rEvent Event being processed.
 * @return True when the changed variable satisfies the comparison.
 */
inline bool TransitionConditionVariableChanged::IsTriggered(const StateMachineEvent& rEvent) const {
    if (rEvent.type != 7) {
        return false;
    }

    if (std::strcmp(rEvent.pArgument0, m_VariableName) != 0) {
        return false;
    }

    const auto* pVariable = static_cast<const StateMachineVariable*>(rEvent.pArgument1);
    const int valueType = *reinterpret_cast<const int*>(reinterpret_cast<const u8*>(pVariable) + 72);
    switch (m_CompareOp) {
    case TransitionVariableCompareOp_Greater:
        if (valueType == 0) {
            return *reinterpret_cast<const bool*>(&pVariable->value) && !m_BoolValue;
        }

        if (valueType == 1) {
            return pVariable->value > m_Value;
        }

        return false;
    case TransitionVariableCompareOp_Less:
        if (valueType == 1) {
            return pVariable->value < m_Value;
        }

        if (valueType == 0) {
            return !*reinterpret_cast<const bool*>(&pVariable->value) && m_BoolValue;
        }

        return false;
    default:
        return false;
    }
}
}  // namespace nn::ui2d
