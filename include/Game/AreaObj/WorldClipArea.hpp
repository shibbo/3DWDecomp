#pragma once

#include <prim/seadBitFlag.h>

#include "Project/AreaObj/AreaObj.hpp"

/**
 * @brief Area deciding which worlds of the course select map are active (not clipped).
 */
class WorldClipArea : public al::AreaObj {
public:
    WorldClipArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;

    /**
     * Gets the flags of the worlds active while the player is in the area.
     * @return The world flags.
     */
    const sead::BitFlag32& getWorldFlag() const { return mWorldFlag; }

private:
    sead::BitFlag32 mWorldFlag;  // 0x84
};
