#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorJointLookController;
class KoopaChaseKoopa : public al::LiveActor {
public:
    explicit KoopaChaseKoopa(al::LiveActor* pHost);
    ~KoopaChaseKoopa() override;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void doDamage();
    void doDamageSmall();
    void setStateWait();
    void setStateWarpProvocation();
    bool isStateDamage() const;
    bool isStateWarpProvocation() const;
    void updatePose(const sead::Vector3f& rTarget);
    void exeStop();
    void exeWait();
    void exeDamage();
    void exeDamageSmall();
    void exeWarpProvocation();
private:
    float mSpineRotation = 0.0f;
    ActorJointLookController* mLookController;
};
static_assert(sizeof(KoopaChaseKoopa) == 0x150);
