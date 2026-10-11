#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace al {
    class IUseCollision;
};  // namespace al

class IUseRigidBodyCollision;
class IUseRigidBodyCollisionSphereList;
class RigidBodyPower;

/**
 * Small rigid body simulation: integrates position, velocity and a column-wise rotation rate
 * in four sub-steps per update, resolving collision sphere hits as impulses.
 */
class RigidBodyCore {
public:
    /**
     * Center-of-gravity position and orientation of the body.
     */
    struct Pose {
        sead::Vector3f mTrans;
        sead::Matrix34f mMtx;
    };

    RigidBodyCore(IUseRigidBodyCollision* pBodyCollision, const al::IUseCollision* pCollision);

    void initMembers();
    void initPose(const sead::Matrix34f* pMtx);
    void requestPower(const sead::Vector3f& rPos, const sead::Vector3f& rForce);
    void update();
    void startPhysics();
    void calcCollisionImpact();
    void addGravity();
    void endPhysics();
    void calcSoundParam(sead::Vector3f prevVelocity);
    const sead::Vector3f& getPos() const;
    void calcPoseMtx(sead::Matrix34f* pMtx) const;
    void setGravity(const sead::Vector3f& rGravity);
    void setSphereHolder(const IUseRigidBodyCollisionSphereList* pSphereHolder);
    void impact(const sead::Vector3f& rPos, const sead::Vector3f& rNormal, f32 overlap);

    void setMaxSpeed(f32 speed) { mMaxSpeed = speed; }
    void setRepulsion(f32 repulsion) { mRepulsion = repulsion; }
    void setRotDamping(f32 damping) { mRotDamping = damping; }
    void setPosCorrection(f32 correction) { mPosCorrection = correction; }
    void setFriction(f32 friction) { mFriction = friction; }
    void setRadius(f32 radius) { mRadius = radius; }
    void setCenterOffset(sead::Vector3f offset) { mCenterOffset = offset; }

    virtual ~RigidBodyCore() {}

    virtual void control() {}

private:
    const al::IUseCollision* mCollision;
    Pose mPose;
    sead::Matrix34f mStepMtx;
    f32 mRadius;
    sead::Vector3f mGravity;
    f32 mRepulsion;
    f32 mRotDamping;
    f32 mPosCorrection;
    f32 mFriction;
    sead::Vector3f mVelocity;
    sead::Vector2f mRotVelocity[3];
    f32 mMaxSpeed;
    bool _c4;
    void* _c8;
    void* _d0;
    void* _d8;
    bool mIsCollided;
    IUseRigidBodyCollision* mBodyCollision;
    bool mIsSoundTrigger;
    f32 mSoundScalar;
    RigidBodyPower* mPower;
    RigidBodyPower* mRequestPower;
    const IUseRigidBodyCollisionSphereList* mSphereHolder;
    sead::Vector3f mCenterOffset;
};

static_assert(sizeof(RigidBodyCore) == 0x120);

/**
 * Accumulates the positional, linear and rotational contributions applied to a RigidBodyCore
 * during one physics step. Every axis keeps its most negative and most positive contribution,
 * so overlapping impulses along the same direction are not summed up.
 */
class RigidBodyPower {
public:
    /**
     * Most negative / most positive contribution per axis.
     */
    template <typename T>
    struct Peak {
        T mNeg;
        T mPos;

        T calcSum() const { return mNeg + mPos; }

        /**
         * Records a contribution to one axis: it replaces the negative peak when it is below
         * it, otherwise the positive peak when it is above it.
         * @param axis Axis index.
         * @param rValue Contribution to record.
         */
        void add(s32 axis, const f32& rValue) {
            if (rValue < mNeg.e[axis]) {
                mNeg.e[axis] = rValue;
            } else if (rValue > mPos.e[axis]) {
                mPos.e[axis] = rValue;
            }
        }

        /**
         * Records a contribution to every axis.
         * @param rValue Contribution to record.
         */
        void add(const T& rValue) {
            for (s32 i = 0; i < s32(sizeof(T) / sizeof(f32)); i++) {
                add(i, rValue.e[i]);
            }
        }
    };

    RigidBodyPower();

    void power(const RigidBodyCore::Pose& rPose, f32 radius, const sead::Vector3f& rPos,
               const sead::Vector3f& rForce);
    void addPos(const sead::Vector3f& rPos);
    void zero();
    void addVec(const sead::Vector3f& rVec);
    void zeroVec();
    void zeroRot();
    void addRot(const RigidBodyCore::Pose& rPose, const sead::Vector3f& rArmDir,
                const sead::Vector3f& rForce);

    sead::Vector3f calcPos() const { return mPos.calcSum(); }
    sead::Vector3f calcVec() const { return mVec.calcSum(); }
    sead::Vector2f calcRot(s32 axis) const { return mRot[axis].calcSum(); }

private:
    Peak<sead::Vector3f> mPos;
    Peak<sead::Vector3f> mVec;
    Peak<sead::Vector2f> mRot[3];
};

static_assert(sizeof(RigidBodyPower) == 0x60);
