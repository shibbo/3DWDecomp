#include "Layout/TestLayoutPartsActor.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(TestLayoutPartsActor, Wait);
NERVE_DECL(TestLayoutPartsActor, Warning);
NERVES_MAKE_NOSTRUCT(TestLayoutPartsActor, Wait, Warning)
}

/** @brief Creates idle test parts inside the parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent layout. */
TestLayoutPartsActor::TestLayoutPartsActor(const al::LayoutInitInfo& rInfo, const char* pName,
                                         const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvTestLayoutPartsActorWait, 0);
}

/** @brief Starts the warning state. */
void TestLayoutPartsActor::start() { al::setNerve(this, &NrvTestLayoutPartsActorWarning); }

/** @brief Leaves the idle parts unchanged. */
void TestLayoutPartsActor::exeWait() {}

/** @brief Starts the warning animation on entry. */
void TestLayoutPartsActor::exeWarning() {
    if (al::isFirstStep(this))
        al::startAction(this, "Warning", nullptr);
}
