#include "Layout/WindowProcessing.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
NERVE_DECL(WindowProcessing, Wait);
NERVE_DECL(WindowProcessing, Appear);
NERVE_DECL(WindowProcessing, End);
NERVES_MAKE_NOSTRUCT(WindowProcessing, Wait, Appear, End)
}  // namespace

/**
 * @brief Creates an initially hidden processing window.
 * @param rInfo Layout initialization context.
 * @param pName Optional layout archive suffix.
 */
WindowProcessing::WindowProcessing(const al::LayoutInitInfo& rInfo, const char* pName)
    : al::LayoutActor("処理待ちウインドウ") {
    al::initLayoutActor(this, rInfo, "WindowProcessing", pName);
    initNerve(&NrvWindowProcessingWait, 0);
    kill();
}

/**
 * @brief Displays a system message while processing an operation.
 * @param pCategory Message file name.
 * @param pLabel Message label.
 * @param minFrame Minimum wait-state duration; zero selects 50 frames.
 * @param isUseSound Whether to play processing audio and use the lower window position.
 */
void WindowProcessing::appearWithSystemMessage(const char* pCategory, const char* pLabel,
                                               int minFrame, bool isUseSound) {
    mIsRequestClose = false;
    mIsUseSound = isUseSound;
    al::setPaneSystemMessage(this, "TxtGuide", pCategory, pLabel);
    if (isUseSound) {
        al::startAction(this, "SetPositionLow", "Mode");
    } else {
        al::startAction(this, "SetPositionHigh", "Mode");
    }
    mMinFrame = minFrame == 0 ? 50 : minFrame;
    al::startAction(this, "Appear", nullptr);
    al::startFreezeAction(this, "Mario", 0.0f, "Mario");
    al::LayoutActor::appear();
    al::setNerve(this, &NrvWindowProcessingAppear);
}

/**
 * @brief Checks whether the closing state is active.
 * @return Whether the end state has begun, even if its animation is still playing.
 */
bool WindowProcessing::isEnd() const {
    return al::isNerve(this, &NrvWindowProcessingEnd);
}

/** @brief Plays the appearance animation before entering the processing loop. */
void WindowProcessing::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startFreezeAction(this, "Mario", 0.0f, "Mario");
    }
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvWindowProcessingWait);
    }
}

/** @brief Loops the processing animation until closure is requested and the delay expires. */
void WindowProcessing::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
        al::startAction(this, "Mario", "Mario");
        if (mIsUseSound) {
            al::startSe(this, "Processing", nullptr);
        }
    }
    if (mIsRequestClose && !al::isLessStep(this, mMinFrame)) {
        if (mIsUseSound) {
            al::stopSeByName(this, "Processing");
        }
        al::setNerve(this, &NrvWindowProcessingEnd);
    }
}

/** @brief Plays the closing animation and kills the layout when it finishes. */
void WindowProcessing::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }
    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}
