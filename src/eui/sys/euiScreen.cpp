#include <eui/euiScreen.h>
#include <eui/euiScreenMgr.h>
#include <eui/euiScreenViewer.h>
#include <eui/euiBoxCursorNode.h>
#include <eui/euiBoxCursorMgr.h>
#include <eui/euiAnimator.h>
#include <eui/euiAnimButton.h>
#include <eui/euiLayoutEx.h>
#include <eui/euiPartsEx.h>
#include <eui/euiTextBoxEx.h>
#include <eui/euiButtonGroup.h>
#include <eui/euiCheckButton.h>
#include <eui/euiCheckKeepButton.h>
#include <eui/euiUniteButton.h>
#include <eui/euiHoverButton.h>
#include <eui/euiTapButton.h>
#include <eui/euiUIController.h>
#include <eui/euiTagProcessor.h>
#include <eui/euiTextSearcher.h>
#include <eui/euiMessageMgr.h>
#include <eui/euiMessageSet.h>
#include <eui/euiMessageString.h>
#include <eui/euiArcResourceMgr.h>
#include <eui/euiMultiArcResourceAccessor.h>
#include <eui/euiNwAllocator.h>
#include <eui/euiConstantBuffer.h>
#include <controller/seadController.h>
#include <controller/seadControllerMgr.h>
#include <gfx/nin/seadGraphicsNvn.h>
#include <gfx/seadDrawContext.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <prim/seadSafeString.h>

namespace eui {
namespace {

/**
 * Compares two resource names the way the layout library does.
 * @param pName Name to look for.
 * @param pOther Name stored in a resource.
 * @return Whether the names match within the first N characters.
 */
template <s32 N>
inline bool IsEqualResName(const char* pName, const char* pOther) {
    for (s32 i = 0; i < N; i++) {
        if (pName[i] != pOther[i]) {
            return false;
        }

        if (pName[i] == '\0') {
            return true;
        }
    }

    return true;
}

/** @return The graphics device used to build and finalize layouts. */
inline nn::gfx::Device* GetDevice() {
    return reinterpret_cast<nn::gfx::Device*>(sead::GraphicsNvn::instance()->getGfxDevice());
}

/**
 * @param pLink Link of one child in a pane's child list.
 * @return The child pane owning pLink.
 */
inline nn::ui2d::Pane* GetChildPane(nn::util::IntrusiveListNode* pLink) {
    using Traits = nn::util::IntrusiveListMemberNodeTraits<nn::ui2d::detail::PaneBase,
                                                           &nn::ui2d::detail::PaneBase::m_Link>;
    return static_cast<nn::ui2d::Pane*>(&Traits::GetItem(*pLink));
}

/**
 * @param pScreenMgr Screen manager holding the per-display hit flags.
 * @param drawUnitId Draw unit whose display is looked up.
 * @return Flag that allows pointer hits on the display of drawUnitId.
 */
inline bool& GetHitEnableFlag(ScreenMgr* pScreenMgr, s8 drawUnitId) {
    const s32 index = pScreenMgr->getViewer()->getDrawTarget(drawUnitId);
    return pScreenMgr->getHitEnableFlags()[index];
}

/**
 * @param pButton Button to look up.
 * @return Layout around pButton; for a parts control, the layout containing its parts pane.
 */
inline const LayoutEx* GetButtonOuterLayout(const AnimButton* pButton) {
    return pButton->IsPartsControl() ? pButton->getLayout()->getParentLayout() : pButton->getLayout();
}

}  // namespace

/** @brief Constructs an uninitialized screen; initialize() builds its layout. */
Screen::Screen()
    : mScreenMgr(nullptr), mLayout(nullptr), mButtonGroup(nullptr), mController(nullptr),
      mDrawInfo(nullptr), mTagProcessor(nullptr), mInitializeHeap(nullptr), mScreenId(-1),
      mName(), mPrimaryCursor(nullptr), mLastActiveCursor(nullptr),
      mEffectLinkUser(nullptr), mSoundLinkUser(nullptr), _e0(0.7853981852531433f), mDrawLayer(-1),
      mOpenRequest(0), mState(cState_Closed), mCloseMode(cCloseMode_Normal), mIsInitialized(0),
      mIsDrawDisabled(0), mIsUpdatePaused(0), mIsTouch(0), mIsCaptureRequested(0),
      mNoOperationButtonOnSE(1), mFlags(3) {
    mCursorNodes.initOffset(offsetof(BoxCursorNode, mListNode));
}

/** @brief Releases the screen's id, cursor links, controls, link users and layout. */
Screen::~Screen() {
    if (mScreenId != -1) {
        mScreenMgr->resetScreenId(mScreenId);
    }

    BoxCursorMgr* boxCursorMgr = mScreenMgr->getBoxCursorMgr();

    if (boxCursorMgr != nullptr) {
        for (auto& node : mCursorNodes) {
            boxCursorMgr->eraseNodeLinks(&node);
        }
    }

    for (auto& control : mControls) {
        control.~ControlBase();
    }

    for (auto& control : mStaticControls) {
        control.~ControlBase();
    }

    if (mEffectLinkUser != nullptr) {
        mEffectLinkUser->finalize();
    }

    if (mSoundLinkUser != nullptr) {
        mSoundLinkUser->finalize();
    }

    if (mLayout != nullptr) {
        nn::ui2d::ResourceAccessor* accessor = mLayout->GetResourceAccessor();
        deleteLayout_();

        if (accessor != nullptr) {
            accessor->UnregisterTextureViewFromDescriptorPool(UnregisterSlotForTexture, nullptr);
            NwAllocator::initialize(mInitializeHeap);

            if (eui::DynamicCast<MultiArcResourceAccessor>(accessor) != nullptr) {
                accessor->Finalize(GetDevice());
            }

            NwAllocator::finalize();
        }
    }
}

/** @brief Finalizes and deletes the layout when this screen built and owns it. */
void Screen::deleteLayout_() {
    if (mLayout != nullptr && (mFlags & 0xa) == 2) {
        NwAllocator::initialize(mInitializeHeap);
        mLayout->Finalize(GetDevice());
        delete mLayout;
        NwAllocator::finalize();
        mLayout = nullptr;
    }
}

/**
 * @brief Builds the layout and all screen resources.
 * @param pScreenMgr Manager that owns this screen.
 * @param pHeap Heap for every resource created by the screen.
 * @param pName Screen name.
 * @param screenId Index of this screen in the manager.
 * @param drawLayer Draw unit the screen is drawn in.
 * @param isTouch Whether buttons start in touch mode.
 */
void Screen::initialize(ScreenMgr* pScreenMgr, sead::Heap* pHeap, const char* pName, int screenId,
                        s8 drawLayer, bool isTouch) {
    mScreenMgr = pScreenMgr;
    mInitializeHeap = pHeap;
    mScreenId = screenId;
    mDrawLayer = drawLayer;
    mName = pName;
    mIsInitialized = true;
    mIsTouch = isTouch;
    mDrawInfo = doCreateDrawInfoEx_(pHeap);
    mButtonGroup = doCreateButtonGroup_(pHeap);
    mButtonGroup->SetTouchDevice(isTouch);
    NwAllocator::initialize(pHeap);
    mLayout = doCreateLayout_(pHeap);

    if (isEnableControl()) {
        mController = doCreateUIController_(pHeap);
    }

    MultiArcResourceAccessor* accessor = doCreateResourceAccessor_(pHeap);
    mTagProcessor = doCreateTagProcessor_(pHeap);
    sead::Graphics::instance()->lockDrawContext();

    {
        sead::FixedSafeString<72> path;
        path.format("%s.bflyt", getLayoutName_());
        doBuildLayout_(path, accessor);
    }

    accessor->RegisterTextureViewToDescriptorPool(RegisterSlotForTexture, nullptr);
    doAfterBuildLayout_(pHeap);
    sead::Graphics::instance()->unlockDrawContext();
    u32 effectLinkPaneNum = 0;
    iteratePaneForSetupPaneAfterBuild_(mLayout->getRootPane(), mLayout, &effectLinkPaneNum);
    createSoundLink2User_(pHeap);
    updateStaticControl_();
    doSetupDrawInfo_();
    mLayout->CalculateGlobalMatrix(*mDrawInfo, true);
    doLoadResource_(pHeap);
    sead::Graphics::instance()->lockDrawContext();
    doInitialize_(pHeap);
    sead::Graphics::instance()->unlockDrawContext();
    createEffectLinkUser_(pHeap, effectLinkPaneNum);
    accessor->RegisterTextureViewToDescriptorPool(RegisterSlotForTexture, nullptr);
    NwAllocator::finalize();
}

/**
 * @brief Sets up a pane and its children after the layout was built.
 * @param pPane Pane to set up.
 * @param pLayout Layout owning pPane; parts panes switch to their own layout.
 * @param pCount Receives the number of panes that need an effect link.
 */
void Screen::iteratePaneForSetupPaneAfterBuild_(nn::ui2d::Pane* pPane, LayoutEx* pLayout, u32* pCount) {
    auto* parts = nn::ui2d::DynamicCast<PartsEx*>(pPane);

    if (parts != nullptr) {
        pLayout = static_cast<LayoutEx*>(parts->m_pLayout);
    }

    setupPaneAfterBuild_(pPane, pLayout, pCount);

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children; link = link->GetNext()) {
        iteratePaneForSetupPaneAfterBuild_(GetChildPane(link), pLayout, pCount);
    }
}

