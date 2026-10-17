#pragma once

#include <container/seadRingBuffer.h>

#include "Library/Collision/CollisionMultiSphereBase.hpp"

namespace al {
/// A CollisionMultiSphereBase that owns room for N hit results and 16 sphere works.
template <s32 N>
class CollisionMultiSphere : public CollisionMultiSphereBase {
public:
    using FixedResultRingBuffer = sead::FixedRingBuffer<Result, N>;

    CollisionMultiSphere(LiveActor* pActor, const Sphere* pSpheres, s32 sphereNum,
                         s32 sphereStride)
        : CollisionMultiSphereBase(pActor, pSpheres, sphereNum, sphereStride, mWorks) {}

    ResultRingBuffer* getResultRingBuffer() override { return &mResults; }

    const ResultRingBuffer* getResultRingBuffer() const override { return &mResults; }

    void setBaseMtx(const sead::Matrix34f* pMtx, const sead::Vector3f* pTrans) {
        mBaseMtx = pMtx;
        mTrans = pTrans;
    }

private:
    FixedResultRingBuffer mResults;
    SphereWork mWorks[16];
};
}  // namespace al
