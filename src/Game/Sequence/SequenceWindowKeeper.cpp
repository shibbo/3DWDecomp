#include "Sequence/SequenceWindowKeeper.hpp"

#include "Layout/WindowMessage.hpp"

/**
 * @brief Create the ordinary and sequence-group message windows.
 * @param rInfo Layout initialization services shared by both windows.
 */
SequenceWindowKeeper::SequenceWindowKeeper(const al::LayoutInitInfo& rInfo) {
    mpMessage = new WindowMessage(rInfo, "WindowMessage", "メッセージウインドウ", nullptr);
    mpSequenceMessage = new WindowMessage(rInfo, "WindowMessage", "メッセージウインドウ", "Sequence");
}

/**
 * @brief Display a system message in the selected window.
 * @param pMessageId Message identifier in the WindowMessage message system.
 * @param padPort Controller port used by the window.
 * @param isSequence Whether to use the window in the Sequence layout group.
 */
void SequenceWindowKeeper::appearMessage(const char* pMessageId, int padPort, bool isSequence) {
    mIsSequence = isSequence;
    WindowMessage* pWindow = isSequence ? mpSequenceMessage : mpMessage;
    pWindow->appearWithSystemMessage("WindowMessage", pMessageId, padPort);
}

/**
 * @brief Check whether the most recently selected message window is active.
 * @return Whether that window's layout actor is alive.
 */
bool SequenceWindowKeeper::isMessageActive() const {
    return (mIsSequence ? mpSequenceMessage : mpMessage)->isAlive();
}
