#pragma once
#include <container/seadPtrArray.h>
#include <container/seadRingBuffer.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include "Boss/ITentackSwingTentacleHolder.hpp"
#include "Boss/TentackBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"

namespace al {
template <class T>
class DeriveActorGroup;
}
class BossStateDemoStart;
class TentackMagmaBall;
class TentackResourceParamHolder;
struct TentackResourceParamInfo;
class TentackStateAttackShot;
class TentackTentacle;
class TentackTentacleGroup;

/** @brief Final Tentack battle with two heads, their tentacles and falling lava balls. */
class TentackLv3 : public al::LiveActor, public ITentackSwingTentacleHolder, public TentackBase {
public:
    typedef al::DeriveActorGroup<TentackTentacle> TentacleGroup;
    typedef sead::RingBuffer<TentackTentacleGroup*> TentacleGroupBuffer;

    /** @brief Bits of mAttackStartFlags. */
    enum AttackStartFlag : u16 {
        AttackStartFlag_HeadLv1 = 1 << 0,
        AttackStartFlag_HeadLv2 = 1 << 1,
    };

    explicit TentackLv3(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void makeActorAppeared() override;
    void appear() override;
    void kill() override;
    void control() override;
    void receiveDamage(const TentackHead* pHead, bool isLast) override;

    /** @brief Gets the first head. @return Head Lv1. */
    TentackHead* getHead() const override { return mHeadLv1; }

    /** @brief Gets the holder of the items the tentacles carry. @return Item holder. */
    TentackAttachItemHolder* getAttachItemHolder() const override {
        return mAttachItemHolder;
    }

    /** @brief Gets the battle level. @return Always 3. */
    s32 getLevel() const override { return 3; }

    const sead::Vector3f& getTentackTrans() const override;
    TentackRockBase* tryGetDeadRock() const override;
    bool tryFindTransNearPlayer(sead::Vector3f* pPosition) override;
    void returnRockAppearPointPtr(const sead::Vector2f* pPoint) override;
    s32 calcSwingTentacleId(const TentackTentacle* pTentacle) const override;

    void endSwingForceAllTentacle();
    void exeDemoStart();
    void exeFallMagma();
    void startFallMagmaBall(bool isNearPlayer);
    void exeBackGroup();
    bool updateHeadSinkCursor();
    void exeAttackTentacleStart();
    void makeTentacleGroupAndTurnNext(bool isReset);
    TentackTentacleGroup* getCurrentTentacleGroup() const;
    void exeAttackTentacle();
    const TentackResourceParamInfo* getResParamInfo() const;
    bool tryStartBackGroup();
    void exeCryRequest();
    void exeCryStart();
    void exeCry();
    void exeCryEnd();
    void exeDamage();
    void exeDemoEnd();
    TentackTentacle* getTentacle(s32 index) const;
    s32 getTentacleNumMax() const;
    s32 getDamage() const;

private:
    sead::Matrix34f mBaseMtx = sead::Matrix34f::ident;    // 0x158
    TentackBase* mHeadLv2Host = nullptr;                   // 0x188
    TentackHead* mHeadLv1 = nullptr;                       // 0x190
    TentackHead* mHeadLv2 = nullptr;                       // 0x198
    TentacleGroup* mTentaclesLv1 = nullptr;                // 0x1A0
    TentacleGroup* mTentaclesLv2 = nullptr;                // 0x1A8
    TentacleGroupBuffer mTentacleGroups;                   // 0x1B0
    TentacleGroupBuffer mAttackGroups;                     // 0x1C8
    al::DeriveActorGroup<TentackMagmaBall>* mMagmaBalls = nullptr;  // 0x1E0
    TentackAttachItemHolder* mAttachItemHolder;            // 0x1E8
    TentackResourceParamHolder* mResParamHolder;           // 0x1F0
    BossStateDemoStart* mDemoStartState = nullptr;         // 0x1F8
    TentackStateAttackShot* mAttackShotState = nullptr;    // 0x200
    sead::Vector3f mStageCenterPos = {0.0f, 0.0f, 0.0f};   // 0x208
    s32 mPlayerIndex = 0;                                  // 0x214
    s32 mGroupIndex = 0;                                   // 0x218
    sead::PtrArray<const sead::Vector2f> mRockFallPoints;  // 0x220
    u16 mAttackStartFlags = 0;                             // 0x230
    s32 mHeadSinkCursor = 0;                               // 0x234
};
static_assert(sizeof(TentackLv3) == 0x238);
