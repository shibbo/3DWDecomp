#include "Layout/StaffRollFin.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(StaffRollFin, Hide);
NERVE_DECL(StaffRollFin, Appear);
NERVE_DECL(StaffRollFin, Wait);
NERVE_DECL(StaffRollFin, End);
NERVES_MAKE_NOSTRUCT(StaffRollFin, Hide, Appear, Wait, End)
}  // namespace

/**
 * @brief Creates the staff roll's ending title in its hidden state.
 * @param rInfo Layout initialization context.
 */
StaffRollFin::StaffRollFin(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("スタッフロール『おしまい』") {
    initNerve(&NrvStaffRollFinHide, 0);
    al::initLayoutActor(this, rInfo, "StaffRollFin", nullptr);
    al::startAction(this, "Hide", nullptr);
}

/** @brief Shows the title and starts its appear state. */
void StaffRollFin::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvStaffRollFinAppear);
}

/** @brief Plays the appear action and switches to waiting when it finishes. */
void StaffRollFin::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvStaffRollFinWait);
    }
}

/** @brief Starts the wait action on entry. */
void StaffRollFin::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
}

/** @brief Starts the end action on entry. */
void StaffRollFin::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
}

/** @brief Starts the hide action on entry. */
void StaffRollFin::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }
}

/** @brief Enters the end state. */
void StaffRollFin::startEnd() {
    al::setNerve(this, &NrvStaffRollFinEnd);
}

/**
 * @brief Checks whether the end state's animation has finished.
 * @return Whether the end state is active and its action has ended.
 */
bool StaffRollFin::isEnd() const {
    return al::isNerve(this, &NrvStaffRollFinEnd) && al::isActionEnd(this, nullptr);
}
