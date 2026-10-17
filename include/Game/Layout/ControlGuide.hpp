#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class BasicActionGuide;

/**
 * @brief Controller button guide shown from the pause menu.
 *
 * The guide is a ring of pages ("Guide01".."Guide09" panes) scrolled with L/R. Page 2 hosts the
 * animated basic actions guide.
 */
class ControlGuide : public al::LayoutActor {
public:
    ControlGuide(const al::LayoutInitInfo& rInfo, bool isSingleMode);

    void appearWithPort(s32 port);
    void updatePageIndicator();
    void updateHeader(s32 page);
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeScrollL();
    void exeScrollR();
    void exeAfterScrollL();
    void exeAfterScrollR();
    bool isEnding();

    /** @brief Requests the guide to close on its next update. */
    void requestEnd() { mIsRequestEnd = true; }

private:
    void rotatePagesRight(const char* const* pPageNames);
    void rotatePagesLeft(const char* const* pPageNames);

    /** @brief Index of the page left of the current one (wraps around). */
    s32 getPrevPage() const { return (mPage == 0 ? mPageNum : mPage) - 1; }

    /** @brief Index of the page right of the current one (wraps around). */
    s32 getNextPage() const { return (mPage + 1) % mPageNum; }

    bool mIsSingleMode;
    s32 mPage;
    s32 mPageNum;
    s32 mPort;
    sead::Vector3f* mPageTrans;
    sead::Vector3f* mSlotTrans;
    bool mIsRequestEnd;
    BasicActionGuide* mBasicActionGuide;
    al::LayoutActor* mHeader;
};
static_assert(sizeof(ControlGuide) == 0x158);
