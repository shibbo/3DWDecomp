#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <prim/seadSafeString.h>
#include <prim/seadDelegate.h>
namespace al { class LayoutInitInfo; }
class ButtonGroup;
/** @brief Displays Miiverse post details and their action buttons. */
class WindowMiiverseDetail : public al::LayoutActor {
public:
    explicit WindowMiiverseDetail(const al::LayoutInitInfo& rInfo);
    bool lineBreakCallback(const sead::WSafeString& rText, u32 index);
    void startShow();
    void endShow();
    void reset();
    void exeAppear();
    void exeWait();
    void exeDecided();
    void exeDisappear();
    bool isDecidedAny() const;
    bool isDecidedClose() const;
    bool isDecidedYeah() const;
    bool isDecidedComment() const;
    void invalidateButton() const;
private:
    sead::Delegate2R<WindowMiiverseDetail, const sead::WSafeString&, u32, bool> mLineBreakCallback;
    sead::WFixedSafeString<256> mText;
    ButtonGroup* mButtons = nullptr;
    int mPort = 5;
    bool mHasYeah = false;
};
static_assert(sizeof(WindowMiiverseDetail) == 0x370);
