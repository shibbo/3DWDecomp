#pragma once
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "System/GameDataHolderWriter.hpp"
namespace al {
class PadRumbleKeeper;
}
namespace sead {
class IDelegate;
}
class HeadgearPoseBuilder;
class HeadgearStateBlow;
class BoxPropellerPropeller;
class BoxPropeller : public al::LiveActor {
public:
  explicit BoxPropeller(const char *);
  ~BoxPropeller() override;
  void init(const al::ActorInitInfo &) override;
  void appear() override;
  void respawn() override;
  void kill() override;
  void startClipped() override;
  void endClipped() override;
  bool hideActor() override;
  bool showActor() override;
  void pause() override;
  void resume() override;
  void attackSensor(al::HitSensor *, al::HitSensor *) override;
  bool receiveMsg(const al::SensorMsg *, al::HitSensor *,
                  al::HitSensor *) override;
  void control() override;
  bool isCarry();
  void validateWaitShadow();
  void validateBodyShadow();
  void updateInChangeDemo();
  void deletePropellerEffect();
  bool arePropellerJumpsComplete();
  bool tryShiftCarry(al::HitSensor *, al::HitSensor *);
  void decideAnimRate();
  void setSklAnimFrameRate(float);
  void startAction(const char *);
  void startCharaMtpAnim();
  bool tryUpdateGuideMessage();
  void showModel();
  void hideModel();
  void exeAppear();
  void exeWait();
  void exeCarry();
  void exeKill();
  static void resetPropellerJumpGuideMessage(GameDataHolderWriter writer);
  void setAppearFromHipDrop() { mAppearFromHipDrop = true; }

private:
  static int sPropellerJumps;
  al::HitSensor *mPlayerSensor = nullptr;
  HeadgearPoseBuilder *mPoseBuilder = nullptr;
  float mAnimRate = 1.0f;
  int _15c = 0;
  int _160 = 0;
  bool mFloorTouch = false;
  bool mAppearFromHipDrop = false;
  bool mHasPropellerAction = false;
  bool mWasRising = false;
  bool mPlaced = false;
  bool mGuideShown = false;
  bool mSingle = false;
  bool mAlphaLinked = false;
  int mPauseFrames = 0;
  BoxPropellerPropeller *mPropeller = nullptr;
  HeadgearStateBlow *mBlowState = nullptr;
  al::PadRumbleKeeper *mRumble = nullptr;
  sead::IDelegate *mDelegate = nullptr;
  al::ModelDrawerArray mDrawers;
  int mGuideType = 0;
};
static_assert(sizeof(BoxPropeller) == 0x1a8);
