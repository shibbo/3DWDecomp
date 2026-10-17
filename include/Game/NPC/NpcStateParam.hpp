#pragma once
#include "NPC/NpcStateWait.hpp"

class NpcStateParam {
public:
    /** @return Gravity acceleration. */
    float getGravity() const { return mGravity; }
    /** @return Cliff check drop distance. */
    float getFallCheckDrop() const { return mParamC; }
    NpcStateParam();
    NpcStateParam(float gravity, float airVelocityScale, float groundVelocityScale,
                  float unused3, float unused4, float unused5, float unused6,
                  float paramC, int groundCheckFrames, float groundCheckMargin);

    float mGravity;
    float mAirVelocityScale;
    float mGroundVelocityScale;
    float mParamC;
    int mGroundCheckFrames;
    float mGroundCheckMargin;
};
