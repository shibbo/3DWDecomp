#pragma once

#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::ui2d {
class CaptureTexture;
class Layout;
struct ResCaptureTexture;
struct ResCaptureTextureList;
struct ResVectorGraphicsTextureList;

namespace detail {
class VectorGraphicsTexture;
class BuildPaneTreeContextInstancePool;

/** @brief Capture texture registered while a layout is built, with its initialization source. */
struct CaptureTextureShareInfo {
    /** @brief Construct an empty, unlinked entry. */
    CaptureTextureShareInfo()
        : pCaptureTexture(nullptr), pResCaptureTexture(nullptr), pCopySource(nullptr), pPaneName(nullptr) {}

    CaptureTexture* pCaptureTexture;
    const ResCaptureTexture* pResCaptureTexture;
    const CaptureTexture* pCopySource;
    const char* pPaneName;
    nn::util::IntrusiveListNode m_Link;
};

/** @brief Vector-graphics texture registered while a layout is built. */
struct VectorGraphicsTextureShareInfo {
    /** @brief Construct an empty, unlinked entry. */
    VectorGraphicsTextureShareInfo() : pVectorGraphicsTexture(nullptr) {}

    VectorGraphicsTexture* pVectorGraphicsTexture;
    nn::util::IntrusiveListNode m_Link;
};

/** @brief Dynamic textures created by one layout (or parts layout) of a pane tree. */
struct DynamicTextureShareInfo {
    using CaptureTextureShareInfoList = nn::util::IntrusiveList<
        CaptureTextureShareInfo,
        nn::util::IntrusiveListMemberNodeTraits<CaptureTextureShareInfo, &CaptureTextureShareInfo::m_Link>>;
    using VectorGraphicsTextureShareInfoList =
        nn::util::IntrusiveList<VectorGraphicsTextureShareInfo,
                                nn::util::IntrusiveListMemberNodeTraits<VectorGraphicsTextureShareInfo,
                                                                        &VectorGraphicsTextureShareInfo::m_Link>>;

    DynamicTextureShareInfo();
    ~DynamicTextureShareInfo();

    void Initialize(Layout* pLayout, const Layout::PartsBuildDataAccessor* pAccessor,
                    BuildPaneTreeContextInstancePool* pInstancePool);
    void Finalize();
    void SetResCaptureTextureList(const ResCaptureTextureList* pList);
    void SetCaptureTextureAllAllocateInitialized(bool isInitialized);
    void SetIgnoreCaptureEffectFirstFrameOnlyFlag(bool isIgnored);
    void SetResVectorGraphicsTextureList(const ResVectorGraphicsTextureList* pList);
    int GetCaptureTextureShareInfoCount() const;
    int GetVectorGraphicsTextureShareInfoCount() const;
    const CaptureTextureShareInfo* GetCaptureTextureShareInfoByIndex(int index) const;
    VectorGraphicsTexture* GetVectorGraphicsTextureShareInfoByIndex(int index) const;
    CaptureTextureShareInfo* AddCaptureTexture(CaptureTexture* pTexture);
    CaptureTextureShareInfo* AddCaptureTexture(CaptureTexture* pTexture, const CaptureTexture* pSource,
                                               const char* pPaneName);
    VectorGraphicsTextureShareInfo* AddVectorGraphicsTexture(VectorGraphicsTexture* pTexture);
    CaptureTexture* CreateCaptureTexture(const char* pName);
    VectorGraphicsTexture* CreateVectorGraphicsTexture(const char* pName);
    CaptureTexture* CreateAndSetupCaptureTexture(const char* pName);
    void SetupCaptureTextureInitializeResource(CaptureTextureShareInfo* pInfo);
    CaptureTexture* CopyCaptureTexture(bool* pIsCreated, const CaptureTexture* pSource);
    CaptureTexture* FindCaptureTexture(const char* pName) const;
    VectorGraphicsTexture* CopyVectorGraphicsTexture(nn::gfx::Device* pDevice, const Layout* pLayout,
                                                     const VectorGraphicsTexture* pSource);
    void SetupCaptureTextureInitializeResource(const char* pName);
    CaptureTextureShareInfo* FindCaptureTextureShareInfo(const char* pName) const;
    const ResCaptureTexture* FindCapturePaneOverrideResource(const char* pPaneName) const;
    VectorGraphicsTexture* FindVectorGraphicsTexture(const char* pName) const;
    void InitializeVectorGraphicsTexture(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                         Layout* pLayout, ResourceAccessor* pResourceAccessor,
                                         const char* pName);
    Layout* GetLayout() const;
    bool IsCaptureTextureAllAllocateInitialized() const;
    bool IsIgnoreCaptureEffectFirstFrameOnlyFlag() const;

