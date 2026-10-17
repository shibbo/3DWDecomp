#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>

/// The player's physical state shared by the actions.
struct PlayerProperty {
    PlayerProperty();
    void setFrontVec(const sead::Vector3f& rFront);
    void setUpVec(const sead::Vector3f& rUp);

    const sead::Vector3f& getTrans() const { return mTrans; }
    const sead::Vector3f& getFront() const { return mFront; }
    const sead::Vector3f& getGroundUp() const { return mGroundUp; }
    const sead::Vector3f& getVelocity() const { return mVelocity; }
    const sead::Vector3f& getGravity() const { return mGravity; }
    const sead::Vector3f& getUpDir() const { return mUpDir; }

    sead::Vector3f mTrans;     // 0x0
    sead::Vector3f mFront;     // 0xc
    sead::Vector3f mGroundUp;  // 0x18
    sead::Vector3f mVelocity;  // 0x24
    sead::Matrix34f mTurnMtx;  // 0x30, extra model rotation while turning in mid-air
    sead::Vector3f mGravity;   // 0x60
    sead::Vector3f mUpDir;     // 0x6c
    f32 _78;  // 0.9 while the player squeezes into a side pipe, 1.0 otherwise
    f32 mTilt;  // 0x7c, sideways lean while running (radians)
};
