#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
class LanternRing;
class LanternSwingMovement;
class Lantern : public al::LiveActor {
public:
    explicit Lantern(const char*);
    ~Lantern() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool tryStartSwing(int, const sead::Vector3f&);
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeSwing();
    void updateSwingMovement();
    void exeSwingLerp();
private:
    LanternRing** mRings = nullptr;
    s32 mRingCount = 6;
    LanternSwingMovement* mSwing;
    sead::Quatf mInitialQuat = sead::Quatf::unit;
    sead::Vector3f mInitialTrans{0.0f, 0.0f, 0.0f};
    sead::Quatf mPreviousQuat = sead::Quatf::unit;
    sead::Vector3f mPreviousTrans{0.0f, 0.0f, 0.0f};
    sead::Vector3f mSwingAxis{0.0f, 0.0f, 0.0f};
    bool mCanAppearItem = false;
};
static_assert(sizeof(Lantern) == 0x1a8);
