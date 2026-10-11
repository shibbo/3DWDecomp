#include "Layout/CursorTarget.hpp"

#include "Util/ControlUserUtil.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"

/**
 * @brief Creates a selectable layout part with no controller assigned.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 */
CursorTarget::CursorTarget(const al::LayoutInitInfo& rInfo, const char* pName,
                           const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
}

/** @brief Sets the controller port and resolves its associated touch-panel port.
 * @param port Controller port. */
void CursorTarget::setPort(int port) {
    mPort = port;
    mTouchPort = rc::calcTouchPanelPortByPortNum(port);
}

/** @brief Returns the assigned controller port.
 * @return Controller port, or -1 before assignment. */
int CursorTarget::getPort() const { return mPort; }

/** @brief Returns the assigned touch-panel port.
 * @return Touch-panel port, or -1 before assignment. */
int CursorTarget::getTouchPort() const { return mTouchPort; }
