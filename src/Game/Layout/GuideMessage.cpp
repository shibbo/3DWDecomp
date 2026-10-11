#include "Layout/GuideMessage.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadVector.h>
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

/// Declares a guide message nerve whose state function may differ from the nerve's name.
#define GUIDE_MESSAGE_NERVE_DECL(Action, Func)                                                     \
    class GuideMessageNrv##Action : public al::Nerve {                                             \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<GuideMessage>())->exe##Func();                                     \
        }                                                                                          \
    };

namespace {
GUIDE_MESSAGE_NERVE_DECL(Appear, Appear)
GUIDE_MESSAGE_NERVE_DECL(WaitToChangeText, WaitToChangeText)
GUIDE_MESSAGE_NERVE_DECL(Wait, Wait)
GUIDE_MESSAGE_NERVE_DECL(End, End)
GUIDE_MESSAGE_NERVE_DECL(ChangeText, ChangeText)
GUIDE_MESSAGE_NERVE_DECL(HideAppear, Hide)
GUIDE_MESSAGE_NERVE_DECL(WaitToChangeTextAssist, WaitToChangeText)
GUIDE_MESSAGE_NERVE_DECL(ChangeTextAssist, ChangeText)
GUIDE_MESSAGE_NERVE_DECL(HideWait, Hide)
GUIDE_MESSAGE_NERVE_DECL(HideEnd, Hide)
GUIDE_MESSAGE_NERVE_DECL(AppearFromHide, Appear)
GUIDE_MESSAGE_NERVE_DECL(EndFromHide, End)

NERVES_MAKE_NOSTRUCT(GuideMessage, Appear, WaitToChangeText, Wait, End, ChangeText, HideAppear,
                     WaitToChangeTextAssist, ChangeTextAssist, HideWait, HideEnd, AppearFromHide,
                     EndFromHide)

/// Frames the 2P assist controls guide stays on screen.
constexpr s32 cAssistTextFrame = 720;
}  // namespace

/**
 * @brief Creates the guide message layout; it starts hidden.
 * @param rInfo Layout initialization context.
 */
GuideMessage::GuideMessage(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("ガイド表示レイアウト") {
    al::initLayoutActor(this, rInfo, "GuideMessage", nullptr);
    initNerve(&NrvGuideMessageAppear, 0);
    kill();
}

/**
 * @brief Shows an already resolved message.
 * @param pText Message text.
 * @param frame Frames to stay on screen (negative: until ended).
 * @param posY Vertical position of the window.
 */
void GuideMessage::startAppear(const char16_t* pText, s32 frame, f32 posY) {
    if (isAlive()) {
        return;
    }

    mIsHide = false;
    mFrame = frame;
    al::setNerve(this, &NrvGuideMessageAppear);
    al::tryStartAction(this, "Hide", nullptr);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor(this))) {
        sead::FixedSafeString<64> text;
        text.convertFromWideCharString(sead::WSafeString(pText), -1);
        if (al::searchSubString(text.cstr(), "BowserJr") != nullptr) {
            al::startAction(this, "SetTextIconKoopaJr", "TextMode");
        } else {
            al::startAction(this, "SetTextIconMario", "TextMode");
        }
    } else {
        al::startAction(this, "SetTextSingle", "TextMode");
    }

    al::setPaneString(this, "TxtGuide", pText, 0, -1);
    appear();
    al::setLocalTrans(this, sead::Vector3f(0.0f, posY, 0.0f));
}

/**
 * @brief Shows a system message, or replaces the shown one.
 * @param pCategory System message file name.
 * @param pLabel System message label.
 * @param frame Frames to stay on screen (negative: until ended).
 * @param posY Vertical position of the window.
 * @param isForce Whether to replace the message when the window is already shown.
 */
