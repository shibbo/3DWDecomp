#pragma once

struct FlyerStateParam {
    FlyerStateParam();
    FlyerStateParam(float friction, float heightDamping);

    float mFriction;
    float mHeightDamping;
};

static_assert(sizeof(FlyerStateParam) == 8);
