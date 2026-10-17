#pragma once

#include <container/seadRingBuffer.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/Joint/JointControllerBase.hpp"

namespace al {

class JointLookAtController : public JointControllerBase {
public:
    struct JointLookAtInfo {
        s32 jointIndex = -1;
        sead::Vector2f yawRange = {0.0f, 0.0f};
        sead::Vector2f pitchRange = {0.0f, 0.0f};
        sead::Vector3f localUp = {0.0f, 1.0f, 0.0f};
        sead::Vector3f localSide = {1.0f, 0.0f, 0.0f};
        sead::Vector3f localFront = {0.0f, 0.0f, 1.0f};
        sead::Quatf currentQuat = sead::Quatf(0.0f, 0.0f, 0.0f, 1.0f);
        sead::Quatf targetQuat;
        bool isLookAt = false;
        bool isInvalid = false;
        bool isNoJudge = false;
        f32 rate = 0.15f;
        const sead::Matrix34f* judgeMtx = nullptr;
        sead::Vector3f judgeFront = {0.0f, 0.0f, 1.0f};
        sead::Vector3f judgeUp = {0.0f, 1.0f, 0.0f};
        sead::Vector3f judgeSide = {1.0f, 0.0f, 0.0f};
        sead::Vector3f prevJudgeFront = sead::Vector3f::ez;
    };

    static_assert(sizeof(JointLookAtInfo) == 0x98);

    JointLookAtController(s32 maxJoints, const sead::Matrix34f* pBaseMtx);

    void calcJointCallback(s32 jointIndex, sead::Matrix34f* pMtx) override;
    void appendJoint(s32 jointIndex, f32 rate, const sead::Vector2f& rYawRange,
                     const sead::Vector2f& rPitchRange, const sead::Vector3f& rLocalFront,
                     const sead::Vector3f& rLocalUp);
    void appendJointLocalJudge(s32 jointIndex, f32 rate, const sead::Vector2f& rYawRange,
                               const sead::Vector2f& rPitchRange, const sead::Vector3f& rLocalFront,
                               const sead::Vector3f& rLocalUp, const sead::Matrix34f* pJudgeMtx,
                               const sead::Vector3f& rJudgeFront, const sead::Vector3f& rJudgeUp,
                               bool isNoJudge);
    void appendJointNoJudge(s32 jointIndex, f32 rate, const sead::Vector2f& rYawRange,
                            const sead::Vector2f& rPitchRange, const sead::Vector3f& rLocalFront,
                            const sead::Vector3f& rLocalUp);
    void requestJointLookAt(const sead::Vector3f& rTargetPos);
    bool invalidJoint(s32 jointIndex);
    void validAllJoint();

    /** @brief Gets a configured joint. @param index Joint order. @return Joint information. */
    JointLookAtInfo* getJointInfo(int index) { return &mJointInfos[index]; }
    /** @brief Controls whether the target rotation is retained. @param keep Retain rotation. */
    void setKeepTargetQuat(bool keep) { mIsKeepTargetQuat = keep; }
    /** @brief Transfers the current look request to the animation update. */
    void update() {
        bool wasRequested = mIsRequestLookAt;
        mIsRequestLookAt = false;
        mIsLookAt = wasRequested;
    }

private:
    sead::Vector3f mTargetPos = {0.0f, 0.0f, 0.0f};
    const sead::Matrix34f* mBaseMtx;
    sead::RingBuffer<JointLookAtInfo> mJointInfos;
    bool mIsRequestLookAt = false;
    bool mIsLookAt = false;
    bool mIsKeepTargetQuat = false;
    bool mIsValid = true;
};

static_assert(sizeof(JointLookAtController) == 0xe0);

} // namespace al
