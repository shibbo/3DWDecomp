#include "Layout/ControlGuide.hpp"

#include <prim/seadSafeString.h>
#include "Layout/Switch/BasicActionGuide.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/InputUtil.hpp"

namespace {
NERVE_DECL(ControlGuide, Appear)
NERVE_DECL(ControlGuide, Wait)
NERVE_DECL(ControlGuide, End)
NERVE_DECL(ControlGuide, ScrollL)
NERVE_DECL(ControlGuide, ScrollR)
NERVE_DECL(ControlGuide, AfterScrollL)
NERVE_DECL(ControlGuide, AfterScrollR)

NERVES_MAKE_NOSTRUCT(ControlGuide, Appear, Wait, End, ScrollL, ScrollR, AfterScrollL, AfterScrollR)

/// Page panes in page order (Bowser's Fury / single mode).
const char* const cPageNamesSingle[] = {"Guide03", "Guide04", "Guide05", "Guide06",
                                        "Guide07", "Guide08", "Guide09"};

/// Page panes in page order (Super Mario 3D World).
const char* const cPageNamesDefault[] = {"Guide01", "Guide02", "Guide05", "Guide06",
                                         "Guide07", "Guide08", "Guide09"};

/// Number of on-screen page slots.
constexpr s32 cSlotNum = 9;

/// Page that hosts the animated basic actions guide.
constexpr s32 cBasicActionsPage = 2;

/// Frame of the scroll animation at which the header text is switched.
constexpr f32 cHeaderSwitchFrame = 7.0f;
}  // namespace

/**
 * @brief Creates the control guide and its header / basic action parts.
 * @param rInfo Layout initialization context.
 * @param isSingleMode Whether the guide is shown in single mode (Bowser's Fury).
 */
ControlGuide::ControlGuide(const al::LayoutInitInfo& rInfo, bool isSingleMode)
    : al::LayoutActor("ControlGuide"), mIsSingleMode(isSingleMode), mPage(0), mPageNum(7),
      mPort(-1), mIsRequestEnd(false) {
    al::initLayoutActor(this, rInfo, "RCS_ControlGuide", nullptr);
    initNerve(&NrvControlGuideAppear, 0);

    mBasicActionGuide =
        new BasicActionGuide(rInfo, "BasicActionsGuide", "ParBasicActionsMaster", this);
    mHeader = new al::LayoutActor("RCS_ControlGuideHeader");
    al::initLayoutPartsActor(mHeader, this, rInfo, "ControlGuideHeader", nullptr);

    mPageTrans = new sead::Vector3f[mPageNum];
    mSlotTrans = new sead::Vector3f[cSlotNum];
    mSlotTrans[0].set(al::getPaneLocalTrans(this, "Guide04"));
    mSlotTrans[1].set(al::getPaneLocalTrans(this, "Guide03"));
    mSlotTrans[2].set(al::getPaneLocalTrans(this, "Guide01"));
    mSlotTrans[3].set(al::getPaneLocalTrans(this, "Guide02"));
    mSlotTrans[4].set(al::getPaneLocalTrans(this, "Guide05"));
    mSlotTrans[5].set(al::getPaneLocalTrans(this, "Guide06"));
    mSlotTrans[6].set(al::getPaneLocalTrans(this, "Guide07"));
    mSlotTrans[7].set(al::getPaneLocalTrans(this, "Guide08"));
    mSlotTrans[8].set(al::getPaneLocalTrans(this, "Guide09"));
}

/**
 * @brief Opens the guide for a controller.
 * @param port Controller port whose buttons are shown.
 */
