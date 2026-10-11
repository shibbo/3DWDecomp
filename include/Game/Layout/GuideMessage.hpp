#pragma once

#include <attributes.h>
#include <prim/seadSafeString.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

/** @brief Guide message window ("GuideMessage" layout) that shows a system message for a time. */
class GuideMessage : public al::LayoutActor {
public:
    GuideMessage(const al::LayoutInitInfo& rInfo);

    void startAppear(const char16_t* pText, s32 frame, f32 posY);
    void startAppear(const char* pCategory, const char* pLabel, s32 frame, f32 posY,
                     bool isForce);
    void startAppearSplit(const char16_t* pTextLeft, const char16_t* pTextRight, s32 frame,
                          f32 posY);
    void startAppearAssist();
    NOINLINE void setAssistText();
    void startEnd();
    void hide();
    void unHide();

    void exeAppear();
    void exeHide();
    void exeWait();
    void exeWaitToChangeText();
    void exeChangeText();
    void exeEnd();

private:
    bool mIsHide = false;                     // 0x121
    s32 mFrame = -1;                          // 0x124
    sead::FixedSafeString<64> mCategory;      // 0x128
    sead::FixedSafeString<64> mLabel;         // 0x180
};

static_assert(sizeof(GuideMessage) == 0x1D8);
