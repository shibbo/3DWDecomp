#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

/**
 * @brief An invisible actor the camera can follow in place of the player.
 * @note Only what reconstructed code needs is declared so far.
 */
class DummyCameraTarget : public al::LiveActor {
public:
    DummyCameraTarget(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;

    void onTarget();
    void offTarget();
    void setRequestDistance(f32 distance);
    void setNoCameraReset(bool isNoReset);
    void setTrans(sead::Vector3f& rTrans);
    f32 getRequestDistance();
    void setRotateY(f32 rotateY);
    void setFollowExact(bool isExact);

private:
    u8 _148[0x158 - 0x148];
};

static_assert(sizeof(DummyCameraTarget) == 0x158);
