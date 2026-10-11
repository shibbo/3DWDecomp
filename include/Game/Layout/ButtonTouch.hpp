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
    void update();
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
    bool mIsValid;
    int mPort;
    int mTouchPort;
};
static_assert(sizeof(ButtonTouch) == 0x40);
