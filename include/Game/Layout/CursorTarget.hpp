#pragma once
#include "Library/Layout/LayoutActor.hpp"
namespace al { class LayoutInitInfo; }

/** @brief Common interface for layout controls selectable by a button cursor. */
class CursorTarget : public al::LayoutActor {
public:
    CursorTarget(const al::LayoutInitInfo& rInfo, const char* pName,
                 const char* pPartsName, al::LayoutActor* pParent);
    /** @brief Accepts an optional selection sound override; unused by the base class.
     * @param pName Sound name. */
    virtual void setCustomSE(const char* pName) {}
    virtual void decide() = 0;
    virtual void select() = 0;
    virtual void wait() = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual bool isDisable() const = 0;
    virtual bool isValid() const = 0;
    virtual void invalidate() = 0;
    virtual void validate() = 0;
    virtual bool isDecide() const = 0;
    virtual bool isDecideEnd() const = 0;
    virtual bool isTouch() const = 0;
    virtual bool up() = 0;
    virtual bool down() = 0;
    virtual bool left() = 0;
    virtual bool right() = 0;

    void setPort(int port);
    int getPort() const;
    int getTouchPort() const;
private:
    int mPort = -1;
    int mTouchPort = -1;
};
static_assert(sizeof(CursorTarget) == 0x130);
