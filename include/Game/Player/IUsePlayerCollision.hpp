#pragma once

#include <math/seadVector.h>

namespace al {
    class CollisionParts;
}

/// Player collision queries and solving (implemented by PlayerCollider).
class IUsePlayerCollision {
public:
    /// What the player touches on one side: the hit's normal, its codes and parts.
    struct Info {
        sead::Vector3f mNormal = {0.0f, 0.0f, 0.0f};  // 0x0
        const char* mMapCode = "";                     // 0x10
        const char* mWallCode = "";                    // 0x18
        const char* mMaterialCode = "";                // 0x20
        const al::CollisionParts* mParts = nullptr;    // 0x28
    };

    virtual void clear() = 0;
    virtual void moveSimple(bool) = 0;
    virtual void solveAir() = 0;
    virtual void solveAirNoFloor() = 0;
    virtual void snapGround() = 0;
    virtual void snapWall(bool) = 0;
    virtual void push(const sead::Vector3f&) = 0;
    virtual void clearPush() = 0;
    virtual void arrangeJumpFollowVel() = 0;
    virtual void clearJumpFollowVel() = 0;
    virtual bool isOnFloor() const = 0;
    virtual bool isOnFrontWall() const = 0;
    virtual bool isOnBackWall() const = 0;
    virtual bool isOnCeiling() const = 0;
    virtual bool isOnRightWall() const = 0;
    virtual bool isOnLeftWall() const = 0;
    virtual bool isOnAnyWall() const = 0;
    virtual bool isHeadOnFrontWall() const = 0;
    virtual bool isCenterOnFloor() const = 0;
    virtual void setDisableLegCheck(bool) = 0;
    virtual void setDisableLegCheckForce(bool) = 0;
    virtual void getFloorInfo(Info*) const = 0;
    virtual void getFrontWallInfo(Info*) const = 0;
    virtual void getBackWallInfo(Info*) const = 0;
    virtual void getCeilingInfo(Info*) const = 0;
    virtual void getRightWallInfo(Info*) const = 0;
    virtual void getLeftWallInfo(Info*) const = 0;
    virtual void shrinkBody() = 0;
    virtual void growBody() = 0;
    virtual void cutVelocity(u32) = 0;
};
