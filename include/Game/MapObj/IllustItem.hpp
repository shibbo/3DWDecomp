#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ItemStateAssistRotate;
class IllustItem : public al::LiveActor {
public:
    IllustItem(const char*, bool = false);
    ~IllustItem() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void startAppear();
    bool isEnableMsgItemGet(const al::SensorMsg*) const;
    void doGet();
    void exeWait();
    void exeAppear();
    void exeSpinDrc();
    void exeGot();
private:
    bool mInRouteDokan = false;
    bool mAlreadyAcquired = false;
    bool mDisableAssist;
    bool mGot = false;
    float mRotate = 0.0f;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    ItemStateAssistRotate* mAssistRotate = nullptr;
    bool mSpinSoundPlayed = false;
    al::HitSensor* mCollector = nullptr;
};
static_assert(sizeof(IllustItem) == 0x170);
