#include "Layout/StaffRollJob.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(StaffRollJob, Hide);
NERVE_DECL(StaffRollJob, Appear);
NERVE_DECL(StaffRollJob, Wait);
NERVE_DECL(StaffRollJob, End);
NERVES_MAKE_NOSTRUCT(StaffRollJob, Hide, Appear, Wait, End)
}  // namespace

/**
 * @brief Creates the staff roll's job heading in its hidden state.
 * @param rInfo Layout initialization context.
 */
StaffRollJob::StaffRollJob(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("スタッフロール『職種』") {
    initNerve(&NrvStaffRollJobHide, 0);
    al::initLayoutActor(this, rInfo, "StaffRollJob", nullptr);
    al::startAction(this, "Hide", nullptr);
}

/** @brief Shows the job heading and starts its appear state. */
void StaffRollJob::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvStaffRollJobAppear);
}

/** @brief Plays the appear action and switches to waiting when it finishes. */
void StaffRollJob::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvStaffRollJobWait);
    }
}

/** @brief Starts the wait action on entry. */
void StaffRollJob::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
}

/** @brief Starts the end action on entry. */
void StaffRollJob::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
}

/** @brief Starts the hide action on entry. */
void StaffRollJob::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }
}

/**
 * @brief Checks whether the end state's animation has finished.
 * @return Whether the end state is active and its action has ended.
 */
bool StaffRollJob::isEnd() const {
    return al::isNerve(this, &NrvStaffRollJobEnd) && al::isActionEnd(this, nullptr);
}

/**
 * @brief Converts and displays a narrow job heading.
 * @param pString Null-terminated job title, formatted into a 128-character wide buffer.
 */
void StaffRollJob::setString(const char* pString) {
    sead::WFormatFixedSafeString<128> text(u"%s", pString);
    setStringW(text.cstr());
}

/**
 * @brief Displays a wide job heading.
 * @param pString Null-terminated UTF-16 job title.
 */
void StaffRollJob::setStringW(const char16_t* pString) {
    al::setPaneString(this, "TxtJob", pString, 0, -1);
}
