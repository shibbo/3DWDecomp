#include "Layout/GuideWindowParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(GuideWindowParts, Wait);
NERVE_DECL(GuideWindowParts, Appear);
NERVE_DECL(GuideWindowParts, End);
NERVES_MAKE_NOSTRUCT(GuideWindowParts, Wait, Appear, End)
}  // namespace

/**
 * @brief Creates a guide window embedded in a parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 */
GuideWindowParts::GuideWindowParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                 const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvGuideWindowPartsWait, 0);
}

/** @brief Starts the requested setup action before appearing.
 * @param pActionName Initial layout action. */
void GuideWindowParts::appear(const char* pActionName) {
    al::startAction(this, pActionName, nullptr);
    al::setNerve(this, &NrvGuideWindowPartsAppear);
}

/** @brief Sets the guide's displayed text.
 * @param pText Null-terminated UTF-16 guide text. */
void GuideWindowParts::setText(const char16_t* pText) {
    al::setPaneString(this, "TxtGuide", pText, 0, -1);
}

/** @brief Starts the closing state. */
void GuideWindowParts::end() { al::setNerve(this, &NrvGuideWindowPartsEnd); }

/** @brief Checks whether no transition is playing.
 * @return Whether the wait state is active. */
bool GuideWindowParts::isWait() const { return al::isNerve(this, &NrvGuideWindowPartsWait); }

/** @brief Allows one setup frame before playing the appearance animation. */
void GuideWindowParts::exeAppear() {
    if (al::isFirstStep(this)) {
        return;
    }
    if (al::isStep(this, 1)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvGuideWindowPartsWait);
    }
}

/** @brief Holds the current layout action. */
void GuideWindowParts::exeWait() {}

/** @brief Plays the end animation and returns to the idle state. */
void GuideWindowParts::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvGuideWindowPartsWait);
    }
}
