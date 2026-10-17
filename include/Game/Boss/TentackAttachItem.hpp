#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
#include <prim/seadEnum.h>
SEAD_ENUM(ETentackAttachItemType, Coin, CoinThree, SuperBell)
/** @brief Item group carried by a Tentack scaffold. */
class TentackAttachItem : public al::LiveActor {
public:
    TentackAttachItem(const char* pName, ETentackAttachItemType type);
    ~TentackAttachItem() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void updateItemTrans();
    void attach(const sead::Vector3f* pTrans);
    void releaseAndTryKill();
    void eat(bool isForce);
    ETentackAttachItemType getType() const { return mType; }
    bool isAttached() const { return mIsAttached; }
private:
    int mFrame = 0;
    sead::PtrArray<al::LiveActor> mItems;
    sead::ObjArray<sead::Vector3f> mOffsets;
    ETentackAttachItemType mType;
    const sead::Vector3f* mHostTrans = nullptr;
    bool mIsAttached = false;
};
static_assert(sizeof(TentackAttachItem) == 0x190);
