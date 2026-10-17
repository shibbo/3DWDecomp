/**
 * @file ui2d_Layout.h
 * @brief UI Layout implementation.
 */

#pragma once

#include <nn/types.h>
#include <nn/ui2d/ui2d_Animator.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <new>
#include <nn/font/font_Util.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/util/util_IntrusiveList.h>
namespace nn::font {
template <class T> class TagProcessorBase;
struct Rectangle;
} // namespace nn::font
namespace nn::gfx {
class DescriptorSlot;
class TextureInfo;
} // namespace nn::gfx

namespace nn {
namespace ui2d {
class AnimTransform;
class Pane;
class DrawInfo;
class AnimResource;
class ResourceAccessor;
class LayoutPaneFactory;
enum RenderTargetTextureLifetime : int;
class GroupAnimator;
class GroupContainer;
class GroupArrayAnimator;
struct BuildResultInformation {
    void SetDefault();

    size_t requiredUi2dConstantBufferSize;
    u64 _8;
};
class ControlCreator;
class TextSearcher;
class ShaderInfo;
struct BuildArgSet;
struct ResVectorGraphicsTextureList;
struct ResCaptureTextureList;
struct ResTextureList;
struct ResFontList;
struct ResMaterialList;
struct ResShapeInfoList;
struct ResExtUserDataList;
struct ResVec2;
class Parts;
class CaptureTexture;
class StateMachine;
namespace detail {
class BuildPaneTreeContext;
struct DynamicTextureShareInfo;
class DynamicRenderingTexture;
}  // namespace detail

/** @brief Layout block of a layout resource; the layout name follows it. */
struct ResLayout {
    u32 signature;
    u32 blockSize;
    u8 originType;
    u8 padding[3];
    nn::util::Float2 layoutSize;
    nn::util::Float2 partsSize;

    /**
     * @brief Access the null-terminated layout name stored after the block.
     * @return Layout name.
     */
    const char* GetName() const { return reinterpret_cast<const char*>(this + 1); }
};

class Layout {
  public:
    /** @brief Read-only view of the property overrides stored in a parts pane resource. */
    class PartsBuildDataAccessor {
      public:
        explicit PartsBuildDataAccessor(const ResParts* pResParts);
        const ResPartsProperty* FindPartsPropertyFromName(const char* pName) const;
        bool IsOverwriting() const;
        const void* GetPropertyResBlock(const ResPartsProperty* pProperty) const;
        const ResExtUserDataList* GetExtUserDataListResBlock(bool* pIsOverride,
                                                             const ResPartsProperty* pProperty) const;
        const ResExtUserDataList* GetExtUserDataListResBlockRaw(const ResPartsProperty* pProperty) const;
        const void* GetPartsPaneBasicInfoResBlock(const ResPartsProperty* pProperty) const;

        int m_PropertyCount;
        const ResPartsProperty* m_pProperties;
        const ResParts* m_pResParts;
    };

    /** @brief Parts pane overrides and scaling passed to the build of a parts layout. */
    class PartsBuildDataSet : public PartsBuildDataAccessor {
      public:
        PartsBuildDataSet(Parts* pPartsPane, const ResParts* pResParts, const BuildResSet* pBuildResSet,
                          const ResVec2* pOriginalSize);

        Parts* m_pPartsPane;
        const BuildResSet* m_pPropertyBuildResSet;
        nn::util::Float2 m_Magnify;
    };

    /** @brief Cursor over the blocks of a layout resource while it is built. */
    struct LayoutBuildContext {
        BuildResSet buildResSet;
        BuildArgSet buildArgSet;
        int blockIndex;
        const void* pBlock;
        const ResExtUserDataList* pExtUserDataList;
        u32 blockKind;
        int nextBlockIndex;
        const void* pNextBlock;
        Pane* pLastBuiltPane;
        int groupNestLevel;
    };

    struct BuildOption {
        u64 _0 = 0;
        u64 _8 = 0;
        u64 _10 = 0;
        u64 _18 = 0;

        /**
         * @brief Read one option flag byte.
         * @param offset Byte offset of the flag inside the option block.
         * @return Stored flag.
         */
        bool GetFlag(int offset) const { return reinterpret_cast<const bool*>(this)[offset]; }
    };
    NN_RUNTIME_TYPEINFO_BASE();
    Layout();

    virtual ~Layout();

    virtual void DeleteAnimTransform(AnimTransform* pTransform);
    virtual void BindAnimation(AnimTransform* pTransform);
    virtual void UnbindAnimation(AnimTransform* pTransform);
    virtual void UnbindAnimation(Pane* pPane);
    virtual void UnbindAllAnimation();

