#pragma once
#include <nn/ui2d/ui2d_Pane.h>

namespace nn::ui2d {
class ResourceAccessor;
class Group;
class TextureInfo;
struct ResAnimationBlock;
struct ResAnimationContent;
struct ResExtUserData;

class AnimTransform {
public:
    AnimTransform();
    NN_RUNTIME_TYPEINFO_BASE();
    virtual ~AnimTransform();
    virtual void UpdateFrame(float step);
    virtual void SetEnabled(bool enabled);
    virtual void Animate() = 0;
    virtual void AnimatePane(Pane* pPane) = 0;
    virtual void AnimateMaterial(Material* pMaterial) = 0;
    virtual void SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                             const ResAnimationBlock* pResource) = 0;
    virtual void SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                             const ResAnimationBlock* pResource, u16 capacity) = 0;
    virtual void BindPane(Pane* pPane, bool recursive) = 0;
    virtual void BindGroup(Group* pGroup) = 0;
    virtual void BindMaterial(Material* pMaterial) = 0;
    virtual void ForceBindPane(Pane* pPane, const Pane* pSource) = 0;
    virtual void UnbindPane(const Pane* pPane) = 0;
    virtual void UnbindGroup(const Group* pGroup) = 0;
    virtual void UnbindMaterial(const Material* pMaterial) = 0;
    virtual void UnbindAll() = 0;
    u16 GetFrameSize() const;
    bool IsLoopData() const;
    bool IsWaitData() const;

    nn::util::IntrusiveListNode m_Link;
    const ResAnimationBlock* m_pResource;
    float mFrame;
    bool mEnabled;
};
static_assert(sizeof(AnimTransform) == 0x28, "AnimTransform size");

class AnimTransformBasic : public AnimTransform {
public:
    AnimTransformBasic();
    ~AnimTransformBasic() override;
    NN_RUNTIME_TYPEINFO(AnimTransform);
    void Animate() override;
    void AnimatePane(Pane* pPane) override;
    void AnimateMaterial(Material* pMaterial) override;
    void SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                     const ResAnimationBlock* pResource) override;
    void SetResource(nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                     const ResAnimationBlock* pResource, u16 capacity) override;
    void BindPane(Pane* pPane, bool recursive) override;
    void BindGroup(Group* pGroup) override;
    void BindMaterial(Material* pMaterial) override;
    void ForceBindPane(Pane* pPane, const Pane* pSource) override;
    void UnbindPane(const Pane* pPane) override;
    void UnbindGroup(const Group* pGroup) override;
    void UnbindMaterial(const Material* pMaterial) override;
    void UnbindAll() override;
    virtual void ResetAnimResource();
    virtual void AnimatePaneImpl(Pane* pPane, const ResAnimationContent* pContent);
    virtual void AnimateMaterialImpl(Material* pMaterial, const ResAnimationContent* pContent);
    virtual void AnimateExtUserDataImpl(ResExtUserData* pUserData,
                                        const ResAnimationContent* pContent);
    virtual void AnimatePartsStateLayerImpl(Pane* pPane, const ResAnimationContent* pContent);

    /** @brief Animation content bound to a pane, material or extended user data. */
    struct BindPair {
        void* pTarget;
        const ResAnimationContent* pAnimContent;
    };

    bool CheckBindAnimationDoubly(const void* pTarget, const ResAnimationContent* pContent) const;
    bool BindPaneImpl(Pane* pTarget, const ResAnimationContent* pContent);
    bool BindMaterialImpl(Material* pTarget, const ResAnimationContent* pContent);
    bool BindExtUserDataToPane(Pane* pPane, const ResAnimationContent& rContent);
    bool BindExtUserDataImpl(ResExtUserData* pTarget, const ResAnimationContent* pContent);
    void EraseBindPair(int index);

    union {
        void* _28;
        const TextureInfo** m_pFileResArray;
    };
    union {
        void* _30;
        BindPair* m_pBindPairArray;
    };
    union { u32 _38; struct { u16 mBindCount; u16 mBindCapacity; }; };
};
static_assert(sizeof(AnimTransformBasic) == 0x40, "AnimTransformBasic size");
}
