#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
    class LiveActor;
};

class TargetFinderParam {
public:
    TargetFinderParam();
    TargetFinderParam(f32, f32, f32, u32, f32, f32, f32, f32, bool);

    f32 _0;
    f32 _4;
    f32 _8;
    u32 _C;
    f32 _10;
    f32 _14;
    f32 _18;
    f32 _1C;
    bool _20;
};

class TargetFinder {
public:
    TargetFinder(al::LiveActor*, const TargetFinderParam*);

    void refindTarget();
    void update();
    void setFrontDir(sead::Vector3f* pFrontDir);
    const sead::Vector3f& getTargetPos() const;

    bool isExistTarget() const { return _8 != nullptr; }

    /**
     * @brief Gets the current target actor.
     * @return Target actor, or nullptr if there is none.
     */
    al::LiveActor* getTarget() const { return _8; }

    /**
     * @brief Checks whether a target exists and has been found (not only kept).
     * @return Whether a found target exists.
     */
    bool isFoundTarget() const { return _8 != nullptr && mIsFound; }

    /**
     * @brief Checks whether a target exists and the flag at 0x11 is set (found this update).
     * @return Whether a target exists and is currently found.
     */
    bool isFoundTargetNow() const { return _8 != nullptr && _11; }

    al::LiveActor* mActor;  // 0x00
    al::LiveActor* _8;
    bool mIsFound;  // 0x10
    bool _11;
    u8 _12[6];
    sead::Vector3f* mFrontDir;      // 0x18
    sead::Vector3f* mSupportUpDir;  // 0x20
    u64 _28;
    TargetFinderParam* mParams;  // 0x30
    al::LiveActor* _38;
};