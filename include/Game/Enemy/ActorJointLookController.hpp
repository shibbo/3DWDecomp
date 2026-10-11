#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
    class LiveActor;
}

/** @brief Per-joint settings for ActorJointLookController. */
class ActorJointLookControllerParam {
public:
    ActorJointLookControllerParam();
    ActorJointLookControllerParam(f32 speed, const sead::Vector2f& rRange, bool isVertical,
                                  const sead::Matrix34f* pBaseMtx, const sead::Matrix34f* pAxisMtx);

    f32 mSpeed;                          // 0x00
    sead::Vector2f mRange;               // 0x04 (min, max degrees)
    bool mIsVertical;                    // 0x0C
    const sead::Matrix34f* mBaseMtx;     // 0x10 (joint origin, may be null)
    const sead::Matrix34f* mAxisMtx;     // 0x18 (local axes, may be null)
};

/** @brief Rotates a set of joints so the actor looks at a target point. */
class ActorJointLookController {
public:
    ActorJointLookController(const al::LiveActor* pActor, s32 jointNumMax);

    void appendJoint(const char* pJointName, const sead::Vector3f& rAxis,
                     const ActorJointLookControllerParam* pParam);
    void update();
    void setLookAtNearestPlayer(f32 distance);
    void resetRotate(bool isStopLook);

    /** @brief Stops looking at anything and forgets the current target player. */
    void stopLook() {
        mIsLook = false;
        mTargetPlayer = nullptr;
    }

    /**
     * @brief Looks at a fixed position.
     * @param rTarget Position to look at.
     */
    void setLookTarget(const sead::Vector3f& rTarget) {
        mLookTarget.e = rTarget.e;
        mIsLook = true;
    }

    /**
     * @brief Sets the horizontal and vertical look limits.
     * @param rLimit Horizontal (x) and vertical (y) limits in degrees.
     */
    void setLimit(const sead::Vector2f& rLimit) {
        // The two limits are stored as one Vector2f (copied as a single 64-bit value).
        reinterpret_cast<sead::Vector2f*>(&mLimitH)->e = rLimit.e;
    }

    const al::LiveActor* mActor;                                  // 0x00
    al::LiveActor* mTargetPlayer;                                 // 0x08
    s32 mSearchTimer;                                             // 0x10
    f32* mDegrees;                                                // 0x18
    sead::PtrArray<const ActorJointLookControllerParam> mParams;  // 0x20
    bool mIsLook;                                                 // 0x30
    sead::Vector3f mLookTarget;                                   // 0x34
    f32 mLimitH;                                                  // 0x40
    f32 mLimitV;                                                  // 0x44
};