/** @brief Updates every static control with the screen's animation step. */
void Screen::updateStaticControl_() {
    const float step = getAnimationStep_();

    for (auto& control : mStaticControls) {
        control.Update(step);
    }
}

/**
 * @brief Requests the screen to open on its next update.
 * @param option How the open animation starts.
 */
void Screen::open(OpenOption option) {
    if (mOpenRequest <= option) {
        mOpenRequest = option;
    }

    mScreenMgr->activateScreen(mScreenId);
}

/**
 * @brief Requests the screen to close on its next update.
 * @param option How the close animation starts.
 */
void Screen::close(CloseOption option) {
    if (mOpenRequest >= option) {
        mOpenRequest = option;
    }

    if (mState == cState_Closed) {
        mScreenMgr->inactivateScreen(mScreenId);
    }
}

/** @return Whether the screen is fully open and no close is pending. */
bool Screen::isOpened() const { return mState == cState_Opened && mOpenRequest >= 0; }

/** @return Whether the screen is fully closed and no open is pending. */
bool Screen::isClosed() const { return mState == cState_Closed && mOpenRequest < 1; }

/** @return Whether the screen is opening or an open request is pending. */
bool Screen::isOpening() const {
    if (mState == cState_Opening) {
        return true;
    }

    return mOpenRequest >= 1 && ((mState == cState_Closed) | (mState == cState_Closing));
}

/** @return Whether the screen is closing or a close request is pending. */
bool Screen::isClosing() const {
    if (mState == cState_Closing) {
        return true;
    }

    if (mOpenRequest < 0) {
        return mState == cState_Opening || mState == cState_Opened;
    }

    return false;
}

/** @return Viewer type of the screen; the base screen uses the default viewer. */
int Screen::getViewerType() const { return 0; }

/**
 * @param pBounds Cursor bounds a derived screen may adjust.
 * @param pNode Node the cursor is on.
 */
void Screen::adjstBoxCursor(sead::BoundBox2f* pBounds, const BoxCursorNode* pNode) const {}

/**
 * @param pHeap Heap for the node.
 * @return A new box cursor node.
 */
BoxCursorNode* Screen::createBoxCursorNode(sead::Heap* pHeap) { return new (pHeap, 8) BoxCursorNode; }

/** @param pAnimator Animator appended to the active animator list. */
void Screen::setAnimatorActive(Animator* pAnimator) { mActiveAnimators.push_back(*pAnimator); }

/** @param pAnimator Animator removed from the active animator list. */
void Screen::eraseAnimatorFromActiveList(Animator* pAnimator) {
    mActiveAnimators.erase(mActiveAnimators.iterator_to(*pAnimator));
}

/** @param own Whether this screen owns its initialization heap. */
void Screen::setOwnInitializeHeap(bool own) {
    if (own) {
        mFlags |= 1;
    } else {
        mFlags &= ~1;
    }
}

/** @param pNode Node removed from every cursor node's routes. */
void Screen::eraseBoxCursorNodeFromRouteNodes(const BoxCursorNode* pNode) {
    for (auto& node : mCursorNodes) {
        node.eraseNodeFromRouteNodes(pNode);
    }
}

