#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class JointRumbler; }
namespace sead { class IDelegate; }
class HeadgearPoseBuilder;
class HeadgearStateBlow;
class BoxKuribo : public al::LiveActor {
public:
    explicit BoxKuribo(const char*);
    ~BoxKuribo() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool hideActor() override;
    void appearPopUp(const sead::Vector3f&);
    void exePopUp();
    void exeWaitItem();
    void exeAttach();
    void exeWait();
    void exeBlow();
private:
    HeadgearPoseBuilder* mPoseBuilder = nullptr;
    sead::IDelegate* mPoseDelegate = nullptr;
    HeadgearStateBlow* mBlowState = nullptr;
    al::JointRumbler* mRumbler = nullptr;
    bool mLargeCollider = false;
    float mColliderRadius = 0.0f;
};
static_assert(sizeof(BoxKuribo) == 0x170);
