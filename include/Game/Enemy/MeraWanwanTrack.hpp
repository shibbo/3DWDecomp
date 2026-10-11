#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CollisionParts;
}  // namespace al

/** @brief Burning track left on the ground behind a rolling MeraWanwan. */
class MeraWanwanTrack : public al::LiveActor {
public:
    explicit MeraWanwanTrack(const char* pName);

    void start(const sead::Vector3f& rTrans, const sead::Vector3f& rNormal,
               const al::CollisionParts* pParts);

private:
    u8 _144[0x2c];
};
static_assert(sizeof(MeraWanwanTrack) == 0x170);
