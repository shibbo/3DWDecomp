#pragma once

#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Player/IUsePlayerFireBall.hpp"

class ActorStateRouteDokanMove;
struct PlayerProperty;

/**
 * @brief Fire ball thrown by Fire Mario. It bounces along the ground, dies after a while or when
 * it hits something, and can travel through route pipes.
 */
class PlayerFireBall : public al::LiveActor, public IUsePlayerFireBall {
public:
    PlayerFireBall(const sead::SafeString& rName, const PlayerProperty* pProperty,
                   al::LiveActor* pPlayer, const al::ActorInitInfo& rInfo,
                   al::HitSensor* pPlayerSensor);

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isMove() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void shoot() override;
    void startMoveAction();
    bool isVanished() const override;
    void forceEnd() override;
    void exeMove();
    void boundGround(const sead::Vector3f& rPos);
    void boundWall(const sead::Vector3f& rNormal, const sead::Vector3f& rPos);
    void applyGravity();
    void boundRoof();
    void turn(const sead::Vector3f& rDir);
    void exeRouteDokan();
    void exeDead();
    void exeDeadDeathCode();
    void exeDeadNoEffect();
    void exeDeadInRouteDokan();
    void reflect(const sead::Vector3f& rPos, const sead::Vector3f& rNormal);

private:
    const PlayerProperty* mProperty;                    // 0x150
    al::LiveActor* mPlayer;                             // 0x158
    al::HitSensor* mPlayerSensor;                       // 0x160
    ActorStateRouteDokanMove* mStateRouteDokan = nullptr;  // 0x168
    u32 mStep = 0;                                      // 0x170
    bool mIsEnableGravity = true;                       // 0x174
    bool mIsInWater = false;                            // 0x175
    sead::Vector3f _178 = sead::Vector3f::zero;
    sead::Vector3f _184 = sead::Vector3f::ez;
    s32 mAppearType = 0;                                // 0x190
    bool mIsSingleMode = false;                         // 0x194
};

static_assert(sizeof(PlayerFireBall) == 0x198);
