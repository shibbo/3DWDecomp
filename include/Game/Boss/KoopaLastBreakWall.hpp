#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class JointAimInfo; class JointDirectionInfo; }
class KoopaLastBreakWall : public al::LiveActor {
public:
    explicit KoopaLastBreakWall(const char*);
    ~KoopaLastBreakWall() override;
    void init(const al::ActorInitInfo&) override;
    void start();
    void stop();
    void killBySwitch();
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void exeInsideWall();
    void exeBreakSign();
    void exeBreakWall();
    void exeAttack();
    void exeAttackEnd();
    void exeWait();
private:
    sead::Vector3f mAimDir = -sead::Vector3f::ey;
    sead::Vector3f mAimUp = sead::Vector3f::ey;
    int mAnimType = 1;
    bool mStopped = false;
    al::JointAimInfo* mNeck = nullptr;
    al::JointAimInfo* mEyeL;
    al::JointAimInfo* mEyeR;
    al::JointDirectionInfo* mFace = nullptr;
};
static_assert(sizeof(KoopaLastBreakWall) == 0x188);