/** @return Whether the screen is open with its open/close animation at the open pose. */
bool Screen::isOpenedInitial() const {
    if (isOpened()) {
        Animator* openAnimator = mLayout->mInAnimator;
        Animator* closeAnimator = mLayout->mOutAnimator;

        if (openAnimator != nullptr && !openAnimator->isPlaying() && openAnimator->getFrame() == 0) {
            return true;
        }

        if (closeAnimator != nullptr && !closeAnimator->isPlaying()) {
            return closeAnimator->isFrameMax();
        }
    }

    return false;
}

/**
 * @param pButton Button whose node the cursor moves to.
 * @return Whether the cursor was moved.
 */
bool Screen::moveBoxCursorByButton(const AnimButton* pButton) {
    BoxCursorMgr* boxCursorMgr = mScreenMgr->getBoxCursorMgr();

    if (boxCursorMgr == nullptr) {
        return false;
    }

    if (!boxCursorMgr->isEnable(getDrawTarget())) {
        return false;
    }

    BoxCursorNode* node = findBoxCursorNodeByButton_(pButton);

    if (node == nullptr) {
        return false;
    }

    boxCursorMgr->moveBoxCursor(node);
    return true;
}

/** @return Display target the screen's draw unit is shown on. */
DrawTarget Screen::getDrawTarget() const { return mScreenMgr->getViewer()->getDrawTarget(mDrawLayer); }

/**
 * @param pButton Button to look up.
 * @return The cursor node of pButton, or nullptr.
 */
BoxCursorNode* Screen::findBoxCursorNodeByButton_(const AnimButton* pButton) {
    for (auto& node : mCursorNodes) {
        if (node.getButton() == pButton) {
            return &node;
        }
    }

    return nullptr;
}

/**
 * @param rPosition Position in layout coordinates.
 * @return The innermost layout hit at rPosition.
 */
LayoutEx* Screen::findHitLayout(const sead::Vector2f& rPosition) { return FindHitLayout(rPosition, mLayout); }

/** @brief Processes open/close requests, input, controls and animations, then calculates the layout. */
void Screen::update() {
    const s8 request = mOpenRequest;
    const bool isPaused = mIsUpdatePaused;

    if (request > 0) {
        mOpenRequest = 0;

        if (request == cOpenOption_Restart || mState == cState_Closing || mState == cState_Closed) {
            openStart_(OpenOption(request));

            if (request == cOpenOption_Immediate) {
                openEnd_();
            }
        } else if (request == cOpenOption_Skip && mState == cState_Opening) {
            mLayout->startAnimOpenImpl_(isPlayPartsInOut_(), LayoutEx::cOpenAnim_End, true);
        }
    }

    if (mState == cState_Opening && isOpenEnd_()) {
        openEnd_();
    }

    if (!isPaused) {
        doUpdate_();
        updateButton_();
    }

    if (mButtonGroup->mHitButton != nullptr && isDisallowHitLowerScreenOnButtonHit_()) {
        setDisallowHitLowerScreen_();
    }

    if (!isPaused) {
        updateControl_();
    }

    const s8 closeRequest = mOpenRequest;

    if (closeRequest < 0) {
        mOpenRequest = 0;

        if (mState == cState_Opening || mState == cState_Opened) {
            closeStart_(CloseOption(closeRequest));
        } else if (closeRequest == cCloseOption_Immediate && mState == cState_Closing &&
                   mLayout->mOpenState != LayoutEx::cOpenState_Closed) {
            mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), true);
        }
    }

    if (mState == cState_Closing && isCloseEnd_() && mCloseMode != cCloseMode_ExtendEnd) {
        if (mCloseMode == cCloseMode_Normal && mOpenRequest <= 0) {
            mScreenMgr->inactivateScreen(mScreenId);
        }

        closeEnd_();
    }

    if (mEffectLinkUser != nullptr) {
        mEffectLinkUser->calc();
    }

    if (!isPaused) {
        updateAnimator_();
    }

    if (mIsCaptureRequested && (mFlags & 0x10)) {
        IteratePaneForRequestCapture(mLayout->getRootPane());
        mIsCaptureRequested = false;
    }

    mLayout->CalculateImpl(*mDrawInfo, isForceGlbMtxDirty_());

    if (mEffectLinkUser != nullptr) {
        mEffectLinkUser->calcAfterLayout();
    }

    if (mSoundLinkUser != nullptr) {
        mSoundLinkUser->calc();
    }
}

/** @brief Stops screens below this one on the same display from receiving pointer hits. */
void Screen::setDisallowHitLowerScreen_() {
    GetHitEnableFlag(mScreenMgr, mDrawLayer) = false;
}

/** @return Frame count of the open animation, or 0 without one. */
float Screen::getOpenFrameSize() const {
    Animator* animator = mLayout->mInAnimator;

    if (animator != nullptr) {
        return animator->GetFrameSize();
    }

    return 0.0f;
}

/** @return Frame count of the close animation (or the open one played back), or 0. */
float Screen::getCloseFrameSize() const {
    Animator* animator = mLayout->mOutAnimator;

    if (animator == nullptr) {
        animator = mLayout->mInAnimator;
    }

    if (animator != nullptr) {
        return animator->GetFrameSize();
    }

    return 0.0f;
}

/** @param frame Frame the open animation stops at. */
void Screen::stopOpenAtFrame(float frame) {
    Animator* animator = mLayout->mInAnimator;

    if (animator != nullptr) {
        animator->Stop(frame);
    }
}

/** @param frame Frame of the close animation to stop at; the open animation is used in reverse. */
void Screen::stopCloseAtFrame(float frame) {
    Animator* closeAnimator = mLayout->mOutAnimator;

    if (closeAnimator != nullptr) {
        closeAnimator->Stop(frame);
        return;
    }

    Animator* openAnimator = mLayout->mInAnimator;

    if (openAnimator != nullptr) {
        openAnimator->Stop(openAnimator->GetFrameSize() - frame);
    }
}

/**
 * @brief Sets up a screen that only uses an existing layout.
 * @param pScreenMgr Manager that owns this screen.
 * @param pLayoutName Screen name.
 */
void Screen::initializeForMinimum(ScreenMgr* pScreenMgr, const char* pLayoutName) {
    mScreenMgr = pScreenMgr;
    mName = pLayoutName;
    mFlags |= 8;
}

