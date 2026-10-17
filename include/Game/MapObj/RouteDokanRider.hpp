#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class RouteDokanEntrance;
class IUsePlayerPuppet;
class PuppetStickRouteSelecter;
class RouteDokanInOutEffect;
class PlayerBindEndParam;
namespace al {
class BlockRailRider;
}
class RouteDokanRider : public al::LiveActor {
public:
  RouteDokanRider(const char *, s32);
  ~RouteDokanRider() override;
  void init(const al::ActorInitInfo &) override;
  void attackSensor(al::HitSensor *, al::HitSensor *) override;
  bool receiveMsg(const al::SensorMsg *, al::HitSensor *,
                  al::HitSensor *) override;
  void control() override;
  const sead::Matrix34f *getBaseMtx() const override;
  bool isEnablePlayerRefrect() const;
  void setMoveSpeed(f32);
  void setOutSpeed(f32, f32);
  void setMoveActionName(const char *);
  void setBaseUpDir(const sead::Vector3f &);
  bool isActive(s32) const;
  bool isEndBind() const;
  bool canBindSingleMode();
  void startBind(RouteDokanEntrance *, al::HitSensor *, al::HitSensor *);
  void startBind(int, RouteDokanEntrance *, IUsePlayerPuppet *);
  void addRouteSelectPuppet(IUsePlayerPuppet *);
  void tryCancelBind(al::HitSensor *);
  void forceCancelBind();
  bool damage(al::HitSensor *);
  void exeStart();
  void exeMove();
  void exeEnd();
  void exeInvalid();
  void emitAndEndPuppet(IUsePlayerPuppet **, float, float);
  void emitPuppet(IUsePlayerPuppet **, float, float);
  const PlayerBindEndParam *getPlayerBindEndParam() const;

private:
  IUsePlayerPuppet *mPuppet = nullptr;
  al::BlockRailRider *mRailRider = nullptr;
  PuppetStickRouteSelecter *mRouteSelecter;
  RouteDokanInOutEffect *mInOutEffect = nullptr;
  sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;
  int mPlayerIndex = -1;
  sead::Quatf mQuat = sead::Quatf::unit;
  sead::Vector3f mTrans = sead::Vector3f::zero;
  sead::Quatf mStartQuat = sead::Quatf::unit;
  sead::Vector3f mStartTrans = sead::Vector3f::zero;
  sead::Vector3f mOutDir = sead::Vector3f::ez;
  sead::Vector3f mBaseUpDir = sead::Vector3f::ey;
  float mMoveSpeed = 20.0f;
  float mOutSpeedH = 10.0f;
  float mOutSpeedV = 25.0f;
  float mRollAngle = 0.0f;
  int mRollTimer = 0;
  float mSpeedOffset = 0.0f;
  int mReflectTimer = 0;
  const char *mMoveActionName = "RouteDokanMove";
};
static_assert(sizeof(RouteDokanRider) == 0x210);
