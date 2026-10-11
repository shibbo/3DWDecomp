#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class LiftMikeSlide : public al::LiveActor {
public:
    explicit LiftMikeSlide(const char*);
    ~LiftMikeSlide() override;
    void init(const al::ActorInitInfo&) override;
    void makeActorAppeared() override;
    void control() override;
    void exeWait();
    void exeMove();
    void updateMove();
    void exeHold();
    void exeBack();
private:
    al::KeyPoseKeeper* mKeyPose = nullptr;
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;
    float mAnimRate = 1.0f;
    float mSpeed = 0.0f;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
};
static_assert(sizeof(LiftMikeSlide) == 0x170);
