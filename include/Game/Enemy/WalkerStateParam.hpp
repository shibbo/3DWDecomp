#pragma once

struct WalkerStateParam {
    WalkerStateParam();
    WalkerStateParam(float gravity, float airFriction, float groundFriction,
                     float unused1, float unused2, float unused3, float unused4, float valueC);

    float mGravity;
    float mAirFriction;
    float mGroundFriction;
    float mValueC;
};

static_assert(sizeof(WalkerStateParam) == 0x10);