/** @brief Clears per-frame flags of a minimum screen; nothing to clear in the base screen. */
void Screen::clearFlagsAfterCalculateMtxForMinimum() {}

/** @param pLayout Layout a debug build would attach to a minimum screen. */
void Screen::setLayoutForMinimunDebug(LayoutEx* pLayout) {}

/** @brief Per-frame hook for derived screens. */
void Screen::doUpdate_() {}

/** @brief Feeds the pointer state into the button group. */
void Screen::updateButton_() {
    if (!isEnableControl()) {
        return;
    }

    if (mController->isPointerOn() && isAllowedHit_()) {
        const nn::font::Rectangle rect = mLayout->GetLayoutRect();
        const sead::Vector2f& pointer = mController->getPointer();
        sead::Vector2f position(pointer.x - rect.GetWidth() * 0.5f,
                                -pointer.y - rect.GetHeight() * 0.5f);
        mButtonGroup->Update(&position, mController->isTrig(sead::Controller::cPadMask_Touch),
                             mController->isRelease(sead::Controller::cPadMask_Touch),
                             mController->isRepeat(sead::Controller::cPadMask_Touch));
    } else {
        mButtonGroup->Update(nullptr, false, mController->isRelease(sead::Controller::cPadMask_Touch),
                             false);
    }
}

/** @return Whether this screen's display still accepts pointer hits this frame. */
bool Screen::isAllowedHit_() const {
    return GetHitEnableFlag(mScreenMgr, mDrawLayer);
}

/**
 * @brief Dispatches button state changes to the screen hooks and sound link.
 * @param pButton Button whose state changed.
 * @param oldState State the button left.
 * @param newState State the button entered.
 */
void Screen::buttonStateChangeCallback(AnimButton* pButton, ButtonBase::State oldState,
                                       ButtonBase::State newState) {
    switch (oldState) {
    case ButtonBase::cState_OnStart:
        doButtonOnEnd_(pButton);
        break;
    case ButtonBase::cState_OffStart:
        doButtonOffEnd_(pButton);
        break;
    case ButtonBase::cState_DownStart:
        doButtonDownEnd_(pButton);
        break;
    case ButtonBase::cState_CancelStart:
        doButtonCancelEnd_(pButton);
        break;
    default:
        break;
    }

    switch (newState) {
    case ButtonBase::cState_OnStart:
        doButtonOnStart_(pButton);

        if (mSoundLinkUser == nullptr) {
            return;
        }

        if (!mNoOperationButtonOnSE) {
            mNoOperationButtonOnSE = true;
            return;
        }

        if (!pButton->IsDowning()) {
            invokeSoundLink2ButtonEvent_(pButton, "_on");
        }

        return;
    case ButtonBase::cState_OffStart:
        doButtonOffStart_(pButton);
        return;
    case ButtonBase::cState_DownStart: {
        doButtonDownStart_(pButton);

        if (mSoundLinkUser == nullptr) {
            return;
        }

        mSoundLinkUser->setButtonSoundType(pButton->GetSoundType());

        if (pButton->IsPlayDisableAnim()) {
            invokeSoundLink2ButtonEvent_(pButton, "_noeffect");
            return;
        }

        bool isPlayed = false;

        if (eui::DynamicCast<CheckKeepButton>(pButton) != nullptr) {
            invokeSoundLink2ButtonEvent_(pButton, "_check");
            isPlayed = true;
        } else {
            auto* checkButton = eui::DynamicCast<CheckButton>(pButton);

            if (checkButton != nullptr && checkButton->mCheckEnabled) {
                if (checkButton->mChecked) {
                    invokeSoundLink2ButtonEvent_(pButton, "_check");
                } else {
                    invokeSoundLink2ButtonEvent_(pButton, "_uncheck");
                }

                isPlayed = true;
            }
        }

        auto* uniteButton = eui::DynamicCast<UniteButton>(pButton);

        if (uniteButton != nullptr) {
            if (uniteButton->mButtonType == 1) {
                if (!uniteButton->mChecked) {
                    invokeSoundLink2ButtonEvent_(pButton, "_check");
                    return;
                }
            } else if (uniteButton->mButtonType == 0) {
                if (uniteButton->mChecked) {
                    invokeSoundLink2ButtonEvent_(pButton, "_check");
                } else {
                    invokeSoundLink2ButtonEvent_(pButton, "_uncheck");
                }

                return;
            }
        }

        if (!isPlayed) {
            invokeSoundLink2ButtonEvent_(pButton, "_decide");
        }

        return;
    }
    case ButtonBase::cState_CancelStart:
        doButtonCancelStart_(pButton);
        return;
    default:
        return;
    }
}

/** @brief Updates every control with the screen's animation step. */
void Screen::updateControl_() {
    const float step = getAnimationStep_();

    for (auto& control : mControls) {
        control.Update(step);
    }
}

/** @brief Advances active animators and drops the ones that stopped. */
void Screen::updateAnimator_() {
    const float step = getAnimationStep_();

    for (auto it = mActiveAnimators.begin(); it != mActiveAnimators.end();) {
        Animator& animator = *it++;
        animator.Animate();

        if (animator.isPlaying() && animator.mEnabled) {
            animator.UpdateFrame(step);
            continue;
        }

        if (animator.mFlags & 2) {
            mActiveAnimators.erase(mActiveAnimators.iterator_to(animator));
            animator.clearFrameEvents();
            continue;
        }

        animator.disableKeepActive();
        animator.mLayout->animatorDisableCallback(&animator);
        const bool isPlayEnd = animator.isPlayEnd();
        animator.clearFrameEvents();

        if (isPlayEnd) {
            animator.mFlags |= 2;
        } else {
            mActiveAnimators.erase(mActiveAnimators.iterator_to(animator));
        }
    }
}

/** @brief Attaches the screen's input controller to the first controller. */
void Screen::registerController_() {
    if (isEnableControl()) {
        mController->registerWith(sead::ControllerMgr::instance()->getController(0), true);
    }
}

/** @brief Detaches the screen's input controller. */
void Screen::unregisterController_() {
    if (isEnableControl()) {
        mController->unregister();
        mController->setIdle();
    }
}

/**
 * @brief Creates the buttons and text controls a pane describes.
 * @param pPane Pane being set up.
 * @param pLayout Layout owning pPane.
 * @param pCount Receives the number of panes that need an effect link.
 */
