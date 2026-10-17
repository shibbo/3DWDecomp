#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }
class ButtonTouch;

class ButtonArrowParts : public al::LayoutActor {
public:
    ButtonArrowParts(const al::LayoutInitInfo& rInfo, const char* pName,
                     const char* pPartsName, al::LayoutActor* pParent);
    void exeWait();
    void exeDecide();
    void exeHide();
    bool isTrigerDecide() const;
    void startDecide();
    bool isHide() const;
    void setHide();
    void setShow();
    void setPort(int port);
private:
    ButtonTouch* mTouch = nullptr;
};
