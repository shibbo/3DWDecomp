#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al {
class LayoutActor;
class IUseLayout;
class IUseLayoutAction;
class IUseAudioKeeper;
}

struct ButtonTouchParam {
    ButtonTouchParam();
    ButtonTouchParam(const char* pHitPane, const char* pActionSlot);
    const char* mHitPane;
    const char* mActionSlot;
};
static_assert(sizeof(ButtonTouchParam) == 0x10);

class ButtonTouch : public al::NerveExecutor {
public:
    ButtonTouch(const char* pName, al::LayoutActor* pActor, const ButtonTouchParam* pParam);
    ~ButtonTouch() override = default;
    void update();
    void hidePane();
    void showPane();
    void startAppear();
    void startEnd();
    void setTouchEnable();
    void setTouchDisable();
    bool isTrigerTouch() const;
    bool isDecide() const;
    bool isTouchPane() const;
    bool isTouchHold() const;
    bool isWait() const;
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    void exeTouch();
    void exeOutsideHold();
    void exeDecide();
    void exeDecideEnd();
    void reset();
    bool isDecideEnd() const;
    bool isTrigerDecide() const;
    void setPort(int port);
    void startDecide();
private:
    al::IUseLayout* mLayout;
    al::IUseLayoutAction* mLayoutAction;
    al::IUseAudioKeeper* mAudio;
    const ButtonTouchParam* mParam;
    bool mIsValid = true;
    int mPort = 0;
    int mTouchPort = -1;
};
static_assert(sizeof(ButtonTouch) == 0x40);