void Screen::setupPaneAfterBuild_(nn::ui2d::Pane* pPane, LayoutEx* pLayout, u32* pCount) {
    SetupPaneAfterBuild(pPane, pLayout);
    HoverButton::CreateHoverButton(pPane, pLayout, mButtonGroup);
    TapButton::CreateTapButton(pPane, pLayout, mButtonGroup);
    auto* textBox = eui::DynamicCast<TextBoxEx>(pPane);

    if (textBox != nullptr) {
        textBox->createLetterAnimControl_(&mControls, pLayout);
    }

    countEffectLinkPane_(pPane, pCount);
}

/**
 * @param pPane Pane whose text boxes are resized, recursively.
 * @param pLayout Layout owning pPane; parts panes switch to their own layout.
 */
void Screen::iteratePaneForAdjustPaneSizeToTextSize_(nn::ui2d::Pane* pPane, LayoutEx* pLayout) {
    auto* parts = nn::ui2d::DynamicCast<PartsEx*>(pPane);

    if (parts != nullptr) {
        pLayout = static_cast<LayoutEx*>(parts->m_pLayout);
    }

    AdjustPaneSizeToTextSize(pPane, pLayout);

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children; link = link->GetNext()) {
        iteratePaneForAdjustPaneSizeToTextSize_(GetChildPane(link), pLayout);
    }
}

/**
 * @param pScreen Screen that registered the callback.
 * @param pUser Sound link user whose properties are defined.
 */
void Screen::setPropertyDefinitionCallbackStatic_(void* pScreen, xlink2::UserInstanceSLink* pUser) {
    static_cast<Screen*>(pScreen)->setSlink2PropertyDefinition_(pUser);
}

/**
 * @brief Starts opening the screen.
 * @param option How the open animation starts.
 */
void Screen::openStart_(OpenOption option) {
    if (mEffectLinkUser != nullptr) {
        mEffectLinkUser->onOpenStart();
    }

    if (mSoundLinkUser != nullptr) {
        mSoundLinkUser->onOpenStart();

        if (option == cOpenOption_Normal || option == cOpenOption_Restart) {
            invokeSoundLink2Event_("open");
        }
    }

    switch (option) {
    case cOpenOption_Restart:
        mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), true);
        // fallthrough
    case cOpenOption_Normal:
        mLayout->startAnimOpenImpl_(isPlayPartsInOut_(), LayoutEx::cOpenAnim_Play, true);
        break;
    case cOpenOption_Skip:
        mLayout->startAnimOpenImpl_(isPlayPartsInOut_(), LayoutEx::cOpenAnim_End, true);
        break;
    case cOpenOption_Immediate:
        mLayout->startAnimOpenImpl_(true, LayoutEx::cOpenAnim_Start, true);
        break;
    default:
        break;
    }

    mState = cState_Opening;
    mCloseMode = cCloseMode_Normal;
    mNoOperationButtonOnSE = false;
    doOpenStart_();
}

/** @return Whether the open animation finished. */
bool Screen::isOpenEnd_() { return mLayout->isAnimOpenEnd(isPlayPartsInOut_()); }

/** @brief Finishes opening and enables input. */
void Screen::openEnd_() {
    registerController_();
    mState = cState_Opened;
    doOpenEnd_();
}

/**
 * @brief Starts closing the screen.
 * @param option How the close animation starts.
 */
void Screen::closeStart_(CloseOption option) {
    if (mEffectLinkUser != nullptr) {
        mEffectLinkUser->onCloseStart();
    }

    mCloseMode = cCloseMode_Normal;

    switch (option) {
    case cCloseOption_Immediate:
        mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), true);
        break;
    case cCloseOption_NoState:
        mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), false);
        return;
    case cCloseOption_Extend:
        mCloseMode = cCloseMode_Extend;
        mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), false);
        break;
    case cCloseOption_Normal:
        mLayout->startAnimCloseImpl_(isPlayPartsInOut_(), false);

        if (mSoundLinkUser != nullptr) {
            invokeSoundLink2Event_("close");
        }

        break;
    default:
        break;
    }

    mState = cState_Closing;
    unregisterController_();
    doCloseStart_();
}

/** @return Whether the close animation finished. */
bool Screen::isCloseEnd_() { return mLayout->isAnimCloseEnd(isPlayPartsInOut_()); }

/** @brief Finishes closing; an extended close keeps the screen in the closing state. */
void Screen::closeEnd_() {
    switch (mCloseMode) {
    case cCloseMode_Normal:
        mState = cState_Closed;
        break;
    case cCloseMode_Extend:
        mCloseMode = cCloseMode_ExtendEnd;
        break;
    default:
        break;
    }

    mOpenRequest = 0;
    doCloseEnd_();

    if (mSoundLinkUser != nullptr) {
        mSoundLinkUser->onCloseEnd();
    }
}

/** @param pInfo Render buffers to draw into. */
void Screen::draw(const DrawInfoEx::RenderBufferInfo* pInfo) { doDraw_(pInfo); }

/**
 * @param pName Name of the parts layout.
 * @param pParts Parts pane being built.
 * @param pLayout Layout owning pParts.
 * @return Name of the layout to build for pParts.
 */
const char* Screen::replacePartsLayoutName(const char* pName, PartsEx* pParts, LayoutEx* pLayout) {
    return pName;
}

/**
 * @param pPane Pane that was built.
 * @param pLayout Layout owning pPane.
 * @param rArgs Build arguments.
 */
void Screen::afterBuildPaneCallback(nn::ui2d::Pane* pPane, LayoutEx* pLayout,
                                    const nn::ui2d::BuildArgSet& rArgs) {}

/**
 * @param operation Operation applied to pAnimator.
 * @param pAnimator Animator that changed.
 */
void Screen::animatorOperationCallback(AnimatorOperationType operation, Animator* pAnimator) {}

/** @return Whether the screen takes controller input. */
bool Screen::isEnableControl() const { return false; }

/** @return Name of the layout to load. */
const char* Screen::getLayoutName_() const { return nullptr; }

/** @return Name of the message set; defaults to the layout name. */
const char* Screen::getMessageName_() const { return getLayoutName_(); }

/** @return Name of the archive; defaults to the layout name. */
const char* Screen::getArchiveName_() const { return getLayoutName_(); }

/** @return Whether parts in/out animations play with the open/close animations. */
bool Screen::isPlayPartsInOut_() const { return false; }

