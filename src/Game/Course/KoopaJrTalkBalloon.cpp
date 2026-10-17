#include "Course/KoopaJrTalkBalloon.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(KoopaJrTalkBalloon, Wait);
NERVES_MAKE_NOSTRUCT(KoopaJrTalkBalloon, Wait)
} // namespace

/**
 * @brief Creates Bowser Jr.'s talk balloon with the standard appear/end actions.
 * @param pName Layout actor name.
 * @param pLayoutName Layout resource name.
 * @param rInfo Layout initialization context.
 */
KoopaJrTalkBalloon::KoopaJrTalkBalloon(const char* pName, const char* pLayoutName,
                                     const al::LayoutInitInfo& rInfo)
    : al::SimpleLayoutAppearWaitEnd(pName, pLayoutName, rInfo, nullptr, false) {}

/** @brief Enters the thought animation state after the appear action ends. */
void KoopaJrTalkBalloon::exeAppear() {
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKoopaJrTalkBalloonWait);
    }
}

/** @brief Starts the balloon's waiting thought animation. */
void KoopaJrTalkBalloon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitThought");
    }
}
