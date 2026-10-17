#include "Layout/ButtonShortCutAndTouchParts.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(ButtonShortCutAndTouchParts, Deactive);
NERVE_DECL(ButtonShortCutAndTouchParts, Appear);
NERVE_DECL(ButtonShortCutAndTouchParts, Hide);
NERVE_DECL(ButtonShortCutAndTouchParts, End);
NERVE_DECL(ButtonShortCutAndTouchParts, Wait);
NERVE_DECL(ButtonShortCutAndTouchParts, Decide);
NERVE_DECL(ButtonShortCutAndTouchParts, Touch);
class ButtonShortCutAndTouchPartsNrvDecideEnd : public al::Nerve {
public:
    void execute(al::NerveKeeper*) const override {}
};
NERVES_MAKE_NOSTRUCT(ButtonShortCutAndTouchParts, Deactive, Appear, Hide, End, Wait, Decide, DecideEnd, Touch)
/** @brief Tests a port against the optional active-input mask. */
inline bool isInputPortActive(GameDataHolder* pGameData, int port) {
    return !pGameData || static_cast<u16>(rc::getActiveInputPortList(GameDataHolderAccessor(pGameData)) & (1 << port));
}
}
/** @brief Creates shortcut and touch button parts.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 * @param port Controller port.
 * @param type Shortcut button to accept. */
ButtonShortCutAndTouchParts::ButtonShortCutAndTouchParts(const al::LayoutInitInfo& rInfo,
    const char* pName, const char* pPartsName, al::LayoutActor* pParent, s32 port, ShortCutType type)
    : al::LayoutActor(pName), mShortCutType(type), mPort(port),
      mTouchPort(rc::calcTouchPanelPortByPortNum(port)), mIsInputEnable(true) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvButtonShortCutAndTouchPartsDeactive, 0);
}
/** @brief Shows the button with its appearance animation. */
void ButtonShortCutAndTouchParts::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvButtonShortCutAndTouchPartsAppear);
}
/** @brief Updates controller and touch ports. @param port Controller port. */
void ButtonShortCutAndTouchParts::setPort(s32 port) {
    mPort = port;
    mTouchPort = rc::calcTouchPanelPortByPortNum(port);
}
/** @brief Selects the hidden state. */
void ButtonShortCutAndTouchParts::hide() { al::setNerve(this, &NrvButtonShortCutAndTouchPartsHide); }
/** @brief Starts the end animation. */
void ButtonShortCutAndTouchParts::end() { al::setNerve(this, &NrvButtonShortCutAndTouchPartsEnd); }
/** @brief Restores the waiting state. */
void ButtonShortCutAndTouchParts::wait() { al::setNerve(this, &NrvButtonShortCutAndTouchPartsWait); }
/** @brief Selects the button unless a touch is ongoing. @return Whether selection started. */
bool ButtonShortCutAndTouchParts::tryDecide() {
    if (isTouch()) return false;
    al::setNerve(this, &NrvButtonShortCutAndTouchPartsDecide);
    return true;
}
/** @brief Checks touch state. @return Whether the button is touched. */
bool ButtonShortCutAndTouchParts::isTouch() const { return al::isNerve(this, &NrvButtonShortCutAndTouchPartsTouch); }
/** @brief Checks selection state. @return Whether selection is playing or complete. */
bool ButtonShortCutAndTouchParts::isDecide() const {
    return al::isNerve(this, &NrvButtonShortCutAndTouchPartsDecide) ||
           al::isNerve(this, &NrvButtonShortCutAndTouchPartsDecideEnd);
}
/** @brief Checks selection completion. @return Whether selection has finished. */
bool ButtonShortCutAndTouchParts::isDecideEnd() const { return al::isNerve(this, &NrvButtonShortCutAndTouchPartsDecideEnd); }
/** @brief Checks idle state. @return Whether the button is waiting. */
bool ButtonShortCutAndTouchParts::isWait() const { return al::isNerve(this, &NrvButtonShortCutAndTouchPartsWait); }
/** @brief Checks hidden state. @return Whether the button is hidden. */
bool ButtonShortCutAndTouchParts::isHide() const { return al::isNerve(this, &NrvButtonShortCutAndTouchPartsHide); }
/** @brief Checks whether the button remains active. @return Whether it is neither inactive, complete, nor hidden. */
bool ButtonShortCutAndTouchParts::isActive() const {
    return !al::isNerve(this, &NrvButtonShortCutAndTouchPartsDeactive) &&
           !al::isNerve(this, &NrvButtonShortCutAndTouchPartsDecideEnd) &&
           !al::isNerve(this, &NrvButtonShortCutAndTouchPartsHide);
}
/** @brief Holds the inactive state. */
void ButtonShortCutAndTouchParts::exeDeactive() {}
/** @brief Plays appearance before waiting for input. */
void ButtonShortCutAndTouchParts::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonShortCutAndTouchPartsWait);
}
/** @brief Accepts shortcut or touch input from active ports. */
void ButtonShortCutAndTouchParts::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (!mIsInputEnable) return;
    if (isInputPortActive(mGameData, mPort)) {
        switch (mShortCutType) {
        case ShortCutType_Minus:
            if (al::isPadTriggerMinus(mPort)) {
                al::setNerve(this, &NrvButtonShortCutAndTouchPartsDecide);
                return;
            }
            break;
        case ShortCutType_Plus:
            if (al::isPadTriggerPlus(mPort)) {
                al::setNerve(this, &NrvButtonShortCutAndTouchPartsDecide);
                return;
            }
            break;
        }
    }
    if (isInputPortActive(mGameData, mTouchPort) && al::isPadTriggerTouch(mTouchPort) &&
        al::isContainPointPane(this, mTouchPort, "Hit"))
        al::setNerve(this, &NrvButtonShortCutAndTouchPartsTouch);
}
/** @brief Tracks touch movement, selection, and release. */
void ButtonShortCutAndTouchParts::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", nullptr);
        al::tryStartSe(this, "TouchedPG", nullptr);
    }
    int port = al::getMainControllerPort();
    if (!isInputPortActive(mGameData, port)) {
        al::setNerve(this, &NrvButtonShortCutAndTouchPartsWait);
        al::tryStartSe(this, "ReleasedPG", nullptr);
        return;
    }
    if (al::isPadHoldTouch(mTouchPort)) {
        bool isTouch = al::isActionPlaying(this, "Touch", nullptr);
        bool isInside = al::isContainPointPane(this, mTouchPort, "Hit");
        if (isTouch) {
            if (!isInside) al::startAction(this, "Wait", nullptr);
        } else if (isInside) al::startAction(this, "Touch", nullptr);
    } else if (al::isContainPointPane(this, mTouchPort, "Hit")) {
        al::setNerve(this, &NrvButtonShortCutAndTouchPartsDecide);
    } else {
        al::setNerve(this, &NrvButtonShortCutAndTouchPartsWait);
        al::tryStartSe(this, "ReleasedPG", nullptr);
    }
}
/** @brief Plays selection feedback when the sound exists. */
void ButtonShortCutAndTouchParts::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Decide", nullptr);
        if (al::isExistSePlayNameInUserInfo(this, "DecidedPG"))
            al::tryStartSe(this, "DecidedPG", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonShortCutAndTouchPartsDecideEnd);
}
/** @brief Starts the hidden animation. */
void ButtonShortCutAndTouchParts::exeHide() {
    if (al::isFirstStep(this)) al::startAction(this, "Hide", nullptr);
}
/** @brief Plays the end animation and becomes inactive. */
void ButtonShortCutAndTouchParts::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonShortCutAndTouchPartsDeactive);
}
