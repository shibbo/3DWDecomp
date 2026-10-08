#pragma once

#include <attributes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class FlashingCtrl;
}  // namespace al

class PlayerActor;

/**
 * @brief Crown worn by the player with the best score of the last stage.
 *
 * Follows the head joint of its host player while attached, and is blown off (Fall) when the
 * player takes damage or reaches the goal; another player can then pick it up.
 */
class PlayerCrown : public al::LiveActor {
public:
    PlayerCrown(const al::ActorInitInfo& rInfo, const char* pSuffix);

    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void shiftFall(u32 goalType);
    bool isNerveGettable() const;
    void changeHost(PlayerActor* pHost);
    void appear() override;
    al::HitSensor* getPlayerSensor() const;
    void kill() override;
    void exeAttach();
    bool checkModelHideCondition() const;
    void hide();
    void show();
    void calcFollowMtx();
    void exeFall();
    void startAction(const char* pActionName);
    void calcQuatFromFollowMtx();
    f32 getFallGravity() const;
    f32 getFallSpeedMax() const;
    void exeLand();
    void exeWait();
    ALWAYS_INLINE void exeSinkSand();
    bool isAttach() const;
    void control() override;
    bool isModelHideAnim() const;
    ~PlayerCrown() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;

    /** @brief Gets the player wearing the crown. @return The host player. */
    PlayerActor* getHost() const { return mHost; }

private:
    PlayerActor* mHost = nullptr;                          // 0x148
    sead::Matrix34f mFollowMtx = sead::Matrix34f::ident;   // 0x150
    sead::Vector3f _180{0.0f, 0.0f, 0.0f};       // 0x180
    sead::Vector3f _18c{0.0f, 0.0f, 0.0f};       // 0x18c
    sead::Matrix33f mInertiaTensor;                        // 0x198
    sead::Vector3f _1bc{0.0f, 0.0f, 0.0f};       // 0x1bc
    sead::Vector3f _1c8{0.0f, 0.0f, 0.0f};       // 0x1c8
    sead::Vector3f _1d4{0.0f, 0.0f, 0.0f};       // 0x1d4
    sead::Vector3f _1e0{0.0f, 0.0f, 0.0f};       // 0x1e0
    f32 _1ec = 0.0f;                                       // 0x1ec
    s32 mDropTimer = -1;                                   // 0x1f0, -1 while not dropped
    u32 mGoalType = 0;                                     // 0x1f4, 0 unless released at the goal
    bool mIsInWater = false;                               // 0x1f8
    sead::FixedSafeString<128> mActionName;                // 0x200, player anim being synced
    al::FlashingCtrl* mFlashingCtrl = nullptr;             // 0x298
};

static_assert(sizeof(PlayerCrown) == 0x2a0);
