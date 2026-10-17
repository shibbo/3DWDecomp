#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al {
class ActorInitInfo;
class HitSensor;
class SensorMsg;
struct ActorParamMove;
} // namespace al
class Bunbun;
class BunbunChameleon;
struct BunbunShellMoveParam;
class BunbunStateShellAttack : public al::ActorStateBase {
  public:
    BunbunStateShellAttack(al::LiveActor *, const al::ActorInitInfo &);
    ~BunbunStateShellAttack() override;
    void appear() override;
    void kill() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg *, al::HitSensor *, al::HitSensor *);
    bool isEnableAttack() const;
    bool isEnableTransparent() const;
    bool tryReflectWall();
    void exeChangeShell();
    void exeAttackSign();
    void exeAttackStart();
    void exeAttack();
    void exeAttackEnd();
    void exeRecoverShellStart();
    void exeRecoverShell();

  private:
    void walkToPlayer(const al::ActorParamMove *);
    void walkForward();
    Bunbun *mBunbun = nullptr;
    al::LiveActor *mShell = nullptr;
    BunbunChameleon *mChameleon = nullptr;
    BunbunShellMoveParam *mParams = nullptr;
    bool mTransparent = false;
    const al::ActorParamMove *mAttackParam = nullptr;
    int mHitCooldown = 0;
    bool mIsSingleMode = false;
};
static_assert(sizeof(BunbunStateShellAttack) == 0x58);
