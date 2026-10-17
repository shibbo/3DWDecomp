#pragma once
#include <heap/seadDisposer.h>
#include <hostio/seadHostIONode.h>
#include <prim/seadRuntimeTypeInfo.h>
#include <container/seadOffsetList.h>
#include <math/seadBoundBox.h>
#include <eui/euiDrawInfoEx.h>
#include <eui/euiControlCreator.h>
#include <eui/euiButtonBase.h>
#include <eui/euiAnimator.h>
#include <eui/euiUtility.h>
namespace xlink2 { class UserInstanceSLink; class System; }
namespace nn::ui2d { class ResourceAccessor; }
namespace eui {
class ScreenMgr; class LayoutEx; class BoxCursorNode; class PartsEx; class Animator;
class UIController; class MultiArcResourceAccessor; class TagProcessor; class MessageSet;
class MessageString; class ControlBase;

/**
 * @brief Effect link user owned by a screen; created by derived screens.
 *
 * Only the slots a screen calls are declared.
 */
class ScreenEffectLinkUser {
public:
    virtual void calc();
    virtual void calcAfterLayout();
    virtual void finalize();
    virtual void onOpenStart();
    virtual void onCloseStart();
};

/**
 * @brief Sound link user owned by a screen; created by derived screens.
 *
 * Only the slots a screen calls are declared.
 */
class ScreenSoundLinkUser {
public:
    virtual void calc();
    virtual void finalize();
    virtual void onOpenStart();
    virtual void onCloseEnd();
    virtual void setDrawTarget(int target);
    virtual void setButtonSoundType(int type);
};

class Screen : public sead::IDisposer, public sead::hostio::Node {
public:
    /** @brief Open request passed to open(); stored in the pending request. */
    enum OpenOption {
        cOpenOption_Normal = 1,
        cOpenOption_Restart = 2,
        cOpenOption_Skip = 3,
        cOpenOption_Immediate = 4,
    };

    /** @brief Close request passed to close(); stored as a negative pending request. */
    enum CloseOption {
        cCloseOption_Normal = -1,
        cCloseOption_Extend = -2,
        cCloseOption_NoState = -3,
        cCloseOption_Immediate = -4,
    };

    enum State { cState_Closed, cState_Opening, cState_Opened, cState_Closing };

    /** @brief Progress of an extended close (cCloseOption_Extend). */
    enum CloseMode { cCloseMode_Normal, cCloseMode_Extend, cCloseMode_ExtendEnd };

    enum AnimatorOperationType { cPlay, cPlayFromCurrent, cStop, cStopCurrent, cStopAtMin, cStopAtMax };
    Screen();
    ~Screen() override;
    SEAD_RTTI_BASE(Screen);
    virtual bool isEnableControl() const;
    virtual void open(OpenOption option);
    virtual void close(CloseOption option);
    virtual void adjstBoxCursor(sead::BoundBox2f*, const BoxCursorNode*) const;
    virtual BoxCursorNode* createBoxCursorNode(sead::Heap*);
    virtual const char* replacePartsLayoutName(const char*, PartsEx*, LayoutEx*);
    virtual void afterBuildPaneCallback(nn::ui2d::Pane*, LayoutEx*, const nn::ui2d::BuildArgSet&);
    virtual void animatorOperationCallback(AnimatorOperationType, Animator*);
    virtual void initialize(ScreenMgr*, sead::Heap*, const char*, int, s8, bool);
    virtual void update();
    virtual void draw(const DrawInfoEx::RenderBufferInfo*);
    virtual const char* getLayoutName_() const;
    virtual const char* getMessageName_() const;
    virtual const char* getArchiveName_() const;
    virtual bool isPlayPartsInOut_() const;
    virtual bool isDisallowHitLowerScreenOnButtonHit_() const;
    virtual LayoutEx* doCreateLayout_(sead::Heap*);
    virtual DrawInfoEx* doCreateDrawInfoEx_(sead::Heap*);
    virtual ButtonGroup* doCreateButtonGroup_(sead::Heap*);
    virtual void doAfterBuildLayout_(sead::Heap*);
    virtual void doSetupDrawInfo_();
    virtual UIController* doCreateUIController_(sead::Heap*);
    virtual MultiArcResourceAccessor* doCreateResourceAccessor_(sead::Heap*);
    virtual TagProcessor* doCreateTagProcessor_(sead::Heap*);
    virtual void doBuildLayout_(const sead::SafeString&, nn::ui2d::ResourceAccessor*);
    virtual void doLoadResource_(sead::Heap*);
    virtual void doInitialize_(sead::Heap*);
    virtual void doUpdate_();
    virtual float getAnimationStep_() const;
    virtual void doDraw_(const DrawInfoEx::RenderBufferInfo*);
    virtual void doOpenStart_();
    virtual void doOpenEnd_();
    virtual void doCloseStart_();
    virtual void doCloseEnd_();
    virtual void doButtonOnStart_(AnimButton*);
    virtual void doButtonOnEnd_(AnimButton*);
    virtual void doButtonOffStart_(AnimButton*);
    virtual void doButtonOffEnd_(AnimButton*);
    virtual void doButtonDownStart_(AnimButton*);
    virtual void doButtonDownEnd_(AnimButton*);
    virtual void doButtonCancelStart_(AnimButton*);
    virtual void doButtonCancelEnd_(AnimButton*);
    virtual xlink2::System* getElinkSystem_() const;
    virtual const char* getSlink2ResourceList_(xlink2::UserInstanceSLink*) const;
    virtual u32 getSlink2LocalPropertyNum_() const;
    virtual void setSlink2PropertyDefinition_(xlink2::UserInstanceSLink*);
    virtual void updateButton_();
    virtual void updateControl_();
    virtual void openStart_(OpenOption option);
    virtual bool isOpenEnd_();
    virtual void openEnd_();
    virtual void closeStart_(CloseOption option);
    virtual bool isCloseEnd_();
    virtual void closeEnd_();
    virtual bool isForceGlbMtxDirty_() const;
    virtual void updateAnimator_();
    virtual void registerController_();
    virtual void unregisterController_();
    virtual void setupPaneAfterBuild_(nn::ui2d::Pane*, LayoutEx*, u32*);

