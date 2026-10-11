#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
struct ActorParamMove;
class CameraInfo;
class RumbleCalculatorCosMultLinear;
}  // namespace al

class ActorJointLookController;
class BunbunStateShellAttack;
class BunbunStateSpinAttack;
class GateKeeperStateDemo;
class Punpun;

/** @brief Movement parameters Bunbun uses while it walks and turns towards the player. */
struct BunbunMoveParam {
    const al::ActorParamMove* mWaitMove;
};

/**
 * @brief Bunbun (Boom Boom): a gate keeper boss that spins its arms or attacks with its shell
 * and has to be stomped three times.
 */
class Bunbun : public al::LiveActor {
public:
    /** @brief Attack pattern selected by the "AttackType" placement argument. */
    enum AttackType : s32 {
        /** Spins its arms and recovers in place. */
        AttackType_Spin = 0,
        /** Throws its shell and warps to another move point. */
        AttackType_KouraThrow = 1,
    };

    explicit Bunbun(const char* pName);
    ~Bunbun() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void startDown(al::HitSensor* pOther);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableTransparent() const;
    bool isEnableKouraThrow() const;
    bool isDemoStarted();
    void exeDemo();
    void setInvalidateClippingFlag();
    void exePreDemoDelay();
    void exeDebug();
    void exeWait();
    void exeMysteryBoxWait();
    void exeSpinAttack();
    void exeTired();
    void exeRecoverSign();
    void exeRecover();
    void exePressDown();
    void exeDown();
    void exeDiePressDown();
    void turnToCameraDir();
    void exeDie();
    void exeShellAttack();
    void exeWarp();
    void exeWarpWait();

private:
    void walkAndTurnToPlayer();

    s32 mHp = 3;                                                // 0x144
    s32 mFireBallHitCount = 0;                                  // 0x148
    s32 mExplosionHitCooldown = 0;                              // 0x14C
    GateKeeperStateDemo* mStateDemo = nullptr;                  // 0x150
    BunbunStateShellAttack* mStateShellAttack = nullptr;        // 0x158
    BunbunStateSpinAttack* mStateSpinAttack = nullptr;          // 0x160
    AttackType mAttackType = AttackType_Spin;                   // 0x168
    bool mIsUseMysteryBox = false;                              // 0x16C
    s32 mWarpPointNum = 0;                                      // 0x170
    s32 mWarpPointIndex = 0;                                    // 0x174
    sead::Vector3f* mWarpPoints = nullptr;                      // 0x178
    s32 mWarpFrame = 0;                                         // 0x180
    sead::Vector3f mWarpStartTrans = sead::Vector3f::zero;      // 0x184
    Punpun* mPunpun = nullptr;                                  // 0x190
    al::RumbleCalculatorCosMultLinear* mRumble = nullptr;       // 0x198
    ActorJointLookController* mLookController;                  // 0x1A0
    al::CameraInfo* mShellAttackCamera = nullptr;               // 0x1A8
    BunbunMoveParam* mMoveParam;                                // 0x1B0
    bool mIsSingleMode = false;                                 // 0x1B8
    s32 mKoopaJrHitCooldown = 0;                                // 0x1BC
};

static_assert(sizeof(Bunbun) == 0x1c0);
