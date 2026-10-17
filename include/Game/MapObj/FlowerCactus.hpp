#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class EnemyStateBlowDown;
struct EnemyStateBlowDownParam;
class FlowerCactus : public al::LiveActor {
public:
    explicit FlowerCactus(const char* name);
    ~FlowerCactus() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void doBreak(const al::SensorMsg*, al::HitSensor*);
    bool tryChangeNerveToReaction();
    void exeWait();
    void exeTrampled();
    void exeBlowDown();
    void exeBreak();
    void exeReaction();
    void exeReactionEnd();
    void exeReactionTouch();
private:
    int mColor = 0;
    EnemyStateBlowDown* mBlowDown = nullptr;
    EnemyStateBlowDownParam* mBlowDownParam = nullptr;
    al::LiveActor* mTraceModel = nullptr;
};
static_assert(sizeof(FlowerCactus) == 0x160);
