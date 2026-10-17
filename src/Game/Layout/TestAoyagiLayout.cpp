#include "Layout/TestAoyagiLayout.hpp"
#include "Layout/TestLayoutPartsActor.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestAoyagiLayout, Wait);
NERVE_DECL(TestAoyagiLayout, TextAppear);
NERVE_DECL(TestAoyagiLayout, TextWait);
NERVE_DECL(TestAoyagiLayout, TextEnd);
NERVES_MAKE_NOSTRUCT(TestAoyagiLayout, Wait, TextAppear, TextWait, TextEnd)
}
/** @brief Creates the test parts and sample text. */
TestAoyagiLayout::TestAoyagiLayout(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("テスト青柳レイアウト") {
    al::initLayoutActor(this, rInfo, "TestAoyagiLayout", nullptr);
    initNerve(&NrvTestAoyagiLayoutWait, 0);
    mLeft = new TestLayoutPartsActor(rInfo, "Left", "PartsLeft", this);
    mRight = new TestLayoutPartsActor(rInfo, "Right", "PartsRight", this);
    mUp = new TestLayoutPartsActor(rInfo, "Up", "PartsUp", this);
    mDown = new TestLayoutPartsActor(rInfo, "Down", "PartsDown", this);
    mParts = new al::LayoutActor("Down");
    al::initLayoutPartsActor(mParts, this, rInfo, "Parts_00", nullptr);
    al::setPaneString(mLeft, "TextBox_00", u"ABCDEFGHIJ", 0, -1);
    al::setPaneString(mDown, "TextBox_00", u"1", 0, -1);
    appear();
}
/** @brief The test layout needs no per-frame control outside its states. */
void TestAoyagiLayout::control() {}
/** @brief Starts test animations in response to controller input. */
void TestAoyagiLayout::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (al::isPadTriggerB(5)) al::setNerve(this, &NrvTestAoyagiLayoutTextAppear);
    if (al::isPadTriggerUp(5)) mUp->start();
    if (al::isPadTriggerDown(5)) mDown->start();
    if (al::isPadTriggerLeft(5)) mLeft->start();
    if (al::isPadTriggerRight(5)) mRight->start();
}
/** @brief Runs the text entrance animation. */
void TestAoyagiLayout::exeTextAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvTestAoyagiLayoutTextWait);
}
/** @brief Holds the text for sixty frames. */
void TestAoyagiLayout::exeTextWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (al::isGreaterEqualStep(this, 60)) al::setNerve(this, &NrvTestAoyagiLayoutTextEnd);
}
/** @brief Returns to input after the text exit animation. */
void TestAoyagiLayout::exeTextEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvTestAoyagiLayoutWait);
}
