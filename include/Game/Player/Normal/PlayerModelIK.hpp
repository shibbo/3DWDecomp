#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}

class PlayerModelWorldMtxCallbackHolder;

/// Foot IK of a player model.
class PlayerModelIK {
public:
    PlayerModelIK(al::LiveActor* pActor, PlayerModelWorldMtxCallbackHolder* pCallbackHolder);

    void updateWorldMatrix();
    bool calcTargetPos(sead::Vector3f* pOut, const char* pJointName) const;
    void calc(const char* pJointName, const char* pTipJointName, const sead::Vector3f& rTarget);

    /** @brief Enables the IK (if it isn't already). */
    void validate() {
        if (!mIsValid) {
            mIsValid = true;
        }
    }

    /** @brief Disables the IK (if it isn't already). */
    void invalidate() {
        if (mIsValid) {
            mIsValid = false;
        }
    }

    /** @brief Checks if the IK is enabled. @return True if enabled. */
    bool isValid() const { return mIsValid; }

private:
    al::LiveActor* mActor;  // 0x0
    bool mIsValid;          // 0x8
};