void GuideMessage::startAppear(const char* pCategory, const char* pLabel, s32 frame, f32 posY,
                               bool isForce) {
    if (!isAlive()) {
        mIsHide = false;
        mFrame = frame;
        al::setNerve(this, &NrvGuideMessageAppear);
        al::tryStartAction(this, "Hide", nullptr);
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
            SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor(this))) {
            if (al::searchSubString(pLabel, "BowserJr") != nullptr) {
                al::startAction(this, "SetTextIconKoopaJr", "TextMode");
            } else {
                al::startAction(this, "SetTextIconMario", "TextMode");
            }
        } else {
            al::startAction(this, "SetTextSingle", "TextMode");
        }

        al::setPaneString(this, "TxtGuide", al::getSystemMessageString(this, pCategory, pLabel), 0,
                          -1);
        appear();
        al::setLocalTrans(this, sead::Vector3f(0.0f, posY, 0.0f));
        return;
    }

    if (!isForce) {
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageAppear)) {
        mCategory.format(pCategory);
        mLabel.format(pLabel);
        al::setNerve(this, &NrvGuideMessageWaitToChangeText);
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageWait) ||
        al::isNerve(this, &NrvGuideMessageChangeText)) {
        al::setNerve(this, &NrvGuideMessageChangeText);
        mCategory.format(pCategory);
        mLabel.format(pLabel);
        return;
    }

    if (!mIsHide) {
        return;
    }

    al::startFreezeAction(this, "Appear", 0.0f, nullptr);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        SingleModeDataFunction::getIs2PAssistMode(GameDataHolderAccessor(this))) {
        if (al::isEqualSubString("BowserJr", pLabel)) {
            al::startAction(this, "SetTextIconKoopaJr", "TextMode");
        } else {
            al::startAction(this, "SetTextIconMario", "TextMode");
        }
    } else {
        al::startAction(this, "SetTextSingle", "TextMode");
    }

    al::setPaneString(this, "TxtGuide", al::getSystemMessageString(this, pCategory, pLabel),
                      0, -1);
    mFrame = frame;
    al::setNerve(this, &NrvGuideMessageHideAppear);
}

/**
 * @brief Shows two messages side by side.
 * @param pTextLeft Message of the left pane.
 * @param pTextRight Message of the right pane.
 * @param frame Frames to stay on screen (negative: until ended).
 * @param posY Vertical position of the window.
 */
void GuideMessage::startAppearSplit(const char16_t* pTextLeft, const char16_t* pTextRight,
                                    s32 frame, f32 posY) {
    if (isAlive()) {
        return;
    }

    mIsHide = false;
    mFrame = frame;
    al::setNerve(this, &NrvGuideMessageAppear);
    al::tryStartAction(this, "Hide", nullptr);
    al::startAction(this, "SetTextDual", "TextMode");
    al::setPaneString(this, "TxtGuideLeft", pTextLeft, 0, -1);
    al::setPaneString(this, "TxtGuideRight", pTextRight, 0, -1);
    appear();
    al::setLocalTrans(this, sead::Vector3f(0.0f, posY, 0.0f));
}

/** @brief Shows the 2P assist controls guide, or switches the shown window to it. */
void GuideMessage::startAppearAssist() {
    if (isAlive()) {
        if (al::isNerve(this, &NrvGuideMessageAppear)) {
            al::setNerve(this, &NrvGuideMessageWaitToChangeTextAssist);
            return;
        }

        if (al::isNerve(this, &NrvGuideMessageWait) ||
            al::isNerve(this, &NrvGuideMessageChangeText)) {
            al::setNerve(this, &NrvGuideMessageChangeTextAssist);
            return;
        }

        if (!mIsHide) {
            return;
        }

        al::startFreezeAction(this, "Appear", 0.0f, nullptr);
        al::startAction(this, "SetTextIconDual", "TextMode");
        setAssistText();
        mFrame = cAssistTextFrame;
        al::setNerve(this, &NrvGuideMessageHideAppear);
        return;
    }

    mIsHide = false;
    mFrame = cAssistTextFrame;
    al::setNerve(this, &NrvGuideMessageAppear);
    al::tryStartAction(this, "Hide", nullptr);
    al::startAction(this, "SetTextIconDual", "TextMode");
    setAssistText();
    appear();
}

/** @brief Sets the controls guide of both players, matching their controller types. */
void GuideMessage::setAssistText() {
    s32 marioPort = al::getMainControllerPort();
    s32 bowserJrPort = rc::getPadPortByUserId(1);
    if (al::isPadTypeJoySingle(marioPort)) {
        al::setPaneSystemMessage(this, "TxtGuideAssistMario", "SingleMode_GuideMessage",
                                 "PlayerControlsGuide_SingleJoycons");
    } else {
        al::setPaneSystemMessage(this, "TxtGuideAssistMario", "SingleMode_GuideMessage",
                                 "PlayerControlsGuide_DualJoycons");
    }

    if (al::isPadTypeJoySingle(bowserJrPort)) {
        al::setPaneSystemMessage(this, "TxtGuideAssistBowserJr", "SingleMode_GuideMessage",
                                 "BowserJrControlsGuide_SingleJoycons");
    } else {
        al::setPaneSystemMessage(this, "TxtGuideAssistBowserJr", "SingleMode_GuideMessage",
                                 "BowserJrControlsGuide_DualJoycons");
    }
}