    /**
     * @param pPane Pane being set up.
     * @param pCount Receives the number of panes that need an effect link.
     */
    virtual void countEffectLinkPane_(nn::ui2d::Pane* pPane, u32* pCount) {}

    /**
     * @param pHeap Heap for the effect link user.
     * @param count Number of panes counted by countEffectLinkPane_.
     */
    virtual void createEffectLinkUser_(sead::Heap* pHeap, u32 count) {}

    /** @param pHeap Heap for the sound link user. */
    virtual void createSoundLink2User_(sead::Heap* pHeap) {}

    /** @param pName Sound event to emit. */
    virtual void invokeSoundLink2Event_(const char* pName) {}

    /**
     * @param pButton Button whose sound event is emitted.
     * @param pName Sound event suffix.
     */
    virtual void invokeSoundLink2ButtonEvent_(AnimButton* pButton, const char* pName) {}

    /**
     * @param pAnimator Animator that started playing.
     * @param pName Sound event suffix.
     */
    virtual void invokeSoundLink2AnimPlayEvent(Animator* pAnimator, const char* pName) {}

    void deleteLayout_();
    void iteratePaneForSetupPaneAfterBuild_(nn::ui2d::Pane* pPane, LayoutEx* pLayout, u32* pCount);
    void updateStaticControl_();
    bool isOpened() const;
    bool isClosed() const;
    bool isOpening() const;
    bool isClosing() const;
    int getViewerType() const;
    void setAnimatorActive(Animator* pAnimator);
    void eraseAnimatorFromActiveList(Animator* pAnimator);
    void setOwnInitializeHeap(bool own);
    void eraseBoxCursorNodeFromRouteNodes(const BoxCursorNode* pNode);
    bool isOpenedInitial() const;
    bool moveBoxCursorByButton(const AnimButton* pButton);
    DrawTarget getDrawTarget() const;
    BoxCursorNode* findBoxCursorNodeByButton_(const AnimButton* pButton);
    LayoutEx* findHitLayout(const sead::Vector2f& rPosition);
    void setDisallowHitLowerScreen_();
    float getOpenFrameSize() const;
    float getCloseFrameSize() const;
    void stopOpenAtFrame(float frame);
    void stopCloseAtFrame(float frame);
    void initializeForMinimum(ScreenMgr* pScreenMgr, const char* pLayoutName);
    void clearFlagsAfterCalculateMtxForMinimum();
    void setLayoutForMinimunDebug(LayoutEx* pLayout);
    bool isAllowedHit_() const;
    void buttonStateChangeCallback(AnimButton* pButton, ButtonBase::State oldState, ButtonBase::State newState);
    void iteratePaneForAdjustPaneSizeToTextSize_(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
    static void setPropertyDefinitionCallbackStatic_(void* pScreen, xlink2::UserInstanceSLink* pUser);
    const MessageSet* getLayoutMessageSet_() const;
    void setDrawUnitId_(s8 drawUnitId);
    bool isActive_() const;
    void setTouch_(bool touch);
    void requestCapture_();
    MessageString findLayoutMessage_(const char* pLabel);
    nn::ui2d::Pane* findPane_(const char* pName);
    LayoutEx* findPartsLayout_(const char* pName);
    ControlBase* findControl_(const char* pName);
    ControlBase* findControlWithParentParts_(const char* pName, const char* pParentParts);
    ControlBase* findControlWithParentLayout_(const char* pName, const LayoutEx* pParentLayout);
    ControlBase* findControlWithParts_(const char* pName, const char* pParts);
    ControlBase* findControlWithLayout_(const char* pName, const LayoutEx* pLayout);
    ControlBase* findStaticControl_(const char* pName);
    void setPrimaryBoxCursorNode_(BoxCursorNode* pNode);
    void setPrimaryBoxCursorNodeByName_(const char* pName);
    BoxCursorNode* findBoxCursorNodeByName_(const char* pName);
    void setPrimaryBoxCursorNodeByTag_(int tag);
    BoxCursorNode* findBoxCursorNodeByTag_(int tag);
    void setPrimaryBoxCursorNodeByButton_(const AnimButton* pButton);
    void moveBoxCursor_(BoxCursorNode* pNode);
    void moveBoxCursorByName_(const char* pName);
    void moveBoxCursorByTag_(int tag);
    void moveBoxCursorByButton_(const AnimButton* pButton);
    BoxCursorNode* findBoxCursorNodeByNameWithParentParts_(const char* pName, const char* pParentParts);
    BoxCursorNode* findBoxCursorNodeByNameWithParentLayout_(const char* pName, const LayoutEx* pParentLayout);
    void setBoxCursorNodeRoute_(BoxCursorNode* pNode, Direction direction, BoxCursorNode* pDestination);
    void setBoxCursorNodeRouteByName_(const char* pName, Direction direction, const char* pDestination);
    void setBoxCursorNodeRouteByTag_(int tag, Direction direction, int destination);
    void setBoxCursorNodeRouteByButton_(const AnimButton* pButton, Direction direction,
                                        const AnimButton* pDestination);
    void setBoxCursorNodeRouteEach_(BoxCursorNode* pNode, Direction direction, BoxCursorNode* pDestination);
    void setBoxCursorNodeRouteEachByName_(const char* pName, Direction direction, const char* pDestination);
    void setBoxCursorNodeRouteEachByTag_(int tag, Direction direction, int destination);
    void setBoxCursorNodeRouteEachByButton_(const AnimButton* pButton, Direction direction,
                                            const AnimButton* pDestination);
    void clearBoxCursorNodeRouteAll_();
    void adjustPaneSizeToTextSizeAll_(LayoutEx* pLayout);
    void createRestAnimators_();
    void endExtendClose_();
    void muteNextNoOperationButtonOnSE_();

    const sead::SafeString& getName() const { return mName; }
    ScreenMgr* getScreenMgr() const { return mScreenMgr; }
    ButtonGroup* getButtonGroup() const { return mButtonGroup; }
    void setLastActiveCursor(const BoxCursorNode* pNode) { mLastActiveCursor = pNode; }
    sead::Heap* getInitializeHeap() const { return mInitializeHeap; }
    bool isOwnInitializeHeap() const { return (mFlags & 1) != 0; }
    s8 getDrawLayer() const { return mDrawLayer; }
    bool isTouchMode() const { return mIsTouch != 0; }

    static bool isTouchMode(const Screen* pScreen) { return pScreen != nullptr ? pScreen->isTouchMode() : false; }

    ScreenMgr* mScreenMgr;
    LayoutEx* mLayout;
    ButtonGroup* mButtonGroup;
    ControlList mControls;
    ControlList mStaticControls;
    UIController* mController;
    DrawInfoEx* mDrawInfo;
    TagProcessor* mTagProcessor;
    nn::util::IntrusiveList<Animator,
        nn::util::IntrusiveListMemberNodeTraits<Animator, &Animator::mActiveLink>> mActiveAnimators;
    sead::OffsetList<BoxCursorNode> mCursorNodes;
    sead::Heap* mInitializeHeap;
    int mScreenId;
    sead::SafeString mName;
    BoxCursorNode* mPrimaryCursor;
    const BoxCursorNode* mLastActiveCursor;
    ScreenEffectLinkUser* mEffectLinkUser;
    ScreenSoundLinkUser* mSoundLinkUser;
    float _e0;
    s8 mDrawLayer;
    s8 mOpenRequest;
    u8 mState;
    u8 mCloseMode;
    u8 mIsInitialized;
    u8 mIsDrawDisabled;
    u8 mIsUpdatePaused;
    u8 mIsTouch;
    u8 mIsCaptureRequested;
    u8 mNoOperationButtonOnSE;
    u8 mFlags;
};

static_assert(sizeof(Screen) == 0xf0, "Screen size");
}
