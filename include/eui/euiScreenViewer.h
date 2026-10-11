#pragma once
#include <basis/seadTypes.h>
#include <eui/euiUtility.h>
namespace eui {
/**
 * @brief Maps screen draw units to display targets for the screen manager.
 *
 * Only the slot used by screens is named; the other virtual functions are placeholders that keep
 * the table layout.
 */
class ScreenViewer {
public:
    virtual ~ScreenViewer();
    virtual void update();
    virtual void _18();
    virtual void _20();
    /**
     * @param drawUnitId Draw unit (layer) assigned to a screen.
     * @return Display target that draws the unit.
     */
    virtual DrawTarget getDrawTarget(s8 drawUnitId) const;
};
}  // namespace eui
