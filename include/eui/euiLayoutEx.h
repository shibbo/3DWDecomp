#pragma once
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <prim/seadSafeString.h>
namespace nn::ui2d { class Group; class GroupContainer; }
namespace eui {
class Screen;
class Animator;
class AnimatorSet;
class LayoutEx : public nn::ui2d::Layout {
public:
    explicit LayoutEx(Screen* pScreen);
    ~LayoutEx() override = default;
    NN_RUNTIME_TYPEINFO(nn::ui2d::Layout);
    bool BuildImpl(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, const void*, nn::ui2d::ResourceAccessor*, const nn::ui2d::BuildArgSet&, const PartsBuildDataSet*) override;
    nn::ui2d::Pane* BuildPaneObj(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, u32, const void*, const void*, const nn::ui2d::BuildArgSet&) override;
    nn::ui2d::Layout* BuildPartsLayout(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, const char*, const PartsBuildDataSet&, const nn::ui2d::BuildArgSet&) override;
    void CalculateImpl(nn::ui2d::DrawInfo&, bool) override;
    virtual void animatorDisableCallback(Animator*);
    virtual LayoutEx* doCreatePartsLayout_(const char*, const PartsBuildDataSet&, const nn::ui2d::BuildArgSet&);
    virtual void attachPartsLayoutArchive_(const sead::SafeString&);
    virtual void doInitializeDefalutAnimator_(bool);
    virtual nn::ui2d::Pane* buildPaneObjImpl_(nn::ui2d::BuildResultInformation*, u32, const void*, const void*, const nn::ui2d::BuildArgSet&);
    virtual void afterBuildPane_(nn::ui2d::Pane*, const nn::ui2d::BuildArgSet&);
    Animator* createAnimatorAuto(const char* pName, bool enabled);
    Animator* tryCreateAnimatorAuto(const char* pName, bool enabled);
    Animator* tryCreateAnimatorAutoWithWarning(const char* pName, bool enabled);
    AnimatorSet* createAnimatorSet(const char* const* pNames, u32 count, bool enabled);
    const void* GetAnimResourceData(const char* pName);
    Animator* createAnimatorWithPane(const char* pName, nn::ui2d::Pane* pPane, bool enabled);
    Animator* createAnimatorWithGroup(const char* pName, nn::ui2d::Group* pGroup, bool enabled);
    Animator* createAnimatorWithGroupIndex(const char* pName, u32 index, bool enabled);
    Animator* tryCreateAnimatorWithGroupIndex(const char* pName, u32 index, bool enabled);
    Animator* createUnbindedAnimator(const char* pName, bool enabled);

    Screen* getScreen() const { return mScreen; }
    LayoutEx* getParentLayout() const { return mParentLayout; }
    nn::ui2d::Pane* getRootPane() const { return mRootPane; }
    nn::ui2d::Pane* findPaneByName(const char* pName) const { return mRootPane->FindPaneByName(pName, true); }

    /** @return Whether a parts pane using this layout is measured as a whole by an AlignPane. */
    bool isAlignAsParts() const { return reinterpret_cast<const u8*>(&mFlags)[1] != 0; }

    nn::ui2d::GroupContainer* getGroupContainer() const { return static_cast<nn::ui2d::GroupContainer*>(_20); }

    const char* getLayoutName() const { return static_cast<const char*>(_30); }

    const char* getRootName() const {
        return mRootPane->mParent != nullptr ? mRootPane->mPanelName : getLayoutName();
    }

    void* _60;
    void* _68;
    void* _70;
    void* _78;
    Screen* mScreen;
    LayoutEx* mParentLayout;
    u16 mFlags;
};

static_assert(sizeof(nn::ui2d::Layout) == 0x60, "Layout size");
static_assert(sizeof(LayoutEx) == 0x98, "LayoutEx size");
}
