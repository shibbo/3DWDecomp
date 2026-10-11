#include "Player/Normal/PlayerModelIK.hpp"

#include <math.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_SkeletonObj.h>
#include <nn/util/util_MatrixApi.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Player/Normal/PlayerModelWorldMtxCallbackHolder.hpp"

typedef al::FunctorV0M<PlayerModelIK*, void (PlayerModelIK::*)()> PlayerModelIKFunctor;
typedef nn::util::Matrix4x3fType JointMtx;

namespace {

    /**
     * @brief Gets the skeleton of an actor's model.
     * @param pActor Actor with a model.
     * @return The skeleton.
     */
    inline nn::g3d::SkeletonObj* getSkeleton(const al::LiveActor* pActor) {
        return pActor->getModelKeeper()->getModelCafe()->getModelG3D()->getModelObj()->GetSkeleton();
    }

    /**
     * @brief Copies the xyz part of a matrix row into a vector.
     * @param pOut Receives the row.
     * @param row Matrix row.
     */
    inline void storeRow(sead::Vector3f* pOut, float32x4_t row) {
        pOut->x = row[0];
        pOut->y = row[1];
        pOut->z = row[2];
    }

    /**
     * @brief Sets the xyz part of a matrix row.
     * @param pRow Matrix row.
     * @param rVec New xyz values.
     */
    inline void setRow(float32x4_t* pRow, const sead::Vector3f& rVec) {
        (*pRow)[0] = rVec.x;
        (*pRow)[1] = rVec.y;
        (*pRow)[2] = rVec.z;
    }

    /**
     * @brief Sets the length of a vector, unless it is zero.
     * @param pVec Vector to scale.
     * @param length New length.
     */
    inline void setLength(sead::Vector3f* pVec, f32 length) {
        f32 current = pVec->length();
        if (current > 0.0f) {
            *pVec *= length / current;
        }
    }

    /**
     * @brief Projects a point onto a plane.
     * @param pPos Point to project.
     * @param rOrigin Point on the plane.
     * @param rNormal Plane normal.
     */
    inline void projectOnPlane(sead::Vector3f* pPos, const sead::Vector3f& rOrigin,
                               const sead::Vector3f& rNormal) {
        *pPos -= rNormal * rNormal.dot(*pPos - rOrigin);
    }

    /**
     * @brief Rotates the xyz lanes of a vector to yzx.
     * @param vector Vector to shuffle.
     * @return (y, z, x, w).
     */
    inline float32x4_t shuffleYzx(float32x4_t vector) {
        const uint8x8_t indexYz = {4, 5, 6, 7, 8, 9, 10, 11};
        const uint8x8_t indexXw = {0, 1, 2, 3, 12, 13, 14, 15};
        uint8x8x2_t table = {{vreinterpret_u8_f32(vget_low_f32(vector)),
                               vreinterpret_u8_f32(vget_high_f32(vector))}};
        return vreinterpretq_f32_u8(
            vcombine_u8(vtbl2_u8(table, indexYz), vtbl2_u8(table, indexXw)));
    }

    /**
     * @brief Rotates the xyz lanes of a vector to zxy.
     * @param vector Vector to shuffle.
     * @return (z, x, y, w).
     */
    inline float32x4_t shuffleZxy(float32x4_t vector) {
        const uint8x8_t indexZx = {8, 9, 10, 11, 0, 1, 2, 3};
        const uint8x8_t indexYw = {4, 5, 6, 7, 12, 13, 14, 15};
        uint8x8x2_t table = {{vreinterpret_u8_f32(vget_low_f32(vector)),
                               vreinterpret_u8_f32(vget_high_f32(vector))}};
        return vreinterpretq_f32_u8(
            vcombine_u8(vtbl2_u8(table, indexZx), vtbl2_u8(table, indexYw)));
    }

    /**
     * @brief Calculates the cross product of two vectors.
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return lhs x rhs.
     */
    inline float32x4_t vectorCross(float32x4_t lhs, float32x4_t rhs) {
        return vfmsq_f32(vmulq_f32(shuffleYzx(lhs), shuffleZxy(rhs)), shuffleYzx(rhs),
                         shuffleZxy(lhs));
    }

