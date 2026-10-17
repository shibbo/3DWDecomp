#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class ComboCounter; }
/** @brief Rolling spiked ball launched by Bowser. */
class KoopaNeedleBall : public al::LiveActor {
public:
    explicit KoopaNeedleBall(const char* pName);
    ~KoopaNeedleBall() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void launch();
    void exeWait();
    void exeFall();
    void exeMove();
    void exeBreak();
private:
    void updateVelocity(bool isOnGround);
    void updateRolling(bool isOnGround);
    al::ComboCounter* mComboCounter;
};
static_assert(sizeof(KoopaNeedleBall) == 0x150);
