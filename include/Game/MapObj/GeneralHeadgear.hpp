#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <prim/seadSafeString.h>
class GeneralHeadgear : public al::LiveActor {
public:
    explicit GeneralHeadgear(const char*);
    ~GeneralHeadgear() override;
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void shiftKill(bool);
    bool tryShiftCarry(al::HitSensor*, al::HitSensor*);
    void exeAppear();
    void exeWait();
    void exeCarry();
    void startCharaMtpAnim();
    void exeKill();
private:
    sead::FixedSafeString<64> mName;
    const char* mArchive;
    al::HitSensor* mPlayerSensor = nullptr;
    bool mMini = false;
    bool mAppearHipDrop = false;
    bool mActionExists = false;
    bool mFloorTouched = false;
};
static_assert(sizeof(GeneralHeadgear) == 0x1b8);