    /**
     * @brief Inverts a matrix. A singular matrix gives a zero matrix.
     * @param pOut Receives the inverse.
     * @param rMtx Matrix to invert.
     */
    inline void invertMtx(JointMtx* pOut, const JointMtx& rMtx) {
        float32x4_t row0 = rMtx._m.val[0];
        float32x4_t row1 = rMtx._m.val[1];
        float32x4_t row2 = rMtx._m.val[2];
        float32x4_t row3 = rMtx._m.val[3];

        float32x4_t product =
            vsubq_f32(vmulq_f32(vmulq_f32(row0, shuffleYzx(row1)), shuffleZxy(row2)),
                      vmulq_f32(vmulq_f32(row0, shuffleZxy(row1)), shuffleYzx(row2)));
        float32x2_t sum = vpadd_f32(vget_low_f32(product), vget_high_f32(product));
        float32x4_t determinant = vdupq_n_f32(vpadds_f32(sum));

        float32x4_t inverseDeterminant = vrecpeq_f32(determinant);
        inverseDeterminant =
            vmulq_f32(inverseDeterminant, vrecpsq_f32(inverseDeterminant, determinant));
        inverseDeterminant =
            vmulq_f32(inverseDeterminant, vrecpsq_f32(inverseDeterminant, determinant));

        float32x4x4_t matrix;
        matrix.val[0] = row0;
        matrix.val[1] = row1;
        matrix.val[2] = row2;
        matrix.val[3] = vdupq_n_f32(0.0f);
        float32x4x4_t transposed = nn::util::detail::Matrix4x4fTranspose(matrix);

        float32x4_t inverse0 =
            vmulq_f32(inverseDeterminant, vectorCross(transposed.val[1], transposed.val[2]));
        float32x4_t inverse1 =
            vmulq_f32(inverseDeterminant, vectorCross(transposed.val[2], transposed.val[0]));
        float32x4_t inverse2 =
            vmulq_f32(inverseDeterminant, vectorCross(transposed.val[0], transposed.val[1]));
        float32x4_t inverse3 = vnegq_f32(vmulq_laneq_f32(inverse0, row3, 0));
        inverse3 = vfmsq_laneq_f32(inverse3, inverse1, row3, 1);
        inverse3 = vfmsq_laneq_f32(inverse3, inverse2, row3, 2);

        uint32x4_t mask = vmvnq_u32(vceqzq_f32(determinant));
        pOut->_m.val[0] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse0), mask));
        pOut->_m.val[1] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse1), mask));
        pOut->_m.val[2] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse2), mask));
        pOut->_m.val[3] = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(inverse3), mask));
    }

    /**
     * @brief Multiplies two matrices.
     * @param pOut Receives the product, may be one of the operands.
     * @param rLhs Matrix applied first.
     * @param rRhs Matrix applied second.
     */
    inline void multiplyMtx(JointMtx* pOut, const JointMtx& rLhs, const JointMtx& rRhs) {
        float32x4x4_t lhs = rLhs._m;
        float32x4x4_t rhs = rRhs._m;
        float32x4_t row0 = vmulq_laneq_f32(rhs.val[0], lhs.val[0], 0);
        row0 = vfmaq_laneq_f32(row0, rhs.val[1], lhs.val[0], 1);
        row0 = vfmaq_laneq_f32(row0, rhs.val[2], lhs.val[0], 2);
        float32x4_t row1 = vmulq_laneq_f32(rhs.val[0], lhs.val[1], 0);
        row1 = vfmaq_laneq_f32(row1, rhs.val[1], lhs.val[1], 1);
        row1 = vfmaq_laneq_f32(row1, rhs.val[2], lhs.val[1], 2);
        float32x4_t row2 = vmulq_laneq_f32(rhs.val[0], lhs.val[2], 0);
        row2 = vfmaq_laneq_f32(row2, rhs.val[1], lhs.val[2], 1);
        row2 = vfmaq_laneq_f32(row2, rhs.val[2], lhs.val[2], 2);
        float32x4_t row3 = vmulq_laneq_f32(rhs.val[0], lhs.val[3], 0);
        row3 = vfmaq_laneq_f32(row3, rhs.val[1], lhs.val[3], 1);
        row3 = vfmaq_laneq_f32(row3, rhs.val[2], lhs.val[3], 2);
        pOut->_m.val[0] = row0;
        pOut->_m.val[1] = row1;
        pOut->_m.val[2] = row2;
        pOut->_m.val[3] = vaddq_f32(rhs.val[3], row3);
    }

    /// A joint of the IK chain: its world position and world matrix.
    struct IKJoint {
        /**
         * @brief Takes the position from a joint's world matrix.
         * @param pMtx World matrix of the joint.
         */
        void init(JointMtx* pMtx) {
            storeRow(&mPos, pMtx->_m.val[3]);
            mMtx = pMtx;
        }

        /**
         * @brief Moves the joint.
         * @param rPos New world position.
         */
        void setPos(const sead::Vector3f& rPos) {
            mPos = rPos;
            setRow(&mMtx->_m.val[3], rPos);
        }

        void rotateToward(const sead::Vector3f& rTarget);

        sead::Vector3f mPos;
        JointMtx* mMtx = nullptr;
    };

    /**
     * @brief Rotates the joint matrix so that its x axis points at a position.
     * @param rTarget Position to point at.
     */
    void IKJoint::rotateToward(const sead::Vector3f& rTarget) {
        sead::Vector3f axisX;
        storeRow(&axisX, mMtx->_m.val[0]);
        al::normalizeOrZero(&axisX);

        sead::Vector3f dir = rTarget - mPos;
        al::normalizeOrZero(&dir);

        sead::Quatf rotation;
        rotation.makeVectorRotation(axisX, dir);

        for (s32 i = 0; i < 3; i++) {
            float32x4_t row = mMtx->_m.val[i];
            sead::Vector3f axis(row[0], row[1], row[2]);
            axis.rotate(rotation);
            mMtx->_m.val[i][0] = axis.x;
            mMtx->_m.val[i][1] = axis.y;
            mMtx->_m.val[i][2] = axis.z;
        }
    }

    /// The three joints of a leg and the plane their IK is solved in.
    struct IKChain {
        sead::Vector3f mNormal;
        IKJoint mRoot;
        IKJoint mMiddle;
        IKJoint mEnd;
    };

}  // namespace

