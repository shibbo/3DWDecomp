#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class KeyPoseKeeper;
}
class LiftMike : public al::LiveActor {
public:
  explicit LiftMike(const char *);
  void init(const al::ActorInitInfo &) override;
  void makeActorAppeared() override;
  void control() override;
  void exeWait();
  void exeUp();
  void updateMovement();
  void exeUpReleaseHigh();
  void exeUpReleaseLow();
  void exeHoldTop();
  void exeHold();
  void exeDown();
  void exeBottomVibrate();

private:
  float calcSignedDistanceToTop() const;
  void holdMoveSound(const char *, float);
  void updateVibration(float);
  al::KeyPoseKeeper *mKeyPose = nullptr;
  s32 mGuideBalloonType = 0;
  s32 _154 = 0;
  sead::Vector3f mBaseTrans = sead::Vector3f::zero;
  float _164 = 0.0f;
  float mSpeed = 0.0f;
  s32 _16c;
  float mInput = 0.0f;
  float mPrevInput = 0.0f;
  sead::Vector3f mClippingCenter = sead::Vector3f::zero;
  float mAnimRate = 1.0f;
  float mVibrationPhase = 0.0f;
  float mVibrationY = 0.0f;
};
static_assert(sizeof(LiftMike) == 0x190);
