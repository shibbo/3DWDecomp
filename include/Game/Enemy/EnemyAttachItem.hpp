#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadVector.h>

/**
 * @brief Item carried by an enemy that follows a host position until it is released.
 */
class EnemyAttachItem : public al::LiveActor {
public:
    EnemyAttachItem(al::LiveActor* pHost, const char* pName, const char* pItemName, s32 itemType,
                    const sead::Vector3f* pFollowTrans, const sead::Matrix34f* pFollowMtx,
                    const sead::Vector3f& rOffset, bool isBubble);

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void respawn() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void startClipped() override;
    void endClipped() override;
    void initPosture();
    void updatePosture();
    void exeAttach();
    void exeFall();
    void exeBreakBubble();
    void exePopUp();
    void exeWait();
    void setFollowMtxPtr(const sead::Matrix34f* pMtx);
    void setVelocity(const sead::Vector3f& rVelocity);
    void setFollowTransPtr(const sead::Vector3f* pTrans);
    void endAttach(bool isPopUp);

    /** @brief Gets the item actor created for this attachment (nullptr before init). */
    al::LiveActor* getItemActor() const { return mItemActor; }

/** @brief Gets the attached item kind. @return Item type identifier. */
    s32 getItemType() const { return mItemType; }

private:
    al::LiveActor* mItemActor;
    al::LiveActor* mHost;
    const char* mItemName;
    const sead::Vector3f* mFollowTrans;
    const sead::Matrix34f* mFollowMtx;
    sead::Vector3f mOffset;
    s32 mItemType;
    bool mIsBubble;
    bool mKeepItemRotation;
};

static_assert(sizeof(EnemyAttachItem) == 0x188);
