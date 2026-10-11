#pragma once

#include <math/seadVector.h>

namespace al {
class HitSensor;
}

/// Hold offsets of an item carried by the player, one per player size and pose.
class ItemStatePlayerHoldParam {
public:
    ItemStatePlayerHoldParam(const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&, const sead::Vector3f&,
                             const sead::Vector3f&, const sead::Vector3f&);

    const sead::Vector3f& getPlayerHoldPos(const al::HitSensor* pPlayerSensor) const;

    /** @brief Rotation (degrees) of the held item relative to the player's hold matrix. */
    const sead::Vector3f& getHoldRotate() const { return mHoldRotate; }

private:
    sead::Vector3f mOffsets[10];       // 0x0
    unsigned char _78[0x90 - 0x78];
    sead::Vector3f mHoldRotate;        // 0x90 (last constructor argument)
};
