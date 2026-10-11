#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadVector.h>
class GustWind : public al::LiveActor {
public:
    GustWind(const char*);
    ~GustWind() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    float calcBlowPowerRate() const;
    void exeWait();
    void exeBlowSign();
    void exeBlow();
    void exeBlowEnd();
    const sead::Matrix34f* getBaseMtx() const override { return &mBaseMtx; }
private:
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    float mFrontStart = 1200.0f;
    float mFrontEnd = 1400.0f;
    float mSideStart = 200.0f;
    float mSideEnd = 400.0f;
    int mWaitTime = 60;
    int mBlowTime = 60;
    bool mIsNeverBlow = true;
};
static_assert(sizeof(GustWind) == 0x1a0);
