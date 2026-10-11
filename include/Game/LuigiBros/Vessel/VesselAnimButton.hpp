#pragma once

#include <nn/ui2d/ui2d_AnimButton.h>
#include <nn/ui2d/ui2d_Animator.h>
#include <nn/ui2d/ui2d_ButtonGroup.h>
#include <nn/ui2d/ui2d_DefaultControlCreator.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::ui2d {
class ControlSrc;
class Layout;
}  // namespace nn::ui2d

/**
 * @brief Animated button of the Luigi Bros "Vessel" menu UI.
 *
 * Extends the ui2d animated button with its own down/up state machine, a set of select, decide,
 * enter and leave animations driven by key and pointer (DPD) input, and a decide repeater.
 */
class VesselAnimButton : public nn::ui2d::AnimButton {
public:
    /// Button state; values from cState_NeedSelect on are only reported through the callback.
    enum State {
        cState_None = 0,
        cState_DownStart = 1,
        cState_Down = 2,
        cState_UpStart = 3,
        cState_Up = 4,
        cState_NeedSelect = 5,
        cState_Select = 6,
        cState_Decide = 8,
        cState_DecideDPD = 9,
        cState_DecideKey = 10,
        cState_DecideEnd = 11,
        cState_EnterDPD = 12,
        cState_LeaveDPD = 13,
        cState_EnterKey = 14,
        cState_LeaveKey = 15,
        cState_EnterKeyToDown = 16,
        cState_EnterKeyToEnterDPD = 17,
        cState_EnterDPDToEnterKey = 18,
        cState_DecideRepeat = 19,
    };

    /// Queued input action.
    enum Action {
        cAction_Down = 0,
        cAction_Up = 1,
    };

    /// Progress of one of the select/decide/enter animations.
    enum AnimState {
        cAnimState_None = 0,
        cAnimState_Start = 1,
        cAnimState_Active = 2,
        cAnimState_End = 3,
        cAnimState_Finish = 4,
    };

    /// Bits of mVesselFlags.
    enum Flag {
        cFlag_Down = 1 << 0,
        cFlag_Up = 1 << 1,
        cFlag_Active = 1 << 2,
    };

    using StateChangeCallback = void (*)(VesselAnimButton*, State, State, void*);

    /**
     * @brief Two-entry queue of pending down/up actions.
     */
    struct ActionQueue {
        void PushWithOmit(Action action);
        bool IsDownExist() const;
        void Pop();
        void ExcludeDown();

        Action mActions[2];
        s32 mCount = 0;
    };

    /**
     * @brief Fires repeated decide events while a button is held down.
     */
    class Repeater {
    public:
        Repeater();
        void Update(VesselAnimButton* pButton);
        void SetRepeat(VesselAnimButton* pButton, int firstFrame, int intervalFrame);
        void Reset();
        bool IsRepeatable() const;
        int CountForEvents(int frame);

        VesselAnimButton* mButton;
        s32 mFirstFrame;
        s32 mIntervalFrame;
        s32 mCounter;
    };

    VesselAnimButton();

    void Build(const nn::ui2d::ControlSrc& rSrc, nn::ui2d::Layout* pLayout);
    void SetStateChangeCallback(StateChangeCallback callback, void* pArg);
    void Down() override;
    virtual void Up();
    void ForceDown() override;
    virtual void ForceUp();
    void Reset();
    void EnableAnim(nn::ui2d::Animator* pAnim);
    void Update() override;
    void ProcessActionFromQueue();
    bool UpdateSelect();
    bool UpdateNonSelect();
    bool UpdateDecide();
    bool UpdateNonDecide();
    bool UpdateDecideDPD();
    bool UpdateDecideKey();
    bool UpdateEnterDPD();
    bool UpdateLeaveDPD();
    bool UpdateEnterKey();
    bool UpdateLeaveKey();
    bool UpdateEnterKeyToDown();
    bool UpdateEnterKeyToEnterDPD();
    bool UpdateEnterDPDToEnterKey();
    void Select(bool isNotify);
    void NonSelect(bool isNotify);
    void Decide(bool isNotify);
    void DecideRepeat(bool isNotify);
    void NonDecide(bool isNotify);
    void DecideDPD(bool isNotify);
    void DecideKey(bool isNotify);
    void EnterDPD(bool isNotify);
    void EnterKeyToEnterDPD(bool isNotify);
    void LeaveDPD(bool isNotify);
    void EnterKey(bool isNotify);
    void EnterDPDToEnterKey(bool isNotify);
    void LeaveKey(bool isNotify);
    void EnterKeyToDown(bool isNotify);
    void NeedSelect();
    void SetActive(bool isActive) override;
    bool IsDowning() const;
    bool IsEnterKey() const;
    bool ProcessDown() override;
    virtual bool ProcessUp();
    bool UpdateDown() override;
    virtual bool UpdateUp();
    void StartDown() override;
    virtual void StartUp();
    void FinishDown() override;
    virtual void FinishUp();
    virtual void ChangeStateVessel(State state);

