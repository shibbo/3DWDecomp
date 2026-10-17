#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(DemoSkipLayout, Hidden);
NERVE_DECL(DemoSkipLayout, Appear);
NERVE_DECL(DemoSkipLayout, End);
NERVE_DECL(DemoSkipLayout, Wait);
NERVES_MAKE_NOSTRUCT(DemoSkipLayout, Hidden, Appear, End, Wait)
}
/** @brief Creates the skip prompt in its hidden state. */
DemoSkipLayout::DemoSkipLayout(const al::LayoutInitInfo& rInfo, bool isSingleMode)
    : al::LayoutActor("RCS_SceneSkip"), mIsSingleMode(isSingleMode) {
    al::initLayoutActor(this, rInfo, "RCS_SceneSkip", nullptr);
    initNerve(&NrvDemoSkipLayoutHidden, 0);
}
/** @brief Resets the prompt to its hidden state. */
void DemoSkipLayout::startHidden() {
    mIsForcedEnd = false;
    al::setNerve(this, &NrvDemoSkipLayoutHidden);
}
/** @brief Shows the prompt once the hidden animation has completed. */
void DemoSkipLayout::show() {
    if (al::isNerve(this, &NrvDemoSkipLayoutHidden) && al::isActionEnd(this, "Main"))
        al::setNerve(this, &NrvDemoSkipLayoutAppear);
}
/** @brief Reports whether the prompt is still visible. */
bool DemoSkipLayout::isVisible() {
    return !(al::isNerve(this, &NrvDemoSkipLayoutHidden) && al::isActionEnd(this, "Main"));
}
/** @brief Starts hiding a visible prompt. */
void DemoSkipLayout::end() {
    if (al::isNerve(this, &NrvDemoSkipLayoutHidden) && al::isActionEnd(this, "Main")) return;
    al::setNerve(this, &NrvDemoSkipLayoutEnd);
}
/** @brief Forces the exit animation or resets the prompt after clearing the override. */
void DemoSkipLayout::setForcedEnd(bool isForced) {
    if (isForced == mIsForcedEnd) return;
    mIsForcedEnd = isForced;
    bool visible = isVisible();
    if (isForced) {
        if (visible) al::startAction(this, "End", nullptr);
    } else if (visible) startHidden();
}
/** @brief Reports whether the prompt accepts a skip confirmation. */
bool DemoSkipLayout::isWait() { return al::isNerve(this, &NrvDemoSkipLayoutWait); }
/** @brief Shows or confirms the prompt using any eligible controller. */
bool DemoSkipLayout::isSkip(sead::BitFlag<u16> ports) {
    if (mIsSingleMode && al::tryGetSceneObj(this, 8) &&
        SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor(this)))
        ports.setBit(rc::getPadPortByUserId(1));
    bool skipPressed = false;
    bool anyPressed = false;
    for (int port = 0; port <= al::getMaxControllerPorts(); ++port) {
        if (ports.isOnBit(port) && al::isPadConnected(port)) {
            skipPressed |= al::isPadTriggerPlus(port) || al::isPadTriggerMinus(port);
            anyPressed |= al::isPadTriggerAny(port);
        }
    }
    if (skipPressed) {
        if (isWait()) return true;
        show();
    } else if (anyPressed && al::isGreaterEqualStep(this, 5)) show();
    return false;
}
/** @brief Shows the prompt unless the exit override is active. */
void DemoSkipLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        if (mIsForcedEnd) al::setNerve(this, &NrvDemoSkipLayoutWait);
        else al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvDemoSkipLayoutWait);
}
/** @brief Hides the prompt after four seconds without confirmation. */
void DemoSkipLayout::exeWait() {
    if (al::isGreaterEqualStep(this, 240)) al::setNerve(this, &NrvDemoSkipLayoutEnd);
}
/** @brief Places the exit animation at its final frame. */
void DemoSkipLayout::exeHidden() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        al::setActionFrame(this, al::getActionFrameMax(this, nullptr), nullptr);
    }
}
/** @brief Completes the exit animation, or immediately hides a forced exit. */
void DemoSkipLayout::exeEnd() {
    if (al::isFirstStep(this)) {
        if (mIsForcedEnd) {
            al::setNerve(this, &NrvDemoSkipLayoutHidden);
            return;
        }
        al::startAction(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvDemoSkipLayoutHidden);
}
