#pragma once
#include <nn/ui2d/ui2d_AnimButton.h>
#include <nn/ui2d/ui2d_AnimatorEx.h>
#include <nn/ui2d/ui2d_ButtonGroup.h>
#include <nn/ui2d/ui2d_DefaultControlCreator.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Types.h>
#include <nn/util/util_IntrusiveList.h>
#include <nn/util/util_MathTypes.h>

namespace nn::ui2d {
struct Size;
class DrawInfo;
class ControlCreator;
class ResourceAccessor;
class LayoutEx;
class AnimatorEx;
class ScreenManager;

/** @brief Interface of a screen driven by a screen manager. */
class ScreenBase {
public:
    /** @brief Pointer input forwarded to the screens every frame. */
    struct InputDeviceState {
        bool isPointerValid;
        nn::util::Float2 pointerPosition;
        bool isTrigger;
        bool isRelease;
    };

    NN_RUNTIME_TYPEINFO_BASE();
    virtual void DrawCaptureTexture(nn::gfx::Device*, nn::gfx::CommandBuffer&) = 0;
    virtual void DrawLayout(nn::gfx::CommandBuffer&) = 0;
    virtual DrawInfo* CreateDrawInfo_() = 0;
    virtual ControlCreator* CreateControlCreator_() = 0;
};

/** @brief Layout shown by a screen manager, with its buttons, controls and animators. */
class Screen : public ScreenBase {
public:
    enum AnimatorOperationType : int {
        AnimatorOperationType_Play,
        AnimatorOperationType_PlayFromCurrent,
        AnimatorOperationType_StopAt,
        AnimatorOperationType_StopAtCurrentFrame,
        AnimatorOperationType_StopAtStartFrame,
        AnimatorOperationType_StopAtEndFrame,
        AnimatorOperationType_CrossStart,
        AnimatorOperationType_CrossEnd
    };

    /** @brief How a screen is opened; larger values take priority over pending requests. */
    enum OpenOption : int {
        OpenOption_None = 0,
        OpenOption_Normal = 1,
        OpenOption_Restart = 2,
        OpenOption_Direct = 3,
        OpenOption_Immediate = 4,
    };

    /** @brief How a screen is closed; smaller values take priority over pending requests. */
    enum CloseOption : int {
        CloseOption_None = 0,
        CloseOption_Normal = -1,
        CloseOption_Direct = -4,
    };

    /** @brief Open/close state of a screen. */
    enum State : int {
        State_Closed = 0,
        State_Opening = 1,
        State_Opened = 2,
        State_Closing = 3,
    };

    /** @brief Whether the screen is shown by the layout viewer. */
    enum ViewerType : int {
        ViewerType_None = 0,
        ViewerType_Preview = 1,
    };

    using AnimatorList = nn::util::IntrusiveList<
        AnimatorEx, nn::util::IntrusiveListMemberNodeTraits<AnimatorEx, &AnimatorEx::mActiveLink>>;

    Screen();

    void Initialize(nn::gfx::Device* pDevice, ScreenManager* pScreenManager,
                    const char* pLayoutName, int screenIndex);
    void Finalize(nn::gfx::Device* pDevice);
    void OpenWithOption_(OpenOption option);
    void CloseWithOption_(CloseOption option);
    bool IsOpened() const;
    bool IsClosed() const;
    bool IsOpening() const;
    bool IsClosing() const;
    void OpenStart_(OpenOption option);
    bool IsOpenEnd_();
    void OpenEnd_();
    void CloseStart_(CloseOption option);
    bool IsCloseEnd_();
    void CloseEnd_();
    bool IsActive_() const;
    bool IsPaused() const;
    void SetDrawUnitId_(int drawUnitId);
    int CountActiveAnimator_() const;
    void SetAnimatorActive(AnimatorEx* animator);
    void EraseAnimatorFromActiveListInternal_(AnimatorEx* pAnimator);
    void EraseAnimatorFromActiveList(AnimatorEx* animator);
    void Update(nn::gfx::Device* pDevice, const InputDeviceState& rInput);
    int CopyLayoutBelongingControls(nn::gfx::Device* pDevice, const Layout* pSource,
                                    Layout* pDestination);
    void SetViewSize(const Size& rLayoutSize, const Size& rViewSize);

    /**
     * @brief Get the group of the buttons of the screen.
     * @return Button group.
     */
    ButtonGroup* GetButtonGroup() { return &mButtonGroup; }

    NN_RUNTIME_TYPEINFO(ScreenBase);
    void DrawCaptureTexture(nn::gfx::Device*, nn::gfx::CommandBuffer&) override;
    void DrawLayout(nn::gfx::CommandBuffer&) override;
    DrawInfo* CreateDrawInfo_() override;
    ControlCreator* CreateControlCreator_() override;
    virtual ~Screen();
    virtual const char* GetLayoutName() const;
    virtual Size GetViewportSize() const;

