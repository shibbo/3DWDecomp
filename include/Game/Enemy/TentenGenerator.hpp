#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
class Nerve;
template <class T>
class DeriveActorGroup;
}  // namespace al
class ActorRailBrakeMover;
class BgmBeatRateTrigger;
class EnemyAttachItem;
class Tenten;

/**
 * @brief Item attached to one column of a Tenten formation, or to the generator's rail.
 */
struct TentenGeneratorAttachItemInfo {
    EnemyAttachItem* mItem;
    s32 mWidthIndex;
    Tenten* mHostTenten;
    bool mIsAttachToRail;
    sead::Vector3f mRailPos;
};

static_assert(sizeof(TentenGeneratorAttachItemInfo) == 0x28);

/**
 * @brief Spawns a grid of Tentens and moves them together along a rail to the BGM beat.
 */
class TentenGenerator : public al::LiveActor {
public:
    explicit TentenGenerator(const char* pName);
    ~TentenGenerator() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void reverseRailAll();
    Tenten* tryFindTopTenten(s32 widthIndex) const;
    void startMove();
    void killBySwitch();
    void initAfterPlacement() override;
    void reappear() override;
    void control() override;
    void killComplete(bool isForce) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool tryStartSupportFreeze(const al::Nerve* pNerve);
    void endSupportFreeze();
    void receiveMsgTouchAssist(const al::LiveActor* pTouchActor);
    void startClipped() override;
    void endClipped() override;
    void noticeDeadChild(const Tenten* pTenten);
    void exeWatch();
    void exeStop();
    void exeMove();
    bool tryChangeWaitHipDrop();
    void updateBgmBeatRateTrigger();
    void exeTurn();
    void exeWait();
    void exeWaitHipDrop();
    void exeSupportFreeze();
    void exeSyncSupportFreeze();
    f32 getTurnSpeed() const;
    bool isBgmSingleModeIsland04();

private:
    al::DeriveActorGroup<Tenten>* mTentens = nullptr;
    s32 mWidthNum = 1;
    s32 mHeightNum = 1;
    f32 mOffsetWidth = 160.0f;
    f32 mOffsetHeight = 130.0f;
    ActorRailBrakeMover* mRailBrakeMover;
    s32 mSupportFreezeStep = 0;
    sead::Vector3f mClippingPos = {0.0f, 0.0f, 0.0f};
    TentenGeneratorAttachItemInfo* mAttachItemInfo = nullptr;
    bool mIsInvalidTurn = false;
    const al::Nerve* mNerveBeforeSupportFreeze = nullptr;
    BgmBeatRateTrigger* mBgmBeatRateTrigger = nullptr;
    s32 mVoiceIndex = 0;
    sead::FixedSafeString<64>* mBgmName = nullptr;
    s32 mBeatCount = 0;
    const al::LiveActor* mSupportFreezeTouchActor = nullptr;
    bool mIsSingleMode = false;
    s32 mPlacementIndex = -1;
    sead::Vector3f mInitTrans = sead::Vector3f::zero;
};

static_assert(sizeof(TentenGenerator) == 0x1d0);
