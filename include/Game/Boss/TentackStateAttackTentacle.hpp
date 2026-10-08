#pragma once
#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <container/seadRingBuffer.h>
#include "Boss/ITentackSwingTentacleHolder.hpp"
#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
}
class Tentack;
class TentackResourceParamHolder;
struct TentackResourceParamInfo;
class TentackStateAttackShot;
class TentackTentacle;
class TentackTentacleGroup;

/** @brief Tuning of the tentacle attack of the first two Tentack battles. */
struct TentackStateAttackTentacleParam {
    TentackStateAttackTentacleParam();

    constexpr TentackStateAttackTentacleParam(s32 damageStage, s32 endWaitStep, f32 placementRadius,
                                              s32 shotInterval)
        : mDamageStage(damageStage), mEndWaitStep(endWaitStep), mPlacementRadius(placementRadius),
          mShotInterval(shotInterval) {}

    s32 mDamageStage;      // 0x0 damage stage used to look up the resource parameters
    s32 mEndWaitStep;      // 0x4 frames to wait after the attack ends
    f32 mPlacementRadius;  // 0x8 radius of the half circle the tentacles are placed on
    s32 mShotInterval;     // 0xc groups between two fire shots (0: shoot every time)
};
static_assert(sizeof(TentackStateAttackTentacleParam) == 0x10);

/** @brief State where Tentack attacks with groups of tentacles around its head. */
class TentackStateAttackTentacle : public al::HostStateBase<Tentack>,
                                   public ITentackSwingTentacleHolder {
public:
    typedef sead::PtrArray<TentackTentacle> TentacleArray;
    typedef sead::RingBuffer<TentackTentacleGroup*> TentacleGroupBuffer;

    /** @brief Value of Tentack::mLevel for the battle where the head also shoots. */
    static constexpr s32 cShotLevel = 2;

    TentackStateAttackTentacle(Tentack* pTentack, const al::ActorInitInfo& rInfo,
                               const TentackStateAttackTentacleParam* pParam);
    void appear() override;
    const TentackResourceParamInfo* getCurrentParamInfo() const;
    void kill() override;
    void control() override;
    void receiveDamage(s32 damage, bool isLast);
    void exeStart();
    void placementTentacleHalfCircle();
    void prepareTentacleGroups();
    void exeGroupWait();
    bool isLastPeriod() const;
    void exeOldGroupBack();
    void exeEat();
    void exeShot();
    void exeGroupAppear();
    TentackTentacleGroup* getTentacleGroup(s32 index) const;
    bool isEnd() const;
    void exeAttackEndInit();
    void exeAttackEnd();
    void exeAttackEndShot();
    void exeAttackEndWait();
    s32 calcSwingTentacleId(const TentackTentacle* pTentacle) const override;

private:
    bool isShotLevel() const;
    bool isOldGroupBack() const;
    bool isShotTurn() const;
    void updateShotCount();

    const TentackStateAttackTentacleParam* mParam;    // 0x28
    TentacleArray mSwingTentacles;                    // 0x30
    TentacleArray mAttackTentacles;                   // 0x40 tentacles that only pop up
    TentackTentacleGroup** mGroups = nullptr;         // 0x50
    s32 mGroupNum = 0;                                // 0x58
    TentacleGroupBuffer mAttackGroups;                // 0x60 groups currently attacking
    s32 mGroupIndex;                                  // 0x78 position in the group order
    TentackResourceParamHolder* mResParamHolder;      // 0x80
    s32 mPeriod = 0;                                  // 0x88 rounds through all groups done
    s32 mShotCount = 0;                               // 0x8c position in the shot interval
    TentackStateAttackShot* mShotState = nullptr;     // 0x90
};
static_assert(sizeof(TentackStateAttackTentacle) == 0x98);
