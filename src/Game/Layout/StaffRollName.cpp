#include "Layout/StaffRollName.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(StaffRollName, Hide);
NERVE_DECL(StaffRollName, Appear);
NERVE_DECL(StaffRollName, Wait);
NERVE_DECL(StaffRollName, End);
NERVES_MAKE_NOSTRUCT(StaffRollName, Hide, Appear, Wait, End)
}  // namespace

/**
 * @brief Creates the staff roll's name display in its hidden state.
 * @param rInfo Layout initialization context.
 */
StaffRollName::StaffRollName(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("スタッフロール『名前』") {
    initNerve(&NrvStaffRollNameHide, 0);
    al::initLayoutActor(this, rInfo, "StaffRollName", nullptr);
    al::startAction(this, "Hide", nullptr);
}

/** @brief Shows the name display and starts its appear state. */
void StaffRollName::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvStaffRollNameAppear);
}

/** @brief Plays the appear action and switches to waiting when it finishes. */
void StaffRollName::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvStaffRollNameWait);
    }
}

/** @brief Starts the wait action on entry. */
void StaffRollName::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
}

/** @brief Starts the end action on entry. */
void StaffRollName::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
}

/** @brief Starts the hide action on entry. */
void StaffRollName::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }
}

/**
 * @brief Checks whether the end state's animation has finished.
 * @return Whether the end state is active and its action has ended.
 */
bool StaffRollName::isEnd() const {
    return al::isNerve(this, &NrvStaffRollNameEnd) && al::isActionEnd(this, nullptr);
}

/**
 * @brief Converts and displays a narrow staff name.
 * @param pString Null-terminated name, formatted into a 128-character wide buffer.
 */
void StaffRollName::setString(const char* pString) {
    sead::WFormatFixedSafeString<128> text(u"%s", pString);
    setStringW(text.cstr());
}

/**
 * @brief Displays a wide staff name.
 * @param pString Null-terminated UTF-16 name.
 */
void StaffRollName::setStringW(const char16_t* pString) {
    al::setPaneString(this, "TxtName", pString, 0, -1);
}