void ControlGuide::appearWithPort(s32 port) {
    al::LayoutActor::appear();
    mHeader->appear();
    mIsRequestEnd = false;
    mPort = port;
    mBasicActionGuide->setControllerPort(port);
    mBasicActionGuide->resetScrollLocation();

    if (al::isPadTypeJoySingle(mPort)) {
        mPage = 1;
        al::startAction(mHeader, "SetSubHeaderButtonsSLSR", "HeaderButtons");
    } else {
        mPage = 0;
        al::startAction(mHeader, "SetSubHeaderButtonsLR", "HeaderButtons");
    }

    if (mIsSingleMode) {
        al::hidePane(this, "Guide01");
        al::hidePane(this, "Guide02");
        for (s32 i = 0; i < mPageNum; i++) {
            al::setPaneLocalTrans(this, cPageNamesSingle[(i + mPage) % mPageNum],
                                  mSlotTrans[(i + 2) % cSlotNum]);
        }
    } else {
        al::hidePane(this, "Guide03");
        al::hidePane(this, "Guide04");
        for (s32 i = 0; i < mPageNum; i++) {
            al::setPaneLocalTrans(this, cPageNamesDefault[(i + mPage) % mPageNum],
                                  mSlotTrans[(i + 2) % cSlotNum]);
        }
    }

    updatePageIndicator();
    updateHeader(mPage);
    al::startAction(mHeader, "SetSubHeaderAnimation", "ButtonAnim");
    al::setNerve(this, &NrvControlGuideAppear);
}

/**
 * @brief Shows the current page in the header's page indicator.
 */
void ControlGuide::updatePageIndicator() {
    al::StringTmp<32> actionName("SetPage");

    if (mPage < cBasicActionsPage) {
        if (mIsSingleMode) {
            actionName.appendWithFormat("New%d", mPage + 1);
        } else {
            actionName.appendWithFormat("3DW%d", mPage + 1);
        }
    } else if (mPage == cBasicActionsPage) {
        actionName.append("BasicActions");
    } else {
        actionName.appendWithFormat("OtherActions%d", mPage - cBasicActionsPage);
    }

    al::startAction(mHeader, actionName.cstr(), "PageIndicator");
}

/**
 * @brief Sets the header title for a page.
 * @param page Page index.
 */
void ControlGuide::updateHeader(s32 page) {
    const char* pLabel;
    switch (page) {
    case 0:
        pLabel = "DualJoy";
        break;
    case 1:
        pLabel = mIsSingleMode ? "AssistMode" : "SingleJoy";
        break;
    case 2:
        pLabel = "BasicActions";
        break;
    case 3:
        pLabel = "TouchControls";
        break;
    case 4:
        pLabel = "Amiibo";
        break;
    case 5:
        pLabel = "SnapshotMode";
        break;
    case 6:
        pLabel = "NetworkMultiplayer";
        break;
    default:
        return;
    }

    al::setPaneSystemMessage(mHeader, "TxtHeader", "ControlGuide", pLabel);
}

/**
 * @brief Plays the appear animation, then waits for input.
 */
void ControlGuide::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(mHeader, mIsSingleMode ? "SetHeaderColorNew" : "SetHeaderColor3DWorld",
                        "SetColor");
        al::startAction(this, "Appear", "Main");
    }

    if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, &NrvControlGuideWait);
    }
}

/**
 * @brief Waits for page scrolling (L/R) or closing (cancel).
 */
void ControlGuide::exeWait() {
    if (al::isFirstStep(this)) {
        updatePageIndicator();
    }

    if (mIsRequestEnd ||
        (rc::isPadTriggerUiCancelByPort(mPort) &&
         (mPage != cBasicActionsPage || mBasicActionGuide->isWait()))) {
        al::setNerve(this, &NrvControlGuideEnd);
        return;
    }

    if (rc::isPadTriggerUiLByPort(mPort)) {
        al::setNerve(this, &NrvControlGuideScrollL);
        return;
    }

    if (rc::isPadTriggerUiRByPort(mPort)) {
        al::setNerve(this, &NrvControlGuideScrollR);
    }
}

/**
 * @brief Plays the close animation and kills the guide when done.
 */
void ControlGuide::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", "Main");
        if (mPage == cBasicActionsPage) {
            mBasicActionGuide->end();
        }
    }

    if (al::isActionEnd(this, "Main")) {
        kill();
    }
}

/**
 * @brief Scrolls to the previous page.
 */