    /**
     * @brief Switch to a state immediately, without notifying the callback.
     * @param state The new state.
     */
    virtual void ForceChangeStateVessel(State state) { mStateVessel = state; }

    /// Whether this button currently accepts input.
    bool IsActive() const { return (mVesselFlags & cFlag_Active) != 0; }

    /// Whether holding this button down repeats the decide event.
    bool IsRepeatable() const { return mRepeater.IsRepeatable(); }

    /**
     * @brief Report a state or event to the callback, if notification is enabled.
     * @param state The state or event to report.
     */
    void NotifyState(State state) {
        if (mIsNotify && mVesselCallback != nullptr) {
            mVesselCallback(this, mStateVessel, state, mVesselCallbackArg);
        }
    }

    nn::util::IntrusiveListNode mVesselLink;
    nn::ui2d::Animator* mDownAnim = nullptr;
    nn::ui2d::Animator* mUpAnim = nullptr;
    nn::ui2d::Animator* mSelectAnim = nullptr;
    nn::ui2d::Animator* mNonSelectAnim = nullptr;
    nn::ui2d::Animator* mDecideAnim = nullptr;
    nn::ui2d::Animator* mNonDecideAnim = nullptr;
    nn::ui2d::Animator* mDecideDPDAnim = nullptr;
    nn::ui2d::Animator* mDecideKeyAnim = nullptr;
    nn::ui2d::Animator* mEnterDPDAnim = nullptr;
    nn::ui2d::Animator* mLeaveDPDAnim = nullptr;
    nn::ui2d::Animator* mEnterKeyAnim = nullptr;
    nn::ui2d::Animator* mLeaveKeyAnim = nullptr;
    nn::ui2d::Animator* mEnterKeyToDownAnim = nullptr;
    nn::ui2d::Animator* mEnterKeyToEnterDPDAnim = nullptr;
    nn::ui2d::Animator* mEnterDPDToEnterKeyAnim = nullptr;
    const char* mDecideAnimName;
    const char* mDecideDPDAnimName;
    const char* mDecideKeyAnimName;
    State mStateVessel = cState_None;
    u32 mVesselFlags;
    ActionQueue mVesselActions;
    AnimState mSelectState = cAnimState_None;
    AnimState mDecideState = cAnimState_None;
    AnimState mDecideDPDState = cAnimState_None;
    AnimState mDecideKeyState = cAnimState_None;
    AnimState mEnterDPDState = cAnimState_None;
    AnimState mEnterKeyState = cAnimState_None;
    AnimState mEnterKeyToDownState = cAnimState_None;
    AnimState mEnterKeyToEnterDPDState = cAnimState_None;
    AnimState mEnterDPDToEnterKeyState = cAnimState_None;
    StateChangeCallback mVesselCallback = nullptr;
    void* mVesselCallbackArg = nullptr;
    bool mIsBusy = false;
    bool mIsNotify = true;
    bool mIsSelectOnPress = false;
    bool mIsDecideOnPress = false;
    bool mIsEnableReset = false;
    bool mIsDownOnDPD = false;
    bool mIsKeepGroupActive = false;
    bool mIsDecideExclusive = false;
    bool mIsForceEnterKey;
    bool _385 = true;
    bool mIsKeepEnter = true;
    bool mIsHoldDown = false;
    Repeater mRepeater;
};

static_assert(sizeof(VesselAnimButton) == 0x1a0, "VesselAnimButton size");

/**
 * @brief Button group that drives the select/decide/enter logic of its VesselAnimButtons.
 *
 * A group can be paired with a "tandem" group whose buttons of the same name and tag mirror the
 * transitions of this group's buttons.
 */
