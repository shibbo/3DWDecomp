#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadRingBuffer.h>
namespace al {
class MtxConnector;
class PadRumbleKeeper;
class OccludedEffectRequestInfo;
} // namespace al
namespace sead {
class IDelegate;
}
class HeadgearPoseBuilder;
class HeadgearStateBlow;
class BoxLightCollisionChecker;
class BoxLight : public al::LiveActor {
public:
  explicit BoxLight(const char *);
  ~BoxLight() override;
  void init(const al::ActorInitInfo &) override;
  void appear() override;
  void control() override;
  void attackSensor(al::HitSensor *, al::HitSensor *) override;
  bool receiveMsg(const al::SensorMsg *, al::HitSensor *,
                  al::HitSensor *) override;
  bool isLighting() const;
  bool isRollingPlayer() const;
  bool tryShiftCarry(al::HitSensor *, al::HitSensor *);
  bool isFrontNearCamera() const;
  void startCharaMtpAnim();
  void exeAppear();
  void exeWait();
  void exeCarryStart();
  void exeCarry();
  void exeCarryLighting();
  void exeKill();

private:
  al::MtxConnector *mConnector = nullptr;
  al::HitSensor *mPlayerSensor = nullptr;
  HeadgearPoseBuilder *mPoseBuilder = nullptr;
  sead::IDelegate *mDelegate = nullptr;
  al::PadRumbleKeeper *mRumble = nullptr;
  bool mMini = false;
  bool mFloorTouch = false;
  bool mHipDrop = false;
  bool mFlashHit = false;
  bool mEffectOnOff = true;
  int mEffectState = 0;
  HeadgearStateBlow *mBlowState = nullptr;
  float mLightLength = 2000.0f;
  al::OccludedEffectRequestInfo *mOccludedEffect = nullptr;
  BoxLightCollisionChecker *mCollisionChecker;
  sead::RingBuffer<sead::Quatf> mRotationHistory;
  float mRotationDegree = 0.0f;
};
static_assert(sizeof(BoxLight) == 0x1c0);
