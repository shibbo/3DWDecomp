#include "Layout/TestMusaLayout.hpp"
#include "Layout/ProjectReplaceTagProcessor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Layout/LayoutPaneGroup.hpp"
#include "Library/Layout/LayoutPartsActorKeeper.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestMusaLayout, Wait);
NERVES_MAKE_NOSTRUCT(TestMusaLayout, Wait)
}
/** @brief Creates the layout test and formats its sample message.
 * @param rInfo Layout initialization context. */
TestMusaLayout::TestMusaLayout(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("テスト武佐レイアウト") {
    al::initLayoutActor(this, rInfo, "TestMusaLayoutBody", nullptr);
    initNerve(&NrvTestMusaLayoutWait, 0);
    appear();
    ProjectReplaceTagProcessor processor;
    char16_t buffer[128];
    processor.replaceArgs(buffer, 128, this,
                          al::getSystemMessageString(this, "TestWindowMessage", "TestNumber"),
                          3, 22, 33, 899);
    setString(buffer);
}
/** @brief Starts the move animation on every registered parts actor. */
void TestMusaLayout::exeWait() {
    if (al::isFirstStep(this)) {
        al::LayoutPartsActorKeeper* pKeeper = getLayoutPartsActorKeeper();
        for (int i = 0; i < pKeeper->getPartsActorNum(); i++)
            pKeeper->getPartsActors()[i]->getLayoutKeeper()->getGroup("Main")->startAnim("Move");
    }
}
/** @brief Performs the original scale state's entry check. */
void TestMusaLayout::exeScale() { al::isFirstStep(this); }
/** @brief Replaces the test's body text.
 * @param pString UTF-16 text to display. */
void TestMusaLayout::setString(const char16_t* pString) {
    al::setPaneString(this, "TxtBody", pString, 0, -1);
}
