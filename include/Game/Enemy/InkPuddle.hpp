#pragma once
#include "Library/LiveActor/LiveActor.hpp"
/** @brief Expanding ink hazard with an optional burning phase. */
class InkPuddle : public al::LiveActor {
public:
    InkPuddle(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void appear() override;
    void kill() override;
    void start(const sead::Vector3f& rDirection);
    void quickEnd();
    void exeStart();
    void exeWait();
    void exeIgnite();
    void exeQuickEnd();
    void exeEnd();
private:
    f32 mRadius = 100.0f;
    f32 mScaleFactor = 1.0f;
    bool mInfinite = false;
};
static_assert(sizeof(InkPuddle) == 0x150);
