#include "LuigiBros/Vessel/VesselAnimButton.hpp"
#include <agl/driver/aglGraphicsDriverMgr.h>
#include <cstring>
#include <new>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Pane.h>

namespace {

/**
 * @brief Get the graphics device that animators are created with.
 * @return The agl graphics device.
 */
inline nn::gfx::Device* getDevice() {
    return static_cast<nn::gfx::Device*>(
        agl::driver::GraphicsDriverMgr::instance()->getGfxDevice());
}

/**
 * @brief Check whether a functional animation name was given.
 * @param pName The name, possibly null.
 * @return Whether the name is non-null and non-empty.
 */
inline bool isValidName(const char* pName) {
    return pName != nullptr && *pName != '\0';
}

/**
 * @brief Check whether an animation is missing or has stopped playing.
 * @param pAnim The animator, possibly null.
 * @return Whether the animation is finished.
 */
inline bool isAnimFinished(const nn::ui2d::Animator* pAnim) {
    return pAnim == nullptr || pAnim->mSpeed == 0.0f;
}

/**
 * @brief Compare two control names over at most 64 characters.
 * @param pName The name looked for.
 * @param pCandidate The name of a button.
 * @return Whether the names are equal.
 */
inline bool isSameName(const char* pName, const char* pCandidate) {
    for (size_t i = 0; i < 64; ++i) {
        if (pName[i] != pCandidate[i]) {
            return false;
        }

        if (pName[i] == '\0') {
            return true;
        }
    }

    return true;
}

/**
 * @brief Allocate an object with the layout allocator and construct it.
 * @return The new object, or null if the allocation failed.
 */
template <typename T>
inline T* newLayoutObject() {
    void* pMemory = nn::ui2d::Layout::AllocateMemory(sizeof(T));

    if (pMemory == nullptr) {
        return nullptr;
    }

    return new (pMemory) T();
}

}  // namespace

/**
 * @brief Construct a button with no animators, an empty action queue and notification enabled.
 */
VesselAnimButton::VesselAnimButton() {
    mVesselFlags = 0;
}

/**
 * @brief Create the button's animators and find its hit pane.
 * @param rSrc The control source naming the functional animations and panes.
 * @param pLayout The layout that owns the button.
 */
