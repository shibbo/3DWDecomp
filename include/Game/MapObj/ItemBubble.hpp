#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ItemBubble : public al::LiveActor {
public:
    ItemBubble(const char*, int);
    ~ItemBubble() override {}
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void startClipped() override;
    void endClipped() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    virtual sead::Vector3f getItemActorOffset(int) const;
    void exeWait();
    void exeDisappear();
    void initActor(const al::ActorInitInfo&, const char*);
    void setItemType(int);
    void updatePosture();
    bool isDisappear() const;
    void startDisappear();
    bool isEnableGetPlayerSensor() const;
    al::HitSensor* getHitPlayerSensor() const;
protected:
    al::LiveActor* mItemActor = nullptr;
    al::HitSensor* mHitPlayerSensor = nullptr;
    const char* mItemName;
    int mItemType;
    bool mGetItem = false;
    bool mRotateItem = false;
};
static_assert(sizeof(ItemBubble) == 0x168);
