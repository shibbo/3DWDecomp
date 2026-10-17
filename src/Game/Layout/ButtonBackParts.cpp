#include "Layout/ButtonBackParts.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
namespace {
NERVE_DECL(ButtonBackParts, Deactive);
NERVE_DECL(ButtonBackParts, Appear);
NERVE_DECL(ButtonBackParts, Wait);
NERVE_DECL(ButtonBackParts, Touch);
NERVE_DECL(ButtonBackParts, End);
NERVE_DECL(ButtonBackParts, Decide);
class ButtonBackPartsNrvDecideEnd : public al::Nerve {
public:
    void execute(al::NerveKeeper*) const override {}
};
NERVES_MAKE_NOSTRUCT(ButtonBackParts, Deactive, Appear, Wait, Touch, End, Decide, DecideEnd)
}
/** @brief Creates a back button with controller and touch input.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 * @param port Controller port. */
ButtonBackParts::ButtonBackParts(const al::LayoutInitInfo& rInfo, const char* pName,
                               const char* pPartsName, al::LayoutActor* pParent, s32 port)
    : al::LayoutActor(pName), mPort(port), mTouchPort(rc::calcTouchPanelPortByPortNum(port)),
      mIsInputEnable(true) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvButtonBackPartsDeactive, 0);
    al::initLayoutPartsAudioKeeper(this, rInfo, "ButtonBackParts");
}
/** @brief Shows the button with the current controller's cancel icon. */
void ButtonBackParts::appear() {
    al::LayoutActor::appear();
    al::setPaneString(this, "TxtIconB", LayoutFontUtil::getMessageFontButtonCancel(mPort), 0, -1);
    al::setNerve(this, &NrvButtonBackPartsAppear);
}
/** @brief Shows an inactive or completed button. @return Whether the button appeared. */
bool ButtonBackParts::tryAppear() {
    if (al::isNerve(this, &NrvButtonBackPartsDeactive) ||
        al::isNerve(this, &NrvButtonBackPartsDecideEnd)) {
        appear();
        return true;
    }
    return false;
}
/** @brief Checks whether the button is active. @return Whether it can still animate or accept input. */
bool ButtonBackParts::isActive() const {
    return !al::isNerve(this, &NrvButtonBackPartsDeactive) &&
           !al::isNerve(this, &NrvButtonBackPartsDecideEnd);
}
/** @brief Restores the waiting state. */
void ButtonBackParts::reactivate() { al::setNerve(this, &NrvButtonBackPartsWait); }
/** @brief Changes controller and corresponding touch port. @param port Controller port. */
void ButtonBackParts::setPort(s32 port) {
    mPort = port;
    mTouchPort = rc::calcTouchPanelPortByPortNum(port);
}
/** @brief Starts hiding an appearing, waiting, or touched button. */
void ButtonBackParts::trySetEndIfActive() {
    if (al::isNerve(this, &NrvButtonBackPartsAppear) ||
        al::isNerve(this, &NrvButtonBackPartsWait) ||
        al::isNerve(this, &NrvButtonBackPartsTouch))
        al::setNerve(this, &NrvButtonBackPartsEnd);
}
/** @brief Checks whether cancellation was selected. @return Whether selection is playing or complete. */
bool ButtonBackParts::isDecide() const {
    return al::isNerve(this, &NrvButtonBackPartsDecide) ||
           al::isNerve(this, &NrvButtonBackPartsDecideEnd);
}
/** @brief Checks selection completion. @return Whether the selection animation ended. */
bool ButtonBackParts::isDecideEnd() const { return al::isNerve(this, &NrvButtonBackPartsDecideEnd); }
/** @brief Holds the inactive state. */
void ButtonBackParts::exeDeactive() {}
/** @brief Plays the appearance animation and enables the waiting state. */
void ButtonBackParts::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonBackPartsWait);
}
/** @brief Accepts cancel input or a touch inside the button. */
void ButtonBackParts::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (!mIsInputEnable) return;
    if (rc::isPadTriggerUiCancelByPort(mPort)) {
        al::setNerve(this, &NrvButtonBackPartsDecide);
        return;
    }
    if (al::isPadTriggerTouch(mTouchPort) && al::isContainPointPane(this, mTouchPort, "Hit"))
        al::setNerve(this, &NrvButtonBackPartsTouch);
}
/** @brief Tracks touch movement and chooses or releases the button. */
void ButtonBackParts::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Touch", nullptr);
        al::startSe(this, "ButtonBackTouched", nullptr);
    }
    if (al::isPadHoldTouch(mTouchPort)) {
        bool isTouch = al::isActionPlaying(this, "Touch", nullptr);
        bool isInside = al::isContainPointPane(this, mTouchPort, "Hit");
        if (isTouch) {
            if (!isInside) {
                al::startAction(this, "Wait", nullptr);
                al::startSe(this, "ButtonBackReleased", nullptr);
            }
        } else if (isInside) {
            al::startAction(this, "Touch", nullptr);
        }
    } else if (al::isContainPointPane(this, mTouchPort, "Hit")) {
        if (mIsInputEnable) al::setNerve(this, &NrvButtonBackPartsDecide);
        else al::setNerve(this, &NrvButtonBackPartsWait);
    } else {
        al::setNerve(this, &NrvButtonBackPartsWait);
        al::startSe(this, "ButtonBackReleased", nullptr);
    }
}
/** @brief Plays the selected animation and sound, then records completion. */
void ButtonBackParts::exeDecide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Decide", nullptr);
        al::startSe(this, "ButtonBackDecided", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonBackPartsDecideEnd);
}
/** @brief Hides the button and returns to the inactive state. */
void ButtonBackParts::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvButtonBackPartsDeactive);
}
