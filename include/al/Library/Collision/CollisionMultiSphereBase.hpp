#pragma once

#include <basis/seadTypes.h>
#include <container/seadRingBuffer.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Project/Collision/HitDb.hpp"

namespace al {
class CollisionParts;
class KCollisionServer;
class KCPrismData;
class KCPrismHeader;
class LiveActor;

class CollisionMultiSphereBase {
public:
    struct Sphere {
        sead::Vector3f mPos;
        f32 mRadius;
    };

    struct SphereWork {
        sead::Vector3f mPos;
        sead::Vector3f mLocalPos;
    };

    struct Result {
        KCPrismData* mPrismData = nullptr;
        const KCPrismHeader* mPrismHeader = nullptr;
        s32 mSphereIndex = 0;
        HitInfo mHitInfo;
    };

    using ResultRingBuffer = sead::RingBuffer<Result>;

    CollisionMultiSphereBase(LiveActor* pActor, const Sphere* pSpheres, s32 sphereNum,
                             s32 sphereStride, SphereWork* pSphereWorks)
        : mActor(pActor), mSpheres(pSpheres), mSphereNum(sphereNum), mSphereStride(sphereStride),
          mSphereWorks(pSphereWorks), mBaseMtx(nullptr), mTrans(nullptr), mLocalScale(1.0f),
          mKCollisionServer(nullptr), mUpdatedHitNum(0) {}

    virtual ResultRingBuffer* getResultRingBuffer() = 0;
    virtual const ResultRingBuffer* getResultRingBuffer() const = 0;

    bool check();
    void callbackFromParts(CollisionParts* pParts);
    u32 getHitNum() const;
    const HitInfo* getHitInfo(u32 index) const;
    s32 getHitSphereIndex(u32 index) const;
    void callbackFromServer(KCPrismData* pData, const KCPrismHeader* pHeader);

protected:
    bool isResultFull() {
        const ResultRingBuffer* buffer = getResultRingBuffer();
        return buffer->size() >= buffer->capacity();
    }

    const Sphere* getNextSphere(const Sphere* pSphere) const {
        return reinterpret_cast<const Sphere*>(mSphereStride +
                                               reinterpret_cast<uintptr_t>(pSphere));
    }

    LiveActor* mActor;
    const Sphere* mSpheres;
    s32 mSphereNum;
    s32 mSphereStride;
    SphereWork* mSphereWorks;
    const sead::Matrix34f* mBaseMtx;
    const sead::Vector3f* mTrans;
    f32 mLocalScale;
    KCollisionServer* mKCollisionServer;
    s32 mUpdatedHitNum;
};

static_assert(sizeof(CollisionMultiSphereBase::Result) == 0xb8);
}  // namespace al