/**
 * @brief Constructs the foot IK and registers its world matrix callback.
 * @param pActor Player model actor.
 * @param pCallbackHolder Holder to register the callback with, or nullptr to register it on the
 * actor's model directly.
 */
PlayerModelIK::PlayerModelIK(al::LiveActor* pActor,
                             PlayerModelWorldMtxCallbackHolder* pCallbackHolder)
    : mActor(pActor), mIsValid(false) {
    if (pCallbackHolder != nullptr) {
        pCallbackHolder->registerCallback(
            PlayerModelIKFunctor(this, &PlayerModelIK::updateWorldMatrix));
    } else {
        al::setPostUpdateWorldMatrixCallback(
            pActor, PlayerModelIKFunctor(this, &PlayerModelIK::updateWorldMatrix));
    }
}

/**
 * @brief Places both feet on the ground below them, if the IK is enabled.
 */
void PlayerModelIK::updateWorldMatrix() {
    if (!mIsValid) {
        return;
    }

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, mActor);

    sead::Vector3f target;
    if (calcTargetPos(&target, "FootL")) {
        calc("FootL", "ToeL", target);
    }

    if (calcTargetPos(&target, "FootR")) {
        calc("FootR", "ToeR", target);
    }
}

/**
 * @brief Calculates where a foot joint should be placed on the ground below it.
 * @param pOut Receives the foot position.
 * @param pJointName Name of the foot joint.
 * @return True if ground above the actor's position was found.
 */
bool PlayerModelIK::calcTargetPos(sead::Vector3f* pOut, const char* pJointName) const {
    nn::g3d::SkeletonObj* skeleton = getSkeleton(mActor);
    const JointMtx& jointMtx =
        skeleton->GetWorldMtxArray()[skeleton->GetRes()->FindBoneIndex(pJointName)];
    f32 jointHeight = jointMtx._m.val[3][1];
    f32 actorHeight = al::getTrans(mActor).y;

    float32x4_t jointPos = jointMtx._m.val[3];
    sead::Vector3f start(jointPos[0], jointPos[1] + 80.0f, jointPos[2]);
    if (!alCollisionUtil::getFirstPolyOnArrow(mActor, pOut, nullptr, start,
                                              sead::Vector3f::ey * -200.0f, nullptr, nullptr)) {
        return false;
    }

    if (pOut->y <= al::getTrans(mActor).y) {
        return false;
    }

    f32 offset = jointHeight - actorHeight;
    f32 y = pOut->y + offset;
    pOut->y = y;
    f32 maxOffset = fmaxf(offset, 50.0f);
    if (y - al::getTrans(mActor).y > maxOffset) {
        pOut->y = maxOffset + al::getTrans(mActor).y;
    }

    return true;
}

/**
 * @brief Solves the two bone IK chain ending at a joint so that the joint reaches a target.
 * @param pJointName Name of the end joint (the foot).
 * @param pTipJointName Name of a descendant joint (the toe) whose chain follows the end joint.
 * @param rTarget Target position of the end joint.
 */
