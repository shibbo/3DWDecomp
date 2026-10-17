#pragma once

#include <math/seadVector.h>

#include "Library/Joint/JointControllerBase.hpp"

namespace al {

class JointLocalAxisRotator : public JointControllerBase {
public:
    JointLocalAxisRotator(f32* pDegree, const sead::Vector3f& rAxis, bool isLocal);

    void calcJointCallback(s32 jointIndex, sead::Matrix34f* pMtx) override;

    /**
     * @brief Set the axis the joint is rotated around.
     * @param rAxis The new axis.
     */
    void setAxis(const sead::Vector3f& rAxis) { mAxis.set(rAxis); }

private:
    sead::Vector3f mAxis;
    f32* mDegree;
    bool mIsLocal;
};

static_assert(sizeof(JointLocalAxisRotator) == 0xc8);

}  // namespace al
