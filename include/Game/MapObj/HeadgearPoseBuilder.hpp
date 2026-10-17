#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al {
class LiveActor;
class HitSensor;
} // namespace al
struct HeadgearPoseBuilderParam {
  bool _0;
  bool _1;
  bool _2;
};
static_assert(sizeof(HeadgearPoseBuilderParam) == 3);
class HeadgearPoseBuilder : public al::NerveExecutor {
public:
  HeadgearPoseBuilder(al::LiveActor *, const HeadgearPoseBuilderParam *);
  ~HeadgearPoseBuilder() override = default;
  void start(al::HitSensor *);
  void update();
  void updateAndCalcAnimDirect();
  bool isPlayerRaidonActionPlaying() const;
  bool isPlayerActionPlaying(const char *) const;
  const char *getPlayerActionName() const;
  bool isSnapping() const;
  void exeWait();
  void exeSnap();
  void updateSnapPose();
  al::HitSensor *getPlayerSensor() const { return mPlayerSensor; }
  void setChildActor(al::LiveActor *actor) { mChildActor = actor; }
  void clearPlayerSensor() { mPlayerSensor = nullptr; }

private:
  al::LiveActor *mHost;
  al::LiveActor *mChildActor;
  al::HitSensor *mPlayerSensor;
  const HeadgearPoseBuilderParam *mParam;
  s32 mDokanInFrames = 0;
  s32 mClimbMoveFrames = 0;
  bool mFacingRight = true;
  bool mEffectEnabled = true;
  f32 mSnapAngle = 0.0f;
  s32 mClimbPose = 0;
  s32 mClimbPoseBlendFrames = 0;
};
static_assert(sizeof(HeadgearPoseBuilder) == 0x48);
