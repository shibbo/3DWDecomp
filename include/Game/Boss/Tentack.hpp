#pragma once
#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include "Boss/TentackBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
template <class T>
class DeriveActorGroup;
}
class BossDemoStartInfo;
class BossStateDemoStart;
class TentackMagmaBall;
class TentackRock;
class TentackStateAttackTentacle;
class TentackStateFallRock;
class TentackTentacle;

/** @brief Tentack boss of the first two battles: one head with its tentacles. */
class Tentack : public al::LiveActor, public TentackBase {
public:
    typedef al::DeriveActorGroup<TentackTentacle> TentacleGroup;
    typedef al::DeriveActorGroup<TentackRock> RockGroup;
    typedef al::DeriveActorGroup<TentackMagmaBall> MagmaBallGroup;

    /** @brief Value of mLevel for the second battle, which drops lava balls instead of rocks. */
    static constexpr s32 cMagmaLevel = 2;

    /** @brief Maximum number of players the rocks are aimed at. */
    static constexpr s32 cPlayerListSize = 16;

    explicit Tentack(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void appear() override;
    void kill() override;

    // Order of the new virtual slots follows the target vtable (getLevel at 0x1b8,
    // getHead at 0x1c8).
    void receiveDamage(const TentackHead* pHead, bool isLast) override;

    /** @brief Gets the battle level. @return 1 or 2. */
    s32 getLevel() const override { return mLevel; }

    /** @brief Gets the holder of the items the tentacles carry. @return Item holder. */
    TentackAttachItemHolder* getAttachItemHolder() const override { return mAttachItemHolder; }

    /** @brief Gets the head. @return Head. */
    TentackHead* getHead() const override { return mHead; }

    const sead::Vector3f& getTentackTrans() const override;
    TentackRockBase* tryGetDeadRock() const override;
    bool tryFindTransNearPlayer(sead::Vector3f* pPosition) override;

    void breakAllRocks();
    void exeDemoStart();
    void exeFallRock();
    void exeAttackTentacle();
    void exeDamage();
    void exeDemoEnd();
    TentackTentacle* getTentacle(s32 index) const;
    s32 getTentacleNum() const;
    s32 getTentacleNumMax() const;
    bool isEnablePlacementPos(const sead::Vector3f& rPos) const;
    bool isInvalidAttackTentacle();
    bool isEmptyTransOtherTentacleOrHead(const sead::Vector3f& rPos,
                                         const TentackTentacle* pTentacle) const;

private:
    TentackHead* mHead = nullptr;                      // 0x150
    TentacleGroup* mTentacles = nullptr;               // 0x158
    RockGroup* mRocks = nullptr;                       // 0x160 first battle
    MagmaBallGroup* mMagmaBalls = nullptr;             // 0x168 second battle

public:
    s32 mLevel = 1;  // 0x170 battle level (1 or 2)

private:
    const al::LiveActor** mPlayerList = nullptr;       // 0x178 players sorted by distance
    s32 mPlayerNum = 0;                                // 0x180
    s32 mPlayerIndex = 0;                              // 0x184 last player a rock was aimed at
    TentackStateFallRock* mFallRockState = nullptr;    // 0x188
    TentackStateAttackTentacle* mAttackTentacleState = nullptr;  // 0x190
    BossStateDemoStart* mDemoStartState = nullptr;     // 0x198
    BossDemoStartInfo* mDemoStartInfo = nullptr;       // 0x1a0
    TentackAttachItemHolder* mAttachItemHolder;        // 0x1a8
    sead::Vector3f mStageCenterPos = {0.0f, 0.0f, 0.0f};  // 0x1b0
};
static_assert(sizeof(Tentack) == 0x1c0);
