#pragma once
#include "Boss/TentackRockBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class TentackBase;

/** @brief Lava ball dropped onto the arena by the Tentack boss. */
class TentackMagmaBall : public al::LiveActor, public TentackRockBase {
public:
    TentackMagmaBall(const char* pName, TentackBase* pHost);
    ~TentackMagmaBall() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeFall();
    void exeLandStart();
    void exeLand();
    void exeLandEnd();
    void startFall(const sead::Vector3f& rPosition) override;
    bool isDeadRock() const override;
    void startFall(const sead::Vector2f* pPoint, f32 height);

private:
    TentackBase* mHost;
    const sead::Vector2f* mFallPoint = nullptr;
};
static_assert(sizeof(TentackMagmaBall) == 0x160);