    /**
     * @brief Get the creator used to build the controls of the layout.
     * @return Control creator made by CreateControlCreator_.
     */
    virtual ControlCreator* GetControlCreator() const { return mControlCreator; }

    virtual void Open();
    virtual void OpenDirect();
    virtual void Close();
    virtual void CloseDirect();

    /**
     * @brief Called when a button of the screen changes state.
     * @param button Button whose state changed.
     * @param previous State before the change.
     * @param next State after the change.
     */
    virtual void HandleEventOnButtonStateChanged(AnimButton* button, ButtonBase::State previous,
                                                 ButtonBase::State next) {}

    /**
     * @brief Called when an animator of the screen is operated.
     * @param operation Operation applied to the animator.
     * @param animator Operated animator.
     */
    virtual void HandleEventOnAnimatorOperation(AnimatorOperationType operation,
                                                AnimatorEx* animator) {}

    /** @brief Called before the layout viewer replaces the screen. */
    virtual void UnloadForReplaceViewerCallback() {}

    virtual void DoOpenStart_();
    virtual void DoOpenEnd_();
    virtual void DoCloseStart_();
    virtual void DoCloseEnd_();
    virtual bool IsPlayPartsInOut_() const;
    virtual LayoutEx* DoAllocateLayout_(nn::gfx::Device*);
    virtual LayoutEx* DoBuildLayout_(nn::gfx::Device*, const char*, ResourceAccessor*);
    virtual void DeleteLayout_(nn::gfx::Device*);
    virtual void DoBuildAnimatons_(nn::gfx::Device*, const void*);
    virtual void DoDestroyAnimatons_(nn::gfx::Device*);
    virtual void CreateRestAnimators_(nn::gfx::Device*, LayoutEx*);
    virtual void DoInitialize_(nn::gfx::Device*);
    virtual void DoFinalize_(nn::gfx::Device*);
    virtual void SetupPaneAfterBuild_(nn::gfx::Device*, Pane*, Layout*);
    virtual void SetupPaneAfterBuildRecursively_(nn::gfx::Device*, Pane*, Layout*);
    virtual void UpdateScreenOpening_();
    virtual void UpdateScreenClosing_();
    virtual void DoUpdate_(nn::gfx::Device*);
    virtual void UpdateUserInput_(const nn::util::Float2*, bool, bool);
    virtual void UpdateButtons_(nn::gfx::Device*, const InputDeviceState&);
    virtual void UpdateControl_(nn::gfx::Device*);
    virtual void UpdateAnimator_();
    virtual ResourceAccessor* DoCreateResourceAccessor_();

    /**
     * @brief Called after the layout has been calculated.
     * @param pLayout Calculated layout.
     */
    virtual void OnPostCalculate(Layout* pLayout) {}

    nn::util::IntrusiveListNode m_Link;
    ScreenManager* mScreenManager;
    DrawInfo* mDrawInfo;
    Layout* mLayout;
    AnimatorList mActiveAnimators;
    ButtonGroup mButtonGroup;
    ControlList mControlList;
    nn::util::MatrixT4x4fType mProjectionMtx;
    nn::util::MatrixT4x3fType mViewMtx;
    bool mIsPerspective;
    float mFovy;
    float mNear;
    float mFar;
    bool mIsViewDirty;
    bool mIsVisible;
    bool mIsPaused;
    int mOpenRequest;
    State mState;
    ControlCreator* mControlCreator;
    void* _118;
    BuildResultInformation mBuildResultInformation;
    Layout::BuildOption mBuildOption;
    bool mIsUtf8;
    u8 _151;
    ViewerType mViewerType;
    bool mIsUpdated;
    int mScreenIndex;
    /** @brief Draw unit of the screen; the screen manager activates the screen with it. */
    int mScreenId;
};
static_assert(sizeof(Screen) == 0x170, "Screen size");

/** @brief Copies the buttons and controls of a layout into a copied layout. */
class ControlCopier {
public:
    /** @brief Destination of the copied buttons and controls. */
    struct CopyLayoutBelongingControlArg {
        CopyLayoutBelongingControlArg();

        ButtonGroup* pButtonGroup;
        ControlList* pControlList;
        nn::gfx::Device* pDevice;
    };

    ControlCopier();
    virtual ~ControlCopier();
    virtual ControlBase* CopySingleControl(nn::gfx::Device* pDevice, const ControlBase& rControl,
                                           Layout* pLayout);
    virtual AnimButton* CopySingleButton(nn::gfx::Device* pDevice, const AnimButton& rButton,
                                         Layout* pLayout);

    int CopyLayoutBelongingControlsRecursive(const CopyLayoutBelongingControlArg& rArg,
                                             const Layout* pSource, Layout* pDestination);
    int CopyLayoutBelongingControls(const CopyLayoutBelongingControlArg& rArg,
                                    const Layout* pSource, Layout* pDestination);
};
}  // namespace nn::ui2d
