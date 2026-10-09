#include <eui/euiLayoutEx.h>
#include <eui/euiAlignPane.h>
#include <eui/euiAnimator.h>
#include <eui/euiAnimatorSet.h>
#include <eui/euiArcResourceMgr.h>
#include <eui/euiBoundingEx.h>
#include <eui/euiCapturePane.h>
#include <eui/euiDynamicCapturePane.h>
#include <eui/euiDynamicCaptureUsePictureEx.h>
#include <eui/euiDynamicCaptureUseWindowEx.h>
#include <eui/euiFontMgr.h>
#include <eui/euiMassDrawPane.h>
#include <eui/euiMessageString.h>
#include <eui/euiMultiArcResourceAccessor.h>
#include <eui/euiMultiFilterPictureEx.h>
#include <eui/euiMultiFilterWindowEx.h>
#include <eui/euiPartsEx.h>
#include <eui/euiPictureEx.h>
#include <eui/euiRootPane.h>
#include <eui/euiScalableFontTextBoxEx.h>
#include <eui/euiScissorPane.h>
#include <eui/euiScreen.h>
#include <eui/euiTagProcessor.h>
#include <eui/euiTextBoxEx.h>
#include <eui/euiTextSearcher.h>
#include <eui/euiUtility.h>
#include <eui/euiWindowEx.h>
#include <container/seadSafeArray.h>
#include <gfx/nin/seadGraphicsNvn.h>
#include <heap/seadFrameHeap.h>
#include <nn/font/font_ScalableFont.h>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_ExtUserData.h>
#include <nn/ui2d/ui2d_Group.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/util/util_VectorApi.h>
#include <prim/seadEnvUtil.h>
#include <prim/seadStringBuilder.h>

