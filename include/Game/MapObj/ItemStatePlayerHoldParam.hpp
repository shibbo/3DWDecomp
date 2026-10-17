#pragma once

#include <math/seadVector.h>

namespace al {
class HitSensor;
}

/// Hold offsets of an item carried by the player, including offsets selected by player character and size.
class ItemStatePlayerHoldParam {
public:
    ItemStatePlayerHoldParam();
    ItemStatePlayerHoldParam(const sead::Vector3f&, const sead::Vector3f&);
    ItemStatePlayerHoldParam(const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&);

    const sead::Vector3f& getPlayerHoldPos(const al::HitSensor* pPlayerSensor) const;

    /** @brief Rotation (degrees) of the held item relative to the player's hold matrix. */
    const sead::Vector3f& getHoldRotate() const { return mHoldRotate; }

private:
    sead::Vector3f mOffsets[12];       // 0x0
    sead::Vector3f mHoldRotate;        // 0x90 (last constructor argument)
};

static_assert(sizeof(ItemStatePlayerHoldParam) == 0x9c);
