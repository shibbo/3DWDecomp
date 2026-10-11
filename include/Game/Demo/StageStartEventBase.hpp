#pragma once

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Common actor interface for events played when entering a stage. */
class StageStartEventBase : public al::LiveActor {
public:
    /** @brief Creates the event actor. @param pName Actor name. */
    explicit StageStartEventBase(const char* pName) : al::LiveActor(pName) {}
    virtual s64 getEventType() const = 0;
    virtual void startDemo() = 0;
    virtual void endDemo() = 0;
    virtual bool isEndDemo() const = 0;
    /** @brief Allows the opening wipe by default. @return True. */
    virtual bool isEnableOpenStartWipe() const { return true; }
    /** @brief Disables player movement by default. @return False. */
    virtual bool isEnableMovement() const { return false; }
};
