#pragma once

#include <container/seadPtrArray.h>
#include <gfx/seadCamera.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveExecutor.hpp"
#include "Project/Collision/IUseCollision.hpp"

namespace al {
class CollisionParts;

class CameraArrowCollider : public NerveExecutor, public IUseCollision {
public:
    CameraArrowCollider(CollisionDirector* pDirector);

    CollisionDirector* getCollisionDirector() const override { return mCollisionDirector; }

    void start();
    void update(const sead::Vector3f& rPos, const sead::Vector3f& rAt, const sead::Vector3f& rUp);
    void pushBackCollisionParts(CollisionParts* pParts);
    void makeLookAtCamera(sead::LookAtCamera* pCamera) const;
    f32 getPushLength() const;
    void exeKeep();
    void exeShrink();
    bool isShrink() const;

    void setIsInvalidThroughPassCollision(bool isInvalid) {
        mIsInvalidThroughPassCollision = isInvalid;
    }

    void setIsInvalidSearchCollisionParts(bool isInvalid) {
        mIsInvalidSearchCollisionParts = isInvalid;
    }

private:
    class HitResultBuffer;

    CollisionDirector* mCollisionDirector;
    sead::FixedPtrArray<CollisionParts, 192> mCollisionParts;
    HitResultBuffer* mHitResultBuffers = nullptr;
    f32 mPushLength = 0.0f;
    f32 mTargetPushLength = 0.0f;
    sead::Vector3f mPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mAt = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mDir = sead::Vector3f::ez;
    sead::Vector3f mSide = sead::Vector3f::ex;
    sead::Vector3f mUp = sead::Vector3f::ey;
    sead::Vector3f* mArrows = nullptr;
    s32 _688 = -1;
    bool mIsInvalidThroughPassCollision = false;
    bool mIsInvalidSearchCollisionParts = false;
};

static_assert(sizeof(CameraArrowCollider) == 0x690);

}  // namespace al