    nn::util::IntrusiveListNode m_Link;
    Layout* m_pLayout;
    const ResCaptureTextureList* m_pResCaptureTextureList;
    const ResVectorGraphicsTextureList* m_pResVectorGraphicsTextureList;
    const Layout::PartsBuildDataAccessor* m_pPartsBuildDataAccessor;
    BuildPaneTreeContextInstancePool* m_pInstancePool;
    CaptureTextureShareInfoList m_CaptureTextureList;
    int m_CaptureTextureCount;
    VectorGraphicsTextureShareInfoList m_VectorGraphicsTextureList;
    int m_VectorGraphicsTextureCount;
    bool m_IsCaptureTextureAllAllocateInitialized;
    bool m_IsIgnoreCaptureEffectFirstFrameOnly;
};

/** @brief Fixed-capacity storage for the texture-sharing records of one build. */
class BuildPaneTreeContextInstancePool {
public:
    explicit BuildPaneTreeContextInstancePool(void* pMemory);
    ~BuildPaneTreeContextInstancePool();

    CaptureTextureShareInfo* AllocateCaptureTextureShareInfo();
    VectorGraphicsTextureShareInfo* AllocateVectorGraphicsTextureShareInfo();
    DynamicTextureShareInfo* AllocateDynamicTextureShareInfo();

    CaptureTextureShareInfo* m_pCaptureTextureShareInfos;
    int m_CaptureTextureShareInfoUsedCount;
    int m_CaptureTextureShareInfoCount;
    VectorGraphicsTextureShareInfo* m_pVectorGraphicsTextureShareInfos;
    int m_VectorGraphicsTextureShareInfoUsedCount;
    int m_VectorGraphicsTextureShareInfoCount;
    DynamicTextureShareInfo* m_pDynamicTextureShareInfos;
    int m_DynamicTextureShareInfoUsedCount;
    int m_DynamicTextureShareInfoCount;
};

/** @brief Scratch state shared while a pane tree is built or cloned. */
class BuildPaneTreeContext {
public:
    using DynamicTextureShareInfoList = nn::util::IntrusiveList<
        DynamicTextureShareInfo,
        nn::util::IntrusiveListMemberNodeTraits<DynamicTextureShareInfo, &DynamicTextureShareInfo::m_Link>>;

    BuildPaneTreeContext(void* pMemory, size_t memorySize);
    ~BuildPaneTreeContext();

    static size_t CalculateContextRequireMemorySize();

    bool IsInitialized() const;
    void Initialize();
    void Finalize();
    void PushCache(Layout* pLayout, const Layout::PartsBuildDataAccessor* pAccessor);
    void CreateCahceInfoToCurrentStackIndex(Layout* pLayout, const Layout::PartsBuildDataAccessor* pAccessor);
    void PopCache();
    DynamicTextureShareInfo* GetCurrentTextureShareInfo() const;
    DynamicTextureShareInfo* GetTextureShareInfoFromRootOffset(int offset) const;
    DynamicTextureShareInfo* GetTextureShareInfoInstanceIndex(int index) const;
    int GetCurrentShareInfoStackDepth() const;
    int GetCurrentShareInfoStackOffset() const;
    int GetTextureShareInfoInstanceUsedCount() const;
    DynamicTextureShareInfo* GetUpperOffsetedShareInfo(int offset);
    void InitializeCaptureTexturesAfterPaneTreeBuilt(nn::gfx::Device* pDevice);

    DynamicTextureShareInfo** m_pShareInfoStack;
    int m_ShareInfoStackDepth;
    int m_ShareInfoStackMax;
    DynamicTextureShareInfoList* m_pShareInfoList;
    int m_ShareInfoInstanceUsedCount;
    BuildPaneTreeContextInstancePool* m_pInstancePool;
    void* m_pMemory;
};
}  // namespace detail
}  // namespace nn::ui2d
