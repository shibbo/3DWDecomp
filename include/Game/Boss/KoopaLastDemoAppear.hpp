#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BossStateDemoStart;
class BossDemoStartInfo;
/** @brief Bowser's introductory demo and departure jump. */
class KoopaLastDemoAppear : public al::LiveActor {
public:
    explicit KoopaLastDemoAppear(const char* pName);
    ~KoopaLastDemoAppear() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void start();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeHide();
    void exeDemo();
    void exeWait();
    void exeJumpStart();
    void exeJump();
    void exeJumpEnd();
private:
    BossStateDemoStart* mDemoState = nullptr;
    BossDemoStartInfo* mDemoInfo = nullptr;
    sead::Vector3f mInitPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mTargetPos = {0.0f, 0.0f, 0.0f};
};
static_assert(sizeof(KoopaLastDemoAppear) == 0x170);
