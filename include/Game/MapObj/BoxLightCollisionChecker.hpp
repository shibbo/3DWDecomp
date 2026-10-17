#pragma once

#include "Library/Nerve/NerveExecutor.hpp"

class BoxLightCollisionChecker : public al::NerveExecutor {
public:
    BoxLightCollisionChecker();
    void exeNoHit();
    void exeToHitCheck();
    void exeHit();
    void exeToNoHitCheck();
    bool isHit() const;
    void setCollision(bool isColliding) { mIsColliding = isColliding; }

private:
    bool mIsColliding = false;
};

static_assert(sizeof(BoxLightCollisionChecker) == 0x18);