void PlayerModelIK::calc(const char* pJointName, const char* pTipJointName,
                         const sead::Vector3f& rTarget) {
    nn::g3d::SkeletonObj* skeleton = getSkeleton(mActor);
    JointMtx* worldMtxArray = skeleton->GetWorldMtxArray();
    IKChain chain;
    const nn::g3d::ResBone* endBone = skeleton->GetRes()->FindBone(pJointName);

    chain.mEnd.init(&worldMtxArray[endBone->GetIndex()]);
    const nn::g3d::ResBone* middleBone = skeleton->GetBone(endBone->GetParentIndex());
    chain.mMiddle.init(&worldMtxArray[middleBone->GetIndex()]);
    const nn::g3d::ResBone* rootBone = skeleton->GetBone(middleBone->GetParentIndex());
    chain.mRoot.init(&worldMtxArray[rootBone->GetIndex()]);
    JointMtx prevEndMtx = *chain.mEnd.mMtx;

    storeRow(&chain.mNormal, chain.mMiddle.mMtx->_m.val[2]);
    if (al::normalizeOrZero(&chain.mNormal)) {
        return;
    }

    sead::Vector3f rootPos = chain.mRoot.mPos;
    const sead::Vector3f& normal = chain.mNormal;
    sead::Vector3f middlePos = chain.mMiddle.mPos;
    projectOnPlane(&middlePos, rootPos, normal);
    sead::Vector3f endPos = chain.mEnd.mPos;
    projectOnPlane(&endPos, rootPos, normal);
    f32 upperLength = (middlePos - rootPos).length();
    f32 lowerLength = (endPos - middlePos).length();

    sead::Vector3f target = rTarget;
    target -= normal * (target - rootPos).dot(normal);
    sead::Vector3f rootToTarget = target - rootPos;
    f32 maxLength = upperLength + lowerLength;
    f32 minLength = sead::Mathf::abs(upperLength - lowerLength);
    f32 targetDistance = rootToTarget.length();
    if (targetDistance < minLength) {
        setLength(&rootToTarget, minLength);
    } else if (targetDistance > maxLength) {
        setLength(&rootToTarget, maxLength);
    }

    JointMtx* middleMtx = chain.mMiddle.mMtx;
    sead::Vector3f origin = chain.mRoot.mPos;
    sead::Vector3f endGoal = origin + rootToTarget;
    sead::Vector3f bendDir;
    storeRow(&bendDir, middleMtx->_m.val[1]);
    if (al::normalizeOrZero(&bendDir)) {
        return;
    }

    sead::Vector3f rootToGoal = endGoal - origin;
    f32 goalDistance = rootToGoal.length();
    if (goalDistance >= maxLength) {
        middlePos = origin + rootToGoal * (upperLength / goalDistance);
    } else if (al::isNearZero(goalDistance, 0.001f)) {
        middlePos = bendDir;
        middlePos *= upperLength;
        middlePos += origin;
    } else {
        f32 along = (upperLength * upperLength - lowerLength * lowerLength +
                     goalDistance * goalDistance) /
                    (goalDistance * 2.0f);
        f32 side = sead::Mathf::sqrt(fmaxf(upperLength * upperLength - along * along, 0.0f));
        middlePos = rootToGoal;
        al::normalize(&middlePos);

        sead::Vector3f alongOffset = middlePos * along;
        const f32 halfSqrt2 = 0.70710677f;
        sead::Quatf rotation(halfSqrt2, normal.x * halfSqrt2, normal.y * halfSqrt2,
                             normal.z * halfSqrt2);
        sead::Vector3f sideDir;
        sideDir.setRotated(rotation, middlePos);
        middlePos = origin + (alongOffset + sideDir * side);
    }

    chain.mMiddle.mPos = middlePos;
    setRow(&middleMtx->_m.val[3], middlePos);
    chain.mEnd.setPos(endGoal);
    chain.mRoot.rotateToward(chain.mMiddle.mPos);
    chain.mMiddle.rotateToward(chain.mEnd.mPos);

    if (pTipJointName == nullptr) {
        return;
    }

    JointMtx endMtx = worldMtxArray[endBone->GetIndex()];
    const nn::g3d::ResBone* tipBone = skeleton->GetRes()->FindBone(pTipJointName);
    if (tipBone == nullptr || tipBone == endBone) {
        return;
    }

    JointMtx inverse;
    invertMtx(&inverse, prevEndMtx);
    JointMtx delta;
    multiplyMtx(&delta, endMtx, inverse);
    for (const nn::g3d::ResBone* bone = tipBone; bone != nullptr && bone != endBone;
         bone = skeleton->GetBone(bone->GetParentIndex())) {
        JointMtx* mtx = &worldMtxArray[bone->GetIndex()];
        multiplyMtx(mtx, delta, *mtx);
    }
}
