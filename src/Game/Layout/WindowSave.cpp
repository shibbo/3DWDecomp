#include "Layout/WindowSave.hpp"

#include "Layout/ButtonGroup.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(WindowSave, Wait);
NERVE_DECL(WindowSave, Appear);
NERVE_DECL(WindowSave, End);
NERVES_MAKE_NOSTRUCT(WindowSave, Wait, Appear, End)
}  // namespace

/**
 * @brief Creates an initially hidden save-result window and its button controller.
 * @param rInfo Layout initialization context.
 */
WindowSave::WindowSave(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("セーブ完了ウインドウ") {
    al::initLayoutActor(this, rInfo, "WindowSave", nullptr);
    initNerve(&NrvWindowSaveWait, 0);
    mButtonGroup = new ButtonGroup(rInfo, this, "WindowSave", nullptr, false);
    kill();
}

/**
 * @brief Shows the window with its acknowledgement button selected.
 * @param padPort Controller port used for the button group.
 */
void WindowSave::appearWindow(int padPort) {
    al::LayoutActor::appear();
    mPadPort = padPort;
    mButtonGroup->setPort(padPort);
    mButtonGroup->hideCursor();
    mButtonGroup->reset();
    mButtonGroup->select("ボタン");
    al::setNerve(this, &NrvWindowSaveAppear);
}

/** @brief Enables the cursor after the appear action and at least 31 state frames. */
void WindowSave::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isGreaterStep(this, 30) && al::isActionEnd(this, nullptr)) {
        mButtonGroup->showCursor();
        al::setNerve(this, &NrvWindowSaveWait);
    }
}

/** @brief Updates the acknowledgement button and begins closing when it is confirmed. */
void WindowSave::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }
    mButtonGroup->updateAndCursorDefault(mPadPort);
    if (mButtonGroup->isDecide("ボタン")) {
        mButtonGroup->hideCursor();
        al::setNerve(this, &NrvWindowSaveEnd);
    }
}

/** @brief Plays the end action and hides the window when it finishes. */
void WindowSave::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}
