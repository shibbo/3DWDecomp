#pragma once
#include <nn/ui2d/ui2d_Layout.h>
namespace nn::ui2d {
class Animator;
class AnimatorEx;
class Screen;
class LayoutEx : public Layout {
public:
    explicit LayoutEx(Screen* screen);
    AnimatorEx* FindAnimator(const char* name);
    void Open();
    void OpenDirect();
    void Close();
    void CloseDirect();
    NN_RUNTIME_TYPEINFO(Layout);
    bool BuildImpl(BuildResultInformation*, nn::gfx::Device*, const void*, ResourceAccessor*, const BuildArgSet&, const PartsBuildDataSet*) override;
    Layout* DoCreatePartsLayout_(const char*, const PartsBuildDataSet&, const BuildArgSet&) override;
    virtual void UpdateDefaultAnimators();
    virtual void AnimatorDisableCallback(Animator* animator);
    virtual void DoBuildDefaultAnimatons_(nn::gfx::Device* device);
    AnimatorEx* CreateAnimatorExAuto(nn::gfx::Device* device, const char* name, bool enabled);
    AnimatorEx* TryCreateAnimatorExAuto(nn::gfx::Device* device, const char* name, bool enabled);

    Screen* GetScreen() const { return mScreen; }

    Screen* mScreen;
    AnimatorEx* mInOutAnimator;
    AnimatorEx* mLoopAnimator;
    AnimatorEx* mDefaultAnimator;
    u32 mAnimationState;
};
}
