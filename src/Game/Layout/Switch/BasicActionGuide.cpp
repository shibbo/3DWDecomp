#include "Layout/Switch/BasicActionGuide.hpp"

#include <math.h>

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/InputUtil.hpp"

namespace {
NERVE_DECL(BasicActionGuide, End);
NERVE_DECL(BasicActionGuide, Appear);
NERVE_DECL(BasicActionGuide, Wait);
NERVES_MAKE_NOSTRUCT(BasicActionGuide, End, Appear, Wait)

/** Height of the scroll bar track; the bar moves from 0 down to this value. */
constexpr f32 cScrollBarMin = -448.0f;

/** Scroll speed applied to the pad's UI scroll input. */
constexpr f32 cScrollSpeed = 2.5f;

/** Height of the scrollable page in single (Bowser's Fury) mode. */
constexpr f32 cPageHeightSingle = 1908.0f;

/** Height of the scrollable page in 3D World mode. */
constexpr f32 cPageHeight3DW = 2372.0f;
}  // namespace

/**
 * @brief Creates the basic action guide layout as parts of its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Unused name (the actor is always named "BasicActionGuide").
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor, also used to play sounds.
 */
BasicActionGuide::BasicActionGuide(const al::LayoutInitInfo& rInfo, const char* pName,
                                   const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor("BasicActionGuide"), mControllerPort(-1), mScrollRate(1.0f),
      mScrollHeight(0.0f), mAudioKeeper(pParent) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvBasicActionGuideEnd, 0);
}

/**
 * @brief Sets which controller scrolls the guide.
 * @param port Controller port.
 */
void BasicActionGuide::setControllerPort(s32 port) {
    mControllerPort = port;
}

/**
 * @brief Shows the guide and starts its appear state.
 * @param isImmediate Unused.
 */
void BasicActionGuide::startIn(bool isImmediate) {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvBasicActionGuideAppear);
}

/**
 * @brief Ends the guide.
 * @param isImmediate Unused.
 */
void BasicActionGuide::startOut(bool isImmediate) {
    al::setNerve(this, &NrvBasicActionGuideEnd);
}

/** @brief Ends the guide. */
void BasicActionGuide::end() {
    al::setNerve(this, &NrvBasicActionGuideEnd);
}

/**
 * @brief Checks whether the guide is being shown and scrollable.
 * @return Whether the wait state is active.
 */
bool BasicActionGuide::isWait() {
    return al::isNerve(this, &NrvBasicActionGuideWait);
}

/** @brief Scrolls the page and the scroll bar back to the top. */
void BasicActionGuide::resetScrollLocation() {
    al::setPaneLocalTrans(this, "Pages", sead::Vector2f::zero);
    al::setPaneLocalTrans(this, "ScrollBar", sead::Vector2f::zero);
}

/** @brief Picks the page variant for the current game mode, then waits. */
void BasicActionGuide::exeAppear() {
    if (al::isFirstStep(this)) {
        if (GameDataFunction::isSingleMode(this)) {
            mScrollHeight = cPageHeightSingle;
            al::startAction(this, "SetModeSingle", "Mode");
        } else {
            mScrollHeight = cPageHeight3DW;
            al::startAction(this, "SetMode3DW", "Mode");
        }

        mScrollRate = mScrollHeight / cScrollBarMin;
    }

    al::setNerve(this, &NrvBasicActionGuideWait);
}

/** @brief Scrolls the page and the scroll bar with the pad's scroll input. */
void BasicActionGuide::exeWait() {
    sead::Vector3f pageTrans = al::getPaneLocalTrans(this, "Pages");
    sead::Vector2f trans(pageTrans.x, pageTrans.y);
    f32 scroll = rc::getPadUiScrollY(mControllerPort);
    f32 barDelta = scroll * cScrollSpeed;

    trans.y = fmaxf(fminf(trans.y + barDelta * mScrollRate, mScrollHeight), 0.0f);
    al::setPaneLocalTrans(this, "Pages", trans);

    sead::Vector3f barTrans = al::getPaneLocalTrans(this, "ScrollBar");
    trans = sead::Vector2f(barTrans.x, barTrans.y);
    trans.y = fminf(fmaxf(trans.y + barDelta, cScrollBarMin), 0.0f);
    al::setPaneLocalTrans(this, "ScrollBar", trans);

    if (scroll != 0.0f && trans.y > cScrollBarMin && trans.y < 0.0f) {
        al::holdSe(mAudioKeeper, "PgScroll");
    }
}

/** @brief Does nothing while the guide is closed. */
void BasicActionGuide::exeEnd() {}