/** @return Whether a button hit blocks pointer hits on lower screens. */
bool Screen::isDisallowHitLowerScreenOnButtonHit_() const { return true; }

/**
 * @param pHeap Heap for the layout.
 * @return A new layout owned by this screen.
 */
LayoutEx* Screen::doCreateLayout_(sead::Heap* pHeap) { return new (pHeap, 8) LayoutEx(this); }

/**
 * @param pHeap Heap for the draw info.
 * @return A new draw info.
 */
DrawInfoEx* Screen::doCreateDrawInfoEx_(sead::Heap* pHeap) { return new (pHeap, 16) DrawInfoEx; }

/**
 * @param pHeap Heap for the button group.
 * @return A new button group.
 */
ButtonGroup* Screen::doCreateButtonGroup_(sead::Heap* pHeap) { return new (pHeap, 8) ButtonGroup; }

/** @param pHeap Heap for resources created after the layout was built. */
void Screen::doAfterBuildLayout_(sead::Heap* pHeap) {}

/** @brief Sets up the draw info's graphics resource, projection and constant buffer. */
void Screen::doSetupDrawInfo_() {
    mDrawInfo->SetGraphicsResource(&mScreenMgr->mGraphicsResource);
    SetupDrawInfoOrtho(mDrawInfo, reinterpret_cast<const nn::ui2d::Size&>(mLayout->mLayoutSize));
    mScreenMgr->getConstantBuffer()->setToDrawInfo(mDrawInfo);
}

/**
 * @param pHeap Heap for the controller.
 * @return A new input controller.
 */
UIController* Screen::doCreateUIController_(sead::Heap* pHeap) { return new (pHeap, 8) UIController; }

/**
 * @param pHeap Heap for the accessor.
 * @return A resource accessor bound to the screen's archive, or nullptr if it is not loaded.
 */
MultiArcResourceAccessor* Screen::doCreateResourceAccessor_(sead::Heap* pHeap) {
    ArcResourceMgr* arcResourceMgr = mScreenMgr->getArcResourceMgr();
    auto* accessor = new (pHeap, 8) MultiArcResourceAccessor(arcResourceMgr, mScreenMgr->getFontMgr());
    ArcResourceMgr::ArcResource* archive = arcResourceMgr->findArcResource(getArchiveName_());

    if (archive == nullptr) {
        return nullptr;
    }

    accessor->attachArchive(archive->mData, archive->mTextureFile);
    return accessor;
}

/**
 * @param pHeap Heap for the tag processor.
 * @return A new tag processor using the manager's messages and fonts.
 */
TagProcessor* Screen::doCreateTagProcessor_(sead::Heap* pHeap) {
    return new (pHeap, 8) TagProcessor(mScreenMgr->getMessageMgr(), mScreenMgr->getFontMgr());
}

/** @param pHeap Heap for resources loaded by derived screens. */
void Screen::doLoadResource_(sead::Heap* pHeap) {}

/**
 * @brief Builds the layout with the screen's controls and messages.
 * @param rPath File name of the layout.
 * @param pAccessor Accessor for the layout resources.
 */
void Screen::doBuildLayout_(const sead::SafeString& rPath, nn::ui2d::ResourceAccessor* pAccessor) {
    ControlCreator controlCreator(mButtonGroup, &mControls, &mStaticControls);
    TextSearcher textSearcher(mScreenMgr->getMessageMgr()->getLayoutMessageSet(getMessageName_()),
                              mTagProcessor);
    nn::ui2d::BuildResultInformation result;
    mLayout->BuildWithName(&result, GetDevice(), pAccessor, &controlCreator, &textSearcher,
                           nn::ui2d::Layout::BuildOption(), rPath.cstr(), false);
}

/** @return The message set of this screen's layout. */
const MessageSet* Screen::getLayoutMessageSet_() const {
    return mScreenMgr->getMessageMgr()->getLayoutMessageSet(getMessageName_());
}

/** @param pHeap Heap for resources created by derived screens. */
void Screen::doInitialize_(sead::Heap* pHeap) {}

/** @brief Hook called when the screen starts opening. */
void Screen::doOpenStart_() {}

/** @brief Hook called when the screen finished opening. */
void Screen::doOpenEnd_() {}

/** @brief Hook called when the screen starts closing. */
void Screen::doCloseStart_() {}

/** @brief Hook called when the screen finished closing. */
void Screen::doCloseEnd_() {}

/** @return Whether the global matrices are recalculated every frame. */
bool Screen::isForceGlbMtxDirty_() const { return false; }

/** @return Animation step of the screen manager. */
float Screen::getAnimationStep_() const { return mScreenMgr->getAnimationStep(); }

/** @param pInfo Render buffers to draw the layout into. */
void Screen::doDraw_(const DrawInfoEx::RenderBufferInfo* pInfo) {
    if (!mIsInitialized || mIsDrawDisabled) {
        return;
    }

    if (mState == cState_Closed || mCloseMode == cCloseMode_ExtendEnd) {
        return;
    }

    mDrawInfo->setRenderBufferInfo(pInfo);
    mLayout->Draw(*mDrawInfo, *pInfo->pDrawContext->getCommandBuffer());
    mDrawInfo->freeDynamicTexture();
    mDrawInfo->setRenderBufferInfo(nullptr);
}

/** @param pButton Button that started its on transition. */
void Screen::doButtonOnStart_(AnimButton* pButton) {}

/** @param pButton Button that finished its on transition. */
void Screen::doButtonOnEnd_(AnimButton* pButton) {}

/** @param pButton Button that started its off transition. */
void Screen::doButtonOffStart_(AnimButton* pButton) {}

/** @param pButton Button that finished its off transition. */
void Screen::doButtonOffEnd_(AnimButton* pButton) {}

/** @param pButton Button that started its press. */
void Screen::doButtonDownStart_(AnimButton* pButton) {}

/** @param pButton Button that finished its press. */
void Screen::doButtonDownEnd_(AnimButton* pButton) {}

/** @param pButton Button that started cancelling. */
void Screen::doButtonCancelStart_(AnimButton* pButton) {}

/** @param pButton Button that finished cancelling. */
void Screen::doButtonCancelEnd_(AnimButton* pButton) {}

/** @return Effect link system used by the screen. */
xlink2::System* Screen::getElinkSystem_() const { return nullptr; }

/**
 * @param pUser Sound link user.
 * @return Resource list for pUser.
 */
