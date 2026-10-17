#pragma once
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
namespace nn::ui2d { class Group; class GroupContainer; struct ResTextBox; }
namespace eui {
class Screen;
class Animator;
class AnimatorSet;
class MessageString;
class TextBoxEx;
class ControlCreator;
class TextSearcher;
class DrawTarget;

/** @brief Layout extended with the eui open/close animations, parts layouts and messages. */
class LayoutEx : public nn::ui2d::Layout {
public:
    /** @brief How the open animation is started. */
    enum OpenAnim {
        cOpenAnim_Play,   ///< Play the open animation.
        cOpenAnim_End,    ///< Jump to the opened state.
        cOpenAnim_Start,  ///< Jump to the closed state.
    };

    /** @brief Sound link events raised by the open and close animations. */
    enum SoundLink2Event {
        cSoundLink2Event_Open,
        cSoundLink2Event_Close,
    };

    /** @brief State of the open/close animations, stored in mOpenState. */
    enum OpenState {
        cOpenState_Closed,
        cOpenState_Opening,
        cOpenState_Opened,
        cOpenState_Closing,
    };

    /** @brief Bits stored in mFlags. */
    enum Flag {
        cFlag_AdjustPaneSizeToTextSize = 1 << 0,
        cFlag_LoopRandom = 1 << 1,
        cFlag_Copied = 1 << 2,
    };

    /** @brief Pane calculation context that is always initialized before use. */
    class CalculatePaneContext : public nn::ui2d::Pane::CalculateContext {
    public:
        CalculatePaneContext();
        CalculatePaneContext(const nn::ui2d::DrawInfo& rDrawInfo, const nn::ui2d::Layout* pLayout);

    private:
        /** @brief Clear every member before the context is set up. */
        void clear_() {
            pRectDrawer = nullptr;
            pViewMtx = nullptr;
            locationAdjustScale.v[0] = 0.0f;
            locationAdjustScale.v[1] = 0.0f;
            influenceAlpha = 0.0f;
            isLocationAdjust = false;
            isInvisiblePaneCalculateMtx = false;
            isAlphaZeroPaneCalculateMtx = false;
            isInfluenceAlpha = false;
            pLayoutInformation = nullptr;
            globalMatrixDirty = false;
        }
    };

    explicit LayoutEx(Screen* pScreen);
    LayoutEx(const LayoutEx& rOther, const char* pRootName, LayoutEx* pParentLayout, bool isCopied);
    ~LayoutEx() override = default;
    NN_RUNTIME_TYPEINFO(nn::ui2d::Layout);
    bool BuildImpl(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, const void*, nn::ui2d::ResourceAccessor*, const nn::ui2d::BuildArgSet&, const PartsBuildDataSet*) override;
    nn::ui2d::Pane* BuildPaneObj(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, u32, const void*, const void*, const nn::ui2d::BuildArgSet&) override;
    nn::ui2d::Layout* BuildPartsLayout(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, const char*, const PartsBuildDataSet&, const nn::ui2d::BuildArgSet&) override;
    void CalculateImpl(nn::ui2d::DrawInfo&, bool) override;
    virtual void animatorDisableCallback(Animator*);
    virtual LayoutEx* doCreatePartsLayout_(const char*, const PartsBuildDataSet&, const nn::ui2d::BuildArgSet&);
    virtual bool attachPartsLayoutArchive_(const sead::SafeString&);
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
    Animator* createUnbindedAnimatorWithAnimNumMultiple(const char* pName, int animNum, bool enabled);
    int setMessageStringForEachId(const char* pId, const MessageString& rText, bool isAdjust,
                                  void* pUserData);
    int setMessageStringForEachIdRecursive_(nn::ui2d::Pane* pPane, const char* pId,
                                            const MessageString& rText, bool* pHasNext, int page,
                                            bool* pIsAdjustNeeded, void* pUserData);
    void adjustPaneSizeToTextSizeRecursive_(nn::ui2d::Pane* pPane);
    int setMessageStringForEachIdWithPage(const char* pId, const MessageString& rText,
                                          bool* pHasNext, u32 page, bool isAdjust,
                                          void* pUserData);
    TextBoxEx* findTextBoxById(const char* pId);
    TextBoxEx* findTextBoxByIdRecursive_(nn::ui2d::Pane* pPane, const char* pId);
    LayoutEx* findPartsLayout(const char* pName);
    Animator* findAnimator(const char* pName);
    void setRootPaneGlobalMtxDirty();
    void startAnimOpenImpl_(bool isPlayParts, OpenAnim anim, bool isSelf);
    void invokeSoundLink2Event_(SoundLink2Event event);
    void startAnimCloseImpl_(bool isPlayParts, bool isSkip);
    void disableLoopAnimator_(bool isRecursive);
    bool isAnimOpenEnd(bool isRecursive) const;
    bool isAnimCloseEnd(bool isRecursive) const;
    void setDrawTargetAnim(DrawTarget target);
    void adjustPaneSizeToTextSize();
    bool isScalableFontTextBox_(const nn::ui2d::ResTextBox* pResource,
                                const nn::ui2d::ResTextBox* pOverride,
                                const nn::ui2d::BuildArgSet& rArgs);
    void setOpenPaneTreeNodeInHostIO(bool isOpen);
    void setOpenPaneTreeNodeInHostIORecursive(bool isOpen);
    bool isOpenPaneTreeNodeInHostIO() const;
    void beginBuildWithRapidPaneTree(const char* pName, const sead::Vector2f& rSize,
                                     nn::ui2d::ResourceAccessor* pAccessor);
    void setRootPaneForRapidPaneTree(nn::ui2d::Pane* pPane);
    LayoutEx* createPartsLayout(const char* pLayoutName, const char* pPaneName,
                                LayoutEx* pRootLayout, const sead::Vector2f& rSize,
                                ControlCreator* pControlCreator, TextSearcher* pTextSearcher);

    Screen* getScreen() const { return mScreen; }
    LayoutEx* getParentLayout() const { return mParentLayout; }
    nn::ui2d::Pane* getRootPane() const { return mRootPane; }
    nn::ui2d::Pane* findPaneByName(const char* pName) const { return mRootPane->FindPaneByName(pName, true); }

    /** @return Whether a parts pane using this layout is measured as a whole by an AlignPane. */
    bool isAlignAsParts() const { return mOpenState != cOpenState_Closed; }

    /** @return Whether the loop animation starts at a random frame. */
    bool isLoopRandom() const { return (mFlags & cFlag_LoopRandom) != 0; }

    nn::ui2d::GroupContainer* getGroupContainer() const { return static_cast<nn::ui2d::GroupContainer*>(_20); }

    const char* getLayoutName() const { return static_cast<const char*>(_30); }

    const char* getRootName() const {
        return mRootPane->mParent != nullptr ? mRootPane->mPanelName : getLayoutName();
    }

    Animator* mInAnimator;
    Animator* mOutAnimator;
    Animator* mLoopAnimator;
    Animator* mWaitAnimator;
    Screen* mScreen;
    LayoutEx* mParentLayout;
    u8 mFlags;
    u8 mOpenState;
};

static_assert(sizeof(nn::ui2d::Layout) == 0x60, "Layout size");
static_assert(sizeof(LayoutEx) == 0x98, "LayoutEx size");
}
