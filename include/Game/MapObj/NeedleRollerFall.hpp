#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class NeedleRollerFall : public al::LiveActor {
public:
    explicit NeedleRollerFall(const char*);
    ~NeedleRollerFall() override;
    void init(const al::ActorInitInfo&) override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void setArchiveName(const char*);
    void setBasePose(const sead::Quatf&, const sead::Vector3f&);
    void setMoveAccel(float);
    bool isEnableAttack() const;
    void startGenerate();
    void startMove();
    void start();
    void disappear();
    void exeGenerate();
    void exeWait();
    void exeSupportFreeze();
private:
    const char* mArchiveName = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mAxis = sead::Vector3f::ex;
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;
    sead::Vector3f mPreviousTrans;
    sead::Vector3f mMoveDir = sead::Vector3f::ez;
    float mMoveAccel = 0.5f;
    float mRollAngle = 0.0f;
    float mRollSpeed = 0.0f;
    int mFreezeTimer = 0;
    bool mWasOnGround = false;
};
static_assert(sizeof(NeedleRollerFall) == 0x1a8);
