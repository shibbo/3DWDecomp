#pragma once
#include "Boss/TentackRockBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; }

class TentackRock : public al::LiveActor, public TentackRockBase {
public:
    explicit TentackRock(const char* pName);
    ~TentackRock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void startFall(const sead::Vector3f& rPosition) override;
    bool isDeadRock() const override;
    void exeFall();

private:
    al::BreakModel* mBreakModel = nullptr;
};
static_assert(sizeof(TentackRock) == 0x158);
