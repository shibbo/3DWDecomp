#include "LuigiBros/Emulator/ErrorViewer.hpp"
#include "LuigiBros/Emulator/inputdevice.hpp"

ErrorViewer g_ErrorViewer;

/** @brief Reinitializes the viewer and clears its active error state. */
void ErrorViewer::Initialize() {
    Finalize();
    mInitialized = true;
    mNoOperate = false;
    mState = 0;
}

/** @brief Marks an initialized viewer as finalized. */
void ErrorViewer::Finalize() {
    if (mInitialized) {
        mInitialized = false;
    }
}

/** @brief Advances the active error-viewer state. */
void ErrorViewer::Main() {
    if (mInitialized) {
        switch (mState) {
        case 1: Ev_Stat_Open_00(); break;
        case 2: Ev_Stat_Open_01(); break;
        case 3: Ev_Stat_Main_00(); break;
        case 4: Ev_Stat_Close_00(); break;
        case 5: Ev_Stat_Close_01(); break;
        }
    }
}

/** @brief Advances from the opening state to its next phase. */
void ErrorViewer::Ev_Stat_Open_00() { mState = 2; }
/** @brief Platform stub for the second opening phase. */
void ErrorViewer::Ev_Stat_Open_01() {}
/** @brief Platform stub for displaying an error. */
void ErrorViewer::Ev_Stat_Main_00() {}
/** @brief Platform stub for the first closing phase. */
void ErrorViewer::Ev_Stat_Close_00() {}

/** @brief Masks controller input and returns the viewer to idle. */
void ErrorViewer::Ev_Stat_Close_01() {
    m_inputdevice_controller.setMask();
    mNoOperate = false;
    mState = 0;
}

/** @brief Platform stub for drawing to the television. */
void ErrorViewer::DrawTV() {}
/** @brief Platform stub for drawing to the GamePad. */
void ErrorViewer::DrawDRC() {}

/**
 * @brief Platform stub for changing an error button message.
 * @param pMessage Unused message data.
 * @param enabled Unused enabled flag.
 * @param button Unused button identifier.
 */
void chg_button_message(const void* pMessage, bool enabled, unsigned int button) {}

/**
 * @brief Begins an error display when the initialized viewer is idle.
 * @param error Error identifier to display.
 * @param flags Unused opening flags.
 */
void ErrorViewer::Open(int error, unsigned int flags) {
    if (mInitialized && mState == 0) {
        mNoOperate = true;
        mState = 1;
        mError = error;
    }
}

/** @brief Platform stub for requesting closure. */
void ErrorViewer::Close() {}

/** @brief Initializes viewer ownership, state, and button flags. */
ErrorViewer::ErrorViewer()
    : mInitialized(false), mpContext(nullptr), mNoOperate(false), mState(0),
      mButton(false), mLeftButton(false), mRightButton(false) {}

/** @brief Finalizes the viewer. */
ErrorViewer::~ErrorViewer() { Finalize(); }

/** @brief Checks whether the viewer blocks operation. @return Operation-blocking flag. */
bool ErrorViewer::IsNoOperate() { return mNoOperate; }
/** @brief Gets the general button flag. @return Whether the button flag is set. */
bool ErrorViewer::IsButton() { return mButton; }
/** @brief Gets the left button flag. @return Whether the left button flag is set. */
bool ErrorViewer::IsLeftButton() { return mLeftButton; }
/** @brief Gets the right button flag. @return Whether the right button flag is set. */
bool ErrorViewer::IsRightButton() { return mRightButton; }
