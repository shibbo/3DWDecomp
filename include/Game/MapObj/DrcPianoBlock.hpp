#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class DrcPianoBlock : public al::LiveActor {
public:
    explicit DrcPianoBlock(const char*);
    ~DrcPianoBlock() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isTypeOnOff() const;
    void control() override;
    void exeOffWait();
    void exeOn();
    int calcMoveTime() const;
    void exeOnWait();
    bool isTypeTimerOff() const;
    void exeOff();
private:
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;
    int mMoveAxis = 0;
    float mMoveDistance = 400.0f;
    float mMoveRate = 0.0f;
    float mMoveSpeed = 20.0f;
    int mMoveTime = -1;
    int mOnWaitTime = 300;
    int mMoveType = 0;
    int mTouchTimer = 0;
    int mNoteId = 0;
};
static_assert(sizeof(DrcPianoBlock) == 0x190);