const char* Screen::getSlink2ResourceList_(xlink2::UserInstanceSLink* pUser) const { return nullptr; }

/** @return Number of local sound link properties. */
u32 Screen::getSlink2LocalPropertyNum_() const { return 0; }

/** @param pUser Sound link user whose properties are defined. */
void Screen::setSlink2PropertyDefinition_(xlink2::UserInstanceSLink* pUser) {}

/**
 * @brief Moves the screen to another draw unit.
 * @param drawUnitId Draw unit to draw in.
 */
void Screen::setDrawUnitId_(s8 drawUnitId) {
    mDrawLayer = drawUnitId;
    mLayout->setDrawTargetAnim(getDrawTarget());

    if (mSoundLinkUser != nullptr) {
        mSoundLinkUser->setDrawTarget(getDrawTarget());
    }

    if (isActive_()) {
        mScreenMgr->activateScreen(mScreenId);
    }
}

/** @return Whether the screen is assigned to a draw layer in the manager. */
bool Screen::isActive_() const { return mScreenMgr->mScreenLayers[mScreenId] >= 0; }

/** @param touch Whether buttons use touch input. */
void Screen::setTouch_(bool touch) {
    if (mIsTouch == touch) {
        return;
    }

    mIsTouch = touch;
    mButtonGroup->SetTouchDevice(touch);
    registerController_();
}

/** @brief Requests capture panes to be refreshed on the next update. */
void Screen::requestCapture_() { mIsCaptureRequested = true; }

/**
 * @param pLabel Message label.
 * @return The message, or an empty message without a message set.
 */
MessageString Screen::findLayoutMessage_(const char* pLabel) {
    const MessageSet* messageSet = getLayoutMessageSet_();

    if (messageSet != nullptr) {
        return messageSet->findMessage(pLabel);
    }

    return MessageString();
}

/**
 * @param pName Pane name.
 * @return The pane, searched recursively from the root.
 */
nn::ui2d::Pane* Screen::findPane_(const char* pName) { return mLayout->findPaneByName(pName); }

/**
 * @param pName Parts pane name.
 * @return The layout of the parts pane.
 */
LayoutEx* Screen::findPartsLayout_(const char* pName) { return mLayout->findPartsLayout(pName); }

/**
 * @param pName Control name.
 * @return The control, or nullptr.
 */
ControlBase* Screen::findControl_(const char* pName) {
    const sead::SafeString name(pName);

    for (auto& control : mControls) {
        if (name.isEqual(control.getName())) {
            return &control;
        }
    }

    return nullptr;
}

/**
 * @param pName Control name.
 * @param pParentParts Name of the parts pane around the control's layout.
 * @return The control, or nullptr.
 */
ControlBase* Screen::findControlWithParentParts_(const char* pName, const char* pParentParts) {
    const sead::SafeString name(pName);

    for (auto& control : mControls) {
        const LayoutEx* parentLayout = control.getLayout()->getParentLayout();

        if (name.isEqual(control.getName()) && parentLayout != nullptr &&
            IsEqualResName<0x40>(pParentParts, parentLayout->getRootPane()->GetName())) {
            return &control;
        }
    }

    return nullptr;
}

/**
 * @param pName Control name.
 * @param pParentLayout Parent of the control's layout.
 * @return The control, or nullptr.
 */
ControlBase* Screen::findControlWithParentLayout_(const char* pName, const LayoutEx* pParentLayout) {
    const sead::SafeString name(pName);

    for (auto& control : mControls) {
        if (name.isEqual(control.getName()) &&
            control.getLayout()->getParentLayout() == pParentLayout) {
            return &control;
        }
    }

    return nullptr;
}

/**
 * @param pName Control name.
 * @param pParts Name of the parts pane whose layout holds the control.
 * @return The control, or nullptr.
 */
ControlBase* Screen::findControlWithParts_(const char* pName, const char* pParts) {
    const sead::SafeString name(pName);

    for (auto& control : mControls) {
        const LayoutEx* layout = control.getLayout();

        if (name.isEqual(control.getName()) && layout != nullptr &&
            IsEqualResName<0x40>(pParts, layout->getRootPane()->GetName())) {
            return &control;
        }
    }

    return nullptr;
}

/**
 * @param pName Control name.
 * @param pLayout Layout holding the control.
 * @return The control, or nullptr.
 */
ControlBase* Screen::findControlWithLayout_(const char* pName, const LayoutEx* pLayout) {
    const sead::SafeString name(pName);

    for (auto& control : mControls) {
        if (name.isEqual(control.getName()) && control.getLayout() == pLayout) {
            return &control;
        }
    }

    return nullptr;
}

/**
 * @param pName Static control name.
 * @return The static control, or nullptr.
 */
ControlBase* Screen::findStaticControl_(const char* pName) {
    const sead::SafeString name(pName);

    for (auto& control : mStaticControls) {
        if (name.isEqual(control.getName())) {
            return &control;
        }
    }

    return nullptr;
}

/** @param pNode Node the cursor starts on. */
void Screen::setPrimaryBoxCursorNode_(BoxCursorNode* pNode) { mPrimaryCursor = pNode; }

/** @param pName Button name of the node the cursor starts on. */
void Screen::setPrimaryBoxCursorNodeByName_(const char* pName) {
    mPrimaryCursor = findBoxCursorNodeByName_(pName);
}

/**
 * @param pName Button name.
 * @return The node of the button, or nullptr.
 */
BoxCursorNode* Screen::findBoxCursorNodeByName_(const char* pName) {
    for (auto& node : mCursorNodes) {
        if (IsEqualResName<0x18>(pName, node.getButton()->getName())) {
            return &node;
        }
    }

    return nullptr;
}

/** @param tag Button tag of the node the cursor starts on. */
void Screen::setPrimaryBoxCursorNodeByTag_(int tag) { mPrimaryCursor = findBoxCursorNodeByTag_(tag); }

/**
 * @param tag Button tag.
 * @return The node of the button, or nullptr.
 */
BoxCursorNode* Screen::findBoxCursorNodeByTag_(int tag) {
    for (auto& node : mCursorNodes) {
        if (node.getButton()->GetTag() == tag) {
            return &node;
        }
    }

    return nullptr;
}

/** @param pButton Button of the node the cursor starts on. */
void Screen::setPrimaryBoxCursorNodeByButton_(const AnimButton* pButton) {
    mPrimaryCursor = findBoxCursorNodeByButton_(pButton);
}

