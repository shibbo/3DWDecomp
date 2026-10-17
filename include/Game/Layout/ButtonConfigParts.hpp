#pragma once
#include "Layout/CursorTarget.hpp"
/** @brief Configuration button with cursor and touch selection. */
class ButtonConfigParts : public CursorTarget {
public:
    ButtonConfigParts(const al::LayoutInitInfo& rInfo, const char* pName,
                      const char* pPartsName, al::LayoutActor* pParent);
    void select() override;
    void decide() override;
    void wait() override;
    void enable() override;
    void disable() override;
    bool isDisable() const override;
    bool isDecide() const override;
    bool isDecideEnd() const override;
    bool isTouch() const override;
    bool isValid() const override;
    void invalidate() override;
    void validate() override;
    bool up() override;
    bool down() override;
    bool left() override;
    bool right() override;
    void exeWait();
    void exeSelect();
    void exeTouch();
    void exeOutsideHold();
    void exeDecide();
    void exeDisable();
    void exeHide();
private:
    bool mIsValid = true;
};
static_assert(sizeof(ButtonConfigParts) == 0x130);
