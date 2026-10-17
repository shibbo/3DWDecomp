#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class FlashingCtrl;
}
class CheckPoint;
class CheckPointStar : public al::LiveActor {
public:
  CheckPointStar(const char *, CheckPoint *);
  ~CheckPointStar() override;
  void init(const al::ActorInitInfo &) override;
  void setUseCollectedModel();
  void startClipped() override;
  void endClipped() override;
  void startFlashing(s32);
  void updateFlashing();
  void overrideFlashingAudio(al::IUseAudioKeeper *);
  bool isFastFlashing() const;
  void startNightAnim();
  void cancel();
  void handleCountDownEffect(bool);
  void handleWaitEffect(bool);
  bool receiveMsg(const al::SensorMsg *, al::HitSensor *,
                  al::HitSensor *) override;
  bool receiveMsgScreenPoint(const al::SensorMsg *, al::ScreenPointer *,
                             al::ScreenPointTarget *) override;
  void exeSpin();
  void exeSpinSpeedDown();
  void exeAppear();
  void updateShadowLengthToTarget();
  void exeWait();
  void go(CheckPoint *, al::LiveActor *, bool, float, bool);
  void fail(CheckPoint *);
  void reset(CheckPoint *);
  void reappear(bool);
  void startMove();
  void endMove();
  void exeMoveLinear();
  void updateShadowLengthToFloor();
  void exeMoveArc();
  void exeFail();
  void exeNoop();

private:
  void syncCollectedModel();
  void followTarget();
  void updateMoveEnd();
  void *_148;
  s32 _150;
  bool _154;
  s32 _158;
  s32 _15c;
  s32 _160;
  sead::Vector3f mStartTrans;
  CheckPoint *mMoveSource;
  CheckPoint *mOwner;
  al::LiveActor *mCollectedModel;
  al::LiveActor *mTarget;
  float mMoveRate;
  u8 _194[12];
  bool _1a0;
  float mArcHeight;
  bool mSkipAppear;
  bool mUseCollected;
  float mMoveSpeed;
  s32 mMoveFrames;
  float mEndRate;
  al::FlashingCtrl *mFlashing;
};
static_assert(sizeof(CheckPointStar) == 0x1c0);
