#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class NeedleRoller : public al::LiveActor {
public:
    explicit NeedleRoller(const char*);
    ~NeedleRoller() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void startSeHit(float);
    void holdSeMove(float);
    void exeFreeMove();
    void exeWait();
    void exeSupportFreeze();
    void exeMove();
private:
    al::MtxConnector* mConnector = nullptr;
    sead::Vector3f mMoveDir = sead::Vector3f::ez;
    sead::Vector3f mAxis = sead::Vector3f::ex;
    float mDistance = 0.0f;
    float mMinDistance = 0.0f;
    float mMaxDistance = 0.0f;
    float mSpeed = 0.0f;
    float mRollAngle = 0.0f;
    int mFreezeTimer = 0;
    bool mFreeMove = false;
    bool mReverseRoll = false;
};
static_assert(sizeof(NeedleRoller) == 0x188);
