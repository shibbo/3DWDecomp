#include "Layout/Switch/SingleModeCounterBase.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(SingleModeCounterBase, Hide);
NERVE_DECL(SingleModeCounterBase, Appear);
NERVE_DECL(SingleModeCounterBase, Wait);
NERVES_MAKE_NOSTRUCT(SingleModeCounterBase, Hide, Appear, Wait)
}
/** @brief Creates counter parts initially in the hide state.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent HUD layout. */
SingleModeCounterBase::SingleModeCounterBase(const al::LayoutInitInfo& rInfo, const char* pName,
                                           const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvSingleModeCounterBaseHide, 0);
}
/** @brief Shows an inactive counter or reverses its hide state. */
void SingleModeCounterBase::show() {
    if (isAlive()) {
        if (al::isNerve(this, &NrvSingleModeCounterBaseHide))
            al::setNerve(this, &NrvSingleModeCounterBaseAppear);
    } else {
        al::setNerve(this, &NrvSingleModeCounterBaseAppear);
        appear();
    }
}
/** @brief Starts the hide animation unless already hiding. */
void SingleModeCounterBase::hide() {
    if (al::isNerve(this, &NrvSingleModeCounterBaseHide))
        return;
    al::setNerve(this, &NrvSingleModeCounterBaseHide);
}
/** @brief Checks whether the counter is in its hide state. */
bool SingleModeCounterBase::isHide() { return al::isNerve(this, &NrvSingleModeCounterBaseHide); }
/** @brief Plays the entrance animation and then waits. */
void SingleModeCounterBase::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Show", "Main");
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvSingleModeCounterBaseWait);
}
/** @brief Keeps the visible counter steady. */
void SingleModeCounterBase::exeWait() {}
/** @brief Plays the exit animation and kills the counter when it finishes. */
void SingleModeCounterBase::exeHide() {
    if (al::isFirstStep(this))
        al::startAction(this, "Hide", nullptr);
    if (al::isActionEnd(this, nullptr))
        kill();
}