/** @brief Closes the window if it is shown. */
void GuideMessage::startEnd() {
    if (!isAlive() || al::isNerve(this, &NrvGuideMessageEnd)) {
        return;
    }

    al::setNerve(this, &NrvGuideMessageEnd);
}

/** @brief Temporarily hides the window, remembering the state to resume. */
void GuideMessage::hide() {
    if (!isAlive()) {
        return;
    }

    mIsHide = true;
    if (al::isNerve(this, &NrvGuideMessageAppear)) {
        if (al::isActionPlaying(this, "Appear", "Main")) {
            al::pauseAction(this, "Main");
        }

        al::setNerve(this, &NrvGuideMessageHideAppear);
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageWait) ||
        al::isNerve(this, &NrvGuideMessageChangeText)) {
        s32 frame = mFrame;
        if (frame >= 0) {
            mFrame = sead::Mathi::min(frame - al::getNerveStep(this), 0);
        }

        al::setNerve(this, &NrvGuideMessageHideWait);
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageEnd)) {
        if (al::isActionPlaying(this, "End", "Main")) {
            al::pauseAction(this, "Main");
            al::setNerve(this, &NrvGuideMessageHideEnd);
            return;
        }

        kill();
    }
}

/** @brief Lets a hidden window resume. */
void GuideMessage::unHide() {
    mIsHide = false;
}

/** @brief Plays (or resumes) the appear action, then waits. */
void GuideMessage::exeAppear() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvGuideMessageAppearFromHide)) {
            al::unpauseAction(this, "Main");
        } else {
            al::startAction(this, "Appear", "Main");
        }
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvGuideMessageWait);
    }
}

/** @brief Stays hidden until unhidden, then resumes the interrupted state. */
void GuideMessage::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }

    if (mIsHide) {
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageHideAppear)) {
        al::setNerve(this, &NrvGuideMessageAppearFromHide);
    } else if (al::isNerve(this, &NrvGuideMessageHideWait)) {
        al::setNerve(this, &NrvGuideMessageWait);
    } else if (al::isNerve(this, &NrvGuideMessageHideEnd)) {
        al::setNerve(this, &NrvGuideMessageEndFromHide);
    }
}

/** @brief Shows the message until its frame count runs out. */
void GuideMessage::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", nullptr);
    }

    if (mFrame >= 0 && al::isGreaterEqualStep(this, mFrame)) {
        al::setNerve(this, &NrvGuideMessageEnd);
    }
}

/** @brief Waits for the appear action to finish before changing the text. */
void GuideMessage::exeWaitToChangeText() {
    if (al::isActionEnd(this, "Main")) {
        if (al::isNerve(this, &NrvGuideMessageWaitToChangeTextAssist)) {
            al::setNerve(this, &NrvGuideMessageChangeTextAssist);
        } else {
            al::setNerve(this, &NrvGuideMessageChangeText);
        }
    }
}

/** @brief Closes the window, swaps in the new text and opens it again. */
void GuideMessage::exeChangeText() {
    if (al::isFirstStep(this) && !al::isActionPlaying(this, "End", "Main")) {
        al::startAction(this, "End", "Main");
    }

    if (!al::isActionEnd(this, "Main")) {
        return;
    }

    if (al::isNerve(this, &NrvGuideMessageChangeTextAssist)) {
        al::startAction(this, "SetTextIconDual", "TextMode");
        setAssistText();
        mFrame = cAssistTextFrame;
    } else {
        al::setPaneString(this, "TxtGuide",
                          al::getSystemMessageString(this, mCategory.cstr(), mLabel.cstr()), 0,
                          -1);
    }

    al::setNerve(this, &NrvGuideMessageAppear);
}

/** @brief Plays (or resumes) the end action, then kills the window. */
void GuideMessage::exeEnd() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvGuideMessageEndFromHide)) {
            al::unpauseAction(this, "Main");
        } else {
            al::startAction(this, "End", nullptr);
        }
    }

    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}
