#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class NeedleBar;
class NeedleBarRoot : public al::LiveActor {
public:
    explicit NeedleBarRoot(const char*);
    ~NeedleBarRoot() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void setBarRotate(bool);
    void exeWait();
private:
    al::DeriveActorGroup<NeedleBar>* mBars = nullptr;
    float mRotateSpeed = 1.0f;
    float mAngle = 0.0f;
};
static_assert(sizeof(NeedleBarRoot) == 0x158);
