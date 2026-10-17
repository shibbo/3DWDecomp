#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class HitSensor;
class SensorMsg;
}  // namespace al

class DarkBowser;

/** @brief Fury Bowser's damage state: shell hits, side kicks, bomb hits, blocking and stomping. */
class DarkBowserDamage : public al::NerveStateBase {
public:
    /** @brief Which kind of small (non state changing) hit is pending. */
    enum SmallHitType : s32 {
        SmallHitType_None = 0,
        SmallHitType_Bounce = 1,
        SmallHitType_HipDrop = 2,
        SmallHitType_Scratch = 3,
    };

    DarkBowserDamage(DarkBowser* pHost, const al::ActorInitInfo& rInfo);

    void appear() override;
    void kill() override;
    void pushReleasePlayer();
    virtual bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf);
    bool checkDefeat() const;
    void calcEffectMtx(const al::HitSensor* pOther, const al::HitSensor* pSelf, bool isJr);
    void startDamageHitReaction(bool isJr) const;
    void updateSmallHit();
    void exeHipDropHit();
    void exeSideKick();
    void exeBombHit();
    void exeBlocking();
    void exeStomp();
    void exeRecover();
    bool isDefeat() const;
    bool isBlockingOrStomping() const;
    bool checkScratchCountChanceTime() const;
    bool receiveMsgResult();
    ~DarkBowserDamage() override;

    /** @brief Whether the last damage was a side kick. */
    bool isSideKicked() const { return mIsSideKicked; }

    /** @brief Whether the last damage was a bomb hit. */
    bool isBombHit() const { return mIsBombHit; }

    /** @brief Whether the last stomp finished. */
    bool isStompEnd() const { return mIsStompEnd; }

    /** @brief Resets the number of scratches taken. */
    void resetScratchCount() { mScratchCount = 0; }

private:
    DarkBowser* mHost = nullptr;                        // 0x18
    al::HitSensor* mBindSensor = nullptr;               // 0x20
    al::HitSensor* mSmallHitSensor = nullptr;           // 0x28
    const char* mPrevActionName = nullptr;              // 0x30
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;    // 0x38
    sead::Matrix34f mEffectMtxJr = sead::Matrix34f::ident;  // 0x68
    SmallHitType mSmallHitType = SmallHitType_None;     // 0x98
    s32 mSmallHitTimer = 0;                             // 0x9C
    s32 mPrevActionFrame = 0;                           // 0xA0
    s32 mBombHitCooldown = 0;                           // 0xA4
    s32 mScratchCooldown = 0;                           // 0xA8
    s32 mScratchCount = 0;                              // 0xAC
    bool mIsDefeat = false;                             // 0xB0
    bool mIsSideKicked = false;                         // 0xB1
    bool mIsBombHit = false;                            // 0xB2
    bool mIsStompEnd = false;                           // 0xB3
    bool mIsMsgResult = false;                          // 0xB4
    bool mIsJrHit = false;                              // 0xB5
};
static_assert(sizeof(DarkBowserDamage) == 0xb8);
