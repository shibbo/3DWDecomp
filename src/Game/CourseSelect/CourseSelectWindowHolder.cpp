#include "CourseSelect/CourseSelectWindowHolder.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "Layout/WindowConfirm.hpp"
#include "Layout/WindowMessage.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"

/** @brief Create the scene's dialog and fade holder. */
CourseSelectWindowHolder::CourseSelectWindowHolder(const CourseSelectScene* pScene) : mpScene(pScene) {}

/** @brief Allocate confirmation dialogs, a message window, and both fade colors. */
void CourseSelectWindowHolder::init(const al::LayoutInitInfo& rInfo) {
    mpReport = new WindowConfirm(WindowConfirmType::Report, rInfo, nullptr, false);
    mpOneButton = new WindowConfirm(WindowConfirmType::OneButton, rInfo, nullptr, false);
    mpTwoButton = new WindowConfirm(WindowConfirmType::TwoButton, rInfo, nullptr, false);
    mpMessage = new WindowMessage(rInfo, "WindowMessage", "メッセージウインドウ", nullptr);
    mpWhiteFade = new al::WipeSimple("コース選択白フェード", "WipeFadeWhite", rInfo, "Demo");
    mpBlackFade = new al::WipeSimple("コース選択黒フェード", "WipeFadeBlack", rInfo, nullptr);
}

/** @brief Cover the screen in white over the requested number of frames. */
void CourseSelectWindowHolder::startFadeWhite(int frames) { mpWhiteFade->startClose(frames); }
/** @brief Remove the white screen cover over the requested number of frames. */
void CourseSelectWindowHolder::endFadeWhite(int frames) { mpWhiteFade->startOpen(frames); }
bool CourseSelectWindowHolder::isCloseFadeWhite() const { return mpWhiteFade->isCloseEnd(); }
bool CourseSelectWindowHolder::isActiveFadeWhite() const { return mpWhiteFade->isAlive(); }
/** @brief Cover the screen in black over the requested number of frames. */
void CourseSelectWindowHolder::startFadeBlack(int frames) { mpBlackFade->startClose(frames); }
/** @brief Remove the black screen cover over the requested number of frames. */
void CourseSelectWindowHolder::endFadeBlack(int frames) { mpBlackFade->startOpen(frames); }
bool CourseSelectWindowHolder::isCloseFadeBlack() const { return mpBlackFade->isCloseEnd(); }

/** @brief Show an informational confirmation for the main player's controller. */
void CourseSelectWindowHolder::appearConfirmReport(const char* pMessage) {
    mpReport->appearWithSystemMessage("WindowConfirmCourseSelect", pMessage, mpScene->getMainPlayerInputPort(), nullptr);
}
/** @brief Show a single-button confirmation, optionally using a different layout. */
void CourseSelectWindowHolder::appearConfirmOneBtn(const char* pMessage, const char* pLayout) {
    mpOneButton->appearWithSystemMessage(pLayout ? pLayout : "WindowConfirmCourseSelect", pMessage, mpScene->getMainPlayerInputPort(), nullptr);
}
/** @brief Show a two-button confirmation with optional R-button guidance. */
void CourseSelectWindowHolder::appearConfirmTwoBtn(const char* pMessage, bool useRButton) {
    const char* pButton = useRButton ? "Rボタン" : nullptr;
    mpTwoButton->appearWithSystemMessage("WindowConfirmCourseSelect", pMessage, mpScene->getMainPlayerInputPort(), pButton);
}
/** @brief Show a system message for the main player's controller. */
void CourseSelectWindowHolder::appearMessage(const char* pMessage) {
    mpMessage->appearWithSystemMessage("WindowMessage", pMessage, mpScene->getMainPlayerInputPort());
}

bool CourseSelectWindowHolder::isConfirmActive() const {
    return mpTwoButton->isAlive() || mpOneButton->isAlive() || mpReport->isAlive();
}
bool CourseSelectWindowHolder::isConfirmDecideOne() const { return mpOneButton->isDecideOne(); }
bool CourseSelectWindowHolder::isConfirmDecideLeft() const { return mpTwoButton->isDecideLeft(); }
bool CourseSelectWindowHolder::isConfirmDecideRight() const { return mpTwoButton->isDecideRight(); }
bool CourseSelectWindowHolder::isMessageActive() const { return mpMessage->isAlive(); }
