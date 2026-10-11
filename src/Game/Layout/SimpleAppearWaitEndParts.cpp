#include "Layout/SimpleAppearWaitEndParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(SimpleAppearWaitEndParts, Wait);
NERVE_DECL(SimpleAppearWaitEndParts, Appear);
NERVE_DECL(SimpleAppearWaitEndParts, End);
NERVES_MAKE_NOSTRUCT(SimpleAppearWaitEndParts, Wait, Appear, End)
}  // namespace

/**
 * @brief Creates layout parts that return to idle after their appear and end actions.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 */
SimpleAppearWaitEndParts::SimpleAppearWaitEndParts(const al::LayoutInitInfo& rInfo,
                                                 const char* pName, const char* pPartsName,
                                                 al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvSimpleAppearWaitEndPartsWait, 0);
}

/** @brief Shows the parts and enters the appear state. */
void SimpleAppearWaitEndParts::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvSimpleAppearWaitEndPartsAppear);
}

/** @brief Enters the end state. */
void SimpleAppearWaitEndParts::end() {
    al::setNerve(this, &NrvSimpleAppearWaitEndPartsEnd);
}

/**
 * @brief Checks whether the parts are idle.
 * @return Whether the wait state is active.
 */
bool SimpleAppearWaitEndParts::isWait() const {
    return al::isNerve(this, &NrvSimpleAppearWaitEndPartsWait);
}

/** @brief Plays the appear action and returns to idle when it finishes. */
void SimpleAppearWaitEndParts::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvSimpleAppearWaitEndPartsWait);
    }
}

/** @brief Keeps the current pose while idle. */
void SimpleAppearWaitEndParts::exeWait() {}

/** @brief Plays the end action and returns to idle when it finishes. */
void SimpleAppearWaitEndParts::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvSimpleAppearWaitEndPartsWait);
    }
}