/** @param pNode Node the cursor moves to. */
void Screen::moveBoxCursor_(BoxCursorNode* pNode) {
    BoxCursorMgr* boxCursorMgr = mScreenMgr->getBoxCursorMgr();

    if (boxCursorMgr != nullptr) {
        boxCursorMgr->moveBoxCursor(pNode);
    }
}

/** @param pName Button name of the node the cursor moves to. */
void Screen::moveBoxCursorByName_(const char* pName) {
    BoxCursorNode* node = findBoxCursorNodeByName_(pName);

    if (node != nullptr) {
        moveBoxCursor_(node);
    }
}

/** @param tag Button tag of the node the cursor moves to. */
void Screen::moveBoxCursorByTag_(int tag) {
    BoxCursorNode* node = findBoxCursorNodeByTag_(tag);

    if (node != nullptr) {
        moveBoxCursor_(node);
    }
}

/** @param pButton Button of the node the cursor moves to. */
void Screen::moveBoxCursorByButton_(const AnimButton* pButton) {
    BoxCursorNode* node = findBoxCursorNodeByButton_(pButton);

    if (node != nullptr) {
        moveBoxCursor_(node);
    }
}

/**
 * @param pName Button name.
 * @param pParentParts Name of the parts pane around the button.
 * @return The node of the button, or nullptr.
 */
BoxCursorNode* Screen::findBoxCursorNodeByNameWithParentParts_(const char* pName,
                                                               const char* pParentParts) {
    for (auto& node : mCursorNodes) {
        AnimButton* button = node.getButton();
        const LayoutEx* layout = GetButtonOuterLayout(button);

        if (IsEqualResName<0x18>(pName, button->getName()) && layout != nullptr &&
            IsEqualResName<0x40>(pParentParts, layout->getRootPane()->GetName())) {
            return &node;
        }
    }

    return nullptr;
}

/**
 * @param pName Button name.
 * @param pParentLayout Layout around the button.
 * @return The node of the button, or nullptr.
 */
BoxCursorNode* Screen::findBoxCursorNodeByNameWithParentLayout_(const char* pName,
                                                                const LayoutEx* pParentLayout) {
    for (auto& node : mCursorNodes) {
        AnimButton* button = node.getButton();

        if (IsEqualResName<0x18>(pName, button->getName()) &&
            GetButtonOuterLayout(button) == pParentLayout) {
            return &node;
        }
    }

    return nullptr;
}

/**
 * @param pNode Node whose route is set.
 * @param direction Direction of the route.
 * @param pDestination Node the route leads to.
 */
void Screen::setBoxCursorNodeRoute_(BoxCursorNode* pNode, Direction direction,
                                    BoxCursorNode* pDestination) {
    pNode->setRouteNode(direction, pDestination);
}

/**
 * @param pName Button name of the node whose route is set.
 * @param direction Direction of the route.
 * @param pDestination Button name of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteByName_(const char* pName, Direction direction,
                                          const char* pDestination) {
    findBoxCursorNodeByName_(pName)->setRouteNode(direction, findBoxCursorNodeByName_(pDestination));
}

/**
 * @param tag Button tag of the node whose route is set.
 * @param direction Direction of the route.
 * @param destination Button tag of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteByTag_(int tag, Direction direction, int destination) {
    findBoxCursorNodeByTag_(tag)->setRouteNode(direction, findBoxCursorNodeByTag_(destination));
}

/**
 * @param pButton Button of the node whose route is set.
 * @param direction Direction of the route.
 * @param pDestination Button of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteByButton_(const AnimButton* pButton, Direction direction,
                                            const AnimButton* pDestination) {
    findBoxCursorNodeByButton_(pButton)->setRouteNode(direction,
                                                      findBoxCursorNodeByButton_(pDestination));
}

/**
 * @param pNode Node whose route is set in both directions.
 * @param direction Direction of the route from pNode.
 * @param pDestination Node the route leads to.
 */
void Screen::setBoxCursorNodeRouteEach_(BoxCursorNode* pNode, Direction direction,
                                        BoxCursorNode* pDestination) {
    pNode->setRouteNodeEach(direction, pDestination);
}

/**
 * @param pName Button name of the node whose route is set in both directions.
 * @param direction Direction of the route.
 * @param pDestination Button name of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteEachByName_(const char* pName, Direction direction,
                                              const char* pDestination) {
    findBoxCursorNodeByName_(pName)->setRouteNodeEach(direction, findBoxCursorNodeByName_(pDestination));
}

/**
 * @param tag Button tag of the node whose route is set in both directions.
 * @param direction Direction of the route.
 * @param destination Button tag of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteEachByTag_(int tag, Direction direction, int destination) {
    findBoxCursorNodeByTag_(tag)->setRouteNodeEach(direction, findBoxCursorNodeByTag_(destination));
}

/**
 * @param pButton Button of the node whose route is set in both directions.
 * @param direction Direction of the route.
 * @param pDestination Button of the node the route leads to.
 */
void Screen::setBoxCursorNodeRouteEachByButton_(const AnimButton* pButton, Direction direction,
                                                const AnimButton* pDestination) {
    findBoxCursorNodeByButton_(pButton)->setRouteNodeEach(direction,
                                                          findBoxCursorNodeByButton_(pDestination));
}

/** @brief Clears the routes of every cursor node. */
void Screen::clearBoxCursorNodeRouteAll_() {
    for (auto& node : mCursorNodes) {
        node.clearRouteAll();
    }
}

/** @param pLayout Layout whose text boxes are resized. */
void Screen::adjustPaneSizeToTextSizeAll_(LayoutEx* pLayout) {
    iteratePaneForAdjustPaneSizeToTextSize_(pLayout->getRootPane(), pLayout);
}

/** @brief Creates animators not created during initialization; none in the base screen. */
void Screen::createRestAnimators_() {}

/** @brief Ends an extended close and deactivates the screen. */
void Screen::endExtendClose_() {
    mCloseMode = cCloseMode_Normal;
    mScreenMgr->inactivateScreen(mScreenId);
    mState = cState_Closed;
}

/** @brief Suppresses the sound of the next button highlight. */
void Screen::muteNextNoOperationButtonOnSE_() { mNoOperationButtonOnSE = 0; }
}  // namespace eui
