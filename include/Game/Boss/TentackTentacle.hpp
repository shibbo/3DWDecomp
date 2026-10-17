#pragma once
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include "Boss/TentackHill.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Movement/RumbleCalculator.hpp"

namespace al {
class BreakModel;
class CollisionObj;
}
class TentackAttachItem;
class TentackBase;
class TentackHead;
class TentackTentacleStep;

/** @brief Per-attack swing setup of a tentacle. */
class TentackTentacleInfo {
public:
    TentackTentacleInfo();
    void reset();
    void copy(TentackTentacleInfo* pDst);

    f32 mPushHeight = 0.0f;         // 0x0 swing height above the base position
    bool mIsAttack = false;         // 0x4 pops up near the player and retreats right away
    bool mIsBite = false;           // 0x5 bites after pushing up instead of swinging
    bool mIsStepBreakable = false;  // 0x6 carries a breakable scaffold
    s32 mItemType = 0;              // 0x8 type of the item put on the scaffold
};
static_assert(sizeof(TentackTentacleInfo) == 0xc);

/** @brief Small snake tentacle of the Tentack boss. */
class TentackTentacle : public al::LiveActor {
public:
    /** @brief Collision set used by switchCollisionParts(). */
    enum CollisionType : s32 {
        CollisionType_Body = 0,
        CollisionType_Damaged = 1,
        CollisionType_DamageDead = 2,
    };

    TentackTentacle(const char* pName, TentackBase* pHost);
    ~TentackTentacle() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void switchCollisionParts(s32 type);
    void appearDelay(s32 delay);
    void kill() override;
    void tryBreakCrack();
    bool releaseAndTryKillAttachItem();
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isStiff() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool tryStartStiff(const al::SensorMsg* pMsg, const al::HitSensor* pOther);
    bool isDamage() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void receiveDamage(bool isLast, s32 damage);
    TentackHead* getHead() const;
    void exeAppearDelay();
    void exeAppearSign();
    void appearCrackAndAddRandomRotateY();
    void exePushMax();
    f32 getPushHeightMax() const;
    void exePushBack();
    void exePushEnd();
    void exeSwing();
    f32 getSwingCenter() const;
    void exeStiffStart();
    void exeStiff();
    void exeStiffEnd();
    void exeBiteStart();
    void exeBite();
    void exeBackSign();
    void exeBack();
    void exeDamage();
    bool tryStartBite();
    bool tryCancelPush();
    void endSwing();
    void endSwingForce();
    void setSwingYRate(f32 rate);
    void eatAttachItemIfAttached(bool isForce);
    bool isAppear() const;
    bool isDecidedTrans() const;
    bool isEnableEndSwing() const;
    bool isEnableStepBreak() const;
    void calcStepTrans(sead::Vector3f* pTrans) const;
    static f32 getTentacleRadius();

    /** @brief Gets the swing setup of the current attack. @return Tentacle info. */
    TentackTentacleInfo* getInfo() { return &mInfo; }

    /** @brief Checks whether the tentacle is a spare that does not attack. @return True if spare. */
    bool isSpare() const { return mIsSpare; }

    TentackBase* mHost;                    // 0x148
    TentackTentacleInfo mInfo;             // 0x150 setup for the next appearance
    TentackTentacleInfo mCurInfo;          // 0x15c setup of the current appearance
    sead::Vector3f mCrackPos = {0.0f, 0.0f, 0.0f};    // 0x168
    sead::Vector3f mBasePos = {0.0f, 0.0f, 0.0f};     // 0x174 hidden position under the floor
    sead::Vector3f mFrontDir = {0.0f, 0.0f, 0.0f};    // 0x180
    sead::Vector3f mEffectPos = {0.0f, 0.0f, 0.0f};   // 0x18c
    TentackTentacleStep* mStep = nullptr;  // 0x198
    TentackHill* mHill = new TentackHill("テンタック子蛇の足元モデル");               // 0x1a0
    TentackHill* mHillAttack = new TentackHill("テンタック子蛇の足元モデル[攻撃用]");  // 0x1a8
    TentackHill* mCurHill = nullptr;       // 0x1b0
    al::LiveActor* mCrack = nullptr;       // 0x1b8
    al::BreakModel* mCrackBreak = nullptr;  // 0x1c0
    sead::Matrix34f mCrackBreakMtx = sead::Matrix34f::ident;  // 0x1c8
    s32 mAppearDelay = 0;                  // 0x1f8
    f32 mPushHeight = 0.0f;                // 0x1fc
    f32 mBackStartY = 0.0f;                // 0x200
    TentackAttachItem* mAttachItem = nullptr;  // 0x208
    sead::Matrix34f mLightMtx = sead::Matrix34f::ident;  // 0x210
    void* mUnknown240 = nullptr;           // 0x240
    al::HitSensor* mSensorNoseBite = nullptr;  // 0x248
    al::HitSensor* mSensorNoseWait = nullptr;  // 0x250
    al::HitSensor* mSensorFloorPush;       // 0x258
    al::RumbleCalculatorCosMultLinear* mRumbleCalc =
        new al::RumbleCalculatorCosMultLinear(2.5f, 2.0f, 0.02f, 20);  // 0x260
    s32 mRumbleStep = -1;                  // 0x268
    s32 mStepCollisionTime = 0;            // 0x26c
    f32 mStiffHeight = 0.0f;               // 0x270
    bool mIsSpare = false;                 // 0x274
    al::CollisionObj* mCollisionDamaged = nullptr;     // 0x278
    al::CollisionObj* mCollisionDamageDead = nullptr;  // 0x280
};
static_assert(sizeof(TentackTentacle) == 0x288);
