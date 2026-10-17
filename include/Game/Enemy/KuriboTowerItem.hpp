#pragma once
#include "Enemy/KuriboTowerNode.hpp"
#include <math/seadMatrix.h>
class EnemyAttachItem;

class KuriboTowerItem : public KuriboTowerNode {
public:
    KuriboTowerItem(const char* pName, const char* pItemName, bool isItem);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void startClipped() override;
    void endClipped() override;
    void requestRootBehavior() override;
    void requestRelease() override;
    al::LiveActor* getLinkedActor() override;
    f32 getOffsetY() const override;
    void updateItemPosture();
    void exeStandBy();
    void exeFall();
    void exeRootOnGround();
private:
    EnemyAttachItem* mItem = nullptr;
    sead::Vector3f mItemRotate = sead::Vector3f::zero;
    sead::Matrix34f mItemMtx;
    s32 mItemType = 0;
    const char* mItemName;
    bool mUpdateFront = true;
    s32 _1bc = 0;
    bool mIsItem;
};
static_assert(sizeof(KuriboTowerItem) == 0x1c8);
