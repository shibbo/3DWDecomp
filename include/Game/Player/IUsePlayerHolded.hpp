#pragma once

#include <math/seadVector.h>

/// The player while another player holds it (implemented by PlayerActor).
class IUsePlayerHolded {
public:
    virtual void queryHoldedPosture(sead::Vector3f* pTrans, sead::Vector3f* pFront,
                                    sead::Vector3f* pUp) const = 0;
    virtual void queryHoldingHostVelocity(sead::Vector3f* pVelocity) const = 0;
    virtual void pushHoldingHost(const sead::Vector3f& rPush) = 0;
    virtual void queryHoldedPostureOffset(sead::Vector3f* pOffset) const = 0;
    virtual void queryHoldedHostMoveDir(sead::Vector3f& rMoveDir) const = 0;
    virtual void requestClearHoldedHost() = 0;
};
