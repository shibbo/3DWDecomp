#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
    class HitSensor;
    class SensorMsg;
};

class ItemStatePopUpFrontParam {
public:
    ItemStatePopUpFrontParam() : _8("PopUp") { setDefault(); }
    ItemStatePopUpFrontParam(const sead::Vector3f&, f32, f32, int, f32, bool, const char*, bool, al::HitSensor*);

    void setDefault();

    al::HitSensor* _0;
    sead::FixedSafeString< 32 > _8;
    sead::Vector3f _40;
    sead::Vector3f _4C;
    f32 _58;
    f32 _5C;
    f32 _60;
    f32 _64;
    s32 _68;
    s32 _6C;
    f32 _70;
    bool _74;
    bool _75;
    bool _76;
    bool _77;
    bool _78;
};

class ItemStatePopUpFront : public al::ActorStateBase {
public:
    ItemStatePopUpFront(al::LiveActor*);
    void setParam(const ItemStatePopUpFrontParam&, const al::HitSensor*);
    void setParamDefault();
    void setParamOnCollide();
    void setParamBallPopUp();
    void setParamBallAbove();
    void setParamAnimName(const char*);
    void setParamHoming(const al::HitSensor*);
    void setParamInvalidateKillByArea(bool);
    void setParamInvalidateClippingOnKill(bool);
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    void setParamDefaultAbove();
    void changeModel(al::LiveActor*);
private:
    ItemStatePopUpFrontParam mParam;
    float mColliderRadius;
    bool _a4;
    bool _a5;
    al::HitSensor* _a8;
    u8 _b0[8];
};
static_assert(sizeof(ItemStatePopUpFront) == 0xb8);