void ControlGuide::exeScrollL() {
    if (al::isFirstStep(this)) {
        s32 prevPage = getPrevPage();
        if (prevPage == cBasicActionsPage) {
            mBasicActionGuide->startIn(false);
        } else if (mPage == cBasicActionsPage) {
            if (!mBasicActionGuide->isWait()) {
                al::setNerve(this, &NrvControlGuideWait);
                return;
            }

            mBasicActionGuide->startOut(false);
        }

        const char* const* pPageNames = mIsSingleMode ? cPageNamesSingle : cPageNamesDefault;
        al::setPaneLocalTrans(this, pPageNames[prevPage], mSlotTrans[1]);
        al::startAction(this, "OutRight", nullptr);
    }

    if (al::getActionFrame(this, nullptr) == cHeaderSwitchFrame) {
        updateHeader(getPrevPage());
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvControlGuideAfterScrollL);
        mPage--;
        if (mPage < 0) {
            mPage = mPageNum - 1;
        }
    }
}

/**
 * @brief Scrolls to the next page.
 */
void ControlGuide::exeScrollR() {
    if (al::isFirstStep(this)) {
        s32 nextPage = getNextPage();
        if (nextPage == cBasicActionsPage) {
            mBasicActionGuide->startIn(true);
        } else if (mPage == cBasicActionsPage) {
            if (!mBasicActionGuide->isWait()) {
                al::setNerve(this, &NrvControlGuideWait);
                return;
            }

            mBasicActionGuide->startOut(true);
        }

        const char* const* pPageNames = mIsSingleMode ? cPageNamesSingle : cPageNamesDefault;
        al::setPaneLocalTrans(this, pPageNames[nextPage], mSlotTrans[3]);
        al::startAction(this, "OutLeft", nullptr);
    }

    if (al::getActionFrame(this, nullptr) == cHeaderSwitchFrame) {
        updateHeader(getNextPage());
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvControlGuideAfterScrollR);
        mPage = getNextPage();
    }
}

/**
 * @brief Moves every page pane to the position of the page after it.
 * @param pPageNames Page pane names in page order.
 */
inline void ControlGuide::rotatePagesRight(const char* const* pPageNames) {
    for (s32 i = 0; i < mPageNum; i++) {
        mPageTrans[i].set(al::getPaneLocalTrans(this, pPageNames[i]));
    }

    for (s32 i = 0; i < mPageNum; i++) {
        s32 next = (i + 1) % mPageNum;
        al::setPaneLocalTrans(this, pPageNames[i], mPageTrans[next]);
    }
}

/**
 * @brief Moves every page pane to the position of the page before it.
 * @param pPageNames Page pane names in page order.
 */
inline void ControlGuide::rotatePagesLeft(const char* const* pPageNames) {
    for (s32 i = 0; i < mPageNum; i++) {
        mPageTrans[i].set(al::getPaneLocalTrans(this, pPageNames[i]));
    }

    for (s32 i = 0; i < mPageNum; i++) {
        s32 prev = (i == 0 ? mPageNum : i) - 1;
        al::setPaneLocalTrans(this, pPageNames[i], mPageTrans[prev]);
    }
}

/**
 * @brief Rotates the page panes one slot to the right after a left scroll.
 */
void ControlGuide::exeAfterScrollL() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        if (mIsSingleMode) {
            rotatePagesRight(cPageNamesSingle);
        } else {
            rotatePagesRight(cPageNamesDefault);
        }

        al::setNerve(this, &NrvControlGuideWait);
    }
}

/**
 * @brief Rotates the page panes one slot to the left after a right scroll.
 */
void ControlGuide::exeAfterScrollR() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
        if (mIsSingleMode) {
            rotatePagesLeft(cPageNamesSingle);
        } else {
            rotatePagesLeft(cPageNamesDefault);
        }

        al::setNerve(this, &NrvControlGuideWait);
    }
}

/**
 * @brief Checks whether the guide is playing its close animation.
 * @return True while ending.
 */
bool ControlGuide::isEnding() {
    return al::isNerve(this, &NrvControlGuideEnd);
}