    virtual bool BindAnimationAuto(nn::gfx::Device*, const AnimResource&);
    virtual void Animate();
    virtual void UpdateAnimFrame(f32 frame);
    virtual void AnimateAndUpdateAnimFrame(f32 frame);

    virtual void Draw(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    virtual void SetTagProcessor(nn::font::TagProcessorBase<u16>* pProcessor);
    virtual bool BuildImpl(BuildResultInformation*, nn::gfx::Device*, const void*, ResourceAccessor*,
                           const BuildArgSet&, const PartsBuildDataSet*);
    virtual Pane* BuildPartsImpl(BuildResultInformation*, nn::gfx::Device*, const void*,
                                 const PartsBuildDataSet*, BuildArgSet&, BuildResSet&, u32);
    virtual Layout* DoCreatePartsLayout_(const char*, const PartsBuildDataSet&, const BuildArgSet&);
    virtual GroupAnimator* DoCreateAndSetupGroupAnimator_(nn::gfx::Device* pDevice, const char* pName,
                                                          const AnimResource& rResource, bool enabled);
    virtual GroupArrayAnimator* DoCreateAndSetupGroupArrayAnimator_(nn::gfx::Device* pDevice,
                                                                    const char* pName,
                                                                    const AnimResource& rResource,
                                                                    bool enabled);
    virtual Pane* BuildPaneObj(BuildResultInformation*, nn::gfx::Device*, u32, const void*, const void*,
                               const BuildArgSet&);
    virtual Layout* BuildPartsLayout(BuildResultInformation*, nn::gfx::Device*, const char*,
                                     const PartsBuildDataSet&, const BuildArgSet&);
    virtual void CalculateImpl(DrawInfo& rDrawInfo, bool forceDirty);
    virtual void BuildVectorGraphicsTextureList(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                                const ResVectorGraphicsTextureList* pResources,
                                                const char* pName);
    virtual void CalculateVectorGraphicsTexture(DrawInfo&);

    int AcquireAnimTagNameCount() const;
    const char* AcquireAnimTagNameByIndex(int index) const;
    static void FindResPaneByName(const ResPane** ppResPane, const ResExtUserDataList** ppExtUserDataList,
                                  const void* pLayoutResource, const char* pName, const ResPane* pSkipUntil);
    void PrepareBuildArgSet(BuildArgSet& rArgSet, const BuildArgSet& rParentArgSet,
                            const PartsBuildDataSet* pPartsBuildDataSet);
    void SetByResLayout(LayoutBuildContext& rContext);
    void BuildControl(LayoutBuildContext& rContext, nn::gfx::Device* pDevice);
    void BuildPaneByResPane(LayoutBuildContext& rContext, BuildResultInformation* pResult,
                            nn::gfx::Device* pDevice, const PartsBuildDataSet* pPartsBuildDataSet);
    void BuildStateMachine(LayoutBuildContext& rContext, nn::gfx::Device* pDevice,
                           const PartsBuildDataSet* pPartsBuildDataSet);
    void BuildGroup(LayoutBuildContext& rContext);
    void AggregateDynamicTextureList(detail::DynamicTextureShareInfo* pShareInfo);
    void CopyLayoutInstanceImpl(nn::gfx::Device* pDevice, const Layout& rSource, Layout* pPartsLayout,
                                const char* pRootPaneName);
    void CopyLayoutInstanceImpl(nn::gfx::Device* pDevice, const Layout& rSource, Layout* pPartsLayout,
                                const char* pRootPaneName, detail::BuildPaneTreeContext* pContext);
    void DrawCaptureTexture(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    void DiscardDropShadowStaticCachedTexture(const char* pPaneName);
    void DiscardPaneEffectStaticCachedTexture(const char* pPaneName);
    void DiscardPaneEffectStaticCachedTextureImpl(detail::DynamicRenderingTexture* pTexture) const;
    void DiscardPaneEffectStaticCachedTexture();
    void DrawDropShadowStaticCache(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                   nn::gfx::CommandBuffer& rCommands);
    void DrawPaneEffectStaticCache(nn::gfx::Device* pDevice, DrawInfo& rDrawInfo,
                                   nn::gfx::CommandBuffer& rCommands);
    CaptureTexture* FindCaptureTextureByPanePtr(const Pane* pPane);
    void ResetFirstFrameCaptureUpdatedFlag();
    bool CompareCopiedInstanceTest(const Layout& rOther) const;

    using AllocateFunction = void* (*)(size_t, size_t, void*);
    using FreeFunction = void (*)(void*, void*);
    using AnimTransformList = nn::util::IntrusiveList<
        AnimTransform, nn::util::IntrusiveListMemberNodeTraits<AnimTransform, &AnimTransform::m_Link>>;
    using PartsList =
        nn::util::IntrusiveList<Parts, nn::util::IntrusiveListMemberNodeTraits<Parts, &Parts::m_PartsList>>;
    static void SetAllocator(AllocateFunction pAllocate, FreeFunction pFree, void* pArgument);
    static void* AllocateMemory(size_t size, size_t alignment);
    static void* AllocateMemory(size_t size);
    static void FreeMemory(void* pMemory);
    static void SetDynamicTextureInitializationMemoryInfo(int captureCount, int vectorCount, int dynamicCount,
                                                          int stackCount);
    bool BuildWithName(BuildResultInformation* pResult, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                       ControlCreator* pControlCreator, TextSearcher* pTextSearcher,
                       const BuildOption& rOption, const char* pName, bool isUtf8);
    bool Build(BuildResultInformation* pResult, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
               ControlCreator* pControlCreator, TextSearcher* pTextSearcher, const void* pData,
               const BuildOption& rOption, bool isUtf8);
    void Finalize(nn::gfx::Device* pDevice);
    ShaderInfo* AcquireArchiveShader(nn::gfx::Device* pDevice, const char* pName) const;
    ShaderInfo* AcquireArchiveShader(nn::gfx::Device* pDevice, u32 signature, size_t keyCount,
                                     const u32* pKeys) const;
    int CalculateCaptureTextureCountRecursive() const;
    int CalculateVectorGraphicsTextureCountRecursive() const;
    nn::font::Rectangle GetLayoutRect() const;
    Animator* CreateGroupAnimatorAuto(nn::gfx::Device* device, const char* name, bool enabled);
    const void* GetAnimResourceData(const char* pName) const;
    const void* GetLayoutResourceData(const char* pName) const;
    Parts* FindPartsPaneByName(const char* pName);
    const Parts* FindPartsPaneByName(const char* pName) const;
    AnimTransformBasic* CreateAnimTransformBasic();
    AnimTransformBasic* CreateAnimTransformBasic(nn::gfx::Device* pDevice, const void* pData);
    AnimTransformBasic* CreateAnimTransformBasic(nn::gfx::Device* pDevice, const AnimResource& rResource);
    AnimTransformBasic* CreateAnimTransformBasic(nn::gfx::Device* pDevice, const char* pName);
    template <class T> T* CreateAnimTransform(nn::gfx::Device* pDevice, const char* pName);
    template <class T> T* CreateAnimTransform(nn::gfx::Device* pDevice, const AnimResource& rResource);
    template <class T> T* CreateAnimTransform();
    PaneAnimator* CreatePaneAnimator(nn::gfx::Device* pDevice, const char* pName, Pane* pPane, bool enabled);
    GroupAnimator* CreateGroupAnimator(nn::gfx::Device* pDevice, const char* pName, Group* pGroup,
                                       bool enabled);
    GroupAnimator* CreateGroupAnimatorWithIndex(nn::gfx::Device* pDevice, const char* pName, int index,
                                                bool enabled);
    GroupArrayAnimator* CreateGroupArrayAnimator(nn::gfx::Device* pDevice, const AnimResource& rResource,
                                                 bool enabled);
    GroupArrayAnimator* CreateGroupArrayAnimator(nn::gfx::Device* pDevice, const char* pName, bool enabled);
    void CalculateGlobalMatrix(DrawInfo& rDrawInfo, bool forceDirty);
    /**
     * @brief Obtain the typed animation list stored in the layout's intrusive root.
     * @return List of animation transforms owned by this layout.
     */
    AnimTransformList& GetAnimTransformList() {
        return *reinterpret_cast<AnimTransformList*>(&mAnimTransformList);
    }
    /**
     * @brief Obtain the typed list of parts panes whose nested layouts are updated recursively.
     * @return Non-owning list of parts panes registered with this layout.
     */
    PartsList& GetPartsList() { return *reinterpret_cast<PartsList*>(&_48); }

    /**
     * @brief Allocate and value-initialize a contiguous array through the layout allocator.
     * @tparam T Element type to construct.
     * @param count Nonnegative number of elements; the allocation size must fit size_t.
     * @return Constructed array, or nullptr when allocation fails.
     */
    template <typename T> static T* NewArray(int count) {
        void* pMem = Layout::AllocateMemory(sizeof(T) * count);
        if (pMem == nullptr) {
            return nullptr;
        }

        T* const objAry = static_cast<T*>(pMem);

        for (int i = 0; i < count; ++i) {
            new (&objAry[i]) T();
        }

        return objAry;
    }

    /**
     * @brief Destroy array elements and release their layout allocation.
     * @tparam T Allocated element type.
     * @param pObjects Array returned by NewArray, or nullptr to do nothing.
     * @param count Number of constructed elements to destroy.
     */
    template <typename T> static void DeleteArray(T pObjects[], int count) {
        if (pObjects != nullptr) {
            for (int i = 0; i < count; ++i) {
                pObjects[i].~T();
            }
            FreeMemory(pObjects);
        }
    }

    /**
     * @brief Allocate and default-construct one object through the layout allocator.
     * @tparam T Object type to construct.
     * @param args Arguments forwarded to the constructor of T.
     * @return Constructed object, or nullptr when allocation fails.
     */
    template <typename T, typename... Args>
    static T* NewObj(Args&&... args) {
        void* pMem = AllocateMemory(sizeof(T));
        if (pMem == nullptr) {
            return nullptr;
        }

        return new (pMem) T(static_cast<Args&&>(args)...);
    }

    /**
     * @brief Destroy one object and release its layout allocation.
     * @tparam T Object type to destroy.
     * @param pObject Object returned by NewObj, or nullptr to do nothing.
     */
    template <typename T>
    static void DeleteObj(T* pObject) {
        if (pObject != nullptr) {
            pObject->~T();
            FreeMemory(pObject);
        }
    }

    /**
     * @brief Access the layout's root pane.
     * @return Root pane, or nullptr for an empty layout.
     */
    Pane* GetRootPane() const { return mRootPane; }
    /**
     * @brief Access the layout name used when resolving animation resources.
     * @return Stored layout name, or nullptr before a name is assigned.
     */
    const char* GetName() const { return static_cast<const char*>(_30); }
    /**
     * @brief Access the groups owned by this layout.
     * @return Group container, or nullptr before groups are built.
     */
    GroupContainer* GetGroupContainer() const { return static_cast<GroupContainer*>(_20); }
    /**
     * @brief Access the resource provider associated with the layout.
     * @return Resource accessor, or nullptr before the layout is built.
     */
    ResourceAccessor* GetResourceAccessor() const { return mResourceAccessor; }
    /**
     * @brief Dispatch pane calculation through the layout's calculation hook.
     * @param rDrawInfo Drawing state used during calculation.
     * @param isForceGlbMtxDirty Whether global matrices must be refreshed unconditionally.
     */
    void Calculate(DrawInfo& rDrawInfo, bool isForceGlbMtxDirty = false) {
        CalculateImpl(rDrawInfo, isForceGlbMtxDirty);
    }

    /**
     * @brief Access the registered parts panes without modifying the list.
     * @return Non-owning list of parts panes registered with this layout.
     */
    const PartsList& GetPartsList() const { return *reinterpret_cast<const PartsList*>(&_48); }
    struct DynamicTextureList {
        int captureCount;
        int vectorGraphicsCount;
        detail::DynamicRenderingTexture** pTextures;

        /**
         * @brief Count every dynamic texture stored in the list.
         * @return Number of capture and vector-graphics textures.
         */
        int GetTotalCount() const { return captureCount + vectorGraphicsCount; }
    };
    nn::util::IntrusiveListNode mAnimTransformList;
    Pane* mRootPane;
    void* _20;
    nn::util::Float2 mLayoutSize;
    void* _30;
    void* _38;
    ResourceAccessor* mResourceAccessor;
    nn::util::IntrusiveListNode _48;
    DynamicTextureList* mDynamicTextureList;

    static AllocateFunction g_pAllocateFunction;
    static FreeFunction g_pFreeFunction;
    static void* g_pUserDataForAllocator;
    static int g_CaptureTextureShareInfoCountMax;
    static int g_VectorGraphicsTextureShareInfoCountMax;
    static int g_DynamicTextureShareInfoCountMax;
    static int g_DynamicTextureShareInfoPartsStackMax;
    static LayoutPaneFactory* g_pLayoutPaneFactory;
    // Four words of shared random-generator state, initialized from the UI seed.
    static u32 g_Random[4];
    static RenderTargetTextureLifetime (*g_pCreateRenderTargetTextureResourceCallback)(
        nn::gfx::Texture**, nn::gfx::TextureView**, nn::gfx::DescriptorSlot**, const Layout*,
        const nn::gfx::TextureInfo&, void*, RenderTargetTextureLifetime);
    static void (*g_pDestroyRenderTargetTextureResourceCallback)(nn::gfx::Texture*, nn::gfx::TextureView*,
                                                                 nn::gfx::DescriptorSlot*, const Layout*,
                                                                 void*, RenderTargetTextureLifetime);
    static void* g_pRenderTargetTextureCallbackUserData;
};
} // namespace ui2d
} // namespace nn
