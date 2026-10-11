#include "Layout/StaffRollNintendo.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(StaffRollNintendo, Hide);
NERVE_DECL(StaffRollNintendo, Appear);
NERVE_DECL(StaffRollNintendo, Wait);
NERVE_DECL(StaffRollNintendo, End);
NERVES_MAKE_NOSTRUCT(StaffRollNintendo, Hide, Appear, Wait, End)
}  // namespace

/**
 * @brief Creates the staff roll's copyright notice in its hidden state.
 * @param rInfo Layout initialization context.
 */
StaffRollNintendo::StaffRollNintendo(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("スタッフロール『権利表記』") {
    initNerve(&NrvStaffRollNintendoHide, 0);
    al::initLayoutActor(this, rInfo, "StaffRollNintendo", nullptr);
    al::startAction(this, "Hide", nullptr);
}

/** @brief Shows the notice and starts its appear state. */
void StaffRollNintendo::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvStaffRollNintendoAppear);
}

/** @brief Plays the appear action and switches to waiting when it finishes. */
void StaffRollNintendo::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvStaffRollNintendoWait);
    }
}

/** @brief Starts the wait action on entry. */
void StaffRollNintendo::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
}

/** @brief Starts the end action on entry. */
void StaffRollNintendo::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
}

/** @brief Starts the hide action on entry. */
void StaffRollNintendo::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }
}

/**
 * @brief Checks whether the end state's animation has finished.
 * @return Whether the end state is active and its action has ended.
 */
bool StaffRollNintendo::isEnd() const {
    return al::isNerve(this, &NrvStaffRollNintendoEnd) && al::isActionEnd(this, nullptr);
}

/** @brief Enters the end state. */
void StaffRollNintendo::startEnd() {
    al::setNerve(this, &NrvStaffRollNintendoEnd);
}
