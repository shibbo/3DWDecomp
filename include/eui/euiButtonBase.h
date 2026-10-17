#pragma once

#include <eui/euiControlBase.h>
#include <basis/seadTypes.h>

namespace eui {

class ButtonBase : public ControlBase {
public:
    enum State { cState_Off, cState_OnStart, cState_OffStart, cState_On,
                 cState_DownStart, cState_Down, cState_CancelStart };
    enum Action { cAction_On, cAction_Off, cAction_Down, cAction_Cancel };
    struct ActionQueue {
        void PushWithOmit(Action action);
        void Pop();
        bool IsDownExist() const;
        u8 actions[4];
        s32 count = 0;
    };

    ButtonBase();
    ~ButtonBase() override = default;
    const char* getClassName() const override;
    NN_RUNTIME_TYPEINFO(ControlBase);
    void Update(float step) override;
    virtual void On();
    virtual void Off();
    virtual void Down();
    virtual void Cancel();
    virtual void ForceOff();
    virtual void ForceOn();
    virtual void ForceDown();
    virtual void SetActive(bool active);
    virtual bool ProcessOn();
    virtual bool ProcessOff();
    virtual bool ProcessDown();
    virtual bool ProcessCancel();
    virtual bool UpdateOn();
    virtual bool UpdateOff();
    virtual bool UpdateDown();
    virtual bool UpdateCancel();
    virtual void StartOn();
    virtual void StartOff();
    virtual void StartDown();
    virtual void StartCancel();
    virtual void FinishOn();
    virtual void FinishOff();
    virtual void FinishDown();
    virtual void FinishCancel();
    virtual void ChangeState(State state);
    virtual void ForceChangeState(State state);

    void ResumeUpdate();
    void ProcessActionFromQueue();
    bool IsDowning() const;

    bool IsActive() const { return (mFlags & 0x10) != 0; }
    bool IsRepeatOn() const { return (mFlags & 0x80) != 0; }
    bool IsTouch() const { return (mFlags & 0x40) != 0; }

    void SetAllowNoTrigTouch(bool allow) {
        if (allow) {
            mFlags |= 0x100;
        } else {
            mFlags &= ~0x100;
        }
    }

    void SetDownWithTouchOn(bool enabled) {
        if (enabled) {
            mFlags |= 0x200;
        } else {
            mFlags &= ~0x200;
        }
    }

    void ClearActions() { mActions.count = 0; }

    /** @return Sound type passed to the screen's sound link when the button is pressed. */
    u8 GetSoundType() const { return _39; }
    /** @return User tag used to look up the button's box cursor node. */
    int GetTag() const { return _44; }
    /** @return Whether the button is a parts control that owns its whole layout. */
    bool IsPartsControl() const { return (mFlags & 0x2000) != 0; }

protected:
    friend class ButtonGroup;
    friend class BoxCursorNode;
    nn::util::IntrusiveListNode mUpdateLink;
    u8 mState;
    u8 _39;
    u16 mFlags;
    ActionQueue mActions;
    u32 _44;
};

static_assert(sizeof(ButtonBase) == 0x48, "ButtonBase size");

}  // namespace eui
