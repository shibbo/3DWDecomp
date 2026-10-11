#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class DemoAnimatic;
class ItemStatePopUpFront;
class ItemStatePopUpFrontParam;
class KinokoGiga : public al::LiveActor {
public:
    explicit KinokoGiga(const char*);
    void init(const al::ActorInitInfo&) override;
    void appear() override;
    void appearPopUpFront();
    void appearPopUpFront(const ItemStatePopUpFrontParam&);
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void control() override;
    void exeWait();
    void exePopUpFront();
private:
    void* mUnused = nullptr;
    bool mUnusedFlag = false;
    DemoAnimatic* mDemo = nullptr;
    ItemStatePopUpFront* mPopUpState;
};
static_assert(sizeof(KinokoGiga) == 0x168);
