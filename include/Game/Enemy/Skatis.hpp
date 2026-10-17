#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class Skatis : public al::LiveActor {
public:
    explicit Skatis(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    /** @brief Gets the current pursuit target. @return Target actor, or nullptr. */
    const al::LiveActor* getTarget() const { return mTarget; }
private:
    const al::LiveActor* mTarget;
    u8 _150[0x50];
};
static_assert(sizeof(Skatis) == 0x1a0);
