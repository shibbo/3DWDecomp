#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <prim/seadSafeString.h>
namespace al { class LayoutInitInfo; }
class ButtonTouch;
/** @brief Touch-enabled Miiverse balloon attached to a parent layout. */
class MiiverseBalloonParts : public al::LayoutActor {
public:
    MiiverseBalloonParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName, al::LayoutActor* pParent, float scale);
    void control() override;
    void reset();
    bool isWait() const;
    bool isDecide() const;
    bool isDecideEnd() const;
    void setTouchEnable();
    void setTouchDisable();
private:
    sead::WFixedSafeString<256> mText;
    al::LayoutActor* mParent;
    ButtonTouch* mTouch = nullptr;
    float mScale;
};
static_assert(sizeof(MiiverseBalloonParts) == 0x358);
