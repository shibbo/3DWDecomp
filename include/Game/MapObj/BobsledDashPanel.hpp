#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadBoundBox.h>
#include <math/seadMatrix.h>
class BobsledDashPanel : public al::LiveActor {
public:
    BobsledDashPanel(const char*);
    ~BobsledDashPanel() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void exeWait();
    void exeStart();
private:
    sead::BoundBox3f mDashBounds;
    sead::Matrix34f mEffectMatrix = sead::Matrix34f::ident;
};