void VesselAnimButton::Build(const nn::ui2d::ControlSrc& rSrc, nn::ui2d::Layout* pLayout) {
    bool isUpAnimEnable = true;
    const char* pName = rSrc.GetFunctionalAnimName(0);

    if (isValidName(pName)) {
        mDownAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, true);
        mDownAnim->StopAtStartFrame();
        isUpAnimEnable = false;
    }

    pName = rSrc.GetFunctionalAnimName(1);

    if (isValidName(pName)) {
        mUpAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, isUpAnimEnable);

        if (isUpAnimEnable) {
            mUpAnim->StopAtEndFrame();
        }
    }

    pName = rSrc.GetFunctionalAnimName(2);

    if (isValidName(pName)) {
        mSelectAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(3);

    if (isValidName(pName)) {
        mNonSelectAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(4);

    if (isValidName(pName)) {
        mDecideAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
        mDecideAnimName = pName;
    }

    pName = rSrc.GetFunctionalAnimName(5);

    if (isValidName(pName)) {
        mNonDecideAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(6);

    if (isValidName(pName)) {
        mDecideDPDAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
        mDecideDPDAnimName = pName;
    }

    pName = rSrc.GetFunctionalAnimName(7);

    if (isValidName(pName)) {
        mDecideKeyAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
        mDecideKeyAnimName = pName;
    }

    pName = rSrc.GetFunctionalAnimName(8);

    if (isValidName(pName)) {
        mEnterDPDAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(9);

    if (isValidName(pName)) {
        mLeaveDPDAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(10);

    if (isValidName(pName)) {
        mEnterKeyAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    pName = rSrc.GetFunctionalAnimName(11);

    if (isValidName(pName)) {
        mLeaveKeyAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
    }

    if (rSrc.mAnimCount > 12) {
        pName = rSrc.GetFunctionalAnimName(12);

        if (isValidName(pName)) {
            mEnterKeyToDownAnim = pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
        }

        if (rSrc.mAnimCount > 13) {
            pName = rSrc.GetFunctionalAnimName(13);

            if (isValidName(pName)) {
                mEnterKeyToEnterDPDAnim =
                    pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
            }

            if (rSrc.mAnimCount > 14) {
                pName = rSrc.GetFunctionalAnimName(14);

                if (isValidName(pName)) {
                    mEnterDPDToEnterKeyAnim =
                        pLayout->CreateGroupAnimatorAuto(getDevice(), pName, false);
                }
            }
        }
    }

    mHitPane = pLayout->GetRootPane()->FindPaneByName(rSrc.GetFunctionalPaneName(0), true);
    mName = GetControlName_(pLayout);
}

/**
 * @brief Set the callback notified about state changes and events.
 * @param callback The callback, or null.
 * @param pArg The argument passed to the callback.
 */
void VesselAnimButton::SetStateChangeCallback(StateChangeCallback callback, void* pArg) {
    mVesselCallback = callback;
    mVesselCallbackArg = pArg;
}

/**
 * @brief Queue a down action if the button can go down.
 */
void VesselAnimButton::Down() {
    if (mVesselFlags & cFlag_Down) {
        mVesselActions.PushWithOmit(cAction_Down);
    }
}

/**
 * @brief Queue an action, dropping everything queued after an existing copy of it.
 * @param action The action to queue.
 */
void VesselAnimButton::ActionQueue::PushWithOmit(Action action) {
    for (int i = 0; i < mCount; ++i) {
        if (mActions[i] == action) {
            mCount = i + 1;
            return;
        }
    }

    if (mCount < 2) {
        mActions[mCount] = action;
        ++mCount;
    }
}

/**
 * @brief Queue an up action if the button can go up.
 */
void VesselAnimButton::Up() {
    if (mVesselFlags & cFlag_Up) {
        mVesselActions.PushWithOmit(cAction_Up);
    }
}

/**
 * @brief Put the button in the down state without playing an animation.
 */
void VesselAnimButton::ForceDown() {
    ForceChangeStateVessel(cState_Down);
}

/**
 * @brief Put the button in the up state without playing an animation.
 */
void VesselAnimButton::ForceUp() {
    ForceChangeStateVessel(cState_Up);
}

/**
 * @brief Return the button to its initial state, if resetting is enabled.
 */
void VesselAnimButton::Reset() {
    if (mIsEnableReset) {
        if (mUpAnim != nullptr) {
            EnableAnim(mUpAnim);
            mUpAnim->StopAtEndFrame();
        }

        ForceChangeStateVessel(cState_None);
    }
}

/**
 * @brief Enable one of the button's main animators and disable the others.
 * @param pAnim The animator to enable.
 */
void VesselAnimButton::EnableAnim(nn::ui2d::Animator* pAnim) {
    if (mDownAnim != nullptr) {
        mDownAnim->SetEnabled(mDownAnim == pAnim);
    }

    if (mUpAnim != nullptr) {
        mUpAnim->SetEnabled(mUpAnim == pAnim);
    }

    if (mDecideAnim != nullptr) {
        mDecideAnim->SetEnabled(mDecideAnim == pAnim);
    }

    if (mEnterDPDAnim != nullptr) {
        mEnterDPDAnim->SetEnabled(mEnterDPDAnim == pAnim);
    }

    if (mLeaveDPDAnim != nullptr) {
        mLeaveDPDAnim->SetEnabled(mLeaveDPDAnim == pAnim);
    }

    if (mDecideDPDAnim != nullptr) {
        mDecideDPDAnim->SetEnabled(mDecideDPDAnim == pAnim);
    }

    if (mEnterKeyAnim != nullptr) {
        mEnterKeyAnim->SetEnabled(mEnterKeyAnim == pAnim);
    }

    if (mLeaveKeyAnim != nullptr) {
        mLeaveKeyAnim->SetEnabled(mLeaveKeyAnim == pAnim);
    }

    if (mDecideKeyAnim != nullptr) {
        mDecideKeyAnim->SetEnabled(mDecideKeyAnim == pAnim);
    }

    if (mEnterKeyToDownAnim != nullptr) {
        mEnterKeyToDownAnim->SetEnabled(mEnterKeyToDownAnim == pAnim);
    }

    if (mEnterKeyToEnterDPDAnim != nullptr) {
        mEnterKeyToEnterDPDAnim->SetEnabled(mEnterKeyToEnterDPDAnim == pAnim);
    }

    if (mEnterDPDToEnterKeyAnim != nullptr) {
        mEnterDPDToEnterKeyAnim->SetEnabled(mEnterDPDToEnterKeyAnim == pAnim);
    }
}

/**
 * @brief Advance the down/up state machine, the repeater and every animation state.
 */
void VesselAnimButton::Update() {
    if (mDecideState != cAnimState_Start && mDecideState != cAnimState_End) {
        ProcessActionFromQueue();
    }

    if (mStateVessel == cState_DownStart || mStateVessel == cState_Down) {
        mRepeater.Update(this);
    } else if (mStateVessel == cState_Up) {
        if (mRepeater.IsRepeatable() && mRepeater.mButton != nullptr) {
            mRepeater.Reset();
            mRepeater.mButton = nullptr;
        }
    }

    switch (mStateVessel) {
    case cState_DownStart:
        if (UpdateDown()) {
            FinishDown();
            ProcessActionFromQueue();
        }

        break;
    case cState_UpStart:
        if (UpdateUp()) {
            FinishUp();
            ProcessActionFromQueue();
        }

        break;
    case cState_Up:
        ChangeStateVessel(cState_None);
        break;
    default:
        break;
    }

    switch (mSelectState) {
    case cAnimState_Start:
        if (UpdateSelect()) {
            NotifyState(cState_Select);
            mSelectState = cAnimState_Active;
        }

        break;
    case cAnimState_End:
        if (UpdateNonSelect()) {
            mSelectState = cAnimState_Finish;
        }

        break;
    case cAnimState_Finish:
        mSelectState = cAnimState_None;
        break;
    default:
        break;
    }

    switch (mDecideState) {
    case cAnimState_Start:
        if (UpdateDecide()) {
            NotifyState(cState_DecideEnd);
            mDecideState = cAnimState_Finish;

            if (!mIsKeepEnter) {
                mIsBusy = false;
            }
        }

        break;
    case cAnimState_End:
        if (UpdateNonDecide()) {
            mDecideState = cAnimState_Finish;
        }

        break;
    case cAnimState_Finish:
        mDecideState = cAnimState_None;
        break;
    default:
        break;
    }

    switch (mDecideDPDState) {
    case cAnimState_Start:
        if (UpdateDecideDPD()) {
            NotifyState(cState_DecideEnd);
            mDecideDPDState = cAnimState_Finish;

            if (!mIsKeepEnter) {
                mIsBusy = false;
            }
        }

        break;
    case cAnimState_Finish:
        mDecideDPDState = cAnimState_None;
        break;
    default:
        break;
    }

    switch (mDecideKeyState) {
    case cAnimState_Start:
        if (UpdateDecideKey()) {
            NotifyState(cState_DecideEnd);
            mDecideKeyState = cAnimState_Finish;

            if (!mIsKeepEnter) {
                mIsBusy = false;
            }
        }

        break;
    case cAnimState_Finish:
        mDecideKeyState = cAnimState_None;
        break;
    default:
        break;
    }

    switch (mEnterDPDState) {
    case cAnimState_Start:
        if (UpdateEnterDPD()) {
            mEnterDPDState = cAnimState_Active;
        }

        break;
    case cAnimState_End:
        if (UpdateLeaveDPD()) {
            mEnterDPDState = cAnimState_Finish;
            mIsBusy = false;
        }

        break;
    case cAnimState_Finish:
        mEnterDPDState = cAnimState_None;
        break;
    default:
        break;
    }

    switch (mEnterKeyState) {
    case cAnimState_Start:
        if (UpdateEnterKey()) {
            mEnterKeyState = cAnimState_Active;
        }

        break;
    case cAnimState_End:
        if (UpdateLeaveKey()) {
            mEnterKeyState = cAnimState_Finish;
            mIsBusy = false;
        }

        break;
    case cAnimState_Finish:
        mEnterKeyState = cAnimState_None;
        break;
    default:
        break;
    }

    if (mEnterKeyToDownState == cAnimState_Start && UpdateEnterKeyToDown()) {
        mEnterKeyState = cAnimState_Finish;
        mEnterKeyToDownState = cAnimState_None;
        mIsBusy = false;
    }

    if (mEnterKeyToEnterDPDState == cAnimState_Start && UpdateEnterKeyToEnterDPD()) {
        mEnterKeyToEnterDPDState = cAnimState_None;
        mIsBusy = false;
        mEnterDPDState = cAnimState_Active;
    }

    if (mEnterDPDToEnterKeyState == cAnimState_Start && UpdateEnterDPDToEnterKey()) {
        mEnterDPDToEnterKeyState = cAnimState_None;
        mIsBusy = false;
        mEnterKeyState = cAnimState_Active;
    }
}

/**
 * @brief Process the oldest queued action, removing it once it was accepted.
 */
void VesselAnimButton::ProcessActionFromQueue() {
    if (mVesselActions.mCount == 0) {
        return;
    }

    bool isProcessed = false;

    switch (mVesselActions.mActions[0]) {
    case cAction_Down:
        isProcessed = ProcessDown();
        break;
    case cAction_Up:
        isProcessed = ProcessUp();
        break;
    }

    if (isProcessed) {
        mVesselActions.Pop();
    }
}

/**
 * @brief Fire the decide repeat events that are due this frame.
 * @param pButton The button being held down.
 */
void VesselAnimButton::Repeater::Update(VesselAnimButton* pButton) {
    if (!IsRepeatable()) {
        return;
    }

    if (mButton != pButton) {
        Reset();
        mButton = pButton;
    }

    if (mButton == nullptr || !mButton->IsRepeatable()) {
        return;
    }

    for (int count = CountForEvents(1); count > 0; --count) {
        mButton->DecideRepeat(true);
    }
}

/**
 * @brief Check whether the select animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateSelect() {
    return isAnimFinished(mSelectAnim);
}

/**
 * @brief Check whether the non-select animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateNonSelect() {
    return isAnimFinished(mNonSelectAnim);
}

/**
 * @brief Check whether the decide animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateDecide() {
    return isAnimFinished(mDecideAnim);
}

/**
 * @brief Check whether the non-decide animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateNonDecide() {
    return isAnimFinished(mNonDecideAnim);
}

/**
 * @brief Check whether the pointer decide animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateDecideDPD() {
    return isAnimFinished(mDecideDPDAnim);
}

/**
 * @brief Check whether the key decide animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateDecideKey() {
    return isAnimFinished(mDecideKeyAnim);
}

/**
 * @brief Check whether the pointer enter animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateEnterDPD() {
    return isAnimFinished(mEnterDPDAnim);
}

/**
 * @brief Check whether the pointer leave animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateLeaveDPD() {
    return isAnimFinished(mLeaveDPDAnim);
}

/**
 * @brief Check whether the key enter animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateEnterKey() {
    return isAnimFinished(mEnterKeyAnim);
}

/**
 * @brief Check whether the key leave animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateLeaveKey() {
    return isAnimFinished(mLeaveKeyAnim);
}

/**
 * @brief Check whether the key-enter-to-down animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateEnterKeyToDown() {
    return isAnimFinished(mEnterKeyToDownAnim);
}

/**
 * @brief Check whether the key-enter-to-pointer-enter animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateEnterKeyToEnterDPD() {
    return isAnimFinished(mEnterKeyToEnterDPDAnim);
}

/**
 * @brief Check whether the pointer-enter-to-key-enter animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateEnterDPDToEnterKey() {
    return isAnimFinished(mEnterDPDToEnterKeyAnim);
}

/**
 * @brief Start the select animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::Select(bool isNotify) {
    mIsNotify = isNotify;
    mSelectState = cAnimState_Start;

    if (mSelectAnim != nullptr) {
        mSelectAnim->SetEnabled(true);

        if (mNonSelectAnim != nullptr) {
            mNonSelectAnim->SetEnabled(false);
        }

        mSelectAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }
}

/**
 * @brief Start the non-select animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::NonSelect(bool isNotify) {
    mIsNotify = isNotify;
    mSelectState = cAnimState_End;

    if (mNonSelectAnim != nullptr) {
        mNonSelectAnim->SetEnabled(true);

        if (mSelectAnim != nullptr) {
            mSelectAnim->SetEnabled(false);
        }

        mNonSelectAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }
}

/**
 * @brief Start the decide animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::Decide(bool isNotify) {
    mIsBusy = true;
    mIsNotify = isNotify;
    mDecideState = cAnimState_Start;

    if (mDecideAnim != nullptr) {
        EnableAnim(mDecideAnim);
        mDecideAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_Decide);
}

/**
 * @brief Replay the decide animation for a repeated decide.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::DecideRepeat(bool isNotify) {
    mIsBusy = true;
    mIsNotify = isNotify;

    if (mDecideAnim != nullptr) {
        EnableAnim(mDecideAnim);
        mDecideAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_DecideRepeat);
}

/**
 * @brief Start the non-decide animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::NonDecide(bool isNotify) {
    mIsNotify = isNotify;
    mDecideState = cAnimState_End;

    if (mNonDecideAnim != nullptr) {
        EnableAnim(mNonDecideAnim);
        mNonDecideAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }
}

/**
 * @brief Start the pointer decide animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::DecideDPD(bool isNotify) {
    mIsNotify = isNotify;
    mIsBusy = true;
    mDecideDPDState = cAnimState_Start;

    if (mDecideDPDAnim != nullptr) {
        EnableAnim(mDecideDPDAnim);
        mDecideDPDAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_DecideDPD);
}

/**
 * @brief Start the key decide animation.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::DecideKey(bool isNotify) {
    mIsNotify = isNotify;
    mIsBusy = true;
    mDecideKeyState = cAnimState_Start;
    mEnterKeyState = cAnimState_None;

    if (mDecideKeyAnim != nullptr) {
        EnableAnim(mDecideKeyAnim);
        mDecideKeyAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_DecideKey);
}

/**
 * @brief Enter the button with the pointer, coming from the key enter state if needed.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::EnterDPD(bool isNotify) {
    if (mEnterKeyState != cAnimState_None) {
        mEnterKeyState = cAnimState_Finish;
        EnterKeyToEnterDPD(isNotify);
        return;
    }

    mIsNotify = isNotify;
    mEnterDPDState = cAnimState_Start;

    if (mEnterDPDAnim != nullptr) {
        EnableAnim(mEnterDPDAnim);
        mEnterDPDAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_EnterDPD);
}

/**
 * @brief Switch from the key enter state to the pointer enter state.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::EnterKeyToEnterDPD(bool isNotify) {
    mIsNotify = isNotify;
    mEnterKeyToEnterDPDState = cAnimState_Start;

    if (mEnterKeyToEnterDPDAnim != nullptr) {
        EnableAnim(mEnterKeyToEnterDPDAnim);
        mEnterKeyToEnterDPDAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_EnterKeyToEnterDPD);
}

/**
 * @brief Leave the button with the pointer.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::LeaveDPD(bool isNotify) {
    if (mEnterDPDToEnterKeyState == cAnimState_Start) {
        return;
    }

    mIsNotify = isNotify;
    mEnterDPDState = cAnimState_End;

    if (mLeaveDPDAnim != nullptr) {
        EnableAnim(mLeaveDPDAnim);
        mLeaveDPDAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_LeaveDPD);
}

/**
 * @brief Enter the button with the keys, coming from the pointer enter state if needed.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::EnterKey(bool isNotify) {
    if (mEnterDPDState != cAnimState_None) {
        mEnterDPDState = cAnimState_Finish;
        EnterDPDToEnterKey(isNotify);
        return;
    }

    mIsNotify = isNotify;
    mEnterKeyState = cAnimState_Start;

    if (mEnterKeyAnim != nullptr) {
        EnableAnim(mEnterKeyAnim);
        mEnterKeyAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_EnterKey);
}

/**
 * @brief Switch from the pointer enter state to the key enter state.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::EnterDPDToEnterKey(bool isNotify) {
    mIsNotify = isNotify;
    mEnterDPDToEnterKeyState = cAnimState_Start;

    if (mEnterDPDToEnterKeyAnim != nullptr) {
        EnableAnim(mEnterDPDToEnterKeyAnim);
        mEnterDPDToEnterKeyAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_EnterDPDToEnterKey);
}

/**
 * @brief Leave the button with the keys.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::LeaveKey(bool isNotify) {
    if (mEnterKeyToEnterDPDState == cAnimState_Start) {
        return;
    }

    if (mEnterKeyToDownState == cAnimState_Start) {
        return;
    }

    mIsNotify = isNotify;
    mEnterKeyState = cAnimState_End;

    if (mLeaveKeyAnim != nullptr) {
        EnableAnim(mLeaveKeyAnim);
        mLeaveKeyAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_LeaveKey);
}

/**
 * @brief Press the button while it is entered with the keys.
 * @param isNotify Whether the callback is notified about the transition.
 */
void VesselAnimButton::EnterKeyToDown(bool isNotify) {
    mIsNotify = isNotify;
    mEnterKeyToDownState = cAnimState_Start;

    if (mEnterKeyToDownAnim != nullptr) {
        EnableAnim(mEnterKeyToDownAnim);
        mEnterKeyToDownAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }

    NotifyState(cState_EnterKeyToDown);
}

/**
 * @brief Tell the callback that the button wants to be selected.
 */
void VesselAnimButton::NeedSelect() {
    NotifyState(cState_NeedSelect);
}

/**
 * @brief Enable or disable the button.
 * @param isActive Whether the button is active.
 */
void VesselAnimButton::SetActive(bool isActive) {
    mVesselFlags = static_cast<u32>(isActive) | (mVesselFlags & ~cFlag_Active);
}

/**
 * @brief Check whether the button is going down, is down, or has a down action queued.
 * @return Whether the button is downing.
 */
bool VesselAnimButton::IsDowning() const {
    if (mStateVessel == cState_DownStart || mStateVessel == cState_Down) {
        return true;
    }

    return mVesselActions.IsDownExist();
}

/**
 * @brief Check whether a down action is queued.
 * @return Whether a down action is queued.
 */
bool VesselAnimButton::ActionQueue::IsDownExist() const {
    for (int i = 0; i < mCount; ++i) {
        if (mActions[i] == cAction_Down) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check whether the button is entered with the keys.
 * @return Whether the key enter animation is starting or active.
 */
bool VesselAnimButton::IsEnterKey() const {
    if (mEnterKeyState == cAnimState_None || mEnterKeyState == cAnimState_Finish) {
        return false;
    }

    return true;
}

/**
 * @brief Start going down.
 * @return Always true, the action is consumed.
 */
bool VesselAnimButton::ProcessDown() {
    ForceUp();
    mIsNotify = true;
    ChangeStateVessel(cState_DownStart);
    StartDown();
    return true;
}

/**
 * @brief Start going up.
 * @return Always true, the action is consumed.
 */
bool VesselAnimButton::ProcessUp() {
    ForceDown();
    mIsNotify = true;
    ChangeStateVessel(cState_UpStart);
    StartUp();
    return true;
}

/**
 * @brief Check whether the down animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateDown() {
    return isAnimFinished(mDownAnim);
}

/**
 * @brief Check whether the up animation has finished.
 * @return Whether the animation is missing or stopped.
 */
bool VesselAnimButton::UpdateUp() {
    return isAnimFinished(mUpAnim);
}

/**
 * @brief Play the down animation, or the key-enter-to-down one while entered with the keys.
 */
void VesselAnimButton::StartDown() {
    if (IsEnterKey()) {
        EnterKeyToDown(true);
        return;
    }

    if (mDownAnim != nullptr) {
        EnableAnim(mDownAnim);
        mDownAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }
}

/**
 * @brief Play the up animation.
 */
void VesselAnimButton::StartUp() {
    if (mUpAnim != nullptr) {
        EnableAnim(mUpAnim);
        mUpAnim->Play(nn::ui2d::Animator::PlayType_Once, 1.0f);
    }
}

/**
 * @brief Settle in the down state once the down animation is over.
 */
void VesselAnimButton::FinishDown() {
    ChangeStateVessel(cState_Down);
}

/**
 * @brief Settle in the up state once the up animation is over.
 */
void VesselAnimButton::FinishUp() {
    mIsBusy = false;
    ChangeStateVessel(cState_Up);
}

/**
 * @brief Switch to a new state, notifying the callback.
 * @param state The new state.
 */
void VesselAnimButton::ChangeStateVessel(State state) {
    if (mStateVessel == state) {
        return;
    }

    NotifyState(state);
    mStateVessel = state;
}

/**
 * @brief Remove the oldest queued action.
 */
void VesselAnimButton::ActionQueue::Pop() {
    if (mCount > 0) {
        for (int i = 0; i < mCount - 1; ++i) {
            mActions[i] = mActions[i + 1];
        }

        --mCount;
    }
}

/**
 * @brief Construct a repeater that does not repeat.
 */
VesselAnimButton::Repeater::Repeater()
    : mButton(nullptr), mFirstFrame(0), mIntervalFrame(0), mCounter(0) {}

/**
 * @brief Configure the repeater.
 * @param pButton The button to repeat the decide of.
 * @param firstFrame The number of frames before the first repeat.
 * @param intervalFrame The number of frames between the following repeats.
 */
void VesselAnimButton::Repeater::SetRepeat(VesselAnimButton* pButton, int firstFrame,
                                           int intervalFrame) {
    mButton = pButton;
    mFirstFrame = firstFrame;
    mIntervalFrame = intervalFrame;
}

/**
 * @brief Restart counting frames.
 */
void VesselAnimButton::Repeater::Reset() {
    mCounter = 0;
}

/**
 * @brief Check whether repeating is configured.
 * @return Whether the first repeat delay is positive.
 */
bool VesselAnimButton::Repeater::IsRepeatable() const {
    return mFirstFrame > 0;
}

/**
 * @brief Advance the frame counter and count the repeat events that became due.
 * @param frame The number of frames that passed.
 * @return The number of repeat events to fire.
 */
int VesselAnimButton::Repeater::CountForEvents(int frame) {
    if (!IsRepeatable()) {
        return 0;
    }

    bool isBeforeFirst = mCounter < mFirstFrame;
    mCounter += frame;
    int count = isBeforeFirst && mCounter >= mFirstFrame;

    if (mCounter >= mFirstFrame) {
        int over = mCounter - mFirstFrame;
        count += over / mIntervalFrame;
        mCounter = over % mIntervalFrame + mFirstFrame;
    }

    return count;
}

/**
 * @brief Drop queued down actions. Does nothing.
 */
void VesselAnimButton::ActionQueue::ExcludeDown() {}

/**
 * @brief Construct an empty group with no tandem group.
 */
VesselButtonGroup::VesselButtonGroup() = default;

/**
 * @brief Destroy the group; the buttons are owned by their layout.
 */
VesselButtonGroup::~VesselButtonGroup() = default;

/**
 * @brief Update the buttons from the pointer and key input of this frame.
 * @param pPos The pointer position, or null when there is none.
 * @param isPress Whether the pointer was pressed this frame.
 * @param isRelease Whether the pointer was released this frame.
 * @param isDPD Whether the pointer is a pointing device (as opposed to a touch).
 */
void VesselButtonGroup::UpdateUI(const nn::util::Float2* pPos, bool isPress, bool isRelease,
                                 bool isDPD) {
    ButtonGroup::Update(pPos, isPress, isRelease);

    if (mFlags & cFlag_UpdateHitBox) {
        for (auto& button : mVesselButtons) {
            button.UpdateHitBox();
        }
    }

    bool isPressable = !(mFlags & cFlag_ExcludeDown) || !IsExistDowning();
    VesselAnimButton* pHit = nullptr;

    if (isPressable && pPos != nullptr && IsAcceptInput()) {
        pHit = FindHitButton(*pPos);
    }

    if (isRelease && mDownButton != nullptr) {
        mDownButton->Up();
        mDownButton = nullptr;
    }

    if (mHoverButton == nullptr && pHit == nullptr && mEnterDPDButton != nullptr) {
        if (mDecideDPDButton == nullptr ||
            (mEnterDPDButton->IsRepeatable() | !mDecideDPDButton->IsActive())) {
            mEnterDPDButton->LeaveDPD(true);
            VesselAnimButton* pTandem = GetButtonTandem(mEnterDPDButton);

            if (pTandem != nullptr) {
                pTandem->LeaveDPD(false);
            }

            mEnterDPDButton = nullptr;
        }
    }

    if (mHoverButton != pHit) {
        if (mHoverButton != nullptr) {
            mHoverButton->Off();

            if (mEnterDPDButton != nullptr &&
                (mHoverButton->IsRepeatable() || mDecideDPDButton == nullptr ||
                 mDecideDPDButton->IsRepeatable())) {
                mHoverButton->LeaveDPD(true);
                VesselAnimButton* pTandem = GetButtonTandem(mEnterDPDButton);

                if (pTandem != nullptr) {
                    pTandem->LeaveDPD(false);
                }

                mEnterDPDButton = nullptr;
            }
        }

        if (pHit != nullptr) {
            pHit->On();

            if (isDPD && (pHit->IsRepeatable() || mDecideDPDButton == nullptr ||
                          mDecideDPDButton->IsRepeatable())) {
                pHit->EnterDPD(true);
                mEnterDPDButton = pHit;
                VesselAnimButton* pTandem = GetButtonTandem(pHit);

                if (pTandem != nullptr) {
                    pTandem->EnterDPD(false);
                }
            }
        }

        if (mDownButton != nullptr) {
            if (mDownButton == pHit) {
                pHit->Down();
            } else if (mDownButton->IsDowning()) {
                mDownButton->Up();
            }
        }

        if (mPressButton != nullptr && mPressButton != pHit) {
            if (isRelease && mPressButton != mDecideDPDButton) {
                if (!mPressButton->mIsSelectOnPress) {
                    mPressButton->NeedSelect();
                }

                if (!mPressButton->mIsDecideOnPress) {
                    VesselAnimButton* pDecide = mDecideButton;

                    if (pDecide == nullptr || !pDecide->mIsBusy || pDecide->mIsKeepGroupActive) {
                        if (pDecide != nullptr && pDecide != mPressButton) {
                            pDecide->NonDecide(true);
                            VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

                            if (pTandem != nullptr) {
                                pTandem->NonDecide(false);
                            }
                        }

                        mDecideButton = mPressButton;

                        if (mDecideButton != nullptr) {
                            mDecideButton->Decide(true);
                            DeactivateByDecide(mDecideButton);
                            VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

                            if (pTandem != nullptr) {
                                pTandem->Decide(false);
                            }
                        }
                    }
                }
            }

            if (mPressButton != mDecideButton) {
                mPressButton->Up();
            }

            mPressButton = nullptr;
        }

        mHoverButton = pHit;
    }

    bool isDown = isPressable && isPress;

    if (isDown && pHit != nullptr) {
        if (mHoverButton->mIsDownOnDPD || !isDPD) {
            mHoverButton->Down();
            mPressButton = mHoverButton;
        }

        mDownButton = mHoverButton->mIsHoldDown ? mHoverButton : nullptr;

        if (mHoverButton->mIsSelectOnPress || isDPD) {
            mHoverButton->NeedSelect();
        }

        if (mHoverButton->mIsDecideOnPress || isDPD) {
            if (mDecideButton != mHoverButton && mDecideButton != nullptr) {
                mDecideButton->NonDecide(true);
                VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

                if (pTandem != nullptr) {
                    pTandem->NonDecide(false);
                }
            }

            if (isDPD) {
                VesselAnimButton* pDecide = mDecideDPDButton;

                if (pDecide == nullptr || !pDecide->mIsBusy || pDecide->mIsKeepGroupActive) {
                    mDecideDPDButton = mHoverButton;

                    if (mDecideDPDButton != nullptr) {
                        mDecideDPDButton->DecideDPD(true);
                        DeactivateByDecide(mDecideDPDButton);
                        VesselAnimButton* pTandem = GetButtonTandem(mDecideDPDButton);

                        if (pTandem != nullptr) {
                            pTandem->DecideDPD(false);
                        }

                        if (mEnterDPDButton != nullptr && !mEnterDPDButton->mIsKeepEnter) {
                            mEnterDPDButton = nullptr;
                        }
                    }
                }
            } else {
                VesselAnimButton* pDecide = mDecideButton;

                if (pDecide == nullptr || !pDecide->mIsBusy || pDecide->mIsKeepGroupActive) {
                    mDecideButton = mPressButton;

                    if (mDecideButton != nullptr) {
                        mDecideButton->Decide(true);
                        DeactivateByDecide(mDecideButton);
                        VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

                        if (pTandem != nullptr) {
                            pTandem->Decide(false);
                        }
                    }
                }
            }
        }
    }

    for (auto& button : mVesselButtons) {
        button.Update();
    }
}

/**
 * @brief Find the button of the tandem group that mirrors a button of this group.
 * @param pButton The button of this group.
 * @return The tandem button with the same name and tag, or null.
 */
VesselAnimButton* VesselButtonGroup::GetButtonTandem(VesselAnimButton* pButton) {
    if (mTandemGroup == nullptr) {
        return nullptr;
    }

    return mTandemGroup->FindButtonByNameTag(pButton->mName, pButton->mTag);
}

/**
 * @brief Find a button by name, from the oldest one.
 * @param pName The control name.
 * @return The button, or null.
 */
VesselAnimButton* VesselButtonGroup::FindButtonByName(const char* pName) {
    for (auto& button : mVesselButtons) {
        if (isSameName(pName, button.mName)) {
            return &button;
        }
    }

    return nullptr;
}

/**
 * @brief Find a button by name, from the newest one.
 * @param pName The control name.
 * @return The button, or null.
 */
VesselAnimButton* VesselButtonGroup::FindButtonByNameReverse(const char* pName) {
    for (auto it = mVesselButtons.rbegin(); it != mVesselButtons.rend(); ++it) {
        if (isSameName(pName, it->mName)) {
            return &*it;
        }
    }

    return nullptr;
}

/**
 * @brief Find a button by tag.
 * @param tag The tag.
 * @return The button, or null.
 */
VesselAnimButton* VesselButtonGroup::FindButtonByTag(int tag) {
    for (auto& button : mVesselButtons) {
        if (button.mTag == tag) {
            return &button;
        }
    }

    return nullptr;
}

/**
 * @brief Find a button by name and tag.
 * @param pName The control name.
 * @param tag The tag.
 * @return The button, or null.
 */
VesselAnimButton* VesselButtonGroup::FindButtonByNameTag(const char* pName, int tag) {
    for (auto& button : mVesselButtons) {
        if (isSameName(pName, button.mName) && button.mTag == tag) {
            return &button;
        }
    }

    return nullptr;
}

/**
 * @brief Set the state change callback of every button.
 * @param callback The callback, or null.
 * @param pArg The argument passed to the callback.
 */
void VesselButtonGroup::SetStateChangeCallbackAll(VesselAnimButton::StateChangeCallback callback,
                                                  void* pArg) {
    for (auto& button : mVesselButtons) {
        button.SetStateChangeCallback(callback, pArg);
    }
}

/**
 * @brief Reset every button.
 */
void VesselButtonGroup::ResetAll() {
    for (auto& button : mVesselButtons) {
        button.Reset();
    }
}

/**
 * @brief Select the button with a tag, unselecting the previous one.
 * @param tag The tag of the button.
 * @param isForce Whether to select even when input is not accepted.
 * @return Whether a button with the tag exists.
 */
bool VesselButtonGroup::Select(int tag, bool isForce) {
    VesselAnimButton* pButton = FindButtonByTag(tag);

    if (pButton == nullptr) {
        return false;
    }

    if (!isForce && (!IsAcceptInput() || !pButton->IsActive())) {
        return true;
    }

    if (mSelectButton != pButton && mSelectButton != nullptr) {
        mSelectButton->NonSelect(true);
        VesselAnimButton* pTandem = GetButtonTandem(mSelectButton);

        if (pTandem != nullptr) {
            pTandem->NonSelect(false);
        }
    }

    mSelectButton = pButton;
    mSelectButton->Select(true);
    VesselAnimButton* pTandem = GetButtonTandem(mSelectButton);

    if (pTandem != nullptr) {
        pTandem->Select(false);
    }

    return true;
}

/**
 * @brief Decide a button, with the keys if it is the entered one.
 * @param pButton The button.
 * @param isForce Whether to decide even when input is not accepted.
 * @return Always true.
 */
bool VesselButtonGroup::Decide(VesselAnimButton* pButton, bool isForce) {
    if (pButton->mIsDecideExclusive && mEnterKeyButton != nullptr &&
        mEnterKeyButton != pButton) {
        return true;
    }

    if (!isForce && (!IsAcceptInput() || !pButton->IsActive())) {
        return true;
    }

    if (pButton->mIsDecideExclusive && mDecideButton != pButton && mDecideButton != nullptr) {
        mDecideButton->NonDecide(true);
        VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

        if (pTandem != nullptr) {
            pTandem->NonDecide(false);
        }
    }

    if (mDecideKeyButton != nullptr && mDecideKeyButton->mIsBusy &&
        !mDecideButton->mIsKeepGroupActive) {
        return true;
    }

    if (mEnterKeyButton == pButton) {
        mDecideKeyButton = pButton;

        if (mDecideKeyButton != nullptr) {
            mDecideKeyButton->DecideKey(true);
            DeactivateByDecide(mDecideKeyButton);
            VesselAnimButton* pTandem = GetButtonTandem(mDecideKeyButton);

            if (pTandem != nullptr) {
                pTandem->DecideKey(false);
            }

            mEnterKeyButton = nullptr;
        }
    } else {
        mDecideButton = pButton;

        if (mDecideButton != nullptr) {
            mDecideButton->Decide(true);
            DeactivateByDecide(mDecideButton);
            VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

            if (pTandem != nullptr) {
                pTandem->Decide(false);
            }
        }
    }

    return true;
}

/**
 * @brief Decide the button with a tag.
 * @param tag The tag of the button.
 * @param isForce Whether to decide even when input is not accepted.
 * @return Whether a button with the tag exists.
 */
bool VesselButtonGroup::Decide(int tag, bool isForce) {
    VesselAnimButton* pButton = FindButtonByTag(tag);

    if (pButton == nullptr) {
        return false;
    }

    Decide(pButton, isForce);
    return true;
}

/**
 * @brief Decide the button with a name.
 * @param pName The control name of the button.
 * @param isForce Whether to decide even when input is not accepted.
 * @return Whether a button with the name exists.
 */
bool VesselButtonGroup::Decide(const char* pName, bool isForce) {
    VesselAnimButton* pButton = FindButtonByName(pName);

    if (pButton == nullptr) {
        return false;
    }

    Decide(pButton, isForce);
    return true;
}

/**
 * @brief Decide the button entered with the keys.
 * @return Whether a button is entered with the keys.
 */
bool VesselButtonGroup::Decide() {
    VesselAnimButton* pButton = mEnterKeyButton;

    if (pButton == nullptr) {
        return false;
    }

    if (!IsAcceptInput() || !pButton->IsActive()) {
        return true;
    }

    if (mDecideButton != pButton && mDecideButton != nullptr) {
        mDecideButton->NonDecide(true);
        VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

        if (pTandem != nullptr) {
            pTandem->NonDecide(false);
        }
    }

    if (mDecideKeyButton != nullptr && mDecideKeyButton->mIsBusy &&
        !mDecideButton->mIsKeepGroupActive) {
        return true;
    }

    mDecideKeyButton = pButton;
    mDecideKeyButton->DecideKey(true);
    DeactivateByDecide(mDecideKeyButton);
    VesselAnimButton* pTandem = GetButtonTandem(mDecideKeyButton);

    if (pTandem != nullptr) {
        pTandem->DecideKey(false);
    }

    if (!mDecideKeyButton->mIsKeepEnter) {
        mEnterKeyButton = nullptr;

        if (mTandemGroup != nullptr) {
            mTandemGroup->mEnterKeyButton = nullptr;
        }
    }

    return true;
}

/**
 * @brief Forget the button entered with the keys.
 * @param isLeave Whether to play the key leave animation of the button.
 */
void VesselButtonGroup::ClearEnterKey(bool isLeave) {
    if (isLeave && mEnterKeyButton != nullptr) {
        mEnterKeyButton->LeaveKey(true);
        VesselAnimButton* pTandem = GetButtonTandem(mEnterKeyButton);

        if (pTandem != nullptr) {
            pTandem->LeaveKey(false);
        }
    }

    mEnterKeyButton = nullptr;
}

/**
 * @brief Undecide the decided button and forget every decided button.
 */
void VesselButtonGroup::ClearDecide() {
    if (mDecideButton != nullptr) {
        mDecideButton->NonDecide(true);
        VesselAnimButton* pTandem = GetButtonTandem(mDecideButton);

        if (pTandem != nullptr) {
            pTandem->NonDecide(true);
        }

        mDecideButton = nullptr;
    }

    mDecideDPDButton = nullptr;
    mDecideKeyButton = nullptr;
}

/**
 * @brief Leave the button entered with the pointer and forget it.
 */
void VesselButtonGroup::ClearEnterDPD() {
    if (mEnterDPDButton != nullptr) {
        mEnterDPDButton->LeaveDPD(true);
        VesselAnimButton* pTandem = GetButtonTandem(mEnterDPDButton);

        if (pTandem != nullptr) {
            pTandem->LeaveDPD(true);
        }
    }

    mEnterDPDButton = nullptr;
}

/**
 * @brief Enter a button with the keys, leaving the previously entered one.
 * @param pButton The button.
 * @return Whether the button was entered.
 */
bool VesselButtonGroup::EnterKey(VesselAnimButton* pButton) {
    if (!IsAcceptInput()) {
        return false;
    }

    if (!pButton->mIsForceEnterKey && !pButton->IsActive()) {
        return false;
    }

    if (mEnterKeyButton == pButton) {
        return false;
    }

    if (mEnterKeyButton != nullptr) {
        mEnterKeyButton->LeaveKey(true);
        VesselAnimButton* pTandem = GetButtonTandem(mEnterKeyButton);

        if (pTandem != nullptr) {
            pTandem->LeaveKey(false);
        }
    }

    mEnterKeyButton = pButton;

    if (mEnterKeyButton != nullptr) {
        mEnterKeyButton->EnterKey(true);
        VesselAnimButton* pTandem = GetButtonTandem(mEnterKeyButton);

        if (pTandem != nullptr) {
            pTandem->EnterKey(false);
        }
    }

    return true;
}

/**
 * @brief Enter the button with a tag with the keys.
 * @param tag The tag of the button.
 * @return Whether the button was entered.
 */
bool VesselButtonGroup::EnterKey(int tag) {
    VesselAnimButton* pButton = FindButtonByTag(tag);

    if (pButton == nullptr) {
        return false;
    }

    return EnterKey(pButton);
}

/**
 * @brief Enter the button with a name with the keys.
 * @param pName The control name of the button.
 * @return Whether the button was entered.
 */
bool VesselButtonGroup::EnterKey(const char* pName) {
    VesselAnimButton* pButton = FindButtonByName(pName);

    if (pButton == nullptr) {
        return false;
    }

    return EnterKey(pButton);
}

/**
 * @brief Check whether a button is entered with the keys.
 * @return Whether a button is entered with the keys.
 */
bool VesselButtonGroup::IsExistEnterKey() const {
    return mEnterKeyButton != nullptr;
}

/**
 * @brief Check whether a button is entered with the pointer.
 * @return Whether a button is entered with the pointer.
 */
bool VesselButtonGroup::IsExistEnterDPD() const {
    return mEnterDPDButton != nullptr;
}

/**
 * @brief Check whether a button is entered with the keys or the pointer.
 * @return Whether a button is entered.
 */
bool VesselButtonGroup::IsExistEnter() const {
    return IsExistEnterKey() || IsExistEnterDPD();
}

/**
 * @brief Check whether a non-repeating button is decided.
 * @return Whether such a decided button exists.
 */
bool VesselButtonGroup::IsExistDecide() const {
    if (mDecideButton != nullptr && !mDecideButton->IsRepeatable()) {
        return true;
    }

    if (mDecideKeyButton != nullptr && !mDecideKeyButton->IsRepeatable()) {
        return true;
    }

    if (mDecideDPDButton != nullptr && !mDecideDPDButton->IsRepeatable()) {
        return true;
    }

    return false;
}

/**
 * @brief Check whether a button is pressed.
 * @return Whether a button is pressed.
 */
bool VesselButtonGroup::IsExistDown() const {
    return mPressButton != nullptr;
}

/**
 * @brief Pair this group with a group whose buttons mirror this one's.
 * @param pTandem The tandem group, or null.
 */
void VesselButtonGroup::SetButtonGroupTandem(VesselButtonGroup* pTandem) {
    mTandemGroup = pTandem;
}

/**
 * @brief Create the control of a layout part, building a VesselAnimButton for vessel buttons.
 * @param rSrc The control source.
 * @param pLayout The layout of the control.
 */
void VesselControlCreator::CreateControl(const nn::ui2d::ControlSrc& rSrc,
                                         nn::ui2d::Layout* pLayout) {
    if (mVesselGroup == nullptr) {
        return;
    }

    if (std::strcmp("VesselNormalButton", rSrc.mName) == 0) {
        auto* pButton = newLayoutObject<VesselAnimButton>();
        pButton->Build(rSrc, pLayout);

        // Checked only after Build, so a failed allocation falls back to the default control.
        if (pButton != nullptr) {
            mVesselGroup->mVesselButtons.push_back(*pButton);
            return;
        }
    }

    DefaultControlCreator::CreateControl(getDevice(), pLayout, rSrc);
}