namespace eui {
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

/** @brief Kinds of the pane blocks stored in a layout resource. */
enum PaneBlockKind : u32 {
    cPaneBlockKind_Pane = MakeSignature('p', 'a', 'n', '1'),
    cPaneBlockKind_Picture = MakeSignature('p', 'i', 'c', '1'),
    cPaneBlockKind_TextBox = MakeSignature('t', 'x', 't', '1'),
    cPaneBlockKind_Window = MakeSignature('w', 'n', 'd', '1'),
    cPaneBlockKind_Bounding = MakeSignature('b', 'n', 'd', '1'),
    cPaneBlockKind_Alignment = MakeSignature('a', 'l', 'i', '1'),
    cPaneBlockKind_Scissor = MakeSignature('s', 'c', 'r', '1'),
    cPaneBlockKind_Parts = MakeSignature('p', 'r', 't', '1'),
};

/** @brief Number of materials whose names survive a localized pane replacement. */
const int cReplaceMaterialMax = 9;

/** @brief Edge length of the placeholder size given to a parts pane built at run time. */
const float cPartsPaneUnitSize = 40.0f;

using MaterialNameArray = sead::SafeArray<const char*, cReplaceMaterialMax>;

/**
 * @brief Get the graphics device used for layout resources.
 * @return Device of the sead graphics instance.
 */
inline nn::gfx::Device* GetDevice() {
    return reinterpret_cast<nn::gfx::Device*>(sead::GraphicsNvn::instance()->getGfxDevice());
}

/**
 * @brief Compare two names up to a maximum length.
 * @param pName First name.
 * @param pOther Second name.
 * @param length Maximum number of characters to compare.
 * @return Whether the names are equal within length characters.
 */
inline bool IsSameName(const char* pName, const char* pOther, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (pName[i] != pOther[i]) {
            return false;
        }

        if (pName[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Check whether a text box is addressed by a message id.
 * @param pTextBox Text box to check.
 * @param pId Message id, without the leading '@'.
 * @return Whether the text id of the text box is '@' followed by pId.
 */
inline bool IsTextBoxOfId(const TextBoxEx* pTextBox, const char* pId) {
    const char* textId = pTextBox->GetTextId();
    return textId != nullptr && textId[0] == '@' && IsSameName(pId, textId + 1, 24);
}

/**
 * @brief Allocate and construct a pane through the layout allocator.
 * @param args Arguments forwarded to the constructor of T.
 * @return Constructed pane, or nullptr when allocation fails.
 */
template <typename T, typename... Args>
T* NewPane(Args&&... args) {
    void* memory = nn::ui2d::Layout::AllocateMemory((sizeof(T) + 15) & ~size_t(15), 16);

    if (memory == nullptr) {
        return nullptr;
    }

    return new (memory) T(static_cast<Args&&>(args)...);
}

/**
 * @brief Create an animator registered in a layout.
 * @param pLayout Layout that receives the animator.
 * @param pResource Animation data to bind, or nullptr.
 * @return Created animator, or nullptr when there is no data or memory.
 */
inline Animator* CreateAnimator(LayoutEx* pLayout, const nn::ui2d::ResAnimationBlock* pResource) {
    if (pResource == nullptr) {
        return nullptr;
    }

    nn::gfx::Device* device = GetDevice();
    void* memory = nn::ui2d::Layout::AllocateMemory(sizeof(Animator));

    if (memory == nullptr) {
        return nullptr;
    }

    auto* animator = new (memory) Animator;
    pLayout->mAnimTransformList.LinkPrev(&animator->m_Link);
    animator->SetResource(device, pLayout->mResourceAccessor, pResource);
    return animator;
}

/**
 * @brief Get the layout of a parts pane.
 * @param rParts Parts pane.
 * @return Layout built for the parts pane.
 */
inline LayoutEx* GetPartsLayout(const nn::ui2d::Parts& rParts) {
    return static_cast<LayoutEx*>(rParts.m_pLayout);
}
}  // namespace

/**
 * @brief Create an empty layout.
 * @param pScreen Screen that owns this layout.
 */
LayoutEx::LayoutEx(Screen* pScreen)
    : mInAnimator(nullptr), mOutAnimator(nullptr), mLoopAnimator(nullptr), mWaitAnimator(nullptr),
      mScreen(pScreen), mParentLayout(nullptr), mFlags(0), mOpenState(cOpenState_Opened) {}

/**
 * @brief Create a copy of a layout with its own pane tree and groups.
 * @param rOther Layout to copy.
 * @param pRootName New name of the root pane, or nullptr to keep it.
 * @param pParentLayout Layout the copy belongs to, or nullptr to use the parent of rOther.
 * @param isCopied Flag remembered for the default animators of the copy.
 */
LayoutEx::LayoutEx(const LayoutEx& rOther, const char* pRootName, LayoutEx* pParentLayout,
                   bool isCopied)
    : mInAnimator(nullptr), mOutAnimator(nullptr), mLoopAnimator(nullptr), mWaitAnimator(nullptr),
      mScreen(rOther.mScreen),
      mParentLayout(pParentLayout != nullptr ? pParentLayout : rOther.mParentLayout),
      mFlags(rOther.mFlags), mOpenState(cOpenState_Opened) {
    if (isCopied) {
        mFlags |= cFlag_Copied;
    }

    mLayoutSize = rOther.mLayoutSize;
    _30 = rOther._30;
    mResourceAccessor = rOther.mResourceAccessor;
    mRootPane = ClonePaneTree(rOther.mRootPane, this, true);

    if (pRootName != nullptr) {
        mRootPane->SetName(pRootName);
    }

    _20 = NewObj<nn::ui2d::GroupContainer>();

    for (auto& group : rOther.getGroupContainer()->mGroups) {
        getGroupContainer()->AppendGroup(CloneGroup(group, mRootPane));
    }

    if (mParentLayout != nullptr) {
        auto* parts = eui::DynamicCast<PartsEx>(mRootPane);

        if (parts != nullptr) {
            mParentLayout->GetPartsList().push_back(*parts);
        }
    }

    doInitializeDefalutAnimator_(isCopied);
}

/**
 * @brief Create an animator bound to every group of an animation.
 * @param pName Animation name.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr.
 */
Animator* LayoutEx::createAnimatorAuto(const char* pName, bool enabled) {
    return tryCreateAnimatorAuto(pName, enabled);
}

/**
 * @brief Try to create an animator bound to every group of an animation.
 * @param pName Animation name.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr when the animation has no data or no group.
 */
Animator* LayoutEx::tryCreateAnimatorAuto(const char* pName, bool enabled) {
    const void* data = GetAnimResourceData(pName);

    if (data == nullptr) {
        return nullptr;
    }

    nn::ui2d::AnimResource resource;
    resource.Set(data);

    if (!resource.GetGroupCount()) {
        return nullptr;
    }

    Animator* animator = CreateAnimator(this, resource.mAnimation);
    animator->SetupWithGroupAll(resource, this, getGroupContainer(), enabled);
    return animator;
}

/**
 * @brief Create a set of animators of which only one plays at a time.
 * @param pNames Animation names; null or empty entries leave their slot empty.
 * @param count Number of names.
 * @param enabled Whether the first animator starts enabled.
 * @return Created animator set, or nullptr.
 */
AnimatorSet* LayoutEx::createAnimatorSet(const char* const* pNames, u32 count, bool enabled) {
    void* memory = AllocateMemory(sizeof(AnimatorSet) + sizeof(Animator*) * size_t(count));

    if (memory == nullptr) {
        return nullptr;
    }

    auto* set = new (memory) AnimatorSet;
    set->setBuffer(count, reinterpret_cast<Animator**>(set + 1));

    for (size_t i = 0; i != count; ++i) {
        if (pNames[i] != nullptr && *pNames[i]) {
            set->setAnimator(i, tryCreateAnimatorAuto(pNames[i], enabled && i == 0));
        }
    }

    return set;
}

/**
 * @brief Create an animator bound to a pane.
 * @param pName Animation name.
 * @param pPane Pane to animate.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator.
 */
Animator* LayoutEx::createAnimatorWithPane(const char* pName, nn::ui2d::Pane* pPane, bool enabled) {
    nn::ui2d::AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    Animator* animator = CreateAnimator(this, resource.mAnimation);
    animator->SetupWithPane(resource, this, pPane, enabled);
    return animator;
}

/**
 * @brief Find animation data in the archive of this layout.
 * @param pName Animation name.
 * @return Animation data, or nullptr.
 */
const void* LayoutEx::GetAnimResourceData(const char* pName) {
    auto* accessor = eui::DynamicCast<MultiArcResourceAccessor>(mResourceAccessor);

    if (accessor != nullptr) {
        return accessor->findAnimationResource(getLayoutName(), pName, nullptr);
    }

    return nn::ui2d::Layout::GetAnimResourceData(pName);
}

/**
 * @brief Create an animator bound to a group.
 * @param pName Animation name.
 * @param pGroup Group to animate.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator.
 */
Animator* LayoutEx::createAnimatorWithGroup(const char* pName, nn::ui2d::Group* pGroup,
                                            bool enabled) {
    nn::ui2d::AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    Animator* animator = CreateAnimator(this, resource.mAnimation);
    animator->SetupWithGroup(resource, this, pGroup, enabled);
    return animator;
}

/**
 * @brief Create an animator bound to one group of an animation.
 * @param pName Animation name.
 * @param index Index of the group in the animation.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr.
 */
Animator* LayoutEx::createAnimatorWithGroupIndex(const char* pName, u32 index, bool enabled) {
    return tryCreateAnimatorWithGroupIndex(pName, index, enabled);
}

/**
 * @brief Try to create an animator bound to one group of an animation.
 * @param pName Animation name.
 * @param index Index of the group in the animation.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr when there is no data or index is out of range.
 */
Animator* LayoutEx::tryCreateAnimatorWithGroupIndex(const char* pName, u32 index, bool enabled) {
    const void* data = GetAnimResourceData(pName);

    if (data == nullptr) {
        return nullptr;
    }

    nn::ui2d::AnimResource resource;
    resource.Set(data);

    if (index >= resource.GetGroupCount()) {
        return nullptr;
    }

    Animator* animator = CreateAnimator(this, resource.mAnimation);
    animator->SetupWithGroupIndex(resource, this, getGroupContainer(), index, enabled);
    return animator;
}

/**
 * @brief Create an animator that is not bound to any pane.
 * @param pName Animation name.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator.
 */
Animator* LayoutEx::createUnbindedAnimator(const char* pName, bool enabled) {
    nn::ui2d::AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    Animator* animator = CreateAnimator(this, resource.mAnimation);
    animator->SetupBasic(resource, this, enabled);
    return animator;
}

/**
 * @brief Create an unbound animator with room for several bindings of each animation content.
 * @param pName Animation name.
 * @param animNum Number of bindings reserved for each animation content.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr when there is no data.
 */
Animator* LayoutEx::createUnbindedAnimatorWithAnimNumMultiple(const char* pName, int animNum,
                                                              bool enabled) {
    nn::ui2d::AnimResource resource;
    resource.Set(GetAnimResourceData(pName));
    const nn::ui2d::ResAnimationBlock* block = resource.mAnimation;

    if (block == nullptr) {
        return nullptr;
    }

    auto* animator = static_cast<Animator*>(AllocateMemory(sizeof(Animator)));

    if (animator != nullptr) {
        new (animator) Animator;
        mAnimTransformList.LinkPrev(&animator->m_Link);
        animator->SetResource(GetDevice(), mResourceAccessor, block,
                              resource.mAnimation->contentCount * animNum);
    }

    animator->SetupBasic(resource, this, enabled);
    return animator;
}

/**
 * @brief Try to create an animator bound to every group of an animation.
 * @param pName Animation name.
 * @param enabled Whether the animator starts enabled.
 * @return Created animator, or nullptr.
 */
Animator* LayoutEx::tryCreateAnimatorAutoWithWarning(const char* pName, bool enabled) {
    return tryCreateAnimatorAuto(pName, enabled);
}

/**
 * @brief Set a message to every text box addressed by a message id.
 * @param pId Message id.
 * @param rText Message to set.
 * @param isAdjust Whether pane sizes are adjusted to the new text when needed.
 * @param pUserData User data passed to the text boxes.
 * @return Number of text boxes that received the message.
 */
int LayoutEx::setMessageStringForEachId(const char* pId, const MessageString& rText,
                                        bool isAdjust, void* pUserData) {
    bool isAdjustNeeded = false;
    int count = setMessageStringForEachIdRecursive_(mRootPane, pId, rText, nullptr, -1,
                                                    &isAdjustNeeded, pUserData);

    if (count != 0 && isAdjust && isAdjustNeeded) {
        adjustPaneSizeToTextSizeRecursive_(mRootPane);
    }

    return count;
}

/**
 * @brief Set a message to the text boxes addressed by a message id in a pane tree.
 * @param pPane Root of the pane tree; parts panes are not entered.
 * @param pId Message id.
 * @param rText Message to set.
 * @param pHasNext Receives whether a page follows, when page is not negative.
 * @param page Page to set, or a negative value to set the whole message.
 * @param pIsAdjustNeeded Set when a text box asks its pane size to be adjusted.
 * @param pUserData User data passed to the text boxes.
 * @return Number of text boxes that received the message.
 */
int LayoutEx::setMessageStringForEachIdRecursive_(nn::ui2d::Pane* pPane, const char* pId,
                                                  const MessageString& rText, bool* pHasNext,
                                                  int page, bool* pIsAdjustNeeded,
                                                  void* pUserData) {
    int count = 0;
    auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

    if (textBox != nullptr && IsTextBoxOfId(textBox, pId)) {
        if (page < 0) {
            textBox->setMessageString(rText, pUserData);
        } else {
            textBox->setMessageStringWithPage(rText, pHasNext, page, true, pUserData);
        }

        if (textBox->IsLocationAdjust()) {
            *pIsAdjustNeeded = true;
        }

        if (mScreen != nullptr && (mScreen->mFlags & 0x20) != 0) {
            for (nn::ui2d::Pane* parent = pPane->GetParent(); parent != nullptr;
                 parent = parent->GetParent()) {
                auto* alignPane = eui::DynamicCast<AlignPane>(parent);

                if (alignPane != nullptr) {
                    alignPane->mDirty = true;
                    break;
                }
            }
        }

        count = 1;
    }

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        nn::ui2d::Pane* child = nn::ui2d::Pane::FromLink(link);

        if (eui::DynamicCast<nn::ui2d::Parts>(child) == nullptr) {
            count += setMessageStringForEachIdRecursive_(child, pId, rText, pHasNext, page,
                                                         pIsAdjustNeeded, pUserData);
        }
    }

    return count;
}

/**
 * @brief Adjust the size of the panes of a pane tree to their text.
 * @param pPane Root of the pane tree; parts panes are not entered.
 */
void LayoutEx::adjustPaneSizeToTextSizeRecursive_(nn::ui2d::Pane* pPane) {
    AdjustPaneSizeToTextSize(pPane, this);

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        nn::ui2d::Pane* child = nn::ui2d::Pane::FromLink(link);

        if (eui::DynamicCast<nn::ui2d::Parts>(child) == nullptr) {
            adjustPaneSizeToTextSizeRecursive_(child);
        }
    }
}

/**
 * @brief Set one page of a message to every text box addressed by a message id.
 * @param pId Message id.
 * @param rText Message to set.
 * @param pHasNext Receives whether a page follows.
 * @param page Page to set.
 * @param isAdjust Whether pane sizes are adjusted to the new text when needed.
 * @param pUserData User data passed to the text boxes.
 * @return Number of text boxes that received the message.
 */
int LayoutEx::setMessageStringForEachIdWithPage(const char* pId, const MessageString& rText,
                                                bool* pHasNext, u32 page, bool isAdjust,
                                                void* pUserData) {
    bool isAdjustNeeded = false;
    int count = setMessageStringForEachIdRecursive_(mRootPane, pId, rText, pHasNext, page,
                                                    &isAdjustNeeded, pUserData);

    if (count != 0 && isAdjust && isAdjustNeeded) {
        adjustPaneSizeToTextSizeRecursive_(mRootPane);
    }

    return count;
}

/**
 * @brief Find the text box addressed by a message id.
 * @param pId Message id.
 * @return Text box, or nullptr.
 */
TextBoxEx* LayoutEx::findTextBoxById(const char* pId) {
    return findTextBoxByIdRecursive_(mRootPane, pId);
}

/**
 * @brief Find the text box addressed by a message id in a pane tree.
 * @param pPane Root of the pane tree; parts panes are not entered.
 * @param pId Message id.
 * @return Text box, or nullptr.
 */
TextBoxEx* LayoutEx::findTextBoxByIdRecursive_(nn::ui2d::Pane* pPane, const char* pId) {
    auto* textBox = nn::ui2d::DynamicCast<TextBoxEx*>(pPane);

    if (textBox != nullptr && IsTextBoxOfId(textBox, pId)) {
        return textBox;
    }

    for (auto* link = pPane->m_Children.GetNext(); link != &pPane->m_Children;
         link = link->GetNext()) {
        nn::ui2d::Pane* child = nn::ui2d::Pane::FromLink(link);

        if (eui::DynamicCast<nn::ui2d::Parts>(child) == nullptr) {
            TextBoxEx* found = findTextBoxByIdRecursive_(child, pId);

            if (found != nullptr) {
                return found;
            }
        }
    }

    return nullptr;
}

/**
 * @brief Find the layout of a parts pane.
 * @param pName Name of the parts pane.
 * @return Parts layout, or nullptr.
 */
LayoutEx* LayoutEx::findPartsLayout(const char* pName) {
    nn::ui2d::Parts* parts = FindPartsPaneByName(pName);
    return parts != nullptr ? GetPartsLayout(*parts) : nullptr;
}

/**
 * @brief Find an animator of this layout.
 * @param pName Animation name.
 * @return Animator, or nullptr.
 */
Animator* LayoutEx::findAnimator(const char* pName) {
    for (auto& transform : GetAnimTransformList()) {
        auto* animator = eui::DynamicCast<Animator>(&transform);

        if (animator != nullptr && IsSameName(animator->getName(), pName, 64)) {
            return animator;
        }
    }

    return nullptr;
}

/** @brief Mark the global matrix of the root pane dirty. */
void LayoutEx::setRootPaneGlobalMtxDirty() {
    mRootPane->SetGlobalMatrixDirty();
}

/**
 * @brief Start the open animation and the loop animations.
 * @param isPlayParts Whether the parts layouts play their open animation too.
 * @param anim How the open animation is started.
 * @param isSelf Whether this layout plays its open animation.
 */
void LayoutEx::startAnimOpenImpl_(bool isPlayParts, OpenAnim anim, bool isSelf) {
    bool isReverse = false;

    if (mInAnimator != nullptr && mOutAnimator == nullptr) {
        isReverse = mInAnimator->getStep() < 0.0f;
    }

    if (isPlayParts || isSelf) {
        if (mInAnimator != nullptr) {
            if (mOutAnimator != nullptr) {
                mOutAnimator->disableKeepActive();
            }

            switch (anim) {
            case cOpenAnim_End:
                mInAnimator->StopAtMax();
                mOpenState = cOpenState_Opened;
                break;
            case cOpenAnim_Start:
                mInAnimator->StopAtMin();
                mOpenState = cOpenState_Closed;
                break;
            default:
                if (isReverse) {
                    mInAnimator->PlayFromCurrent(Animator::cPlayType_OneTime, 1.0f);
                } else {
                    mInAnimator->PlayAuto(1.0f);
                }

                invokeSoundLink2Event_(cSoundLink2Event_Open);
                mOpenState = cOpenState_Opening;
                break;
            }
        } else if (mOutAnimator != nullptr) {
            mOutAnimator->StopAtMin();
            mOpenState = cOpenState_Opened;
        }
    }

    if (!isReverse && (mInAnimator == nullptr || isSelf || isPlayParts)) {
        if (mLoopAnimator != nullptr) {
            if (isLoopRandom()) {
                mLoopAnimator->PlayRandom(Animator::cPlayType_Loop, 1.0f);
            } else {
                mLoopAnimator->PlayAuto(1.0f);
            }
        }

        if (mWaitAnimator != nullptr && mOpenState == cOpenState_Opened) {
            mWaitAnimator->PlayAuto(1.0f);
        }
    }

    for (auto& parts : GetPartsList()) {
        GetPartsLayout(parts)->startAnimOpenImpl_(isPlayParts, anim, false);
    }
}

/**
 * @brief Raise the sound link event of the open or close animation.
 * @param event Event to raise.
 */
void LayoutEx::invokeSoundLink2Event_(SoundLink2Event event) {
    if (mScreen == nullptr || mScreen->mSoundLinkUser == nullptr || mParentLayout == nullptr ||
        mParentLayout->mRootPane->GetParent() != nullptr) {
        return;
    }

    const char* suffix;

    switch (event) {
    case cSoundLink2Event_Open:
        suffix = "_open";
        break;
    case cSoundLink2Event_Close:
        suffix = "_close";
        break;
    default:
        suffix = nullptr;
        break;
    }

    if (suffix == nullptr) {
        return;
    }

    const char* rootName = mRootPane->GetName();
    sead::FixedStringBuilder<64> name;
    name.copy(rootName);
    name.append(suffix, -1);
    mScreen->invokeSoundLink2Event_(name.cstr());
}

/**
 * @brief Start the close animation.
 * @param isPlayParts Whether the parts layouts play their close animation too.
 * @param isSkip Whether the animation jumps to the closed state.
 */
void LayoutEx::startAnimCloseImpl_(bool isPlayParts, bool isSkip) {
    if (mOutAnimator != nullptr) {
        if (mInAnimator != nullptr) {
            mInAnimator->disableKeepActive();
        }

        if (isSkip) {
            mOutAnimator->StopAtMax();
            mOpenState = cOpenState_Closed;
        } else {
            mOutAnimator->PlayAuto(1.0f);
            invokeSoundLink2Event_(cSoundLink2Event_Close);
            mOpenState = cOpenState_Closing;
        }
    } else if (mInAnimator != nullptr) {
        if (isSkip) {
            mInAnimator->StopAtMin();
            mOpenState = cOpenState_Closed;
        } else {
            if (mInAnimator->getStep() > 0.0f) {
                mInAnimator->PlayFromCurrent(Animator::cPlayType_OneTime, -1.0f);
            } else {
                mInAnimator->PlayAuto(-1.0f);
            }

            invokeSoundLink2Event_(cSoundLink2Event_Close);
            mOpenState = cOpenState_Closing;
        }
    } else {
        disableLoopAnimator_(mRootPane->GetParent() != nullptr && !isPlayParts);
    }

    if (isPlayParts) {
        for (auto& parts : GetPartsList()) {
            GetPartsLayout(parts)->startAnimCloseImpl_(true, isSkip);
        }
    }
}

/**
 * @brief Stop the loop animations.
 * @param isRecursive Whether the loop animations of the parts layouts are stopped too.
 */
void LayoutEx::disableLoopAnimator_(bool isRecursive) {
    if (mLoopAnimator != nullptr) {
        mLoopAnimator->disableKeepActive();
    }

    if (mWaitAnimator != nullptr) {
        mWaitAnimator->StopAtMin();
    }

    if (isRecursive) {
        for (auto& parts : GetPartsList()) {
            GetPartsLayout(parts)->disableLoopAnimator_(true);
        }
    }
}

/**
 * @brief Check whether the open animation reached its end.
 * @param isRecursive Whether the parts layouts are checked too.
 * @return Whether the open animation ended.
 */
bool LayoutEx::isAnimOpenEnd(bool isRecursive) const {
    if (mInAnimator != nullptr && !mInAnimator->isFrameMax()) {
        return false;
    }

    if (isRecursive) {
        for (const auto& parts : GetPartsList()) {
            if (!GetPartsLayout(parts)->isAnimOpenEnd(true)) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Check whether the close animation reached its end.
 * @param isRecursive Whether the parts layouts are checked too.
 * @return Whether the close animation ended.
 */
bool LayoutEx::isAnimCloseEnd(bool isRecursive) const {
    if (mOutAnimator != nullptr) {
        if (!mOutAnimator->isFrameMax() || mOutAnimator->mEnabled) {
            return false;
        }
    } else if (mInAnimator != nullptr) {
        if (mInAnimator->getFrame() != 0.0f || mInAnimator->mEnabled) {
            return false;
        }
    }

    if (isRecursive) {
        for (const auto& parts : GetPartsList()) {
            if (!GetPartsLayout(parts)->isAnimCloseEnd(true)) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Select the draw target of the animations; this layout ignores it.
 * @param target Draw target.
 */
void LayoutEx::setDrawTargetAnim(DrawTarget target) {}

/**
 * @brief Update the open state when the open or close animation stops.
 * @param pAnimator Animator that stopped.
 */
void LayoutEx::animatorDisableCallback(Animator* pAnimator) {
    bool isClosed = false;

    if (mOutAnimator == pAnimator && pAnimator->isFrameMax()) {
        isClosed = true;
    } else if (mInAnimator == pAnimator) {
        if (pAnimator->getFrame() == 0.0f) {
            isClosed = true;
        } else if (pAnimator->isFrameMax()) {
            if (mOpenState == cOpenState_Opening) {
                mOpenState = cOpenState_Opened;
            }

            if (mWaitAnimator != nullptr && mOpenState == cOpenState_Opened) {
                mWaitAnimator->PlayAuto(1.0f);
            }
        }
    }

    if (isClosed) {
        if (mOpenState == cOpenState_Closing) {
            mOpenState = cOpenState_Closed;
        }

        if (mOpenState == cOpenState_Closed) {
            disableLoopAnimator_(mRootPane->GetParent() != nullptr);
        }
    }
}

/** @brief Adjust the size of the panes to their text when enabled for this layout. */
void LayoutEx::adjustPaneSizeToTextSize() {
    if ((mFlags & cFlag_AdjustPaneSizeToTextSize) != 0) {
        adjustPaneSizeToTextSizeRecursive_(mRootPane);
    }
}

/**
 * @brief Calculate the pane tree.
 * @param rDrawInfo Drawing state temporarily associated with this layout.
 * @param forceDirty Whether pane calculations must refresh global matrices.
 */
void LayoutEx::CalculateImpl(nn::ui2d::DrawInfo& rDrawInfo, bool forceDirty) {
    if (mRootPane != nullptr) {
        CalculatePaneContext context(rDrawInfo, this);
        rDrawInfo.m_pLayoutInformation =
            reinterpret_cast<const nn::ui2d::Pane::CalculateContext::LayoutInformation*>(this);
        mRootPane->Calculate(rDrawInfo, context, forceDirty);
        rDrawInfo.m_pLayoutInformation = nullptr;
    }
}

/**
 * @brief Build the layout and create its default animators.
 * @param pResult Receives the build statistics.
 * @param pDevice Graphics device.
 * @param pData Layout resource.
 * @param pAccessor Resource accessor.
 * @param rArgs Build context.
 * @param pPartsBuildDataSet Parts pane overrides, or nullptr.
 * @return Whether the build succeeded.
 */
bool LayoutEx::BuildImpl(nn::ui2d::BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                         const void* pData, nn::ui2d::ResourceAccessor* pAccessor,
                         const nn::ui2d::BuildArgSet& rArgs,
                         const PartsBuildDataSet* pPartsBuildDataSet) {
    bool isSucceeded =
        Layout::BuildImpl(pResult, pDevice, pData, pAccessor, rArgs, pPartsBuildDataSet);

    if (isSucceeded) {
        doInitializeDefalutAnimator_((mFlags & cFlag_Copied) != 0);
    }

    return isSucceeded;
}

/**
 * @brief Build a pane, replacing a localized pane of the current region when requested.
 * @param pResult Receives the build statistics.
 * @param pDevice Graphics device.
 * @param kind Signature of the pane block.
 * @param pBlock Pane block.
 * @param pOverride Override pane block, or nullptr.
 * @param rArgs Build context.
 * @return Built pane, or nullptr when the pane is not used in the current region.
 */
nn::ui2d::Pane* LayoutEx::BuildPaneObj(nn::ui2d::BuildResultInformation* pResult,
                                       nn::gfx::Device* pDevice, u32 kind, const void* pBlock,
                                       const void* pOverride, const nn::ui2d::BuildArgSet& rArgs) {
    const nn::ui2d::ResExtUserData* replaceOn = nullptr;
    int materialCount = 0;
    MaterialNameArray materialNames;
    const nn::ui2d::ResExtUserData* replaceTarget =
        FindExtUserDataFromList(rArgs.pExtUserDataList, "LocalizeReplaceTarget");

    if (replaceTarget != nullptr) {
        sead::RegionID region;
        sead::RegionLanguageID regionLanguage;

        if (sead::EnvUtil::getRegionFromString(&region, replaceTarget->GetString())) {
            if (static_cast<int>(region) != sead::EnvUtil::getRegion()) {
                return nullptr;
            }
        } else if (sead::EnvUtil::getRegionLanguageFromString(&regionLanguage,
                                                               replaceTarget->GetString())) {
            if (static_cast<int>(regionLanguage) != sead::EnvUtil::getRegionLanguage()) {
                return nullptr;
            }
        } else {
            return nullptr;
        }

        replaceOn = FindExtUserDataFromList(rArgs.pExtUserDataList, "LocalizeReplaceOn");

        if (rArgs.pParentPane != nullptr) {
            nn::ui2d::Pane* replaced =
                rArgs.pParentPane->FindPaneByName(replaceOn->GetString(), true);

            if (replaced != nullptr) {
                bool isTexturePane = eui::DynamicCast<PictureEx>(replaced) != nullptr ||
                                     eui::DynamicCast<WindowEx>(replaced) != nullptr;
                materialCount = static_cast<u8>(replaced->GetMaterialCount());

                for (int i = 0; i < materialCount; ++i) {
                    materialNames[i] = replaced->GetMaterial(i)->GetName();
                }

                replaced->Finalize(pDevice);
                rArgs.pParentPane->RemoveChild(replaced);
                delete replaced;

                auto* frameHeap = sead::DynamicCast<sead::FrameHeap>(GetNwAllocatorHeap());
                bool isViewer = mScreen != nullptr && mScreen->getViewerType() != 0;

                if (!(isTexturePane || isViewer || frameHeap == nullptr)) {
                    sead::FrameHeap::State state = frameHeap->getState();

                    if (frameHeap->getDirection() == sead::Heap::cHeapDirection_Forward) {
                        state.mHeadPtr = replaced;
                    } else {
                        state.mTailPtr = replaced;
                    }

                    frameHeap->restoreState(state);
                }
            }
        }
    }

    nn::ui2d::Pane* pane = buildPaneObjImpl_(pResult, kind, pBlock, pOverride, rArgs);

    if (replaceOn != nullptr) {
        pane->SetName(replaceOn->GetString());

        for (int i = 0; i < materialCount; ++i) {
            nn::ui2d::Material* material = pane->GetMaterial(i);
            material->mName = materialNames[i];
        }
    }

    afterBuildPane_(pane, rArgs);

    if (mScreen != nullptr) {
        mScreen->afterBuildPaneCallback(pane, this, rArgs);
    }

    return pane;
}

/**
 * @brief Create the eui pane class matching a pane block.
 * @param pResult Receives the build statistics.
 * @param kind Signature of the pane block.
 * @param pBlock Pane block.
 * @param pOverride Override pane block, or nullptr.
 * @param rArgs Build context.
 * @return Created pane, or nullptr for an unknown kind.
 */
nn::ui2d::Pane* LayoutEx::buildPaneObjImpl_(nn::ui2d::BuildResultInformation* pResult, u32 kind,
                                            const void* pBlock, const void* pOverride,
                                            const nn::ui2d::BuildArgSet& rArgs) {
    switch (kind) {
    case cPaneBlockKind_Bounding:
        return NewPane<BoundingEx>(static_cast<const nn::ui2d::ResBounding*>(pBlock),
                                   static_cast<const nn::ui2d::ResBounding*>(pOverride), rArgs);
    case cPaneBlockKind_Scissor:
        return NewPane<ScissorPane>(static_cast<const nn::ui2d::ResPane*>(pBlock), rArgs);
    case cPaneBlockKind_Alignment:
        return NewPane<AlignPane>(static_cast<const nn::ui2d::ResAlignment*>(pBlock), rArgs);
    case cPaneBlockKind_TextBox: {
        auto* resource = static_cast<const nn::ui2d::ResTextBox*>(pBlock);
        auto* override = static_cast<const nn::ui2d::ResTextBox*>(pOverride);
        nn::ui2d::TextBox::InitializeStringParam param;
        TextBoxEx* textBox;

        if (isScalableFontTextBox_(resource, override, rArgs)) {
            textBox = NewPane<ScalableFontTextBoxEx>(resource, override, rArgs, &param);
        } else {
            textBox = NewPane<TextBoxEx>(resource, override, rArgs, &param);
        }

        textBox->InitializeString(pResult, GetDevice(), rArgs, param);
        return textBox;
    }
    case cPaneBlockKind_Picture: {
        auto* resource = static_cast<const nn::ui2d::ResPicture*>(pBlock);
        auto* override = static_cast<const nn::ui2d::ResPicture*>(pOverride);

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "FrameBufferUse") != nullptr) {
            return NewPane<MultiFilterPictureEx>(resource, override, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "DynamicCaptureUseName") != nullptr) {
            return NewPane<DynamicCaptureUsePictureEx>(resource, override, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "MassDrawOn") != nullptr) {
            return NewPane<MassDrawPane>(resource, override, rArgs);
        }

        return NewPane<PictureEx>(resource, override, rArgs);
    }
    case cPaneBlockKind_Pane: {
        auto* resource = static_cast<const nn::ui2d::ResPane*>(pBlock);

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "CaptureOn") != nullptr) {
            return NewPane<CapturePane>(resource, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "DynamicCaptureOn") != nullptr) {
            return NewPane<DynamicCapturePane>(resource, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "ScissorOn") != nullptr) {
            return NewPane<ScissorPane>(resource, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "AlignOn") != nullptr) {
            return NewPane<AlignPane>(resource, rArgs);
        }

        if (rArgs.pParentPane != nullptr) {
            return NewPane<nn::ui2d::Pane>(resource, rArgs);
        }

        return NewPane<RootPane>(resource, rArgs);
    }
    case cPaneBlockKind_Window: {
        auto* resource = static_cast<const nn::ui2d::ResWindow*>(pBlock);
        auto* override = static_cast<const nn::ui2d::ResWindow*>(pOverride);

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "FrameBufferUse") != nullptr) {
            return NewPane<MultiFilterWindowEx>(resource, override, rArgs);
        }

        if (FindExtUserDataFromList(rArgs.pExtUserDataList, "DynamicCaptureUseName") != nullptr) {
            return NewPane<DynamicCaptureUseWindowEx>(resource, override, rArgs);
        }

        return NewPane<WindowEx>(resource, override, rArgs);
    }
    case cPaneBlockKind_Parts:
        return NewPane<PartsEx>(static_cast<const nn::ui2d::ResParts*>(pBlock),
                                static_cast<const nn::ui2d::ResParts*>(pOverride), rArgs);
    default:
        return nullptr;
    }
}

/**
 * @brief Check whether a text box uses a scalable font.
 * @param pResource Text box block.
 * @param pOverride Override text box block, or nullptr.
 * @param rArgs Build context.
 * @return Whether the font of the text box is a scalable font.
 */
bool LayoutEx::isScalableFontTextBox_(const nn::ui2d::ResTextBox* pResource,
                                      const nn::ui2d::ResTextBox* pOverride,
                                      const nn::ui2d::BuildArgSet& rArgs) {
    auto* textSearcher = static_cast<const TextSearcher*>(rArgs.pTextSearcher);

    if (textSearcher->mProcessor->mFontMgr->mScalableFontMgr == nullptr) {
        return false;
    }

    const nn::ui2d::BuildResSet* resSet;

    if (pOverride != nullptr) {
        bool isOverride = (static_cast<u8>(rArgs.overridePaneUsageFlag) & 1) == 0;
        resSet = isOverride ? rArgs.pOverrideBuildResSet : rArgs.pCurrentBuildResSet;
        pResource = isOverride ? pOverride : pResource;
    } else {
        resSet = rArgs.pCurrentBuildResSet;
    }

    const char* fontName = resSet->pFontList->GetFontName(pResource->fontIdx);
    nn::font::Font* font = mResourceAccessor->AcquireFont(GetDevice(), fontName);
    return font != nullptr && nn::ui2d::IsDerivedFrom<nn::font::ScalableFont>(font);
}

/**
 * @brief Hook called after a pane is built; this layout does nothing.
 * @param pPane Built pane.
 * @param rArgs Build context.
 */
void LayoutEx::afterBuildPane_(nn::ui2d::Pane* pPane, const nn::ui2d::BuildArgSet& rArgs) {}

/**
 * @brief Build the layout of a parts pane.
 * @param pResult Receives the build statistics.
 * @param pDevice Graphics device.
 * @param pName Name of the parts layout.
 * @param rPartsBuildDataSet Parts pane overrides.
 * @param rArgs Build context.
 * @return Built parts layout.
 */
nn::ui2d::Layout* LayoutEx::BuildPartsLayout(nn::ui2d::BuildResultInformation* pResult,
                                             nn::gfx::Device* pDevice, const char* pName,
                                             const PartsBuildDataSet& rPartsBuildDataSet,
                                             const nn::ui2d::BuildArgSet& rArgs) {
    auto* layout = static_cast<LayoutEx*>(rArgs.m_pPartsLayout);
    Screen* screen = layout->getScreen();

    if (screen != nullptr) {
        pName = screen->replacePartsLayoutName(
            pName, static_cast<PartsEx*>(rPartsBuildDataSet.m_pPartsPane), layout);
    }

    attachPartsLayoutArchive_(pName);
    const void* data = GetLayoutResourceData(pName);
    LayoutEx* partsLayout = doCreatePartsLayout_(pName, rPartsBuildDataSet, rArgs);
    partsLayout->mParentLayout = this;
    partsLayout->BuildImpl(pResult, pDevice, data, mResourceAccessor, rArgs, &rPartsBuildDataSet);
    return partsLayout;
}

/**
 * @brief Create the layout object of a parts pane.
 * @param pName Name of the parts layout.
 * @param rPartsBuildDataSet Parts pane overrides.
 * @param rArgs Build context.
 * @return Created layout.
 */
LayoutEx* LayoutEx::doCreatePartsLayout_(const char* pName,
                                         const PartsBuildDataSet& rPartsBuildDataSet,
                                         const nn::ui2d::BuildArgSet& rArgs) {
    return NewObj<LayoutEx>(getScreen());
}

/**
 * @brief Attach the archive of a parts layout to the resource accessor.
 * @param rName Name of the parts layout archive.
 * @return Whether the archive is available.
 */
bool LayoutEx::attachPartsLayoutArchive_(const sead::SafeString& rName) {
    auto* accessor = eui::DynamicCast<MultiArcResourceAccessor>(mResourceAccessor);

    if (accessor == nullptr) {
        return true;
    }

    ArcResourceMgr::ArcResource* archive = accessor->mArchives->findArcResource(rName);

    if (archive == nullptr) {
        return false;
    }

    if (!accessor->isArchiveAttached(archive->mData)) {
        accessor->attachArchive(archive->mData, archive->mTextureFile);
    }

    return true;
}

/**
 * @brief Create the In, Out, Loop and region animators found in the archive.
 * @param isCopied Whether the layout is a copy.
 */
void LayoutEx::doInitializeDefalutAnimator_(bool isCopied) {
    if (GetAnimResourceData("In") != nullptr) {
        mInAnimator = tryCreateAnimatorAuto("In", mScreen != nullptr);

        if (mInAnimator != nullptr) {
            mInAnimator->setSoundLink(false);
            mOpenState = cOpenState_Closed;
        }
    }

    if (GetAnimResourceData("Out") != nullptr) {
        mOutAnimator = tryCreateAnimatorAuto("Out", false);

        if (mOutAnimator != nullptr) {
            mOutAnimator->setSoundLink(false);
        }
    }

    if (GetAnimResourceData("LoopRandom") != nullptr) {
        mLoopAnimator = tryCreateAnimatorAuto("LoopRandom", false);
        mFlags |= cFlag_LoopRandom;
    } else if (GetAnimResourceData("Loop") != nullptr) {
        mLoopAnimator = tryCreateAnimatorAuto("Loop", false);
    }

    if (mLoopAnimator != nullptr) {
        mLoopAnimator->setSoundLink(false);
    }

    if (GetAnimResourceData("RegionType") != nullptr) {
        Animator* animator = tryCreateAnimatorAuto("RegionType", false);

        if (animator != nullptr) {
            animator->setSoundLink(false);

            if (mScreen != nullptr) {
                animator->Stop(sead::EnvUtil::getRegion().getRelativeIndex());
                animator->Animate();
            }
        }
    } else if (GetAnimResourceData("RegionLanguageType") != nullptr) {
        Animator* animator = tryCreateAnimatorAuto("RegionLanguageType", false);

        if (animator != nullptr) {
            animator->setSoundLink(false);

            if (mScreen != nullptr) {
                animator->Stop(sead::EnvUtil::getRegionLanguage().getRelativeIndex());
                animator->Animate();
            }
        }
    }
}

/**
 * @brief Open the pane tree node of this layout in HostIO; unused in release builds.
 * @param isOpen Whether the node is open.
 */
void LayoutEx::setOpenPaneTreeNodeInHostIO(bool isOpen) {}

/**
 * @brief Open the pane tree nodes of this layout and its parts in HostIO; unused in release builds.
 * @param isOpen Whether the nodes are open.
 */
void LayoutEx::setOpenPaneTreeNodeInHostIORecursive(bool isOpen) {}

/**
 * @brief Check whether the pane tree node of this layout is open in HostIO.
 * @return Always true in release builds.
 */
bool LayoutEx::isOpenPaneTreeNodeInHostIO() const {
    return true;
}

/**
 * @brief Prepare the layout for a pane tree built by code instead of a resource.
 * @param pName Layout name.
 * @param rSize Layout size.
 * @param pAccessor Resource accessor.
 */
void LayoutEx::beginBuildWithRapidPaneTree(const char* pName, const sead::Vector2f& rSize,
                                           nn::ui2d::ResourceAccessor* pAccessor) {
    _30 = const_cast<char*>(pName);
    _20 = NewObj<nn::ui2d::GroupContainer>();
    mLayoutSize = nn::util::MakeFloat2(rSize.x, rSize.y);
    mResourceAccessor = pAccessor;
}

/**
 * @brief Set the root of a pane tree built by code.
 * @param pPane Root pane.
 */
void LayoutEx::setRootPaneForRapidPaneTree(nn::ui2d::Pane* pPane) {
    mRootPane = pPane;
}

/**
 * @brief Create a parts pane at run time and build its layout.
 * @param pLayoutName Name of the parts layout.
 * @param pPaneName Name of the parts pane.
 * @param pRootLayout Root layout of the build.
 * @param rSize Size of the parts pane in units of cPartsPaneUnitSize.
 * @param pControlCreator Control creator of the build.
 * @param pTextSearcher Text searcher of the build.
 * @return Built parts layout.
 */
LayoutEx* LayoutEx::createPartsLayout(const char* pLayoutName, const char* pPaneName,
                                      LayoutEx* pRootLayout, const sead::Vector2f& rSize,
                                      ControlCreator* pControlCreator,
                                      TextSearcher* pTextSearcher) {
    nn::ui2d::BuildResultInformation result;
    result.SetDefault();
    auto* parts = NewPane<PartsEx>();
    parts->SetName(pPaneName);
    nn::ui2d::Size unitSize = {cPartsPaneUnitSize, cPartsPaneUnitSize};
    parts->SetSize(unitSize);

    nn::ui2d::ResParts resParts = {};
    resParts.magnify.v[0] = rSize.x;
    resParts.magnify.v[1] = rSize.y;
    nn::ui2d::ResVec2 originalSize = {cPartsPaneUnitSize, cPartsPaneUnitSize};
    nn::ui2d::BuildResSet resSet = {};
    resSet.pResAccessor = mResourceAccessor;
    resSet.pLayout = this;
    PartsBuildDataSet dataSet(parts, &resParts, &resSet, &originalSize);

    nn::ui2d::BuildArgSet args = {};
    args.pControlCreator = pControlCreator;
    args.pTextSearcher = pTextSearcher;
    args.m_pPartsLayout = this;
    args.m_pLayout = pRootLayout;
    nn::ui2d::Layout* layout = BuildPartsLayout(&result, GetDevice(), pLayoutName, dataSet, args);
    parts->m_pLayout = layout;

    nn::ui2d::Size size = {rSize.x * cPartsPaneUnitSize, rSize.y * cPartsPaneUnitSize};
    parts->SetSize(size);
    GetPartsList().push_back(*parts);
    return static_cast<LayoutEx*>(layout);
}

/** @brief Create a calculation context with default settings. */
LayoutEx::CalculatePaneContext::CalculatePaneContext() {
    clear_();
    SetDefault();
}

/**
 * @brief Create a calculation context for a layout.
 * @param rDrawInfo Drawing state.
 * @param pLayout Layout to calculate.
 */
LayoutEx::CalculatePaneContext::CalculatePaneContext(const nn::ui2d::DrawInfo& rDrawInfo,
                                                     const nn::ui2d::Layout* pLayout) {
    clear_();
    Set(rDrawInfo, pLayout);
}
}  // namespace eui