class VesselButtonGroup : public nn::ui2d::ButtonGroup {
public:
    using VesselButtonList = nn::util::IntrusiveList<
        VesselAnimButton,
        nn::util::IntrusiveListMemberNodeTraits<VesselAnimButton, &VesselAnimButton::mVesselLink>>;

    /// Bits of ButtonGroup::mFlags.
    enum Flag {
        cFlag_UpdateHitBox = 1 << 0,
        cFlag_ExcludeDown = 1 << 1,
        cFlag_AcceptInput = 1 << 2,
    };

    VesselButtonGroup();
    ~VesselButtonGroup() override;

    virtual void UpdateUI(const nn::util::Float2* pPos, bool isPress, bool isRelease, bool isDPD);
    VesselAnimButton* GetButtonTandem(VesselAnimButton* pButton);
    VesselAnimButton* FindButtonByName(const char* pName);
    VesselAnimButton* FindButtonByNameReverse(const char* pName);
    VesselAnimButton* FindButtonByTag(int tag);
    VesselAnimButton* FindButtonByNameTag(const char* pName, int tag);
    void SetStateChangeCallbackAll(VesselAnimButton::StateChangeCallback callback, void* pArg);
    void ResetAll();
    bool Select(int tag, bool isForce);
    bool Decide(VesselAnimButton* pButton, bool isForce);
    bool Decide(int tag, bool isForce);
    bool Decide(const char* pName, bool isForce);
    bool Decide();
    void ClearEnterKey(bool isLeave);
    void ClearDecide();
    void ClearEnterDPD();
    bool EnterKey(VesselAnimButton* pButton);
    bool EnterKey(int tag);
    bool EnterKey(const char* pName);
    bool IsExistEnterKey() const;
    bool IsExistEnterDPD() const;
    bool IsExistEnter() const;
    bool IsExistDecide() const;
    bool IsExistDown() const;
    void SetButtonGroupTandem(VesselButtonGroup* pTandem);

    /// Whether input is accepted by the group.
    bool IsAcceptInput() const { return (mFlags & cFlag_AcceptInput) != 0; }

    /**
     * @brief Check whether a button that excludes the others is going down.
     * @return Whether such a button exists.
     */
    bool IsExistDowning() const {
        for (const auto& button : mVesselButtons) {
            if ((button.mFlags & 0x20) && button.IsDowning()) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Find the active button under a position.
     * @param rPos The position.
     * @return The button, or null.
     */
    VesselAnimButton* FindHitButton(const nn::util::Float2& rPos) {
        for (auto& button : mVesselButtons) {
            if (button.IsActive() && button.IsHit(rPos)) {
                return &button;
            }
        }

        return nullptr;
    }

    /**
     * @brief Stop accepting input unless the decided button keeps the group active.
     * @param pButton The button that was just decided.
     */
    void DeactivateByDecide(const VesselAnimButton* pButton) {
        if (!pButton->mIsKeepGroupActive) {
            mFlags &= ~cFlag_AcceptInput;
        }
    }

    VesselButtonList mVesselButtons;
    VesselAnimButton* mHoverButton = nullptr;
    VesselAnimButton* mPressButton = nullptr;
    VesselAnimButton* mSelectButton = nullptr;
    VesselAnimButton* mDecideButton = nullptr;
    VesselAnimButton* mDecideDPDButton = nullptr;
    VesselAnimButton* mDecideKeyButton = nullptr;
    VesselAnimButton* mEnterDPDButton = nullptr;
    VesselAnimButton* mEnterKeyButton = nullptr;
    VesselAnimButton* mDownButton = nullptr;
    VesselButtonGroup* mTandemGroup = nullptr;
};

static_assert(sizeof(VesselButtonGroup) == 0x90, "VesselButtonGroup size");

/**
 * @brief Control creator that builds VesselAnimButtons for "VesselNormalButton" controls.
 */
class VesselControlCreator : public nn::ui2d::DefaultControlCreator {
public:
    using DefaultControlCreator::CreateControl;

    virtual void CreateControl(const nn::ui2d::ControlSrc& rSrc, nn::ui2d::Layout* pLayout);

    VesselButtonGroup* mVesselGroup;
};
