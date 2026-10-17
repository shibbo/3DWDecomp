#pragma once
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class SwingMovement;
class MessageSyncParts;
} // namespace al
class ChainModel;
class NeedleRollerSwing : public al::LiveActor {
public:
  explicit NeedleRollerSwing(const char *);
  ~NeedleRollerSwing() override;
  void init(const al::ActorInitInfo &) override;
  void initAfterPlacement() override;
  void attackSensor(al::HitSensor *, al::HitSensor *) override;
  bool receiveMsg(const al::SensorMsg *, al::HitSensor *,
                  al::HitSensor *) override;
  bool receiveMsgScreenPoint(const al::SensorMsg *, al::ScreenPointer *,
                             al::ScreenPointTarget *) override;
  void start();
  void updateParts();
  void doBreak();
  void exeStandBy();
  void exeMove();
  void exeSupportFreeze();
  void exeSupportFreezeSync();

private:
  sead::Quatf mBaseQuat = sead::Quatf::unit;
  al::SwingMovement *mSwing = nullptr;
  al::MessageSyncParts *mRoller = nullptr;
  al::LiveActor *mLeftHead = nullptr;
  al::LiveActor *mRightHead = nullptr;
  al::DeriveActorGroup<ChainModel> *mChains = nullptr;
  sead::Vector3f mSide = sead::Vector3f::ex;
  sead::Vector3f mFront = sead::Vector3f::ez;
  float mGroundHeight;
  float mLength = 500.0f;
  float mWidth = 400.0f;
  float mRollAngle = 0.0f;
  float mRollSpeed = 0.0f;
  float mChainWidthOffset = 0.0f;
  int mFreezeTimer = 0;
  bool mOnGround = false;
  bool mFindGround = true;
};
static_assert(sizeof(NeedleRollerSwing) == 0x1b8);
